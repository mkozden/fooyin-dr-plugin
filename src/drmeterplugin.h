#pragma once

#include "drscanner.h"

#include <core/plugins/coreplugin.h>
#include <core/plugins/plugin.h>
#include <gui/plugins/guiplugin.h>

#include <QPointer>

class QAction;

namespace Fooyin {
class AudioLoader;
class MusicLibrary;
class TrackSelectionController;
}

namespace Fooyin::DRMeter {
class DRMeterPlugin : public QObject,
                      public Plugin,
                      public CorePlugin,
                      public GuiPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.fooyin.fooyin.plugin/1.0" FILE "drmeter.json")
    Q_INTERFACES(Fooyin::Plugin Fooyin::CorePlugin Fooyin::GuiPlugin)

public:
    void initialise(const CorePluginContext& context) override;
    void initialise(const GuiPluginContext& context) override;
    void shutdown() override;

private:
    void registerActions();
    void startScan(ScanMode mode);

    std::shared_ptr<AudioLoader> m_audioLoader;
    MusicLibrary* m_library{nullptr};
    TrackSelectionController* m_selection{nullptr};
    QPointer<DRScanner> m_scanner;
};
} // namespace Fooyin::DRMeter
