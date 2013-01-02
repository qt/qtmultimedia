/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
#include <qmediaplaylist.h>

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
    void deleteLaterAtEOS();
    void volumeAndMuted();
    void volumeAcrossFiles_data();
    void volumeAcrossFiles();
    void initialVolume();
    void seekPauseSeek();
    void subsequentPlayback();
    void probes();
    void playlist();
    void surfaceTest_data();
    void surfaceTest();

private:
    QMediaContent selectVideoFile(const QStringList& mediaCandidates);
    QMediaContent selectSoundFile(const QStringList& mediaCandidates);

    //one second local wav file
    QMediaContent localWavFile;
    QMediaContent localVideoFile;
    QMediaContent localCompressedSoundFile;

    bool m_inCISystem;
};

/*
    This is a simple video surface which records all presented frames.
*/

class TestVideoSurface : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    explicit TestVideoSurface(bool storeFrames = true);

    void setSupportedFormats(const QList<QVideoFrame::PixelFormat>& formats) { m_supported = formats; }

    //video surface
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const;

    bool start(const QVideoSurfaceFormat &format);
    void stop();
    bool present(const QVideoFrame &frame);

    QList<QVideoFrame> m_frameList;
    int m_totalFrames; // used instead of the list when frames are not stored

private:
    bool m_storeFrames;
    QList<QVideoFrame::PixelFormat> m_supported;
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

QMediaContent tst_QMediaPlayerBackend::selectVideoFile(const QStringList& mediaCandidates)
{
    // select supported video format
    QMediaPlayer player;
    TestVideoSurface *surface = new TestVideoSurface;
    player.setVideoOutput(surface);

    QSignalSpy errorSpy(&player, SIGNAL(error(QMediaPlayer::Error)));

    foreach (QString s, mediaCandidates) {
        QFileInfo videoFile(s);
        if (!videoFile.exists())
            continue;
        QMediaContent media = QMediaContent(QUrl::fromLocalFile(videoFile.absoluteFilePath()));
        player.setMedia(media);
        player.pause();

        for (int i = 0; i < 2000 && surface->m_frameList.isEmpty() && errorSpy.isEmpty(); i+=50) {
            QTest::qWait(50);
        }

        if (!surface->m_frameList.isEmpty() && errorSpy.isEmpty()) {
            return media;
        }
        errorSpy.clear();
    }

    return QMediaContent();
}

QMediaContent tst_QMediaPlayerBackend::selectSoundFile(const QStringList& mediaCandidates)
{
    QMediaPlayer player;

    QSignalSpy errorSpy(&player, SIGNAL(error(QMediaPlayer::Error)));

    foreach (QString s, mediaCandidates) {
        QFileInfo soundFile(s);
        if (!soundFile.exists())
            continue;
        QMediaContent media = QMediaContent(QUrl::fromLocalFile(soundFile.absoluteFilePath()));
        player.setMedia(media);
        player.play();

        for (int i = 0; i < 2000 && player.mediaStatus() != QMediaPlayer::BufferedMedia && errorSpy.isEmpty(); i+=50) {
            QTest::qWait(50);
        }

        if (player.mediaStatus() == QMediaPlayer::BufferedMedia && errorSpy.isEmpty()) {
            return media;
        }
        errorSpy.clear();
    }

    return QMediaContent();
}

void tst_QMediaPlayerBackend::initTestCase()
{
    const QString testFileName = QFINDTESTDATA("testdata/test.wav");
    QFileInfo wavFile(testFileName);

    QVERIFY(wavFile.exists());

    localWavFile = QMediaContent(QUrl::fromLocalFile(wavFile.absoluteFilePath()));

    qRegisterMetaType<QMediaContent>();

    QStringList mediaCandidates;
    mediaCandidates << QFINDTESTDATA("testdata/colors.ogv");
    mediaCandidates << QFINDTESTDATA("testdata/colors.mp4");
    localVideoFile = selectVideoFile(mediaCandidates);

    mediaCandidates.clear();
    mediaCandidates << QFINDTESTDATA("testdata/nokia-tune.mkv");
    mediaCandidates << QFINDTESTDATA("testdata/nokia-tune.mp3");
    localCompressedSoundFile = selectSoundFile(mediaCandidates);

    qgetenv("QT_TEST_CI").toInt(&m_inCISystem,10);
}

void tst_QMediaPlayerBackend::cleanup()
{
}

