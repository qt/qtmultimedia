/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QDebug>
#include "qmediaservice.h"
#include "qmediaplayer.h"

//TESTED_COMPONENT=src/multimedia

#ifndef TESTDATA_DIR
#define TESTDATA_DIR "./"
#endif

QT_USE_NAMESPACE

// Eventually these will make it into qtestcase.h
// but we might need to tweak the timeout values here.
#ifndef QTRY_COMPARE
#define QTRY_COMPARE(__expr, __expected) \
    do { \
        const int __step = 50; \
        const int __timeout = 5000; \
        if ((__expr) != (__expected)) { \
            QTest::qWait(0); \
        } \
        for (int __i = 0; __i < __timeout && ((__expr) != (__expected)); __i+=__step) { \
            QTest::qWait(__step); \
        } \
        QCOMPARE(__expr, __expected); \
    } while(0)
#endif

#ifndef QTRY_VERIFY
#define QTRY_VERIFY(__expr) \
        do { \
        const int __step = 50; \
        const int __timeout = 5000; \
        if (!(__expr)) { \
            QTest::qWait(0); \
        } \
        for (int __i = 0; __i < __timeout && !(__expr); __i+=__step) { \
            QTest::qWait(__step); \
        } \
        QVERIFY(__expr); \
    } while(0)
#endif


#define QTRY_WAIT(code, __expr) \
        do { \
        const int __step = 50; \
        const int __timeout = 5000; \
        if (!(__expr)) { \
            QTest::qWait(0); \
        } \
        for (int __i = 0; __i < __timeout && !(__expr); __i+=__step) { \
            do { code } while(0); \
            QTest::qWait(__step); \
        } \
    } while(0)


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

private:
    //one second local wav file
    QMediaContent localWavFile;
};

void tst_QMediaPlayerBackend::init()
{
}

void tst_QMediaPlayerBackend::initTestCase()
{
    QFileInfo wavFile(QLatin1String(TESTDATA_DIR "testdata/test.wav"));
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


QTEST_MAIN(tst_QMediaPlayerBackend)
#include "tst_qmediaplayerbackend.moc"

