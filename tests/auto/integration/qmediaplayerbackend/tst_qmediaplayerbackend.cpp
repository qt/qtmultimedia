/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QDebug>
#include <qabstractvideosurface.h>
#include "qmediaplayer.h"
#include <qmediaplaylist.h>
#include <qmediametadata.h>
#include <qaudiobuffer.h>
#include <qvideosink.h>

#include "../shared/mediafileselector.h"
//TESTED_COMPONENT=src/multimedia

#include <QtMultimedia/private/qtmultimedia-config_p.h>

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
    void loadMediaInLoadingState();
    void playPauseStop();
    void processEOS();
    void deleteLaterAtEOS();
    void volumeAndMuted();
    void volumeAcrossFiles_data();
    void volumeAcrossFiles();
    void initialVolume();
    void seekPauseSeek();
    void seekInStoppedState();
    void subsequentPlayback();
    void surfaceTest_data();
    void surfaceTest();
    void multipleSurfaces();
    void metadata();
    void playerStateAtEOS();
    void playFromBuffer();

private:
    QUrl selectVideoFile(const QStringList& mediaCandidates);
    bool isWavSupported();

    //one second local wav file
    QUrl localWavFile;
    QUrl localWavFile2;
    QUrl localVideoFile;
    QUrl localCompressedSoundFile;
    QUrl localFileWithMetadata;

    bool m_inCISystem;
};

/*
    This is a simple video surface which records all presented frames.
*/

class TestVideoSink : public QVideoSink
{
    Q_OBJECT
public:
    explicit TestVideoSink(bool storeFrames = true)
        : m_storeFrames(storeFrames)
    {
        connect(this, &QVideoSink::newVideoFrame, this, &TestVideoSink::addVideoFrame);
    }

public Q_SLOTS:
    void addVideoFrame(const QVideoFrame &frame) {
        if (m_storeFrames)
            m_frameList.append(frame);
        ++m_totalFrames;
    }

public:
    QList<QVideoFrame> m_frameList;
    int m_totalFrames = 0; // used instead of the list when frames are not stored

private:
    bool m_storeFrames;
    QList<QVideoFrame::PixelFormat> m_supported;
};

void tst_QMediaPlayerBackend::init()
{
}

QUrl tst_QMediaPlayerBackend::selectVideoFile(const QStringList& mediaCandidates)
{
    // select supported video format
    QMediaPlayer player;
    TestVideoSink *surface = new TestVideoSink;
    player.setVideoOutput(surface);

    QSignalSpy errorSpy(&player, SIGNAL(error(QMediaPlayer::Error)));

    for (const QString &s : mediaCandidates) {
        QFileInfo videoFile(s);
        if (!videoFile.exists())
            continue;
        QUrl media = QUrl(QUrl::fromLocalFile(videoFile.absoluteFilePath()));
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

    return QUrl();
}

bool tst_QMediaPlayerBackend::isWavSupported()
{
    return !localWavFile.isEmpty();
}

void tst_QMediaPlayerBackend::initTestCase()
{
    QMediaPlayer player;
    if (!player.isAvailable())
        QSKIP("Media player service is not available");

    localWavFile = MediaFileSelector::selectMediaFile(QStringList() << QFINDTESTDATA("testdata/test.wav"));
    localWavFile2 = MediaFileSelector::selectMediaFile(QStringList() << QFINDTESTDATA("testdata/_test.wav"));;

    QStringList mediaCandidates;
    mediaCandidates << QFINDTESTDATA("testdata/colors.mp4");
#ifndef SKIP_OGV_TEST
    mediaCandidates << QFINDTESTDATA("testdata/colors.ogv");
#endif
    localVideoFile = MediaFileSelector::selectMediaFile(mediaCandidates);

    mediaCandidates.clear();
    mediaCandidates << QFINDTESTDATA("testdata/nokia-tune.mp3");
    mediaCandidates << QFINDTESTDATA("testdata/nokia-tune.mkv");
    localCompressedSoundFile = MediaFileSelector::selectMediaFile(mediaCandidates);

    localFileWithMetadata = MediaFileSelector::selectMediaFile(QStringList() << QFINDTESTDATA("testdata/nokia-tune.mp3"));

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
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QMediaPlayer player;

    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);

    QSignalSpy stateSpy(&player, SIGNAL(stateChanged(QMediaPlayer::State)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy mediaSpy(&player, SIGNAL(mediaChanged(QUrl)));
    QSignalSpy currentMediaSpy(&player, SIGNAL(currentMediaChanged(QUrl)));

    player.setMedia(localWavFile);

    QCOMPARE(player.state(), QMediaPlayer::StoppedState);

    QVERIFY(player.mediaStatus() != QMediaPlayer::NoMedia);
    QVERIFY(player.mediaStatus() != QMediaPlayer::InvalidMedia);
    QVERIFY(player.media() == localWavFile);

    QCOMPARE(stateSpy.count(), 0);
    QVERIFY(statusSpy.count() > 0);
    QCOMPARE(mediaSpy.count(), 1);
    QCOMPARE(mediaSpy.last()[0].value<QUrl>(), localWavFile);
    QCOMPARE(currentMediaSpy.last()[0].value<QUrl>(), localWavFile);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QVERIFY(player.isAudioAvailable());
    QVERIFY(!player.isVideoAvailable());
}

void tst_QMediaPlayerBackend::unloadMedia()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QMediaPlayer player;
    player.setNotifyInterval(50);

    QSignalSpy stateSpy(&player, SIGNAL(stateChanged(QMediaPlayer::State)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy mediaSpy(&player, SIGNAL(mediaChanged(QUrl)));
    QSignalSpy currentMediaSpy(&player, SIGNAL(currentMediaChanged(QUrl)));
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
    currentMediaSpy.clear();
    positionSpy.clear();
    durationSpy.clear();

    player.setMedia(QUrl());

    QVERIFY(player.position() <= 0);
    QVERIFY(player.duration() <= 0);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(player.media(), QUrl());

    QVERIFY(!stateSpy.isEmpty());
    QVERIFY(!statusSpy.isEmpty());
    QVERIFY(!mediaSpy.isEmpty());
    QVERIFY(!currentMediaSpy.isEmpty());
    QVERIFY(!positionSpy.isEmpty());
}

void tst_QMediaPlayerBackend::loadMediaInLoadingState()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QMediaPlayer player;
    player.setMedia(localWavFile);
    player.play();
    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadingMedia);
    // Sets new media while old has not been finished.
    player.setMedia(localWavFile);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadingMedia);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
}

