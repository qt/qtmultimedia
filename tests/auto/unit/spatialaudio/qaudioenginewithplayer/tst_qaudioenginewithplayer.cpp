// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtSpatialAudio/qaudioengine.h>
#include <QtSpatialAudio/qspatialsound.h>
#include <QtSpatialAudio/private/qaudioengine_p.h>
#include <QtSpatialAudio/private/qaudioengine_withplayer_p.h>
#include <QtSpatialAudio/private/qtspatialaudioglobal_p.h>

#include <QtMultimedia/private/qaudio_qspan_support_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/private/qrtaudioengine_p.h>
#include <QtMultimediaTestLib/private/audiogenerationutils_p.h>
#include <QtMultimediaTestLib/private/qmessagespy_p.h>
#include <QtMultimediaTestLib/private/qsinewavevalidator_p.h>

#include <QtCore/qregularexpression.h>

#include "qmockaudiodevices.h"
#include "qmockintegration.h"

#include <cmath>
#include <numeric>

using namespace Qt::Literals;
using namespace std::chrono_literals;

using QtMultimediaPrivate::QLoggingCategoryEnabler;
using QtMultimediaPrivate::QMessageSpy;
using QtMultimediaPrivate::QRtAudioEngine;
namespace ranges = QtMultimediaPrivate::ranges;
namespace views = QtMultimediaPrivate::views;

Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN

class tst_QAudioEngineWithPlayer : public QObject
{
    Q_OBJECT

public slots:
    void init() { QMockIntegration::instance()->audioDevices()->addAudioOutput(); }
    void cleanup() { QMockIntegration::instance()->resetInstance(); }
    void initTestCase();

private slots:
    void silentBeforeAnySoundIsLoaded();
    void sineWaveIsMixedIntoOutput();
    void pauseProducesExactSilence();
    void twoSourcesMixAdditively();
    void volumeDecreasesWithDistance();
    void positionPansToTheCorrectChannel();
    void frontAndBackAreCenterPanned();

private:
    static constexpr int sampleRate = 44100;

    std::unique_ptr<QTemporaryFile> m_sine440Wav;
    std::unique_ptr<QTemporaryFile> m_sine300Wav;
    std::unique_ptr<QTemporaryFile> m_sine900Wav;
    std::unique_ptr<QTemporaryFile> m_sine2000Wav;

    static QRtAudioEngine *playbackEngineOf(QAudioEngine &engine)
    {
        auto *ep = QAudioEnginePrivate::get(&engine);
        Q_ASSERT(ep);
        auto *withPlayer = static_cast<QAudioEngineWithPlayer *>(ep);
        return withPlayer->playbackEngine();
    }

    static std::vector<float> pumpOneSlice(QRtAudioEngine &rt)
    {
        const int channelCount = rt.audioSink().format().channelCount();
        std::vector<float> buffer(qToUnderlying(QAudioEnginePrivate::framesPerBuffer)
                                  * size_t(channelCount));
        rt.pumpAudioCallback(buffer);
        return buffer;
    }

    static bool isSilent(QSpan<const float> buffer)
    {
        return ranges::all_of(buffer, [](float f) {
            return f == 0.0f;
        });
    }

    static double energy(QSpan<const float> buffer)
    {
        auto squared = buffer | views::transform([](double sample) {
            return sample * sample;
        });
        return std::accumulate(squared.begin(), squared.end(), 0.0);
    }

    template <typename Range>
    static float peakAbs(const Range &buffer)
    {
        return std::accumulate(buffer.begin(), buffer.end(), 0.0f, [](float peak, float sample) {
            return std::max(peak, std::abs(sample));
        });
    }

    static auto extractChannel(QSpan<const float> interleavedBuffer, int channelCount, int channel)
    {
        return QtMultimediaPrivate::drop(interleavedBuffer, channel) | views::stride(channelCount);
    }

    struct ChannelPeaks
    {
        float left = 0.0f;
        float right = 0.0f;
    };

