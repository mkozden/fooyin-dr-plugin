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
#include <map>
#include <ranges>

using namespace Qt::StringLiterals;

namespace Fooyin::DRMeter {
DRResultsDialog::DRResultsDialog(MusicLibrary* library, std::shared_ptr<AudioLoader> audioLoader,
                                 TrackDRResults results, ScanMode mode, QWidget* parent)
    : QDialog{parent}
    , m_library{library}
    , m_audioLoader{std::move(audioLoader)}
    , m_results{std::move(results)}
    , m_mode{mode}
    , m_groupScores(m_results.size())
    , m_groupWritable(m_results.size(), false)
    , m_table{new QTableWidget(static_cast<int>(m_results.size()), 8, this)}
    , m_status{new QLabel(this)}
    , m_buttons{new QDialogButtonBox(QDialogButtonBox::Close, this)}
    , m_writeButton{m_buttons->addButton(tr("Update File Tags"), QDialogButtonBox::AcceptRole)}
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    setWindowTitle(tr("Dynamic Range Scan Results"));
    resize(850, 420);

    m_table->setHorizontalHeaderLabels({tr("Track"), tr("Album"), tr("DR"), tr("Album DR"), tr("Peak"),
                                        tr("RMS"), tr("Duration"), tr("Status")});
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
        if(m_mode == ScanMode::AlbumsByTags) {
            m_table->setItem(row, 1, new QTableWidgetItem(albumGroupLabel(result.track)));
        }
        else if(m_mode == ScanMode::Album) {
            m_table->setItem(row, 1, new QTableWidgetItem(tr("Selection")));
        }
        m_table->setItem(row, 6, new QTableWidgetItem(QString::number(result.track.duration() / 1000) + u" s"_s));

        if(result.value.valid) {
            scores.push_back(result.value.score);
            m_table->setItem(row, 2, new QTableWidgetItem(u"DR%1"_s.arg(result.value.score)));
            m_table->setItem(row, 4, new QTableWidgetItem(u"%1 dBFS"_s.arg(result.value.peakDb, 0, 'f', 2)));
            m_table->setItem(row, 5, new QTableWidgetItem(u"%1 dBFS"_s.arg(result.value.rmsDb, 0, 'f', 2)));
            const bool writable = isWritable(result);
            writableCount += writable ? 1 : 0;
        }
    }

    const bool allValid    = scores.size() == m_results.size() && !scores.empty();
    const bool allWritable = writableCount == static_cast<int>(m_results.size()) && !m_results.empty();
    m_albumScore           = allValid ? DRAnalyzer::albumScore(scores) : 0;

    if(m_mode == ScanMode::AlbumsByTags) {
        struct AlbumGroup
        {
            std::vector<size_t> rows;
        };
        std::map<QString, AlbumGroup> groups;
        for(size_t index{0}; index < m_results.size(); ++index) {
            groups[albumGroupKey(m_results[index].track)].rows.push_back(index);
        }

        int writableAlbums{0};
        for(const auto& group : groups | std::views::values) {
            std::vector<int> groupScores;
            bool groupCanWrite{!group.rows.empty()};
            for(const size_t index : group.rows) {
                const auto& result = m_results[index];
                if(result.value.valid) {
                    groupScores.push_back(result.value.score);
                }
                else {
                    groupCanWrite = false;
                }
                groupCanWrite = groupCanWrite && isWritable(result);
            }

            const bool groupValid = groupScores.size() == group.rows.size() && !groupScores.empty();
            const int groupScore  = groupValid ? DRAnalyzer::albumScore(groupScores) : 0;
            writableAlbums += groupValid && groupCanWrite ? 1 : 0;
            for(const size_t index : group.rows) {
                if(groupValid) {
                    m_groupScores[index] = groupScore;
                    m_table->setItem(static_cast<int>(index), 3,
                                     new QTableWidgetItem(u"DR%1"_s.arg(groupScore)));
                }
                m_groupWritable[index] = groupValid && groupCanWrite;
            }
        }

        for(size_t index{0}; index < m_results.size(); ++index) {
            const auto& result = m_results[index];
            const QString status = !result.value.valid ? result.value.error
                                   : m_groupWritable[index] ? tr("Ready")
                                                            : tr("Album incomplete or display only");
            m_table->setItem(static_cast<int>(index), 7, new QTableWidgetItem(status));
        }
        m_status->setText(tr("%1 of %2 albums can be tagged.").arg(writableAlbums).arg(groups.size()));
        m_writeButton->setEnabled(writableAlbums > 0);
    }
    else if(m_mode == ScanMode::Album) {
        for(size_t index{0}; index < m_results.size(); ++index) {
            if(allValid) {
                m_table->setItem(static_cast<int>(index), 3, new QTableWidgetItem(u"DR%1"_s.arg(m_albumScore)));
            }
            const auto& result = m_results[index];
            const QString status = !result.value.valid ? result.value.error
                                   : allValid && allWritable ? tr("Ready")
                                                             : tr("Album incomplete or display only");
            m_table->setItem(static_cast<int>(index), 7, new QTableWidgetItem(status));
        }
        m_status->setText(allValid ? tr("Official album value: DR%1").arg(m_albumScore)
                                   : tr("Album value unavailable because one or more tracks failed."));
        m_writeButton->setEnabled(allValid && allWritable);
    }
    else {
        for(size_t index{0}; index < m_results.size(); ++index) {
            const auto& result = m_results[index];
            const QString status = !result.value.valid ? result.value.error
                                                       : isWritable(result) ? tr("Ready") : tr("Display only");
            m_table->setItem(static_cast<int>(index), 7, new QTableWidgetItem(status));
        }
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
    for(size_t index{0}; index < m_results.size(); ++index) {
        const auto& result = m_results[index];
        if(!isWritable(result) || (m_mode == ScanMode::AlbumsByTags && !m_groupWritable[index])) {
            continue;
        }
        std::optional<int> albumScore;
        if(m_mode == ScanMode::Album) {
            albumScore = m_albumScore;
        }
        else if(m_mode == ScanMode::AlbumsByTags) {
            albumScore = m_groupScores[index];
        }
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