void tst_QMediaPlayerBackend::playPauseStop()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QMediaPlayer player;
    player.setNotifyInterval(50);

    QSignalSpy stateSpy(&player, SIGNAL(stateChanged(QMediaPlayer::State)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));
    QSignalSpy errorSpy(&player, SIGNAL(error(QMediaPlayer::Error)));

    // Check play() without a media
    player.play();

    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QCOMPARE(player.position(), 0);
    QCOMPARE(stateSpy.count(), 0);
    QCOMPARE(statusSpy.count(), 0);
    QCOMPARE(positionSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);

    // Check pause() without a media
    player.pause();

    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QCOMPARE(player.position(), 0);
    QCOMPARE(stateSpy.count(), 0);
    QCOMPARE(statusSpy.count(), 0);
    QCOMPARE(positionSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);

    // The rest is with a valid media

    player.setMedia(localWavFile);

    QCOMPARE(player.position(), qint64(0));

    player.play();

    QCOMPARE(player.state(), QMediaPlayer::PlayingState);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(statusSpy.count() > 0 &&
                statusSpy.last()[0].value<QMediaPlayer::MediaStatus>() == QMediaPlayer::BufferedMedia);

    QTRY_VERIFY(player.position() > 100);
    QVERIFY(player.duration() > 0);
    QVERIFY(positionSpy.count() > 0);
    QVERIFY(positionSpy.last()[0].value<qint64>() > 0);

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    qint64 positionBeforePause = player.position();
    player.pause();

    QCOMPARE(player.state(), QMediaPlayer::PausedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::PausedState);

    QTest::qWait(2000);

    QVERIFY(qAbs(player.position() - positionBeforePause) < 150);
    QCOMPARE(positionSpy.count(), 1);

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

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    player.play();

    QCOMPARE(player.state(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::PlayingState);
    QCOMPARE(statusSpy.count(), 1); // Should not go through Loading again when play -> stop -> play
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::BufferedMedia);

    player.stop();
    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    player.setMedia(localWavFile2);

    QTRY_VERIFY(statusSpy.count() > 0);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::LoadedMedia);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.count(), 0);

    player.play();

    QTRY_VERIFY(player.position() > 100);

    player.setMedia(localWavFile);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::LoadedMedia);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::StoppedState);
    QCOMPARE(player.position(), 0);
    QCOMPARE(positionSpy.last()[0].value<qint64>(), 0);

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    player.play();

    QTRY_VERIFY(player.position() > 100);

    player.setMedia(QUrl());

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::NoMedia);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::StoppedState);
    QCOMPARE(player.position(), 0);
    QCOMPARE(positionSpy.last()[0].value<qint64>(), 0);
    QCOMPARE(player.duration(), 0);
}