    // skips a few periods to let transients settle (voice startup, panner/distance-model
    // coefficients ramping in), then tracks the per-channel absolute peak over enough periods to
    // get a stable reading despite the short (128-frame) period size
    static ChannelPeaks measureChannelPeaks(QRtAudioEngine &rt, int warmupSlices = 8,
                                            int measureSlices = 16)
    {
        for (int slice = 0; slice != warmupSlices; ++slice)
            pumpOneSlice(rt);

        const int channelCount = rt.audioSink().format().channelCount();
        Q_ASSERT(channelCount == 2);

        ChannelPeaks peaks;
        for (int slice = 0; slice != measureSlices; ++slice) {
            std::vector<float> buffer = pumpOneSlice(rt);
            peaks.left = std::max(peaks.left, peakAbs(extractChannel(buffer, channelCount, 0)));
            peaks.right = std::max(peaks.right, peakAbs(extractChannel(buffer, channelCount, 1)));
        }
        return peaks;
    }

    void waitUntilLoaded(QSpatialSound &sound, const QUrl &url)
    {
        QLoggingCategoryEnabler enabler(qLcSpatialAudioEngine());
        QMessageSpy spy(qLcSpatialAudioEngine());
        auto loaded = spy.expect(QtDebugMsg, "Sound loaded"_L1);
        QTest::ignoreMessage(QtDebugMsg, QRegularExpression("Loading sound: .*"));
        QTest::ignoreMessage(QtDebugMsg, QRegularExpression("Sound playback started: .*"));
        sound.setSource(url);
        QVERIFY(loaded.wait());
    }
};

void tst_QAudioEngineWithPlayer::initTestCase()
{
    m_sine440Wav = makeMonoPcm16WavFile(sampleRate, 2s, 440.0);
    QVERIFY(m_sine440Wav);
    m_sine300Wav = makeMonoPcm16WavFile(sampleRate, 2s, 300.0);
    QVERIFY(m_sine300Wav);
    m_sine900Wav = makeMonoPcm16WavFile(sampleRate, 2s, 900.0);
    QVERIFY(m_sine900Wav);
    m_sine2000Wav = makeMonoPcm16WavFile(sampleRate, 2s, 2000.0);
    QVERIFY(m_sine2000Wav);
}

void tst_QAudioEngineWithPlayer::silentBeforeAnySoundIsLoaded()
{
    QAudioEngine engine;
    engine.start();

    QRtAudioEngine *rt = playbackEngineOf(engine);
    QVERIFY(rt);

    for (int slice = 0; slice != 4; ++slice)
        QVERIFY(isSilent(pumpOneSlice(*rt)));
}

void tst_QAudioEngineWithPlayer::sineWaveIsMixedIntoOutput()
{
    constexpr qreal frequency = 440.0;

    QAudioEngine engine(sampleRate);
    QSpatialSound sound(&engine);
    sound.setLoops(QSpatialSound::Infinite);
    sound.setAutoPlay(true);

    engine.start();
    waitUntilLoaded(sound, QUrl::fromLocalFile(m_sine440Wav->fileName()));

    QRtAudioEngine *rt = playbackEngineOf(engine);
    QVERIFY(rt);

    // warm up: skip the first few slices while resonance-audio's internal pipeline fills up
    for (int slice = 0; slice != 8; ++slice)
        pumpOneSlice(*rt);

    const int channelCount = rt->audioSink().format().channelCount();
    QSineWaveValidator validator{ float(frequency), float(sampleRate) };
    for (int slice = 0; slice != 64; ++slice) {
        std::vector<float> buffer = pumpOneSlice(*rt);
        // feed only the left channel: the validator expects a continuous single-channel signal,
        // not an interleaved multi-channel stream
        for (size_t frameOffset = 0; frameOffset < buffer.size();
             frameOffset += size_t(channelCount))
            validator.feedSample(buffer[frameOffset]);
    }

    QVERIFY(validator.peak() > 0.0f);
    QVERIFY(validator.notchPeak() < 0.1f * validator.peak());
}

