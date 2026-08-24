#include "drscanner.h"

#include <core/engine/audioconverter.h>
#include <core/engine/audioloader.h>

#include <QMetaObject>

#include <algorithm>
#include <atomic>
#include <limits>
#include <mutex>
#include <span>

namespace Fooyin::DRMeter {
class DRScanWorker : public QObject
{
    Q_OBJECT

public:
    explicit DRScanWorker(std::shared_ptr<AudioLoader> audioLoader)
        : m_audioLoader{std::move(audioLoader)}
    { }

    void scan(const TrackList& tracks)
    {
        TrackDRResults results;
        results.reserve(tracks.size());

        const int total = static_cast<int>(tracks.size());
        for(int index{0}; index < total && !m_cancelled.load(std::memory_order_acquire); ++index) {
            const Track& track = tracks.at(static_cast<size_t>(index));
            Q_EMIT currentTrack(index + 1, total, track.prettyFilepath());
            results.push_back({track, scanTrack(track)});
        }

        Q_EMIT scanFinished(results, m_cancelled.load(std::memory_order_acquire));
    }

    void cancel()
    {
        m_cancelled.store(true, std::memory_order_release);
        const std::scoped_lock lock{m_decoderMutex};
        if(AudioDecoder* decoder = m_currentDecoder) {
            decoder->requestAbort();
        }
    }

Q_SIGNALS:
    void currentTrack(int current, int total, const QString& filepath);
    void scanFinished(const Fooyin::DRMeter::TrackDRResults& results, bool cancelled);

private:
    DRValue scanTrack(const Track& track)
    {
        auto loaded = m_audioLoader->loadDecoderForTrack(track, AudioDecoder::NoInfiniteLooping);
        if(!loaded.decoder || !loaded.format || !loaded.format->isValid()) {
            return {.error = tr("No decoder available")};
        }

        AudioDecoder* decoder = loaded.decoder.get();
        {
            const std::scoped_lock lock{m_decoderMutex};
            m_currentDecoder = decoder;
        }
        decoder->start();
        if(track.offset() > 0) {
            decoder->seek(track.offset());
        }

        AudioFormat outputFormat = *loaded.format;
        outputFormat.setSampleFormat(SampleFormat::F64);
        DRAnalyzer analyzer{outputFormat.sampleRate(), outputFormat.channelCount()};
        if(!analyzer.isValid()) {
            decoder->stop();
            const std::scoped_lock lock{m_decoderMutex};
            m_currentDecoder = nullptr;
            return {.error = tr("Unsupported audio format")};
        }

        uint64_t framesRemaining = track.isBoundedSegment()
                                     ? static_cast<uint64_t>(outputFormat.framesForDuration(track.duration()))
                                     : std::numeric_limits<uint64_t>::max();
        QString error;
        constexpr size_t ReadBytes = 256U * 1024U;

        while(!m_cancelled.load(std::memory_order_acquire) && framesRemaining > 0) {
            auto read = decoder->readAudio(ReadBytes);
            if(read.status == AudioDecoder::ReadStatus::EndOfStream) {
                break;
            }
            if(read.status == AudioDecoder::ReadStatus::Error) {
                error = read.error.isEmpty() ? tr("Decoder error") : read.error;
                break;
            }
            if(read.status == AudioDecoder::ReadStatus::NeedMoreInput) {
                continue;
            }

            AudioBuffer converted = Audio::convert(read.buffer, outputFormat);
            if(!converted.isValid()) {
                error = tr("PCM conversion failed");
                break;
            }

            const size_t available = static_cast<size_t>(converted.frameCount());
            const size_t frames    = static_cast<size_t>(std::min<uint64_t>(available, framesRemaining));
            const auto samples = std::span{reinterpret_cast<const double*>(converted.data()),
                                           frames * static_cast<size_t>(outputFormat.channelCount())};
            analyzer.addFrames(samples, frames);
            if(framesRemaining != std::numeric_limits<uint64_t>::max()) {
                framesRemaining -= frames;
            }
        }

        decoder->stop();
        {
            const std::scoped_lock lock{m_decoderMutex};
            m_currentDecoder = nullptr;
        }

        if(m_cancelled.load(std::memory_order_acquire)) {
            return {.error = tr("Cancelled")};
        }
        if(!error.isEmpty()) {
            return {.error = error};
        }
        return analyzer.finish();
    }

    std::shared_ptr<AudioLoader> m_audioLoader;
    std::atomic_bool m_cancelled{false};
    std::mutex m_decoderMutex;
    AudioDecoder* m_currentDecoder{nullptr};
};

DRScanner::DRScanner(std::shared_ptr<AudioLoader> audioLoader, QObject* parent)
    : QObject{parent}
    , m_worker{new DRScanWorker(std::move(audioLoader))}
{
    qRegisterMetaType<TrackDRResults>();
    m_worker->moveToThread(&m_thread);
    QObject::connect(m_worker, &DRScanWorker::currentTrack, this, &DRScanner::currentTrack);
    QObject::connect(m_worker, &DRScanWorker::scanFinished, this, &DRScanner::finished);
    QObject::connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread.start();
}

DRScanner::~DRScanner()
{
    cancel();
    m_thread.quit();
    m_thread.wait();
}

void DRScanner::start(const TrackList& tracks)
{
    QMetaObject::invokeMethod(m_worker, [worker = m_worker, tracks] { worker->scan(tracks); });
}

void DRScanner::cancel()
{
    m_worker->cancel();
}
} // namespace Fooyin::DRMeter

#include "drscanner.moc"
