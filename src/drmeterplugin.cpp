#include "drmeterplugin.h"

#include "drresultsdialog.h"

#include <core/engine/audioloader.h>
#include <core/library/musiclibrary.h>
#include <gui/guiconstants.h>
#include <gui/trackselectioncontroller.h>
#include <gui/widgets/elapsedprogressdialog.h>
#include <utils/utils.h>

#include <QAction>
#include <QMainWindow>
#include <QMenu>

#include <algorithm>

namespace Fooyin::DRMeter {
namespace {
constexpr auto MenuId        = "Fooyin.Menu.DynamicRangeMeter";
constexpr auto TrackActionId = "DynamicRange.Scan.Track";
constexpr auto AlbumActionId = "DynamicRange.Scan.Album";
constexpr auto AlbumsActionId = "DynamicRange.Scan.AlbumsByTags";
}

void DRMeterPlugin::initialise(const CorePluginContext& context)
{
    m_audioLoader = context.audioLoader;
    m_library     = context.library;
}

void DRMeterPlugin::initialise(const GuiPluginContext& context)
{
    m_selection = context.trackSelection;
    registerActions();
}

void DRMeterPlugin::shutdown()
{
    if(m_scanner) {
        m_scanner->cancel();
    }
}

void DRMeterPlugin::registerActions()
{
    m_selection->registerTrackContextSubmenu(this, TrackContextMenuArea::Track,
                                             Constants::Menus::Context::TrackSelection, MenuId,
                                             tr("Dynamic Range Meter"), Constants::Menus::Context::TrackFinalSeparator);

    auto* trackAction = new QAction(tr("Scan per track"), this);
    auto* albumAction = new QAction(tr("Scan as one album"), this);
    auto* albumsAction = new QAction(tr("Scan as albums (by tags)"), this);
    QObject::connect(trackAction, &QAction::triggered, this, [this] { startScan(ScanMode::Track); });
    QObject::connect(albumAction, &QAction::triggered, this, [this] { startScan(ScanMode::Album); });
    QObject::connect(albumsAction, &QAction::triggered, this, [this] { startScan(ScanMode::AlbumsByTags); });

    const auto registerAction = [this](QAction* action, const char* id) {
        m_selection->registerTrackContextAction(
            this, TrackContextMenuArea::Track, MenuId, id, action->text(),
            [action](QMenu* menu, const TrackSelection& selection) {
                action->setEnabled(!selection.tracks.empty());
                menu->addAction(action);
            });
    };
    registerAction(trackAction, TrackActionId);
    registerAction(albumAction, AlbumActionId);
    registerAction(albumsAction, AlbumsActionId);
}

void DRMeterPlugin::startScan(ScanMode mode)
{
    if(m_scanner || !m_audioLoader || !m_library || !m_selection) {
        return;
    }

    TrackList tracks;
    for(const Track& track : m_selection->selectedTracks()) {
        const bool duplicate = std::ranges::any_of(
            tracks, [&track](const Track& existing) { return existing.sameIdentityAs(track); });
        if(!duplicate) {
            tracks.push_back(track);
        }
    }
    if(tracks.empty()) {
        return;
    }

    const int total = static_cast<int>(tracks.size());
    auto* progress  = new ElapsedProgressDialog(tr("Scanning tracks…"), tr("Abort"), 0, total,
                                                Utils::getMainWindow());
    progress->setAttribute(Qt::WA_DeleteOnClose);
    progress->setWindowTitle(tr("Dynamic Range Scan Progress"));
    progress->setValue(0);
    progress->startTimer();
    progress->show();

    auto* scanner = new DRScanner(m_audioLoader, this);
    m_scanner     = scanner;
    QObject::connect(progress, &ElapsedProgressDialog::cancelled, scanner, &DRScanner::cancel);
    QObject::connect(scanner, &DRScanner::currentTrack, progress,
                     [progress](int current, int, const QString& filepath) {
                         progress->setValue(current - 1);
                         progress->setText(DRMeterPlugin::tr("Current file:\n%1").arg(filepath));
                     });
    QObject::connect(scanner, &DRScanner::finished, this,
                     [this, scanner, progress, mode, total](const TrackDRResults& results, bool cancelled) {
                         m_scanner = nullptr;
                         progress->setValue(total);
                         progress->deleteLater();
                         scanner->deleteLater();
                         if(cancelled) {
                             return;
                         }
                         auto* dialog
                             = new DRResultsDialog(m_library, m_audioLoader, results, mode, Utils::getMainWindow());
                         dialog->show();
                     });
    scanner->start(tracks);
}
} // namespace Fooyin::DRMeter