void tst_QMediaPlayerBackend::construction()
{
    QMediaPlayer player;
    QTRY_VERIFY(player.isAvailable());
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

    QTRY_VERIFY(player.position() > 0);
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

    QTRY_VERIFY(player.position() > 0);
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
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

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

// Helper class for tst_QMediaPlayerBackend::deleteLaterAtEOS()
class DeleteLaterAtEos : public QObject
{
    Q_OBJECT
public:
    DeleteLaterAtEos(QMediaPlayer* p) : player(p)
    {
    }

public slots:
    void play()
    {
        QVERIFY(connect(player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
                        this,   SLOT(onMediaStatusChanged(QMediaPlayer::MediaStatus))));
        player->play();
    }

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status)
    {
        if (status == QMediaPlayer::EndOfMedia) {
            player-> deleteLater();
            player = 0;
        }
    }

private:
    QMediaPlayer* player;
};

// Regression test for
// QTBUG-24927 - deleteLater() called to QMediaPlayer from its signal handler does not work as expected
void tst_QMediaPlayerBackend::deleteLaterAtEOS()
{
    QPointer<QMediaPlayer> player(new QMediaPlayer);
    DeleteLaterAtEos deleter(player);
    player->setMedia(localWavFile);

    // Create an event loop for verifying deleteLater behavior instead of using
    // QTRY_VERIFY or QTest::qWait. QTest::qWait makes extra effort to process
    // DeferredDelete events during the wait, which interferes with this test.
    QEventLoop loop;
    QTimer::singleShot(0, &deleter, SLOT(play()));
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    connect(player.data(), SIGNAL(destroyed()), &loop, SLOT(quit()));
    loop.exec();
    // Verify that the player was destroyed within the event loop.
    // This check will fail without the fix for QTBUG-24927.
    QVERIFY(player.isNull());
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
#ifdef Q_OS_LINUX
    if (m_inCISystem)
        QSKIP("QTBUG-26577 Fails with gstreamer backend on ubuntu 10.4");
#endif

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

    QTRY_COMPARE(player.volume(), volume);
    QCOMPARE(player.isMuted(), muted);

    player.setMedia(QMediaContent());
    QTRY_COMPARE(player.volume(), volume);
    QCOMPARE(player.isMuted(), muted);

    player.setMedia(localWavFile);
    player.pause();

    QTRY_COMPARE(player.volume(), volume);
    QCOMPARE(player.isMuted(), muted);
}

void tst_QMediaPlayerBackend::initialVolume()
{
    {
        QMediaPlayer player;
        player.setVolume(1);
        player.setMedia(localWavFile);
        QCOMPARE(player.volume(), 1);
        player.play();
        QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(player.volume(), 1);
    }

    {
        QMediaPlayer player;
        player.setMedia(localWavFile);
        QCOMPARE(player.volume(), 100);
        player.play();
        QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(player.volume(), 100);
    }
}

