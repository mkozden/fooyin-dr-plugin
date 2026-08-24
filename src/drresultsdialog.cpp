#include "drresultsdialog.h"

#include "drtags.h"

#include <core/engine/audioloader.h>
#include <core/library/musiclibrary.h>

#include <QDialogButtonBox>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace Fooyin::DRMeter {
DRResultsDialog::DRResultsDialog(MusicLibrary* library, std::shared_ptr<AudioLoader> audioLoader,
                                 TrackDRResults results, ScanMode mode, QWidget* parent)
    : QDialog{parent}
    , m_library{library}
    , m_audioLoader{std::move(audioLoader)}
    , m_results{std::move(results)}
    , m_mode{mode}
    , m_table{new QTableWidget(static_cast<int>(m_results.size()), 6, this)}
    , m_status{new QLabel(this)}
    , m_buttons{new QDialogButtonBox(QDialogButtonBox::Close, this)}
    , m_writeButton{m_buttons->addButton(tr("Update File Tags"), QDialogButtonBox::AcceptRole)}
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    setWindowTitle(tr("Dynamic Range Scan Results"));
    resize(850, 420);

    m_table->setHorizontalHeaderLabels(
        {tr("Track"), tr("DR"), tr("Peak"), tr("RMS"), tr("Duration"), tr("Status")});
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for(int column{1}; column < m_table->columnCount(); ++column) {
        m_table->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    }
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    std::vector<int> scores;
    int writableCount{0};
    for(size_t index{0}; index < m_results.size(); ++index) {
        const auto& result = m_results[index];
        const int row      = static_cast<int>(index);
        m_table->setItem(row, 0, new QTableWidgetItem(result.track.effectiveTitle()));
        m_table->setItem(row, 4, new QTableWidgetItem(QString::number(result.track.duration() / 1000) + u" s"_s));

        if(result.value.valid) {
            scores.push_back(result.value.score);
            m_table->setItem(row, 1, new QTableWidgetItem(u"DR%1"_s.arg(result.value.score)));
            m_table->setItem(row, 2, new QTableWidgetItem(u"%1 dBFS"_s.arg(result.value.peakDb, 0, 'f', 2)));
            m_table->setItem(row, 3, new QTableWidgetItem(u"%1 dBFS"_s.arg(result.value.rmsDb, 0, 'f', 2)));
            const bool writable = isWritable(result);
            writableCount += writable ? 1 : 0;
            m_table->setItem(row, 5, new QTableWidgetItem(writable ? tr("Ready") : tr("Display only")));
        }
        else {
            m_table->setItem(row, 5, new QTableWidgetItem(result.value.error));
        }
    }

    const bool allValid    = scores.size() == m_results.size() && !scores.empty();
    const bool allWritable = writableCount == static_cast<int>(m_results.size()) && !m_results.empty();
    m_albumScore           = allValid ? DRAnalyzer::albumScore(scores) : 0;

    if(m_mode == ScanMode::Album) {
        m_status->setText(allValid ? tr("Official album value: DR%1").arg(m_albumScore)
                                   : tr("Album value unavailable because one or more tracks failed."));
        m_writeButton->setEnabled(allValid && allWritable);
    }
    else {
        m_status->setText(tr("%1 of %2 tracks can be tagged.").arg(writableCount).arg(m_results.size()));
        m_writeButton->setEnabled(writableCount > 0);
    }

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_table);
    layout->addWidget(m_status);
    layout->addWidget(m_buttons);

    QObject::connect(m_writeButton, &QPushButton::clicked, this, &DRResultsDialog::writeTags);
    QObject::connect(m_buttons, &QDialogButtonBox::rejected, this, &DRResultsDialog::reject);
}

bool DRResultsDialog::isWritable(const TrackDRResult& result) const
{
    const Track& track = result.track;
    return result.value.valid && !track.isRemote() && !track.isInArchive() && !track.isBoundedSegment()
        && m_audioLoader->canWriteMetadata(track);
}

void DRResultsDialog::writeTags()
{
    TrackList tracks;
    tracks.reserve(m_results.size());
    for(const auto& result : m_results) {
        if(!isWritable(result)) {
            continue;
        }
        const std::optional<int> albumScore = m_mode == ScanMode::Album ? std::optional{m_albumScore} : std::nullopt;
        tracks.push_back(prepareTaggedTrack(result.track, result.value.score, albumScore));
    }
    if(tracks.empty()) {
        return;
    }

    m_writing = true;
    m_writeButton->setEnabled(false);
    m_status->setText(tr("Writing tags…"));
    m_buttons->button(QDialogButtonBox::Close)->setText(tr("Abort"));

    auto request  = m_library->writeTrackMetadata(tracks);
    m_cancelWrite = request.cancel;
    auto* watcher = new QFutureWatcher<WriteResult>(this);
    QObject::connect(watcher, &QFutureWatcher<WriteResult>::finished, this, [this, watcher] {
        const WriteResult result = watcher->result();
        m_writing                = false;
        m_cancelWrite            = {};
        m_buttons->button(QDialogButtonBox::Close)->setText(tr("Close"));
        m_status->setText(result.state == WriteState::Cancelled
                              ? tr("Tag writing cancelled: %1 written, %2 failed.").arg(result.succeeded).arg(result.failed)
                              : tr("Tag writing complete: %1 written, %2 failed.").arg(result.succeeded).arg(result.failed));
        watcher->deleteLater();
    });
    watcher->setFuture(request.finished);
}

void DRResultsDialog::reject()
{
    if(m_writing && m_cancelWrite) {
        m_cancelWrite();
    }
    QDialog::reject();
}
} // namespace Fooyin::DRMeter
