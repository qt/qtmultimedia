/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QDebug>
#include <qabstractvideosurface.h>
#include "qmediaservice.h"
#include "qmediaplayer.h"
#include "qaudioprobe.h"
#include "qvideoprobe.h"

//TESTED_COMPONENT=src/multimedia

QT_USE_NAMESPACE

/*
 This is the backend conformance test.

 Since it relies on platform media framework and sound hardware
 it may be less stable.
*/

class tst_QMediaPlayerBackend : public QObject
{
    Q_OBJECT
public slots:
    void init();
    void cleanup();
    void initTestCase();

private slots:
    void construction();
    void loadMedia();
    void unloadMedia();
    void playPauseStop();
    void processEOS();
    void volumeAndMuted();
    void volumeAcrossFiles_data();
    void volumeAcrossFiles();
    void seekPauseSeek();
    void probes();

private:
    //one second local wav file
    QMediaContent localWavFile;
};

/*
    This is a simple video surface which records all presented frames.
*/

class TestVideoSurface : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    TestVideoSurface() { }

    //video surface
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const;

    bool start(const QVideoSurfaceFormat &format);
    void stop();
    bool present(const QVideoFrame &frame);

    QList<QVideoFrame> m_frameList;
};

class ProbeDataHandler : public QObject
{
    Q_OBJECT

public:
    ProbeDataHandler() : isVideoFlushCalled(false) { }

    QList<QVideoFrame> m_frameList;
    QList<QAudioBuffer> m_bufferList;
    bool isVideoFlushCalled;

public slots:
    void processFrame(const QVideoFrame&);
    void processBuffer(const QAudioBuffer&);
    void flushVideo();
    void flushAudio();
};

void tst_QMediaPlayerBackend::init()
{
}

void tst_QMediaPlayerBackend::initTestCase()
{
    const QString testFileName = QFINDTESTDATA("testdata/test.wav");
    QFileInfo wavFile(testFileName);

    QVERIFY(wavFile.exists());

    localWavFile = QMediaContent(QUrl::fromLocalFile(wavFile.absoluteFilePath()));

    qRegisterMetaType<QMediaContent>();
}

void tst_QMediaPlayerBackend::cleanup()
{
}

void tst_QMediaPlayerBackend::construction()
{
    QMediaPlayer player;
    QVERIFY(player.isAvailable());
}