void tst_QMediaPlayerBackend::seekPauseSeek()
{
    if (localVideoFile.isNull())
        QSKIP("Video format is not supported");

    QMediaPlayer player;

    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    TestVideoSurface *surface = new TestVideoSurface;
    player.setVideoOutput(surface);

    player.setMedia(localVideoFile);
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

void tst_QMediaPlayerBackend::subsequentPlayback()
{
#ifdef Q_OS_LINUX
    if (m_inCISystem)
        QSKIP("QTBUG-26769 Fails with gstreamer backend on ubuntu 10.4, setPosition(0)");
#endif

    if (localCompressedSoundFile.isNull())
        QSKIP("Sound format is not supported");

    QMediaPlayer player;
    player.setMedia(localCompressedSoundFile);
    player.play();

    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QTRY_COMPARE(player.state(), QMediaPlayer::PlayingState);
    QTRY_COMPARE_WITH_TIMEOUT(player.mediaStatus(), QMediaPlayer::EndOfMedia, 15000);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    // Could differ by up to 1 compressed frame length
    QVERIFY(qAbs(player.position() - player.duration()) < 100);
    QVERIFY(player.position() > 0);

    player.play();
    QTRY_COMPARE(player.state(), QMediaPlayer::PlayingState);
    QTRY_VERIFY_WITH_TIMEOUT(player.position() > 2000 && player.position() < 5000, 10000);
    player.pause();
    QCOMPARE(player.state(), QMediaPlayer::PausedState);
    // make sure position does not "jump" closer to the end of the file
    QVERIFY(player.position() > 2000 && player.position() < 5000);
    // try to seek back to zero
    player.setPosition(0);
    QTRY_COMPARE(player.position(), qint64(0));
    player.play();
    QCOMPARE(player.state(), QMediaPlayer::PlayingState);
    QTRY_VERIFY_WITH_TIMEOUT(player.position() > 2000 && player.position() < 5000, 10000);
    player.pause();
    QCOMPARE(player.state(), QMediaPlayer::PausedState);
    QVERIFY(player.position() > 2000 && player.position() < 5000);
}

void tst_QMediaPlayerBackend::probes()
{
    if (localVideoFile.isNull())
        QSKIP("Video format is not supported");

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

    player->setMedia(localVideoFile);
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

void tst_QMediaPlayerBackend::playlist()
{
    QMediaPlayer player;

    QSignalSpy mediaSpy(&player, SIGNAL(mediaChanged(const QMediaContent&)));
    QSignalSpy currentMediaSpy(&player, SIGNAL(currentMediaChanged(const QMediaContent&)));
    QSignalSpy stateSpy(&player, SIGNAL(stateChanged(QMediaPlayer::State)));
    QSignalSpy errorSpy(&player, SIGNAL(error(QMediaPlayer::Error)));

    QFileInfo fileInfo(QFINDTESTDATA("testdata/sample.m3u"));
    player.setMedia(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));

    player.play();
    QTRY_COMPARE_WITH_TIMEOUT(player.state(), QMediaPlayer::StoppedState, 10000);
    QCOMPARE(mediaSpy.count(), 2);
    // sample.m3u -> sample.m3u resolved -> test.wav ->
    // nested1.m3u -> nested1.m3u resolved -> test.wav ->
    // nested2.m3u -> nested2.m3u resolved ->
    // test.wav -> _test.wav
    // currentMediaChanged signals not emmitted for
    // nested1.m3u\_test.wav and nested2.m3u\_test.wav
    // because current media stays the same
    QCOMPARE(currentMediaSpy.count(), 11);
    QCOMPARE(stateSpy.count(), 2);
    QCOMPARE(errorSpy.count(), 0);

    mediaSpy.clear();
    currentMediaSpy.clear();
    stateSpy.clear();
    errorSpy.clear();

    player.play();
    QTRY_COMPARE_WITH_TIMEOUT(player.state(), QMediaPlayer::StoppedState, 10000);
    QCOMPARE(mediaSpy.count(), 0);
    QCOMPARE(currentMediaSpy.count(), 8);
    QCOMPARE(stateSpy.count(), 2);
    QCOMPARE(errorSpy.count(), 0);

    mediaSpy.clear();
    currentMediaSpy.clear();
    stateSpy.clear();
    errorSpy.clear();

    // <<< Invalid - 1st pass >>>
    fileInfo.setFile(QFINDTESTDATA("testdata/invalid_media.m3u"));
    player.setMedia(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));

    player.play();
    QTRY_COMPARE(player.state(), QMediaPlayer::StoppedState);
    // playlist -> resolved playlist
    QCOMPARE(mediaSpy.count(), 2);
    // playlist -> resolved playlist -> invalid -> ""
    QCOMPARE(currentMediaSpy.count(), 4);
    QCOMPARE(stateSpy.count(), 2);
    QCOMPARE(errorSpy.count(), 1);

    mediaSpy.clear();
    currentMediaSpy.clear();
    stateSpy.clear();
    errorSpy.clear();

    // <<< Invalid - 2nd pass >>>
    player.play();
    QTRY_COMPARE(player.state(), QMediaPlayer::StoppedState);
    // media is not changed
    QCOMPARE(mediaSpy.count(), 0);
    // resolved playlist -> invalid -> ""
    QCOMPARE(currentMediaSpy.count(), 3);
    QCOMPARE(stateSpy.count(), 2);
    QCOMPARE(errorSpy.count(), 1);

    mediaSpy.clear();
    currentMediaSpy.clear();
    stateSpy.clear();
    errorSpy.clear();

    // <<< Invalid2 - 1st pass >>>
    fileInfo.setFile((QFINDTESTDATA("testdata/invalid_media2.m3u")));
    player.setMedia(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));

    player.play();
    QTRY_COMPARE_WITH_TIMEOUT(player.state(), QMediaPlayer::StoppedState, 20000);
    // playlist -> resolved playlist
    QCOMPARE(mediaSpy.count(), 2);
    // playlist -> resolved playlist -> test.wav -> invalid -> test.wav -> ""
    QCOMPARE(currentMediaSpy.count(), 6);
    QCOMPARE(stateSpy.count(), 2);
    QCOMPARE(errorSpy.count(), 1);

    mediaSpy.clear();
    currentMediaSpy.clear();
    stateSpy.clear();
    errorSpy.clear();

    // <<< Invalid2 - 2nd pass >>>
    player.play();
    QTRY_COMPARE_WITH_TIMEOUT(player.state(), QMediaPlayer::StoppedState, 20000);
    // playlist -> resolved playlist
    QCOMPARE(mediaSpy.count(), 0);
    // playlist -> test.wav -> invalid -> test.wav -> ""
    QCOMPARE(currentMediaSpy.count(), 5);
    QCOMPARE(stateSpy.count(), 2);
    QCOMPARE(errorSpy.count(), 1);

    mediaSpy.clear();
    currentMediaSpy.clear();
    stateSpy.clear();
    errorSpy.clear();

    // <<< Recursive - 1st pass >>>
    fileInfo.setFile((QFINDTESTDATA("testdata/recursive_master.m3u")));
    player.setMedia(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));

    player.play();
    QTRY_COMPARE_WITH_TIMEOUT(player.state(), QMediaPlayer::StoppedState, 20000);
    // master playlist -> resolved master playlist
    QCOMPARE(mediaSpy.count(), 2);
    // master playlist -> resolved master playlist ->
    // recursive playlist -> resolved recursive playlist ->
    // recursive playlist (this URL is already in the chain of playlists, so the playlist is not resolved) ->
    // invalid -> test.wav -> ""
    QCOMPARE(currentMediaSpy.count(), 8);
    QCOMPARE(stateSpy.count(), 2);
    // there is one invalid media in the master playlist
    QCOMPARE(errorSpy.count(), 1);

    mediaSpy.clear();
    currentMediaSpy.clear();
    stateSpy.clear();
    errorSpy.clear();

    // <<< Recursive - 2nd pass >>>
    player.play();
    QTRY_COMPARE_WITH_TIMEOUT(player.state(), QMediaPlayer::StoppedState, 20000);
    QCOMPARE(mediaSpy.count(), 0);
    // resolved master playlist ->
    // resolved recursive playlist ->
    // recursive playlist (this URL is already in the chain of playlists, so the playlist is not resolved) ->
    // invalid -> test.wav -> ""
    QCOMPARE(currentMediaSpy.count(), 6);
    QCOMPARE(stateSpy.count(), 2);
    // there is one invalid media in the master playlist
    QCOMPARE(errorSpy.count(), 1);
}