void tst_QAudioEngineWithPlayer::pauseProducesExactSilence()
{
    QAudioEngine engine(sampleRate);
    QSpatialSound sound(&engine);
    sound.setLoops(QSpatialSound::Infinite);
    sound.setAutoPlay(true);

    engine.start();
    waitUntilLoaded(sound, QUrl::fromLocalFile(m_sine440Wav->fileName()));

    QRtAudioEngine *rt = playbackEngineOf(engine);
    QVERIFY(rt);

    for (int slice = 0; slice != 8; ++slice)
        pumpOneSlice(*rt);
    QVERIFY(!isSilent(pumpOneSlice(*rt)));

    engine.setPaused(true);
    for (int slice = 0; slice != 4; ++slice)
        QVERIFY(isSilent(pumpOneSlice(*rt)));
}

void tst_QAudioEngineWithPlayer::twoSourcesMixAdditively()
{
    QAudioEngine engine(sampleRate);
    QSpatialSound soundA(&engine);
    soundA.setLoops(QSpatialSound::Infinite);
    soundA.setAutoPlay(true);
    soundA.setPosition({ -1.f, 0.f, 0.f });

    engine.start();
    waitUntilLoaded(soundA, QUrl::fromLocalFile(m_sine300Wav->fileName()));

    QRtAudioEngine *rt = playbackEngineOf(engine);
    QVERIFY(rt);

    for (int slice = 0; slice != 8; ++slice)
        pumpOneSlice(*rt);

    double energyOneSource = 0;
    for (int slice = 0; slice != 32; ++slice)
        energyOneSource += energy(pumpOneSlice(*rt));
    QVERIFY(energyOneSource > 0.0);

    QSpatialSound soundB(&engine);
    soundB.setLoops(QSpatialSound::Infinite);
    soundB.setAutoPlay(true);
    soundB.setPosition({ 1.f, 0.f, 0.f });
    waitUntilLoaded(soundB, QUrl::fromLocalFile(m_sine900Wav->fileName()));

    for (int slice = 0; slice != 8; ++slice)
        pumpOneSlice(*rt);

    double energyTwoSources = 0;
    for (int slice = 0; slice != 32; ++slice)
        energyTwoSources += energy(pumpOneSlice(*rt));

    // sanity check of resonance-audio's own internal multi-source mixing (both QSpatialSounds
    // are mixed inside the same QResonanceAudioPlayer voice, not across separate voices, so this
    // does not exercise QRtAudioEngineVoice::play()'s additive-mixing contract; see
    // tst_qrtaudioengine.cpp for that)
    QVERIFY(energyTwoSources > 1.5 * energyOneSource);

    soundA.stop();
    soundB.stop();
    for (int slice = 0; slice != 4; ++slice)
        QVERIFY(isSilent(pumpOneSlice(*rt)));
}

// coordinate system, per qaudioengine.cpp: +x = right, +y = up, +z = backwards (behind the
// listener); the default listener sits at the origin facing -z, and no QAudioRoom is created
// here, so these tests see plain anechoic distance/direction attenuation

void tst_QAudioEngineWithPlayer::volumeDecreasesWithDistance()
{
    // distances well beyond QSpatialSound's default "size" (near-field reference distance) of
    // 0.1 m, so plain logarithmic distance attenuation dominates rather than near-field gain
    QAudioEngine nearEngine(sampleRate);
    QSpatialSound nearSound(&nearEngine);
    nearSound.setLoops(QSpatialSound::Infinite);
    nearSound.setAutoPlay(true);
    nearSound.setPosition({ 0.f, 0.f, -100.f });

    nearEngine.start();
    waitUntilLoaded(nearSound, QUrl::fromLocalFile(m_sine2000Wav->fileName()));

    QRtAudioEngine *nearRt = playbackEngineOf(nearEngine);
    QVERIFY(nearRt);
    const ChannelPeaks nearPeaks = measureChannelPeaks(*nearRt);

    QAudioEngine farEngine(sampleRate);
    QSpatialSound farSound(&farEngine);
    farSound.setLoops(QSpatialSound::Infinite);
    farSound.setAutoPlay(true);
    farSound.setPosition({ 0.f, 0.f, -1000.f });

    farEngine.start();
    waitUntilLoaded(farSound, QUrl::fromLocalFile(m_sine2000Wav->fileName()));

    QRtAudioEngine *farRt = playbackEngineOf(farEngine);
    QVERIFY(farRt);
    const ChannelPeaks farPeaks = measureChannelPeaks(*farRt);

    QVERIFY(std::max(nearPeaks.left, nearPeaks.right) > std::max(farPeaks.left, farPeaks.right));
}