void tst_QMediaPlayerBackend::processEOS()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

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
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.count(), 2);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::StoppedState);

    //at EOS the position stays at the end of file
    QCOMPARE(player.position(), player.duration());
    QVERIFY(positionSpy.count() > 0);
    QCOMPARE(positionSpy.last()[0].value<qint64>(), player.duration());

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    player.play();

    //position is reset to start
    QTRY_VERIFY(player.position() < 100);
    QTRY_VERIFY(positionSpy.count() > 0);
    QCOMPARE(positionSpy.first()[0].value<qint64>(), 0);

    QCOMPARE(player.state(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::PlayingState);
    QVERIFY(statusSpy.count() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::BufferedMedia);

    player.setPosition(900);
    //wait up to 5 seconds for EOS
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QVERIFY(statusSpy.count() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::EndOfMedia);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.count(), 2);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::StoppedState);

    //position stays at the end of file
    QCOMPARE(player.position(), player.duration());
    QVERIFY(positionSpy.count() > 0);
    QCOMPARE(positionSpy.last()[0].value<qint64>(), player.duration());

    //after setPosition EndOfMedia status should be reset to Loaded
    stateSpy.clear();
    statusSpy.clear();
    player.setPosition(500);

    //this transition can be async, so allow backend to perform it
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QCOMPARE(stateSpy.count(), 0);
    QTRY_VERIFY(statusSpy.count() > 0 &&
        statusSpy.last()[0].value<QMediaPlayer::MediaStatus>() == QMediaPlayer::LoadedMedia);

    player.play();
    player.setPosition(900);
    //wait up to 5 seconds for EOS
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(player.position(), player.duration());

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    // pause() should reset position to beginning and status to Buffered
    player.pause();

    QTRY_COMPARE(player.position(), 0);
    QTRY_VERIFY(positionSpy.count() > 0);
    QCOMPARE(positionSpy.first()[0].value<qint64>(), 0);

    QCOMPARE(player.state(), QMediaPlayer::PausedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::State>(), QMediaPlayer::PausedState);
    QVERIFY(statusSpy.count() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::BufferedMedia);
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
            player = nullptr;
        }
    }

private:
    QMediaPlayer* player;
};

// Regression test for
// QTBUG-24927 - deleteLater() called to QMediaPlayer from its signal handler does not work as expected
void tst_QMediaPlayerBackend::deleteLaterAtEOS()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

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

    player.setMedia(QUrl());
    QTRY_COMPARE(player.volume(), volume);
    QCOMPARE(player.isMuted(), muted);

    player.setMedia(localWavFile);
    player.pause();

    QTRY_COMPARE(player.volume(), volume);
    QCOMPARE(player.isMuted(), muted);
}

