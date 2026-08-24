#include "dranalyzer.h"

#include <QtTest>

#include <cmath>
#include <vector>

using namespace Fooyin::DRMeter;

namespace {
void addSineBlock(DRAnalyzer& analyzer, double leftAmplitude, double rightAmplitude = -1.0)
{
    const bool stereo = rightAmplitude >= 0.0;
    const size_t frames = analyzer.blockFrames();
    std::vector<double> samples(frames * (stereo ? 2U : 1U));
    for(size_t frame{0}; frame < frames; ++frame) {
        const double wave = std::sin(2.0 * std::numbers::pi * static_cast<double>(frame) / 4.0);
        samples[frame * (stereo ? 2U : 1U)] = wave * leftAmplitude;
        if(stereo) {
            samples[frame * 2U + 1U] = wave * rightAmplitude;
        }
    }
    analyzer.addFrames(samples, frames);
}
} // namespace

class DRAnalyzerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void blockSize()
    {
        const DRAnalyzer at48k{48000, 2};
        const DRAnalyzer at44k{44100, 2};
        QCOMPARE(at48k.blockFrames(), 144000U);
        QCOMPARE(at44k.blockFrames(), 132360U);
    }

    void steadySine()
    {
        DRAnalyzer analyzer{100, 1};
        addSineBlock(analyzer, 1.0);
        addSineBlock(analyzer, 1.0);
        const auto result = analyzer.finish();
        QVERIFY(result.valid);
        QCOMPARE(result.score, 0);
        QVERIFY(std::abs(result.scoreDb) < 1e-12);
        QVERIFY(std::abs(result.peakDb) < 1e-12);
        QVERIFY(std::abs(result.rmsDb) < 1e-12);
    }

    void loudestTwentyPercentAndSecondPeak()
    {
        DRAnalyzer analyzer{100, 1};
        addSineBlock(analyzer, 1.0);
        for(int i{0}; i < 9; ++i) {
            addSineBlock(analyzer, 0.5);
        }
        const auto result = analyzer.finish();
        QVERIFY(result.valid);
        QCOMPARE(result.score, -4);
        QVERIFY(std::abs(result.scoreDb - (-3.9794000867)) < 1e-6);
    }

    void channelAverage()
    {
        DRAnalyzer analyzer{100, 2};
        addSineBlock(analyzer, 1.0, 1.0);
        addSineBlock(analyzer, 1.0, 0.5);
        const auto result = analyzer.finish();
        QVERIFY(result.valid);
        QCOMPARE(result.score, -3);
        QVERIFY(std::abs(result.scoreDb - (-3.0102999566)) < 1e-6);
    }

    void partialBlockAndStreaming()
    {
        DRAnalyzer whole{100, 1};
        DRAnalyzer split{100, 1};
        std::vector<double> samples(137);
        for(size_t i{0}; i < samples.size(); ++i) {
            samples[i] = std::sin(2.0 * std::numbers::pi * static_cast<double>(i) / 4.0);
        }
        whole.addFrames(samples, samples.size());
        split.addFrames(std::span{samples}.first(41), 41);
        split.addFrames(std::span{samples}.subspan(41), samples.size() - 41);
        const auto wholeResult = whole.finish();
        const auto splitResult = split.finish();
        QVERIFY(wholeResult.valid);
        QVERIFY(splitResult.valid);
        QCOMPARE(splitResult.score, wholeResult.score);
        QCOMPARE(splitResult.scoreDb, wholeResult.scoreDb);
    }

    void invalidInput()
    {
        DRAnalyzer empty{100, 1};
        QVERIFY(!empty.finish().valid);

        DRAnalyzer silent{100, 1};
        std::vector<double> samples(silent.blockFrames(), 0.0);
        silent.addFrames(samples, samples.size());
        QVERIFY(!silent.finish().valid);

        DRAnalyzer invalid{0, 1};
        QVERIFY(!invalid.finish().valid);
    }

    void roundingAndAlbumScore()
    {
        QCOMPARE(DRAnalyzer::roundHalfEven(2.5), 2);
        QCOMPARE(DRAnalyzer::roundHalfEven(3.5), 4);
        QCOMPARE(DRAnalyzer::roundHalfEven(-2.5), -2);
        QCOMPARE(DRAnalyzer::albumScore({9, 10, 11}), 10);
        QCOMPARE(DRAnalyzer::albumScore({10, 11}), 10);
    }
};

QTEST_GUILESS_MAIN(DRAnalyzerTest)

#include "dranalyzertest.moc"
