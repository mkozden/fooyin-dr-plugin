#include "dranalyzer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace Fooyin::DRMeter {
namespace {
constexpr double SilenceFloor = 1e-15;

double toDb(double amplitude)
{
    return 20.0 * std::log10(amplitude);
}
} // namespace

DRAnalyzer::DRAnalyzer(int sampleRate, int channels)
    : m_sampleRate{sampleRate}
    , m_channels{channels}
    , m_blockFrames{sampleRate > 0 ? static_cast<size_t>(sampleRate) * 3U + (sampleRate == 44100 ? 60U : 0U) : 0U}
    , m_blockSquares(static_cast<size_t>(std::max(channels, 0)), 0.0)
    , m_blockPeaks(static_cast<size_t>(std::max(channels, 0)), 0.0)
    , m_totalSquares(static_cast<size_t>(std::max(channels, 0)), 0.0)
    , m_totalPeaks(static_cast<size_t>(std::max(channels, 0)), 0.0)
    , m_rmsBlocks(static_cast<size_t>(std::max(channels, 0)))
    , m_peakBlocks(static_cast<size_t>(std::max(channels, 0)))
{ }

bool DRAnalyzer::isValid() const
{
    return m_sampleRate > 0 && m_channels > 0 && m_channels <= 32;
}

size_t DRAnalyzer::blockFrames() const
{
    return m_blockFrames;
}

void DRAnalyzer::addFrames(std::span<const double> interleaved, size_t frames)
{
    if(m_finished || !isValid() || interleaved.size() < frames * static_cast<size_t>(m_channels)) {
        m_badInput = true;
        return;
    }

    for(size_t frame{0}; frame < frames; ++frame) {
        for(int channel{0}; channel < m_channels; ++channel) {
            const double sample = interleaved[frame * static_cast<size_t>(m_channels) + static_cast<size_t>(channel)];
            if(!std::isfinite(sample)) {
                m_badInput = true;
                return;
            }
            const double square = sample * sample;
            const double peak   = std::abs(sample);
            const auto index    = static_cast<size_t>(channel);
            m_blockSquares[index] += square;
            m_totalSquares[index] += square;
            m_blockPeaks[index] = std::max(m_blockPeaks[index], peak);
            m_totalPeaks[index] = std::max(m_totalPeaks[index], peak);
        }

        ++m_currentFrames;
        ++m_totalFrames;
        if(m_currentFrames == m_blockFrames) {
            finishBlock();
        }
    }
}

void DRAnalyzer::finishBlock()
{
    if(m_currentFrames == 0) {
        return;
    }

    for(int channel{0}; channel < m_channels; ++channel) {
        const auto index = static_cast<size_t>(channel);
        const double rms = std::sqrt(2.0 * m_blockSquares[index] / static_cast<double>(m_currentFrames));
        m_rmsBlocks[index].push_back(rms);
        m_peakBlocks[index].push_back(m_blockPeaks[index]);
        m_blockSquares[index] = 0.0;
        m_blockPeaks[index]   = 0.0;
    }
    m_currentFrames = 0;
}

DRValue DRAnalyzer::finish()
{
    if(m_finished) {
        return {.error = QStringLiteral("Analyzer has already been finished")};
    }
    m_finished = true;

    if(!isValid() || m_badInput) {
        return {.error = QStringLiteral("Invalid PCM input")};
    }
    finishBlock();
    if(m_totalFrames == 0) {
        return {.error = QStringLiteral("No audio samples")};
    }

    double scoreSum{0.0};
    double correctedSquareSum{0.0};
    double overallPeak{0.0};

    for(int channel{0}; channel < m_channels; ++channel) {
        const auto index = static_cast<size_t>(channel);
        auto rmsValues   = m_rmsBlocks[index];
        auto peakValues  = m_peakBlocks[index];
        std::ranges::sort(rmsValues, std::greater{});
        std::ranges::sort(peakValues, std::greater{});

        const size_t loudCount = std::max<size_t>(1, static_cast<size_t>(std::ceil(rmsValues.size() * 0.2)));
        const double loudSquares
            = std::inner_product(rmsValues.cbegin(), rmsValues.cbegin() + static_cast<ptrdiff_t>(loudCount),
                                 rmsValues.cbegin(), 0.0);
        const double loudRms = std::sqrt(loudSquares / static_cast<double>(loudCount));
        const double peakRef = peakValues.size() > 1 ? peakValues[1] : peakValues.front();
        if(loudRms <= SilenceFloor || peakRef <= SilenceFloor) {
            return {.error = QStringLiteral("Audio is silent")};
        }

        scoreSum += toDb(peakRef / loudRms);
        correctedSquareSum += 2.0 * m_totalSquares[index] / static_cast<double>(m_totalFrames);
        overallPeak = std::max(overallPeak, m_totalPeaks[index]);
    }

    const double scoreDb = scoreSum / static_cast<double>(m_channels);
    const double rms     = std::sqrt(correctedSquareSum / static_cast<double>(m_channels));
    if(!std::isfinite(scoreDb) || rms <= SilenceFloor || overallPeak <= SilenceFloor) {
        return {.error = QStringLiteral("Could not calculate dynamic range")};
    }

    return {
        .valid   = true,
        .score   = roundHalfEven(scoreDb),
        .scoreDb = scoreDb,
        .peakDb  = toDb(overallPeak),
        .rmsDb   = toDb(rms),
    };
}

int DRAnalyzer::roundHalfEven(double value)
{
    const double lower    = std::floor(value);
    const double fraction = value - lower;
    if(fraction < 0.5) {
        return static_cast<int>(lower);
    }
    if(fraction > 0.5) {
        return static_cast<int>(lower + 1.0);
    }
    const auto lowerInteger = static_cast<long long>(lower);
    return static_cast<int>((lowerInteger % 2 == 0) ? lowerInteger : lowerInteger + 1);
}

int DRAnalyzer::albumScore(const std::vector<int>& scores)
{
    if(scores.empty()) {
        return 0;
    }
    const double sum = std::accumulate(scores.cbegin(), scores.cend(), 0.0);
    return roundHalfEven(sum / static_cast<double>(scores.size()));
}
} // namespace Fooyin::DRMeter
