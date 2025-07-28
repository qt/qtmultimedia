// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtCore/qsemaphore.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <QtMultimedia/private/qrtaudioengine_p.h>
#include <QtMultimediaTestLib/private/osdetection_p.h>

#if __cplusplus > 201703L
#  define QT_SET_HAS_CONTAINS
#endif

QT_USE_NAMESPACE
using namespace Qt::Literals;
using namespace std::chrono_literals;

// NOLINTBEGIN(readability-convert-member-functions-to-static)
// NOLINTBEGIN(readability-redundant-access-specifiers)

using QtMultimediaPrivate::QRtAudioEngine;
using QtMultimediaPrivate::QRtAudioEngineVoice;
using QtMultimediaPrivate::VoiceId;
using QtMultimediaPrivate::VoicePlayResult;

namespace ranges = QtMultimediaPrivate::ranges;

namespace {

struct NullMockVoice : public QRtAudioEngineVoice
{
    NullMockVoice(QAudioFormat fmt, VoiceId id) : QRtAudioEngineVoice(id), m_format(fmt) { }
    ~NullMockVoice() override = default;

    bool isActive() noexcept QT_MM_NONBLOCKING override { return m_active; }
    const QAudioFormat &format() noexcept override { return m_format; }
    VoicePlayResult play(QSpan<float> buffer) noexcept QT_MM_NONBLOCKING override
    {
        m_sampleCount += buffer.size();
        ranges::fill(buffer, 0);
        return m_result;
    }

    void setActive(bool v) { m_active = v; }
    void setVoicePlayResult(VoicePlayResult v) { m_result = v; }

    // data
    const QAudioFormat m_format;
    bool m_active = true;
    uint64_t m_sampleCount{};
    VoicePlayResult m_result = VoicePlayResult::Playing;
};

} // namespace

static bool isPipewireBackend()
{
    return QPlatformMediaIntegration::audioBackendName() == "PipeWire";
}

static bool isPulseAudioBackend()
{
    return QPlatformMediaIntegration::audioBackendName() == "PulseAudio";
}

class tst_QRtAudioEngine : public QObject
{
    Q_OBJECT
public:
    tst_QRtAudioEngine(QObject *parent = nullptr) : QObject(parent) { }

private slots:
    void initTestCase()
    {
        if (QMediaDevices::defaultAudioOutput().isNull())
            QSKIP("No audio outputs found");

        if (isMacOS || isWindows || isPipewireBackend() || isPulseAudioBackend())
            return;
        QSKIP("Skipping QRtAudioEngine tests on this platform: callback interface is not "
              "supported");
    }

    void getPlaybackEngine();
    void getPlaybackEngine_nullDevice();
    void getPlaybackEngine_invalidFormat();

    void play_visit_and_stop_voice();
    void play_and_stop_byInactive();
    void play_and_stop_byPlaybackStatus();

    void play_validateDuration();
    void visit_overloadCommandBuffers();

private:
    // helpers
    static QAudioFormat getFormat()
    {
        QAudioDevice device = QMediaDevices::defaultAudioOutput();
        auto format = device.preferredFormat();
        format.setSampleFormat(QAudioFormat::Float);
        if (format.channelCount() > 2) {
            format.setChannelCount(2);
            format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
        }
        return format;
    }

    static std::shared_ptr<QRtAudioEngine> makeEngine()
    {
        return QRtAudioEngine::getEngineFor(QMediaDevices::defaultAudioOutput(), getFormat());
    }

    static std::shared_ptr<NullMockVoice> makeMockVoice(QAudioFormat fmt)
    {
        auto voice = std::make_shared<NullMockVoice>(fmt, QRtAudioEngine::allocateVoiceId());
        return voice;
    }
};

void tst_QRtAudioEngine::getPlaybackEngine()
{
    std::shared_ptr<QRtAudioEngine> engine = makeEngine();
    std::shared_ptr<QRtAudioEngine> engine2 = makeEngine();

    QCOMPARE(engine, engine2);

    // the sink starts suspended
    QCOMPARE(engine->audioSink().state(), QAudio::SuspendedState);
}

void tst_QRtAudioEngine::getPlaybackEngine_nullDevice()
{
    QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                         "QRtAudioEngine needs to be called with a valid device");

    std::shared_ptr<QRtAudioEngine> engine =
            QRtAudioEngine::getEngineFor(QAudioDevice(), QAudioFormat());
    QCOMPARE(engine, nullptr);
}

