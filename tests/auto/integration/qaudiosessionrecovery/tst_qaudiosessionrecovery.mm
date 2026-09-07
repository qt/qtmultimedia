// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qaudiosink.h>
#include <QtMultimedia/qmediadevices.h>

#include <private/audiogenerationutils_p.h>
#include <private/mediabackendutils_p.h>
#include <private/osdetection_p.h>

#include <memory>
#include <optional>

#import <AVFoundation/AVAudioSession.h>
#import <Foundation/Foundation.h>

using namespace Qt::Literals;

namespace {

// Posts a synthetic AVAudioSessionInterruptionNotification. QAVAudioSessionMonitor observes
// object:[AVAudioSession sharedInstance], so a notification posted this way from within the test
// process reaches the exact same handler chain a real interruption would.
void postInterruptionNotification(
        AVAudioSessionInterruptionType type,
        std::optional<AVAudioSessionInterruptionOptions> options = std::nullopt)
{
    NSMutableDictionary<NSString *, id> *userInfo = [NSMutableDictionary dictionary];
    userInfo[AVAudioSessionInterruptionTypeKey] = @(type);
    if (options)
        userInfo[AVAudioSessionInterruptionOptionKey] = @(*options);

    [NSNotificationCenter.defaultCenter postNotificationName:AVAudioSessionInterruptionNotification
                                                      object:AVAudioSession.sharedInstance
                                                    userInfo:userInfo];
}

void postMediaServicesNotification(NSNotificationName name)
{
    [NSNotificationCenter.defaultCenter postNotificationName:name
                                                      object:AVAudioSession.sharedInstance
                                                    userInfo:nil];
}

// Waits until the sink reports either ActiveState or IdleState: a playing sink legitimately
// reports IdleState once the ringbuffer has been filled and briefly runs dry between callbacks,
// see QPlatformAudioEndpointBase::inferState().
void waitUntilActive(const QAudioSink &sink)
{
    QTRY_VERIFY2(sink.state() == QAudio::ActiveState || sink.state() == QAudio::IdleState,
                 qPrintable(u"sink did not become active, state: %1, error: %2"_s.arg(sink.state())
                                    .arg(sink.error())));
}

std::unique_ptr<QAudioSink> createStartedSink(QIODevice &source, const QAudioFormat &format)
{
    auto sink = std::make_unique<QAudioSink>(QMediaDevices::defaultAudioOutput(), format);
    sink->start(&source);
    waitUntilActive(*sink);
    return sink;
}

} // namespace

// Exercises QtMultimediaPrivate::QAVAudioSessionRecovery end to end, on a real QAudioSink, by
// posting synthetic AVAudioSession notifications: see postInterruptionNotification() and
// postMediaServicesNotification() above. This runs on the iOS Simulator without any audio
// hardware, because QAVAudioSessionMonitor only ever inspects the notification and never talks
// to actual hardware.
class tst_QAudioSessionRecovery : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void interruptionSuspendsStream();
    void interruptionEndedResumesStream();
    void interruptionEndedWithoutShouldResumeStaysSuspended();
    void mediaServicesResetReportsError();
    void mediaServicesLostReportsError();
    void userSuspendedStreamSurvivesInterruptionCycle();
};

void tst_QAudioSessionRecovery::initTestCase()
{
    if (QMediaDevices::defaultAudioOutput().isNull())
        QSKIP("No audio outputs found");
}

void tst_QAudioSessionRecovery::interruptionSuspendsStream()
{
    const QAudioFormat format = QMediaDevices::defaultAudioOutput().preferredFormat();
    SineWaveIODevice source(format);
    const std::unique_ptr<QAudioSink> sink = createStartedSink(source, format);

    const qsizetype bufferSize = sink->bufferSize();

    postInterruptionNotification(AVAudioSessionInterruptionTypeBegan);

    QTRY_COMPARE_EQ(sink->state(), QAudio::SuspendedState);
    QCOMPARE(sink->error(), QAudio::NoError);

    // The stream object must not have been torn down by the interruption: the sink is still
    // fully usable, just not currently producing sound.
    QCOMPARE(sink->bufferSize(), bufferSize);
    QVERIFY(sink->processedUSecs() >= 0);
}

void tst_QAudioSessionRecovery::interruptionEndedResumesStream()
{
    const QAudioFormat format = QMediaDevices::defaultAudioOutput().preferredFormat();
    SineWaveIODevice source(format);
    const std::unique_ptr<QAudioSink> sink = createStartedSink(source, format);

    postInterruptionNotification(AVAudioSessionInterruptionTypeBegan);
    QTRY_COMPARE_EQ(sink->state(), QAudio::SuspendedState);

    postInterruptionNotification(AVAudioSessionInterruptionTypeEnded,
                                 AVAudioSessionInterruptionOptionShouldResume);

    waitUntilActive(*sink);
    QCOMPARE(sink->error(), QAudio::NoError);
}

