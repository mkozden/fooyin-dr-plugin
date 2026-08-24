#pragma once

#include "dranalyzer.h"

#include <core/track.h>

#include <QObject>
#include <QThread>

#include <memory>
#include <vector>

namespace Fooyin {
class AudioLoader;
}

namespace Fooyin::DRMeter {
enum class ScanMode : uint8_t
{
    Track,
    Album,
};

struct TrackDRResult
{
    Track track;
    DRValue value;
};
using TrackDRResults = std::vector<TrackDRResult>;

class DRScanWorker;

class DRScanner : public QObject
{
    Q_OBJECT

public:
    explicit DRScanner(std::shared_ptr<AudioLoader> audioLoader, QObject* parent = nullptr);
    ~DRScanner() override;

    void start(const TrackList& tracks);
    void cancel();

Q_SIGNALS:
    void currentTrack(int current, int total, const QString& filepath);
    void finished(const Fooyin::DRMeter::TrackDRResults& results, bool cancelled);

private:
    QThread m_thread;
    DRScanWorker* m_worker;
};
} // namespace Fooyin::DRMeter

Q_DECLARE_METATYPE(Fooyin::DRMeter::TrackDRResults)