void tst_QRtAudioEngine::getPlaybackEngine_invalidFormat()
{
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QRtAudioEngine requires floating point samples");

    std::shared_ptr<QRtAudioEngine> engine =
            QRtAudioEngine::getEngineFor(QMediaDevices::defaultAudioOutput(), QAudioFormat());
    QCOMPARE(engine, nullptr);
}

void tst_QRtAudioEngine::play_visit_and_stop_voice()
{
    std::shared_ptr<QRtAudioEngine> engine = makeEngine();

    auto voice = makeMockVoice(getFormat());
    engine->play(voice);
    QCOMPARE(engine->voices().size(), 1);
#ifdef QT_SET_HAS_CONTAINS
    QVERIFY(engine->voices().contains(voice));
#endif

    QSemaphore sem;
    engine->visitVoiceRt(voice, [&sem](NullMockVoice &) {
        sem.release();
    });

    sem.acquire();

    QSignalSpy finishedSpy(engine.get(), &QRtAudioEngine::voiceFinished);
    engine->stop(voice);

    finishedSpy.wait();
    QCOMPARE(finishedSpy.front().front().value<VoiceId>(), voice->voiceId());
    QCOMPARE(engine->voices().size(), 0);
}

void tst_QRtAudioEngine::play_and_stop_byInactive()
{
    std::shared_ptr<QRtAudioEngine> engine = makeEngine();

    auto voice = makeMockVoice(getFormat());
    engine->play(voice);
    QCOMPARE(engine->voices().size(), 1);

    QSignalSpy finishedSpy(engine.get(), &QRtAudioEngine::voiceFinished);
    engine->visitVoiceRt(voice, [](NullMockVoice &voice) {
        voice.setActive(false);
    });

    finishedSpy.wait();
    QCOMPARE(finishedSpy.front().front().value<VoiceId>(), voice->voiceId());
    QCOMPARE(engine->voices().size(), 0);
}

void tst_QRtAudioEngine::play_and_stop_byPlaybackStatus()
{
    std::shared_ptr<QRtAudioEngine> engine = makeEngine();

    auto voice = makeMockVoice(getFormat());
    engine->play(voice);
    QCOMPARE(engine->voices().size(), 1);

    QSignalSpy finishedSpy(engine.get(), &QRtAudioEngine::voiceFinished);
    engine->visitVoiceRt(voice, [](NullMockVoice &voice) {
        voice.setVoicePlayResult(VoicePlayResult::Finished);
    });

    finishedSpy.wait();
    QCOMPARE(finishedSpy.front().front().value<VoiceId>(), voice->voiceId());
    QCOMPARE(engine->voices().size(), 0);
}

void tst_QRtAudioEngine::play_validateDuration()
{
    std::shared_ptr<QRtAudioEngine> engine = makeEngine();

    auto voice = makeMockVoice(getFormat());
    QSignalSpy finishedSpy(engine.get(), &QRtAudioEngine::voiceFinished);
    engine->play(voice);

    QTest::qWait(1s);

    engine->stop(voice);
    finishedSpy.wait();

    auto samplesForDuration = [&](auto duration) -> uint64_t {
        auto format = getFormat();
        return format.framesForDuration(std::chrono::microseconds(duration).count())
                * format.channelCount();
    };

    auto lowerBound = samplesForDuration(600ms);
    auto upperBound = samplesForDuration(1'400ms);

    QCOMPARE_LT(lowerBound, voice->m_sampleCount);
    QCOMPARE_LT(voice->m_sampleCount, upperBound);
}

void tst_QRtAudioEngine::visit_overloadCommandBuffers()
{
    std::shared_ptr<QRtAudioEngine> engine = makeEngine();

    auto voice = makeMockVoice(getFormat());
    QSignalSpy finishedSpy(engine.get(), &QRtAudioEngine::voiceFinished);
    engine->play(voice);

    constexpr size_t commandCount = 16384; // 8 times the command buffer size

    size_t iteration = 0;
    auto refcountedDummyObject = std::make_shared<int>(0);
    for (size_t i = 0; i < commandCount; ++i) {
        engine->visitVoiceRt(
                voice,
                [&, payload = std::array<char, 128>{}, refcountedDummyObject](NullMockVoice &) {
            Q_UNUSED(payload);
            iteration += 1;
        });
    }

    QTRY_COMPARE_EQ(iteration, commandCount); // all commands should have been executed
    QTRY_COMPARE_EQ(refcountedDummyObject.use_count(),
                    1); // all commands should have been destroyed on the app thread
}

QTEST_MAIN(tst_QRtAudioEngine)

#include "tst_qrtaudioengine.moc"
