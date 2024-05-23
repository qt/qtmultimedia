// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "capturesessionfixture.h"
#include "tst_qmediaframeinputsbackend.h"

#include "mediainfo.h"
#include <QtTest/QtTest>
#include <qvideoframeinput.h>
#include <qaudiobufferinput.h>
#include <qsignalspy.h>
#include <qmediarecorder.h>
#include <qmediaplayer.h>
#include <../shared/testvideosink.h>
#include <../shared/mediabackendutils.h>

QT_BEGIN_NAMESPACE

void tst_QMediaFrameInputsBackend::initTestCase()
{
    QSKIP_GSTREAMER("Not implemented in the gstreamer backend");
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesAudio_whenAudioFramesInputSends_data()
{
    QTest::addColumn<int>("bufferCount");
    QTest::addColumn<QAudioFormat::SampleFormat>("sampleFormat");
    QTest::addColumn<QAudioFormat::ChannelConfig>("channelConfig");
    QTest::addColumn<int>("sampleRate");
    QTest::addColumn<milliseconds>("duration");

#ifndef Q_OS_WINDOWS // sample rate 8000 is not supported. TODO: investigate.
    QTest::addRow("bufferCount: 20; sampleFormat: Int16; channelConfig: Mono; sampleRate: 8000; "
                  "duration: 1000")
            << 20 << QAudioFormat::Int16 << QAudioFormat::ChannelConfigMono << 8000 << 1000ms;
#endif
    QTest::addRow("bufferCount: 30; sampleFormat: Int32; channelConfig: Stereo; sampleRate: "
                  "12000; duration: 2000")
            << 30 << QAudioFormat::Int32 << QAudioFormat::ChannelConfigStereo << 12000 << 2000ms;

    // TODO: investigate fails of channels configuration
    //   QTest::addRow("bufferCount: 10; sampleFormat: UInt8; channelConfig: 2Dot1; sampleRate:
    //   40000; duration: 1500")
    //           << 10 << QAudioFormat::UInt8 << QAudioFormat::ChannelConfig2Dot1 << 40000 << 1500;
    //   QTest::addRow("bufferCount: 10; sampleFormat: Float; channelConfig: 3Dot0; sampleRate:
    //   50000; duration: 2500")
    //           << 40 << QAudioFormat::Float << QAudioFormat::ChannelConfig3Dot0 << 50000 << 2500;
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesAudio_whenAudioFramesInputSends()
{
    QFETCH(const int, bufferCount);
    QFETCH(const QAudioFormat::SampleFormat, sampleFormat);
    QFETCH(const QAudioFormat::ChannelConfig, channelConfig);
    QFETCH(const int, sampleRate);
    QFETCH(const milliseconds, duration);

    CaptureSessionFixture f{ StreamType::Audio, AutoStop::EmitEmpty };
    f.connectPullMode();

    QAudioFormat format;
    format.setSampleFormat(sampleFormat);
    format.setSampleRate(sampleRate);
    format.setChannelConfig(channelConfig);

    f.m_audioGenerator.setFormat(format);
    f.m_audioGenerator.setBufferCount(bufferCount);
    f.m_audioGenerator.setDuration(duration);

    f.m_recorder.record();

    QVERIFY(f.waitForRecorderStopped(60s));

    auto info = MediaInfo::create(f.m_recorder.actualLocation());

    QVERIFY(info->m_hasAudio);
    QCOMPARE_GE(info->m_duration, duration - 50ms);
    QCOMPARE_LE(info->m_duration, duration + 50ms);
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesVideo_whenVideoFramesInputSendsFrames_data()
{
    QTest::addColumn<int>("framesNumber");
    QTest::addColumn<milliseconds>("frameDuration");
    QTest::addColumn<QSize>("resolution");
    QTest::addColumn<bool>("setTimeStamp");

    QTest::addRow("framesNumber: 5; frameRate: 2; resolution: 50x80; with time stamps")
            << 5 << 500ms << QSize(50, 80) << true;
    QTest::addRow("framesNumber: 20; frameRate: 1; resolution: 200x100; with time stamps")
            << 20 << 1000ms << QSize(200, 100) << true;

    QTest::addRow("framesNumber: 20; frameRate: 30; resolution: 200x100; with frame rate")
            << 20 << 250ms << QSize(200, 100) << false;
    QTest::addRow("framesNumber: 60; frameRate: 4; resolution: 200x100; with frame rate")
            << 60 << 24ms << QSize(200, 100) << false;
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesVideo_whenVideoFramesInputSendsFrames()
{
    QFETCH(const int, framesNumber);
    QFETCH(const milliseconds, frameDuration);
    QFETCH(const QSize, resolution);
    QFETCH(const bool, setTimeStamp);

    CaptureSessionFixture f{ StreamType::Video, AutoStop::EmitEmpty };
    f.connectPullMode();
    f.m_videoGenerator.setFrameCount(framesNumber);
    f.m_videoGenerator.setSize(resolution);

    const double frameRate = 1e6 / duration_cast<microseconds>(frameDuration).count();
    if (setTimeStamp)
        f.m_videoGenerator.setPeriod(frameDuration);
    else
        f.m_videoGenerator.setFrameRate(frameRate);

    f.m_recorder.record();

    QVERIFY(f.waitForRecorderStopped(60s));

    auto info = MediaInfo::create(f.m_recorder.actualLocation());

    const qreal actualFrameRate = info->m_frameRate;
    // TODO: investigate the frame rate difference
    QCOMPARE_GE(actualFrameRate, frameRate);
    QCOMPARE_LE(actualFrameRate, qMax(frameRate * 1.07, frameRate + 0.7));

    // TODO: fix it, the duration should be framesNumber * 1000 / frameRate.
    // The first frame seems to be dropped by FFmpeg
    QCOMPARE_GE(info->m_duration, frameDuration * (framesNumber - 1));
    QCOMPARE_LE(info->m_duration, frameDuration * framesNumber);

    QCOMPARE(info->m_size, resolution);

    // The first frame seems to be dropped by FFmpeg; Fix it!
    QCOMPARE_GE(info->m_frameCount, framesNumber - 1);
    QCOMPARE_LE(info->m_frameCount, framesNumber + 1);
}

void tst_QMediaFrameInputsBackend::mediaRecorderStopsRecording_whenInputsReportedEndOfStream_data()
{
    QTest::addColumn<bool>("audioStopsFirst");

    QTest::addRow("audio stops first") << true;
    QTest::addRow("video stops first") << true;
}

void tst_QMediaFrameInputsBackend::mediaRecorderStopsRecording_whenInputsReportedEndOfStream()
{
    QFETCH(const bool, audioStopsFirst);

    CaptureSessionFixture f{ StreamType::AudioAndVideo, AutoStop::No };
    f.m_recorder.setAutoStop(true);
    f.connectPullMode();

    f.m_audioGenerator.setBufferCount(30);
    f.m_videoGenerator.setFrameCount(30);

    QSignalSpy audioDone{ &f.m_audioGenerator, &AudioGenerator::done };
    QSignalSpy videoDone{ &f.m_videoGenerator, &VideoGenerator::done };

    f.m_recorder.record();

    audioDone.wait();
    videoDone.wait();

    if (audioStopsFirst) {
        f.m_audioInput.sendAudioBuffer({});
        QVERIFY(!f.waitForRecorderStopped(300ms)); // Should not stop until both streams stopped
        f.m_videoInput.sendVideoFrame({});
    } else {
        f.m_videoInput.sendVideoFrame({});
        QVERIFY(!f.waitForRecorderStopped(300ms)); // Should not stop until both streams stopped
        f.m_audioInput.sendAudioBuffer({});
    }

    QVERIFY(f.waitForRecorderStopped(60s));

    // check if the file has been written

    const std::optional<MediaInfo> mediaInfo = MediaInfo::create(f.m_recorder.actualLocation());

    QVERIFY(mediaInfo);
    QVERIFY(mediaInfo->m_hasVideo);
    QVERIFY(mediaInfo->m_hasAudio);
}

void tst_QMediaFrameInputsBackend::readyToSend_isEmitted_whenRecordingStarts_data()
{
    QTest::addColumn<StreamType>("streamType");
    QTest::addRow("audio") << StreamType::Audio;
    QTest::addRow("video") << StreamType::Video;
    QTest::addRow("audioAndVideo") << StreamType::AudioAndVideo;
}

void tst_QMediaFrameInputsBackend::readyToSend_isEmitted_whenRecordingStarts()
{
    QFETCH(StreamType, streamType);

    CaptureSessionFixture f{ streamType, AutoStop::No };

    f.m_recorder.record();

    if (f.hasAudio())
        QTRY_COMPARE_EQ(f.readyToSendAudioBuffer.size(), 1);

    if (f.hasVideo())
        QTRY_COMPARE_EQ(f.readyToSendVideoFrame.size(), 1);
}

void tst_QMediaFrameInputsBackend::readyToSendVideoFrame_isEmitted_whenSendVideoFrameIsCalled()
{
    CaptureSessionFixture f{ StreamType::Video, AutoStop::No };

    f.m_recorder.record();
    QVERIFY(f.readyToSendVideoFrame.wait());

    f.m_videoInput.sendVideoFrame(f.m_videoGenerator.createFrame());
    QVERIFY(f.readyToSendVideoFrame.wait());

    f.m_videoInput.sendVideoFrame(f.m_videoGenerator.createFrame());
    QVERIFY(f.readyToSendVideoFrame.wait());
}

void tst_QMediaFrameInputsBackend::readyToSendAudioBuffer_isEmitted_whenSendAudioBufferIsCalled()
{
    CaptureSessionFixture f{ StreamType::Audio, AutoStop::No };

    f.m_recorder.record();
    QVERIFY(f.readyToSendAudioBuffer.wait());

    f.m_audioInput.sendAudioBuffer(f.m_audioGenerator.createAudioBuffer());
    QVERIFY(f.readyToSendAudioBuffer.wait());

    f.m_audioInput.sendAudioBuffer(f.m_audioGenerator.createAudioBuffer());
    QVERIFY(f.readyToSendAudioBuffer.wait());
}

void tst_QMediaFrameInputsBackend::readyToSendVideoFrame_isEmittedRepeatedly_whenPullModeIsEnabled()
{
    CaptureSessionFixture f{ StreamType::Video, AutoStop::EmitEmpty };
    f.connectPullMode();

    constexpr int expectedSignalCount = 4;
    f.m_videoGenerator.setFrameCount(expectedSignalCount - 1);

    f.m_recorder.record();

    f.waitForRecorderStopped(60s);

    QCOMPARE_GE(f.readyToSendVideoFrame.size(), expectedSignalCount - 1);
    QCOMPARE_LE(f.readyToSendVideoFrame.size(), expectedSignalCount + 1);
}

void tst_QMediaFrameInputsBackend::
        readyToSendAudioBuffer_isEmittedRepeatedly_whenPullModeIsEnabled()
{
    CaptureSessionFixture f{ StreamType::Audio, AutoStop::EmitEmpty };
    f.connectPullMode();

    constexpr int expectedSignalCount = 4;
    f.m_audioGenerator.setBufferCount(expectedSignalCount - 1);

    f.m_recorder.record();

    f.waitForRecorderStopped(60s);

    QCOMPARE_GE(f.readyToSendAudioBuffer.size(), expectedSignalCount - 1);
    QCOMPARE_LE(f.readyToSendAudioBuffer.size(), expectedSignalCount + 1);
}

void tst_QMediaFrameInputsBackend::
        readyToSendAudioBufferAndVideoFrame_isEmittedRepeatedly_whenPullModeIsEnabled()
{
    CaptureSessionFixture f{ StreamType::AudioAndVideo, AutoStop::EmitEmpty };
    f.connectPullMode();

    constexpr int expectedSignalCount = 4;
    f.m_audioGenerator.setBufferCount(expectedSignalCount - 1);
    f.m_videoGenerator.setFrameCount(expectedSignalCount - 1);

    f.m_recorder.record();

    f.waitForRecorderStopped(60s);

    QCOMPARE_GE(f.readyToSendAudioBuffer.size(), expectedSignalCount - 1);
    QCOMPARE_LE(f.readyToSendAudioBuffer.size(), expectedSignalCount + 1);

    QCOMPARE_GE(f.readyToSendVideoFrame.size(), expectedSignalCount - 1);
    QCOMPARE_LE(f.readyToSendVideoFrame.size(), expectedSignalCount + 1);
}

QT_END_NAMESPACE

QT_USE_NAMESPACE

QTEST_MAIN(tst_QMediaFrameInputsBackend)

#include "moc_tst_qmediaframeinputsbackend.cpp"
