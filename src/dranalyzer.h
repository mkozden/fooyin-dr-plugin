#pragma once

#include <QString>

#include <cstddef>
#include <span>
#include <vector>

namespace Fooyin::DRMeter {
struct DRValue
{
    bool valid{false};
    int score{0};
    double scoreDb{0.0};
    double peakDb{0.0};
    double rmsDb{0.0};
    QString error;
};

class DRAnalyzer
{
public:
    DRAnalyzer(int sampleRate, int channels);

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] size_t blockFrames() const;
    void addFrames(std::span<const double> interleaved, size_t frames);
    [[nodiscard]] DRValue finish();

    static int roundHalfEven(double value);
    static int albumScore(const std::vector<int>& scores);

private:
    void finishBlock();

    int m_sampleRate;
    int m_channels;
    size_t m_blockFrames;
    size_t m_currentFrames{0};
    uint64_t m_totalFrames{0};
    bool m_finished{false};
    bool m_badInput{false};

    std::vector<double> m_blockSquares;
    std::vector<double> m_blockPeaks;
    std::vector<double> m_totalSquares;
    std::vector<double> m_totalPeaks;
    std::vector<std::vector<double>> m_rmsBlocks;
    std::vector<std::vector<double>> m_peakBlocks;
};
} // namespace Fooyin::DRMeter