void tst_QMediaPlayerBackend::loadMedia()
{
    QMediaPlayer player;
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);

    QSignalSpy stateSpy(&player, SIGNAL(stateChanged(QMediaPlayer::State)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy mediaSpy(&player, SIGNAL(mediaChanged(QMediaContent)));

    player.setMedia(localWavFile);

    QCOMPARE(player.state(), QMediaPlayer::StoppedState);

    QVERIFY(player.mediaStatus() != QMediaPlayer::NoMedia);
    QVERIFY(player.mediaStatus() != QMediaPlayer::InvalidMedia);
    QVERIFY(player.media() == localWavFile);

    QCOMPARE(stateSpy.count(), 0);
    QVERIFY(statusSpy.count() > 0);
    QCOMPARE(mediaSpy.count(), 1);
    QCOMPARE(mediaSpy.last()[0].value<QMediaContent>(), localWavFile);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QVERIFY(player.isAudioAvailable());
    QVERIFY(!player.isVideoAvailable());
}

void tst_QMediaPlayerBackend::unloadMedia()
{
    QMediaPlayer player;
    player.setNotifyInterval(50);

    QSignalSpy stateSpy(&player, SIGNAL(stateChanged(QMediaPlayer::State)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy mediaSpy(&player, SIGNAL(mediaChanged(QMediaContent)));
    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));
    QSignalSpy durationSpy(&player, SIGNAL(positionChanged(qint64)));

    player.setMedia(localWavFile);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QVERIFY(player.position() == 0);
    QVERIFY(player.duration() > 0);

    player.play();

    QTest::qWait(250);
    QVERIFY(player.position() > 0);
    QVERIFY(player.duration() > 0);

    stateSpy.clear();
    statusSpy.clear();
    mediaSpy.clear();
    positionSpy.clear();
    durationSpy.clear();

    player.setMedia(QMediaContent());

    QVERIFY(player.position() <= 0);
    QVERIFY(player.duration() <= 0);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(player.media(), QMediaContent());

    QVERIFY(!stateSpy.isEmpty());
    QVERIFY(!statusSpy.isEmpty());
    QVERIFY(!mediaSpy.isEmpty());
    QVERIFY(!positionSpy.isEmpty());
}


void tst_QMediaPlayerBackend::playPauseStop()
{
    QMediaPlayer player;
    player.setNotifyInterval(50);

    QSignalSpy stateSpy(&player, SIGNAL(stateChanged(QMediaPlayer::State)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    player.setMedia(localWavFile);

    QCOMPARE(player.position(), qint64(0));

    player.play();

    QCOMPARE(player.state(), QMediaPlayer::PlayingState);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(statusSpy.count() > 0 &&
                statusSpy.last()[0].value<QMediaPlayer::MediaStatus>() == QMediaPlayer::BufferedMedia);

    QTest::qWait(500);
    QVERIFY(player.position() > 0);
    QVERIFY(player.duration() > 0);
    QVERIFY(positionSpy.count() > 0);
    QVERIFY(positionSpy.last()[0].value<qint64>() > 0);

    stateSpy.clear();
    statusSpy.clear();

    player.pause();

    QCOMPARE(player.state(), QMediaPlayer::PausedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::PausedState);

    stateSpy.clear();
    statusSpy.clear();

    player.stop();

    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::StoppedState);
    //it's allowed to emit statusChanged() signal async
    QTRY_COMPARE(statusSpy.count(), 1);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::LoadedMedia);

    //ensure the position is reset to 0 at stop and positionChanged(0) is emitted
    QCOMPARE(player.position(), qint64(0));
    QCOMPARE(positionSpy.last()[0].value<qint64>(), qint64(0));
    QVERIFY(player.duration() > 0);
}


void tst_QMediaPlayerBackend::processEOS()
{
    QMediaPlayer player;
    player.setNotifyInterval(50);

    QSignalSpy stateSpy(&player, SIGNAL(stateChanged(QMediaPlayer::State)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    player.setMedia(localWavFile);

    player.play();
    player.setPosition(900);

    //wait up to 5 seconds for EOS
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);

    QVERIFY(statusSpy.count() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::EndOfMedia);

    //at EOS the position stays at the end of file
    QVERIFY(player.position() > 900);

    stateSpy.clear();
    statusSpy.clear();

    player.play();

    //position is reset to start
    QTRY_VERIFY(player.position() < 100);

    QCOMPARE(player.state(), QMediaPlayer::PlayingState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::PlayingState);
    QVERIFY(statusSpy.count() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::BufferedMedia);

    player.setPosition(900);
    //wait up to 5 seconds for EOS
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);

    //ensure the positionChanged() signal is emitted
    QVERIFY(positionSpy.count() > 0);

    QCOMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    //position stays at the end of file
    QVERIFY(player.position() > 900);

    //after setPosition EndOfMedia status should be reset to Loaded
    stateSpy.clear();
    statusSpy.clear();
    player.setPosition(500);

    //this transition can be async, so allow backend to perform it
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QCOMPARE(stateSpy.count(), 0);
    QTRY_VERIFY(statusSpy.count() > 0 &&
        statusSpy.last()[0].value<QMediaPlayer::MediaStatus>() == QMediaPlayer::LoadedMedia);
}

void tst_QMediaPlayerBackend::volumeAndMuted()
{
    //volume and muted properties should be independent
    QMediaPlayer player;
    QVERIFY(player.volume() > 0);
    QVERIFY(!player.isMuted());

    player.setMedia(localWavFile);
    player.pause();

    QVERIFY(player.volume() > 0);
    QVERIFY(!player.isMuted());

    QSignalSpy volumeSpy(&player, SIGNAL(volumeChanged(int)));
    QSignalSpy mutedSpy(&player, SIGNAL(mutedChanged(bool)));

    //setting volume to 0 should not trigger muted
    player.setVolume(0);
    QTRY_COMPARE(player.volume(), 0);
    QVERIFY(!player.isMuted());
    QCOMPARE(volumeSpy.count(), 1);
    QCOMPARE(volumeSpy.last()[0].toInt(), player.volume());
    QCOMPARE(mutedSpy.count(), 0);

    player.setVolume(50);
    QTRY_COMPARE(player.volume(), 50);
    QVERIFY(!player.isMuted());
    QCOMPARE(volumeSpy.count(), 2);
    QCOMPARE(volumeSpy.last()[0].toInt(), player.volume());
    QCOMPARE(mutedSpy.count(), 0);

    player.setMuted(true);
    QTRY_VERIFY(player.isMuted());
    QVERIFY(player.volume() > 0);
    QCOMPARE(volumeSpy.count(), 2);
    QCOMPARE(mutedSpy.count(), 1);
    QCOMPARE(mutedSpy.last()[0].toBool(), player.isMuted());

    player.setMuted(false);
    QTRY_VERIFY(!player.isMuted());
    QVERIFY(player.volume() > 0);
    QCOMPARE(volumeSpy.count(), 2);
    QCOMPARE(mutedSpy.count(), 2);
    QCOMPARE(mutedSpy.last()[0].toBool(), player.isMuted());

}

void tst_QMediaPlayerBackend::volumeAcrossFiles_data()
{
    QTest::addColumn<int>("volume");
    QTest::addColumn<bool>("muted");

    QTest::newRow("100 unmuted") << 100 << false;
    QTest::newRow("50 unmuted") << 50 << false;
    QTest::newRow("0 unmuted") << 0 << false;
    QTest::newRow("100 muted") << 100 << true;
    QTest::newRow("50 muted") << 50 << true;
    QTest::newRow("0 muted") << 0 << true;
}

void tst_QMediaPlayerBackend::volumeAcrossFiles()
{
    QFETCH(int, volume);
    QFETCH(bool, muted);

    QMediaPlayer player;

    //volume and muted should not be preserved between player instances
    QVERIFY(player.volume() > 0);
    QVERIFY(!player.isMuted());

    player.setVolume(volume);
    player.setMuted(muted);

    QTRY_COMPARE(player.volume(), volume);
    QTRY_COMPARE(player.isMuted(), muted);

    player.setMedia(localWavFile);
    QCOMPARE(player.volume(), volume);
    QCOMPARE(player.isMuted(), muted);

    player.pause();

    //to ensure the backend doesn't change volume/muted
    //async during file loading.
    QTest::qWait(50);

    QCOMPARE(player.volume(), volume);
    QCOMPARE(player.isMuted(), muted);

    player.setMedia(QMediaContent());
    QTest::qWait(50);
    QCOMPARE(player.volume(), volume);
    QCOMPARE(player.isMuted(), muted);

    player.setMedia(localWavFile);
    player.pause();

    QTest::qWait(50);

    QCOMPARE(player.volume(), volume);
    QCOMPARE(player.isMuted(), muted);
}

void tst_QMediaPlayerBackend::seekPauseSeek()
{
    QMediaPlayer player;

    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    TestVideoSurface *surface = new TestVideoSurface;
    player.setVideoOutput(surface);

    const QString testFileName = QFINDTESTDATA("testdata/colors.mp4");
    QFileInfo videoFile(testFileName);
    QVERIFY(videoFile.exists());

    player.setMedia(QUrl::fromLocalFile(videoFile.absoluteFilePath()));
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QVERIFY(surface->m_frameList.isEmpty()); // frame must not appear until we call pause() or play()

    positionSpy.clear();
    player.setPosition((qint64)7000);
    QTRY_VERIFY(!positionSpy.isEmpty() && qAbs(player.position() - (qint64)7000) < (qint64)500);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QTest::qWait(250); // wait a bit to ensure the frame is not rendered
    QVERIFY(surface->m_frameList.isEmpty()); // still no frame, we must call pause() or play() to see a frame

    player.pause();
    QTRY_COMPARE(player.state(), QMediaPlayer::PausedState); // it might take some time for the operation to be completed
    QTRY_COMPARE(surface->m_frameList.size(), 1); // we must see a frame at position 7000 here

    {
        QVideoFrame frame = surface->m_frameList.back();
        QVERIFY(qAbs(frame.startTime() - (qint64)7000) < (qint64)500);
        QCOMPARE(frame.width(), 160);
        QCOMPARE(frame.height(), 120);

        // create QImage for QVideoFrame to verify RGB pixel colors
        QVERIFY(frame.map(QAbstractVideoBuffer::ReadOnly));
        QImage image(frame.bits(), frame.width(), frame.height(), QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat()));
        QVERIFY(!image.isNull());
        QVERIFY(qRed(image.pixel(0, 0)) >= 240); // conversion from YUV => RGB, that's why it's not 255
        QCOMPARE(qGreen(image.pixel(0, 0)), 0);
        QCOMPARE(qBlue(image.pixel(0, 0)), 0);
        frame.unmap();
    }

    positionSpy.clear();
    player.setPosition((qint64)12000);
    QTRY_VERIFY(!positionSpy.isEmpty() && qAbs(player.position() - (qint64)12000) < (qint64)500);
    QCOMPARE(player.state(), QMediaPlayer::PausedState);
    QCOMPARE(surface->m_frameList.size(), 2);

    {
        QVideoFrame frame = surface->m_frameList.back();
        QVERIFY(qAbs(frame.startTime() - (qint64)12000) < (qint64)500);
        QCOMPARE(frame.width(), 160);
        QCOMPARE(frame.height(), 120);

        QVERIFY(frame.map(QAbstractVideoBuffer::ReadOnly));
        QImage image(frame.bits(), frame.width(), frame.height(), QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat()));
        QVERIFY(!image.isNull());
        QCOMPARE(qRed(image.pixel(0, 0)), 0);
        QVERIFY(qGreen(image.pixel(0, 0)) >= 240);
        QCOMPARE(qBlue(image.pixel(0, 0)), 0);
        frame.unmap();
    }
}

