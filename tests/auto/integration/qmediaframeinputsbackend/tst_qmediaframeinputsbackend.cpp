// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <qvideoframeinput.h>
#include <qaudiobufferinput.h>
#include <qmediacapturesession.h>
#include <qsignalspy.h>
#include <qmediarecorder.h>
#include <qmediaplayer.h>
#include <qaudiooutput.h>
#include <../shared/testvideosink.h>

QT_USE_NAMESPACE

using VideoFrameModifier = std::function<void(QVideoFrame &, int index)>;

struct FrameRateSetter
{
    void operator()(QVideoFrame &frame, int) const { frame.setStreamFrameRate(frameRate); }

    qreal frameRate = 0.;
};

struct TimeStampsSetter
{
    void operator()(QVideoFrame &frame, int index) const
    {
        frame.setStartTime(static_cast<qint64>((index + 1) * 1000000 / frameRate));
        frame.setEndTime(static_cast<qint64>((index + 2) * 1000000 / frameRate));
    }

    qreal frameRate = 0.;
};

class tst_QMediaFrameInputsBackend : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void mediaRecorderWritesAudio_whenAudioFramesInputSends_data();
    void mediaRecorderWritesAudio_whenAudioFramesInputSends();

    void mediaRecorderWritesVideo_whenVideoFramesInputSendsFrames_data();
    void mediaRecorderWritesVideo_whenVideoFramesInputSendsFrames();

    void mediaRecorderStopsRecording_whenInputsReportedEndOfsteream_data();
    void mediaRecorderStopsRecording_whenInputsReportedEndOfsteream();
};