void tst_QAudioSessionRecovery::interruptionEndedWithoutShouldResumeStaysSuspended()
{
    const QAudioFormat format = QMediaDevices::defaultAudioOutput().preferredFormat();
    SineWaveIODevice source(format);
    const std::unique_ptr<QAudioSink> sink = createStartedSink(source, format);

    postInterruptionNotification(AVAudioSessionInterruptionTypeBegan);
    QTRY_COMPARE_EQ(sink->state(), QAudio::SuspendedState);

    // No AVAudioSessionInterruptionOptionKey at all: the absent-key case, which means "do not
    // resume".
    postInterruptionNotification(AVAudioSessionInterruptionTypeEnded);

    // Negative assertion: let any (same-thread, but potentially queued) delivery run its course,
    // then confirm the state did not change on its own.
    QTRY_COMPARE_EQ(sink->state(), QAudio::SuspendedState);
    QCOMPARE(sink->error(), QAudio::NoError);

    sink->resume();
    waitUntilActive(*sink);
    QCOMPARE(sink->error(), QAudio::NoError);
}

void tst_QAudioSessionRecovery::mediaServicesResetReportsError()
{
    const QAudioFormat format = QMediaDevices::defaultAudioOutput().preferredFormat();
    SineWaveIODevice source(format);
    const std::unique_ptr<QAudioSink> sink = createStartedSink(source, format);

    QSignalSpy stateSpy(sink.get(), &QAudioSink::stateChanged);

    postMediaServicesNotification(AVAudioSessionMediaServicesWereResetNotification);

    QTRY_COMPARE_EQ(sink->state(), QAudio::StoppedState);
    QVERIFY(!stateSpy.isEmpty());
    QCOMPARE(stateSpy.constLast().constFirst().value<QAudio::State>(), QAudio::StoppedState);
    QCOMPARE(sink->error(), QAudio::IOError);

    // Recovery is possible: a fresh sink can be started successfully afterwards.
    SineWaveIODevice recoverySource(format);
    const std::unique_ptr<QAudioSink> recoveredSink = createStartedSink(recoverySource, format);
    QCOMPARE(recoveredSink->error(), QAudio::NoError);
}

void tst_QAudioSessionRecovery::mediaServicesLostReportsError()
{
    const QAudioFormat format = QMediaDevices::defaultAudioOutput().preferredFormat();
    SineWaveIODevice source(format);
    const std::unique_ptr<QAudioSink> sink = createStartedSink(source, format);

    QSignalSpy stateSpy(sink.get(), &QAudioSink::stateChanged);

    postMediaServicesNotification(AVAudioSessionMediaServicesWereLostNotification);

    QTRY_COMPARE_EQ(sink->state(), QAudio::StoppedState);
    QVERIFY(!stateSpy.isEmpty());
    QCOMPARE(stateSpy.constLast().constFirst().value<QAudio::State>(), QAudio::StoppedState);
    QCOMPARE(sink->error(), QAudio::IOError);

    // Recovery is possible: a fresh sink can be started successfully afterwards.
    SineWaveIODevice recoverySource(format);
    const std::unique_ptr<QAudioSink> recoveredSink = createStartedSink(recoverySource, format);
    QCOMPARE(recoveredSink->error(), QAudio::NoError);
}

void tst_QAudioSessionRecovery::userSuspendedStreamSurvivesInterruptionCycle()
{
    const QAudioFormat format = QMediaDevices::defaultAudioOutput().preferredFormat();
    SineWaveIODevice source(format);
    const std::unique_ptr<QAudioSink> sink = createStartedSink(source, format);

    // An application-requested suspend must survive an OS interruption cycle that starts after
    // the stream was already suspended: QAVAudioSessionRecovery only tracks interruptions that
    // begin while the stream was Active/IdleState, see QAVAudioSessionRecovery::isStreamActive().
    sink->suspend();
    QCOMPARE(sink->state(), QAudio::SuspendedState);

    postInterruptionNotification(AVAudioSessionInterruptionTypeBegan);
    postInterruptionNotification(AVAudioSessionInterruptionTypeEnded,
                                 AVAudioSessionInterruptionOptionShouldResume);

    QTRY_COMPARE_EQ(sink->state(), QAudio::SuspendedState);
    QCOMPARE(sink->error(), QAudio::NoError);
}

QTEST_MAIN(tst_QAudioSessionRecovery)

#include "tst_qaudiosessionrecovery.moc"