void tst_QMediaPlayerBackend::probes()
{
    QMediaPlayer *player = new QMediaPlayer;

    TestVideoSurface *surface = new TestVideoSurface;
    player->setVideoOutput(surface);

    QVideoProbe *videoProbe = new QVideoProbe;
    QAudioProbe *audioProbe = new QAudioProbe;

    ProbeDataHandler probeHandler;
    connect(videoProbe, SIGNAL(videoFrameProbed(const QVideoFrame&)), &probeHandler, SLOT(processFrame(QVideoFrame)));
    connect(videoProbe, SIGNAL(flush()), &probeHandler, SLOT(flushVideo()));
    connect(audioProbe, SIGNAL(audioBufferProbed(const QAudioBuffer&)), &probeHandler, SLOT(processBuffer(QAudioBuffer)));
    connect(audioProbe, SIGNAL(flush()), &probeHandler, SLOT(flushAudio()));

    QVERIFY(videoProbe->setSource(player));
    QVERIFY(audioProbe->setSource(player));

    const QString testFileName = QFINDTESTDATA("testdata/colors.mp4");
    QFileInfo videoFile(testFileName);
    QVERIFY(videoFile.exists());
    player->setMedia(QUrl::fromLocalFile(videoFile.absoluteFilePath()));
    QTRY_COMPARE(player->mediaStatus(), QMediaPlayer::LoadedMedia);

    player->pause();
    QTRY_COMPARE(surface->m_frameList.size(), 1);
    QVERIFY(!probeHandler.m_frameList.isEmpty());
    QTRY_VERIFY(!probeHandler.m_bufferList.isEmpty());

    delete player;
    QTRY_VERIFY(probeHandler.isVideoFlushCalled);
    delete videoProbe;
    delete audioProbe;
}