void tst_QMediaPlayerBackend::surfaceTest_data()
{
    QTest::addColumn< QList<QVideoFrame::PixelFormat> >("formatsList");

    QList<QVideoFrame::PixelFormat> formatsRGB;
    formatsRGB << QVideoFrame::Format_RGB32
               << QVideoFrame::Format_ARGB32
               << QVideoFrame::Format_RGB565
               << QVideoFrame::Format_BGRA32;

    QList<QVideoFrame::PixelFormat> formatsYUV;
    formatsYUV << QVideoFrame::Format_YUV420P
               << QVideoFrame::Format_YV12
               << QVideoFrame::Format_UYVY
               << QVideoFrame::Format_YUYV;

    QTest::newRow("RGB formats")
            << formatsRGB;

    QTest::newRow("YVU formats")
            << formatsYUV;

    QTest::newRow("RGB & YUV formats")
            << formatsRGB + formatsYUV;
}

void tst_QMediaPlayerBackend::surfaceTest()
{
    // 25 fps video file
    if (localVideoFile.isNull())
        QSKIP("Video format is not supported");

    QFETCH(QList<QVideoFrame::PixelFormat>, formatsList);

    TestVideoSurface surface(false);
    surface.setSupportedFormats(formatsList);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
    player.setMedia(localVideoFile);
    player.play();
    QTRY_VERIFY(player.position() >= 1000);
    QVERIFY(surface.m_totalFrames >= 25);
}

TestVideoSurface::TestVideoSurface(bool storeFrames):
    m_totalFrames(0),
    m_storeFrames(storeFrames)
{
    // set default formats
    m_supported << QVideoFrame::Format_RGB32
                << QVideoFrame::Format_ARGB32
                << QVideoFrame::Format_ARGB32_Premultiplied
                << QVideoFrame::Format_RGB565
                << QVideoFrame::Format_RGB555;
}

QList<QVideoFrame::PixelFormat> TestVideoSurface::supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType) const
{
    if (handleType == QAbstractVideoBuffer::NoHandle) {
        return m_supported;
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
    if (m_storeFrames)
        m_frameList.push_back(frame);
    m_totalFrames++;
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