void tst_QMediaPlayerBackend::initialVolume()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

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
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    QMediaPlayer player;

    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    TestVideoSink *surface = new TestVideoSink;
    player.setVideoOutput(surface);

    player.setMedia(localVideoFile);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QVERIFY(surface->m_frameList.isEmpty()); // frame must not appear until we call pause() or play()

    positionSpy.clear();
    qint64 position = 7000;
    player.setPosition(position);
    QTRY_VERIFY(!positionSpy.isEmpty() && qAbs(player.position() - position) < (qint64)500);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QTest::qWait(250); // wait a bit to ensure the frame is not rendered
    QVERIFY(surface->m_frameList.isEmpty()); // still no frame, we must call pause() or play() to see a frame

    player.pause();
    QTRY_COMPARE(player.state(), QMediaPlayer::PausedState); // it might take some time for the operation to be completed
    QTRY_VERIFY_WITH_TIMEOUT(!surface->m_frameList.isEmpty(), 10000); // we must see a frame at position 7000 here

    // Make sure that the frame has a timestamp before testing - not all backends provides this
    if (!surface->m_frameList.back().isValid() || surface->m_frameList.back().startTime() < 0)
        QSKIP("No timestamp");

    {
        QVideoFrame frame = surface->m_frameList.back();
        const qint64 elapsed = (frame.startTime() / 1000) - position; // frame.startTime() is microsecond, position is milliseconds.
        QVERIFY2(qAbs(elapsed) < (qint64)500, QByteArray::number(elapsed).constData());
        QCOMPARE(frame.width(), 160);
        QCOMPARE(frame.height(), 120);

        // create QImage for QVideoFrame to verify RGB pixel colors
        QVERIFY(frame.map(QVideoFrame::ReadOnly));
        QImage image(frame.bits(), frame.width(), frame.height(), QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat()));
        QVERIFY(!image.isNull());
        QVERIFY(qRed(image.pixel(0, 0)) >= 230); // conversion from YUV => RGB, that's why it's not 255
        QVERIFY(qGreen(image.pixel(0, 0)) < 20);
        QVERIFY(qBlue(image.pixel(0, 0)) < 20);
        frame.unmap();
    }

    surface->m_frameList.clear();
    positionSpy.clear();
    position = 12000;
    player.setPosition(position);
    QTRY_VERIFY(!positionSpy.isEmpty() && qAbs(player.position() - position) < (qint64)500);
    QCOMPARE(player.state(), QMediaPlayer::PausedState);
    QVERIFY(!surface->m_frameList.isEmpty());

    {
        QVideoFrame frame = surface->m_frameList.back();
        const qint64 elapsed = (frame.startTime() / 1000) - position;
        QVERIFY2(qAbs(elapsed) < (qint64)500, QByteArray::number(elapsed).constData());
        QCOMPARE(frame.width(), 160);
        QCOMPARE(frame.height(), 120);

        QVERIFY(frame.map(QVideoFrame::ReadOnly));
        QImage image(frame.bits(), frame.width(), frame.height(), QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat()));
        QVERIFY(!image.isNull());
        QVERIFY(qRed(image.pixel(0, 0)) < 20);
        QVERIFY(qGreen(image.pixel(0, 0)) >= 230);
        QVERIFY(qBlue(image.pixel(0, 0)) < 20);
        frame.unmap();
    }
}

void tst_QMediaPlayerBackend::seekInStoppedState()
{
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    QMediaPlayer player;
    player.setNotifyInterval(500);

    QSignalSpy stateSpy(&player, SIGNAL(stateChanged(QMediaPlayer::State)));
    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    player.setMedia(localVideoFile);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(player.position(), 0);
    QVERIFY(player.isSeekable());

    stateSpy.clear();
    positionSpy.clear();

    qint64 position = 5000;
    player.setPosition(position);

    QTRY_VERIFY(qAbs(player.position() - position) < qint64(500));
    QCOMPARE(positionSpy.count(), 1);
    QVERIFY(qAbs(positionSpy.last()[0].value<qint64>() - position) < qint64(500));

    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.count(), 0);

    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    positionSpy.clear();

    player.play();

    QCOMPARE(player.state(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);
    QVERIFY(qAbs(player.position() - position) < qint64(500));

    QTest::qWait(2000);
    // Check that it never played from the beginning
    QVERIFY(player.position() > (position - 500));
    for (int i = 0; i < positionSpy.count(); ++i)
        QVERIFY(positionSpy.at(i)[0].value<qint64>() > (position - 500));

    // ------
    // Same tests but after play() --> stop()

    player.stop();
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(player.position(), 0);

    stateSpy.clear();
    positionSpy.clear();

    player.setPosition(position);

    QTRY_VERIFY(qAbs(player.position() - position) < qint64(500));
    QCOMPARE(positionSpy.count(), 1);
    QVERIFY(qAbs(positionSpy.last()[0].value<qint64>() - position) < qint64(500));

    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.count(), 0);

    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    positionSpy.clear();

    player.play();

    QCOMPARE(player.state(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);
    QVERIFY(qAbs(player.position() - position) < qint64(500));

    QTest::qWait(2000);
    // Check that it never played from the beginning
    QVERIFY(player.position() > (position - 500));
    for (int i = 0; i < positionSpy.count(); ++i)
        QVERIFY(positionSpy.at(i)[0].value<qint64>() > (position - 500));

    // ------
    // Same tests but after reaching the end of the media

    player.setPosition(player.duration() - 500);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(player.position(), player.duration());

    stateSpy.clear();
    positionSpy.clear();

    player.setPosition(position);

    QTRY_VERIFY(qAbs(player.position() - position) < qint64(500));
    QCOMPARE(positionSpy.count(), 1);
    QVERIFY(qAbs(positionSpy.last()[0].value<qint64>() - position) < qint64(500));

    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.count(), 0);

    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    positionSpy.clear();

    player.play();

    QCOMPARE(player.state(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);
    QVERIFY(qAbs(player.position() - position) < qint64(500));

    QTest::qWait(2000);
    // Check that it never played from the beginning
    QVERIFY(player.position() > (position - 500));
    for (int i = 0; i < positionSpy.count(); ++i)
        QVERIFY(positionSpy.at(i)[0].value<qint64>() > (position - 500));
}

