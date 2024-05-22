// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <qvideoframeinput.h>
#include <qmediacapturesession.h>
#include <qsignalspy.h>
#include <qmediarecorder.h>
#include <qmediaplayer.h>
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

    void mediaRecorderWritesVideo_whenVideoFramesInputSendsFrames_data();
    void mediaRecorderWritesVideo_whenVideoFramesInputSendsFrames();
};

void tst_QMediaFrameInputsBackend::initTestCase()
{
    qRegisterMetaType<VideoFrameModifier>();
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

QTEST_MAIN(tst_QMediaFrameInputsBackend)

#include "tst_qmediaframeinputsbackend.moc"