void tst_QAudioEngineWithPlayer::positionPansToTheCorrectChannel()
{
    QAudioEngine leftEngine(sampleRate);
    QSpatialSound leftSound(&leftEngine);
    leftSound.setLoops(QSpatialSound::Infinite);
    leftSound.setAutoPlay(true);
    leftSound.setPosition({ -1.f, 0.f, 0.f });

    leftEngine.start();
    waitUntilLoaded(leftSound, QUrl::fromLocalFile(m_sine2000Wav->fileName()));

    QRtAudioEngine *leftRt = playbackEngineOf(leftEngine);
    QVERIFY(leftRt);
    const ChannelPeaks leftPeaks = measureChannelPeaks(*leftRt);
    QVERIFY(leftPeaks.left > leftPeaks.right);

    QAudioEngine rightEngine(sampleRate);
    QSpatialSound rightSound(&rightEngine);
    rightSound.setLoops(QSpatialSound::Infinite);
    rightSound.setAutoPlay(true);
    rightSound.setPosition({ 1.f, 0.f, 0.f });

    rightEngine.start();
    waitUntilLoaded(rightSound, QUrl::fromLocalFile(m_sine2000Wav->fileName()));

    QRtAudioEngine *rightRt = playbackEngineOf(rightEngine);
    QVERIFY(rightRt);
    const ChannelPeaks rightPeaks = measureChannelPeaks(*rightRt);
    QVERIFY(rightPeaks.right > rightPeaks.left);
}

void tst_QAudioEngineWithPlayer::frontAndBackAreCenterPanned()
{
    // left/right should be (near-)equal for a source directly ahead of or behind the listener,
    // unlike the hard left/right bias verified in positionPansToTheCorrectChannel()
    auto isCenterPanned = [](const ChannelPeaks &peaks) {
        return std::abs(peaks.left - peaks.right) < 0.01f * std::max(peaks.left, peaks.right);
    };

    QAudioEngine frontEngine(sampleRate);
    QSpatialSound frontSound(&frontEngine);
    frontSound.setLoops(QSpatialSound::Infinite);
    frontSound.setAutoPlay(true);
    frontSound.setPosition({ 0.f, 0.f, -1.f });

    frontEngine.start();
    waitUntilLoaded(frontSound, QUrl::fromLocalFile(m_sine2000Wav->fileName()));

    QRtAudioEngine *frontRt = playbackEngineOf(frontEngine);
    QVERIFY(frontRt);
    QVERIFY(isCenterPanned(measureChannelPeaks(*frontRt)));

    QAudioEngine backEngine(sampleRate);
    QSpatialSound backSound(&backEngine);
    backSound.setLoops(QSpatialSound::Infinite);
    backSound.setAutoPlay(true);
    backSound.setPosition({ 0.f, 0.f, 1.f });

    backEngine.start();
    waitUntilLoaded(backSound, QUrl::fromLocalFile(m_sine2000Wav->fileName()));

    QRtAudioEngine *backRt = playbackEngineOf(backEngine);
    QVERIFY(backRt);
    QVERIFY(isCenterPanned(measureChannelPeaks(*backRt)));
}

QTEST_MAIN(tst_QAudioEngineWithPlayer)
#include "tst_qaudioenginewithplayer.moc"