QList<QVideoFrame::PixelFormat> TestVideoSurface::supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType) const
{
    if (handleType == QAbstractVideoBuffer::NoHandle) {
        return QList<QVideoFrame::PixelFormat>()
                << QVideoFrame::Format_RGB32
                << QVideoFrame::Format_ARGB32
                << QVideoFrame::Format_ARGB32_Premultiplied
                << QVideoFrame::Format_RGB565
                << QVideoFrame::Format_RGB555;
    } else {
        return QList<QVideoFrame::PixelFormat>();
    }
}

bool TestVideoSurface::start(const QVideoSurfaceFormat &format)
{
    if (!isFormatSupported(format)) return false;

    return QAbstractVideoSurface::start(format);
}

void TestVideoSurface::stop()
{
    QAbstractVideoSurface::stop();
}

bool TestVideoSurface::present(const QVideoFrame &frame)
{
    m_frameList.push_back(frame);
    return true;
}


void ProbeDataHandler::processFrame(const QVideoFrame &frame)
{
    m_frameList.append(frame);
}

void ProbeDataHandler::processBuffer(const QAudioBuffer &buffer)
{
    m_bufferList.append(buffer);
}

void ProbeDataHandler::flushVideo()
{
    isVideoFlushCalled = true;
}

void ProbeDataHandler::flushAudio()
{

}

QTEST_MAIN(tst_QMediaPlayerBackend)
#include "tst_qmediaplayerbackend.moc"