void tst_QMediaPlayerBackend::subsequentPlayback()
{
#ifdef Q_OS_LINUX
    if (m_inCISystem)
        QSKIP("QTBUG-26769 Fails with gstreamer backend on ubuntu 10.4, setPosition(0)");
#endif

    if (localCompressedSoundFile.isEmpty())
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
               << QVideoFrame::Format_YUV422P
               << QVideoFrame::Format_YV12
               << QVideoFrame::Format_UYVY
               << QVideoFrame::Format_YUYV
               << QVideoFrame::Format_NV12
               << QVideoFrame::Format_NV21;

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
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    QFETCH(QList<QVideoFrame::PixelFormat>, formatsList);

    TestVideoSink surface(false);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
    player.setMedia(localVideoFile);
    player.play();
    QTRY_VERIFY(player.position() >= 1000);
    QVERIFY2(surface.m_totalFrames >= 25, qPrintable(QString("Expected >= 25, got %1").arg(surface.m_totalFrames)));
}

void tst_QMediaPlayerBackend::multipleSurfaces()
{
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    QVideoSink surface1;
    QVideoSink surface2;

    QMediaPlayer player;
    player.setVideoOutput(QList<QVideoSink *>() << &surface1 << &surface2);
    player.setMedia(localVideoFile);
    player.play();
    QTRY_VERIFY(player.position() >= 1000);
//    QVERIFY2(surface1.m_totalFrames >= 25, qPrintable(QString("Expected >= 25, got %1").arg(surface1.m_totalFrames)));
//    QVERIFY2(surface2.m_totalFrames >= 25, qPrintable(QString("Expected >= 25, got %1").arg(surface2.m_totalFrames)));
//    QCOMPARE(surface1.m_totalFrames, surface2.m_totalFrames);
}

void tst_QMediaPlayerBackend::metadata()
{
    if (localFileWithMetadata.isEmpty())
        QSKIP("No supported media file");

    QMediaPlayer player;

    QSignalSpy metadataChangedSpy(&player, SIGNAL(metaDataChanged()));

    player.setMedia(localFileWithMetadata);

    QVERIFY(metadataChangedSpy.count() > 0);

    QCOMPARE(player.metaData().value(QMediaMetaData::Title).toString(), QStringLiteral("Nokia Tune"));
    QCOMPARE(player.metaData().value(QMediaMetaData::ContributingArtist).toString(), QStringLiteral("TestArtist"));
    QCOMPARE(player.metaData().value(QMediaMetaData::AlbumTitle).toString(), QStringLiteral("TestAlbum"));

    metadataChangedSpy.clear();

    player.setMedia(QUrl());

    QCOMPARE(metadataChangedSpy.count(), 1);
    QVERIFY(player.metaData().isEmpty());
}

void tst_QMediaPlayerBackend::playerStateAtEOS()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QMediaPlayer player;

    bool endOfMediaReceived = false;
    connect(&player, &QMediaPlayer::mediaStatusChanged, [&](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            QCOMPARE(player.state(), QMediaPlayer::StoppedState);
            endOfMediaReceived = true;
        }
    });

    player.setMedia(localWavFile);
    player.play();

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QVERIFY(endOfMediaReceived);
}

void tst_QMediaPlayerBackend::playFromBuffer()
{
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    TestVideoSink surface(false);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
    QFile file(localVideoFile.toLocalFile());
    if (!file.open(QIODevice::ReadOnly))
        QSKIP("Could not open file");
    player.setMedia(localVideoFile, &file);
    player.play();
    QTRY_VERIFY(player.position() >= 1000);
    QVERIFY2(surface.m_totalFrames >= 25, qPrintable(QString("Expected >= 25, got %1").arg(surface.m_totalFrames)));
}

QTEST_MAIN(tst_QMediaPlayerBackend)
#include "tst_qmediaplayerbackend.moc"