void tst_QMediaFrameInputsBackend::initTestCase()
{
    qRegisterMetaType<VideoFrameModifier>();
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesAudio_whenAudioFramesInputSends_data()
{
    QTest::addColumn<int>("buffersNumber");
    QTest::addColumn<QAudioFormat::SampleFormat>("sampleFormat");
    QTest::addColumn<QAudioFormat::ChannelConfig>("channelConfig");
    QTest::addColumn<int>("sampleRate");
    QTest::addColumn<int>("duration");

#ifndef Q_OS_WINDOWS // sample rate 8000 is not supported. TODO: investigate.
    QTest::addRow("buffersNumber: 20; sampleFormat: Int16; channelConfig: Mono; sampleRate: 8000; duration: 1000")
            << 20 << QAudioFormat::Int16 << QAudioFormat::ChannelConfigMono << 8000 << 1000;
#endif
    QTest::addRow("buffersNumber: 30; sampleFormat: Int32; channelConfig: Stereo; sampleRate: 12000; duration: 2000")
            << 30 << QAudioFormat::Int32 << QAudioFormat::ChannelConfigStereo << 12000 << 2000;

    // TODO: investigate fails of channels configuration
    //   QTest::addRow("buffersNumber: 10; sampleFormat: UInt8; channelConfig: 2Dot1; sampleRate: 40000; duration: 1500")
    //           << 10 << QAudioFormat::UInt8 << QAudioFormat::ChannelConfig2Dot1 << 40000 << 1500;
    //   QTest::addRow("buffersNumber: 10; sampleFormat: Float; channelConfig: 3Dot0; sampleRate: 50000; duration: 2500")
    //           << 40 << QAudioFormat::Float << QAudioFormat::ChannelConfig3Dot0 << 50000 << 2500;
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesAudio_whenAudioFramesInputSends()
{
    QFETCH(const int, buffersNumber);
    QFETCH(const QAudioFormat::SampleFormat, sampleFormat);
    QFETCH(const QAudioFormat::ChannelConfig, channelConfig);
    QFETCH(const int, sampleRate);
    QFETCH(const int, duration);

    QAudioBufferInput audioInput;
    QMediaCaptureSession session;
    QMediaRecorder recorder;

    session.setRecorder(&recorder);
    session.setAudioBufferInput(&audioInput);

    QAudioFormat format;
    format.setSampleFormat(sampleFormat);
    format.setSampleRate(sampleRate);
    format.setChannelConfig(channelConfig);

    int audioBufferIndex = 0;
    auto onReadyToSendAudioBuffer = [&]() {
        QCOMPARE_LE(audioBufferIndex, buffersNumber);

        QByteArray data(format.bytesForDuration(duration * 1000 / buffersNumber), '\0');
        QAudioBuffer buffer(data, format);
        audioInput.sendAudioBuffer(buffer);

        ++audioBufferIndex;

        if (audioBufferIndex == buffersNumber)
            recorder.stop();
    };

    connect(&audioInput, &QAudioBufferInput::readyToSendAudioBuffer, this,
            onReadyToSendAudioBuffer);

    recorder.record();
    QCOMPARE(recorder.recorderState(), QMediaRecorder::RecordingState);

    const QString fileLocation = recorder.actualLocation().toLocalFile();
    QScopeGuard fileDeleteGuard([&]() { QFile::remove(fileLocation); });

    QVERIFY(QFile::exists(fileLocation));

    QTRY_COMPARE(recorder.recorderState(), QMediaRecorder::StoppedState);
    QCOMPARE(recorder.errorString(), u"");
    QCOMPARE(recorder.error(), QMediaRecorder::NoError);

    QAudioOutput audioOutput;
    QMediaPlayer player;
    player.setSource(fileLocation);
    player.setAudioOutput(&audioOutput);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QVERIFY(player.hasAudio());
    QCOMPARE_GE(player.duration(), duration - 50);
    QCOMPARE_LE(player.duration(), duration + 50);
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesVideo_whenVideoFramesInputSendsFrames_data()
{
    QTest::addColumn<int>("framesNumber");
    QTest::addColumn<qreal>("frameRate");
    QTest::addColumn<QSize>("resolution");
    QTest::addColumn<VideoFrameModifier>("videoFrameModifier");

    QTest::addRow("framesNumber: 5; frameRate: 2; resolution: 50x80; with time stamps")
            << 5 << 2. << QSize(50, 80) << VideoFrameModifier(TimeStampsSetter{ 2. });
    QTest::addRow("framesNumber: 20; frameRate: 1; resolution: 200x100; with time stamps")
            << 20 << 1. << QSize(200, 100) << VideoFrameModifier(TimeStampsSetter{ 1. });

    QTest::addRow("framesNumber: 20; frameRate: 30; resolution: 200x100; with frame rate")
            << 20 << 4. << QSize(200, 100) << VideoFrameModifier(FrameRateSetter{ 4. });
    QTest::addRow("framesNumber: 60; frameRate: 4; resolution: 200x100; with frame rate")
            << 60 << 30. << QSize(200, 100) << VideoFrameModifier(TimeStampsSetter{ 30. });
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesVideo_whenVideoFramesInputSendsFrames()
{
    QFETCH(const int, framesNumber);
    QFETCH(const qreal, frameRate);
    QFETCH(const QSize, resolution);
    QFETCH(const VideoFrameModifier, videoFrameModifier);

    QVideoFrameInput videoInput;
    QMediaCaptureSession session;
    QMediaRecorder recorder;
    recorder.setQuality(QMediaRecorder::VeryHighQuality);

    session.setRecorder(&recorder);
    session.setVideoFrameInput(&videoInput);

    QList<QColor> colors = { Qt::red, Qt::green, Qt::blue, Qt::black, Qt::white };

    int frameIndex = 0;
    auto onReadyToSendVideoFrame = [&]() {
        QCOMPARE_LE(frameIndex, framesNumber);

        QImage image(resolution, QImage::Format_ARGB32);
        image.fill(colors[frameIndex % colors.size()]);
        QVideoFrame frame(std::move(image));
        videoFrameModifier(frame, frameIndex);
        videoInput.sendVideoFrame(std::move(frame));

        ++frameIndex;

        if (frameIndex == framesNumber)
            recorder.stop();
    };

    connect(&videoInput, &QVideoFrameInput::readyToSendVideoFrame, this, onReadyToSendVideoFrame);

    recorder.record();
    QCOMPARE(recorder.recorderState(), QMediaRecorder::RecordingState);

    const QString fileLocation = recorder.actualLocation().toLocalFile();
    QScopeGuard fileDeleteGuard([&]() { QFile::remove(fileLocation); });

    QTRY_COMPARE(recorder.recorderState(), QMediaRecorder::StoppedState);
    QCOMPARE(frameIndex, framesNumber);

    QVERIFY(QFile::exists(fileLocation));

    TestVideoSink sink;
    QMediaPlayer player;
    player.setSource(fileLocation);
    player.setVideoSink(&sink);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    const qreal actualFrameRate = player.metaData().value(QMediaMetaData::VideoFrameRate).toReal();
    // TODO: investigate the frame rate difference
    QCOMPARE_GE(actualFrameRate, frameRate);
    QCOMPARE_LE(actualFrameRate, qMax(frameRate * 1.07, frameRate + 0.7));

    // TODO: fix it, the duration should be framesNumber * 1000 / frameRate.
    // The first frame seems to be dropped by FFmpeg
    QCOMPARE_GE(player.duration(), static_cast<qint64>((framesNumber - 1) * 1000 / frameRate));
    QCOMPARE_LE(player.duration(), static_cast<qint64>(framesNumber * 1000 / frameRate));

    QCOMPARE(player.metaData().value(QMediaMetaData::Resolution), resolution);

    player.setPlaybackRate(50); // let's speed it up
    player.play();

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);

    // The first frame seems to be dropped by FFmpeg; Fix it!
    QCOMPARE_GE(sink.m_totalFrames, framesNumber - 1);
    QCOMPARE_LE(sink.m_totalFrames, framesNumber + 1);
}

void tst_QMediaFrameInputsBackend::mediaRecorderStopsRecording_whenInputsReportedEndOfsteream_data()
{
    QTest::addColumn<bool>("audioStopsFirst");

    QTest::addRow("audio stops first") << true;
    QTest::addRow("video stops first") << true;
}

void tst_QMediaFrameInputsBackend::mediaRecorderStopsRecording_whenInputsReportedEndOfsteream()
{
    QFETCH(const bool, audioStopsFirst);

    QVideoFrameInput videoInput;
    QAudioBufferInput audioInput;
    QMediaCaptureSession session;
    QMediaRecorder recorder;

    recorder.setAutoStop(true);

    session.setRecorder(&recorder);
    session.setVideoFrameInput(&videoInput);
    session.setAudioBufferInput(&audioInput);

    int videoFrameIndex = 0;
    auto onReadyToSendVideoFrame = [&]() {
        if (videoFrameIndex == 30)
            return;

        QImage image(QSize(200, 100), QImage::Format_ARGB32);
        image.fill(Qt::blue);
        QVideoFrame frame(std::move(image));
        frame.setStreamFrameRate(30);
        videoInput.sendVideoFrame(std::move(frame));

        ++videoFrameIndex;
    };

    int audioBufferIndex = 0;
    auto onReadyToSendAudioBuffer = [&]() {
        if (audioBufferIndex == 30)
            return;

        QAudioFormat format;
        format.setSampleFormat(QAudioFormat::UInt8);
        format.setSampleRate(8000);
        format.setChannelConfig(QAudioFormat::ChannelConfigMono);
        QByteArray data(format.bytesForDuration(1000000 / 30), '\0');
        QAudioBuffer buffer(data, format);
        audioInput.sendAudioBuffer(buffer);

        ++audioBufferIndex;
    };

    connect(&videoInput, &QVideoFrameInput::readyToSendVideoFrame, this, onReadyToSendVideoFrame);
    connect(&audioInput, &QAudioBufferInput::readyToSendAudioBuffer, this,
            onReadyToSendAudioBuffer);

    recorder.record();

    QCOMPARE(recorder.recorderState(), QMediaRecorder::RecordingState);

    const QString fileLocation = recorder.actualLocation().toLocalFile();
    QScopeGuard fileDeleteGuard([&]() { QFile::remove(fileLocation); });

    QTRY_COMPARE(audioBufferIndex, 30);
    QTRY_COMPARE(videoFrameIndex, 30);

    auto sendEndOfStream = [&](bool isAudio) {
        return isAudio ? audioInput.sendAudioBuffer({}) : videoInput.sendVideoFrame({});
    };

    QVERIFY(sendEndOfStream(audioStopsFirst));

    // wait a bit to give the recaorder a chance to fail if smth went wrong
    QTest::qWait(300);
    QCOMPARE_NE(recorder.recorderState(), QMediaRecorder::StoppedState);

    QVERIFY(sendEndOfStream(!audioStopsFirst));
    QTRY_COMPARE(recorder.recorderState(), QMediaRecorder::StoppedState);

    // check if the file has been written

    TestVideoSink sink;
    QMediaPlayer player;
    player.setSource(fileLocation);
    player.setVideoSink(&sink);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(player.hasAudio());
    QVERIFY(player.hasVideo());
}

QTEST_MAIN(tst_QMediaFrameInputsBackend)

#include "tst_qmediaframeinputsbackend.moc"
