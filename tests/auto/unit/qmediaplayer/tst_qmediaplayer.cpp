/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>

#include <QtCore/qdebug.h>
#include <QtCore/qbuffer.h>

#include <qabstractvideosurface.h>
#include <qmediaplayer.h>
#include <qmediaplayercontrol.h>
#include <qmediaplaylist.h>
#include <qmediaservice.h>
#include <qmediastreamscontrol.h>
#include <qvideorenderercontrol.h>

#include "mockmediaserviceprovider.h"
#include "mockmediaplayerservice.h"
#include "mockvideosurface.h"

QT_USE_NAMESPACE

class AutoConnection
{
public:
    AutoConnection(QObject *sender, const char *signal, QObject *receiver, const char *method)
            : sender(sender), signal(signal), receiver(receiver), method(method)
    {
        QObject::connect(sender, signal, receiver, method);
    }

    ~AutoConnection()
    {
        QObject::disconnect(sender, signal, receiver, method);
    }

private:
    QObject *sender;
    const char *signal;
    QObject *receiver;
    const char *method;
};

class tst_QMediaPlayer: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void testNullService_data();
    void testNullService();
    void testValid();
    void testMedia_data();
    void testMedia();
    void testDuration_data();
    void testDuration();
    void testPosition_data();
    void testPosition();
    void testVolume_data();
    void testVolume();
    void testMuted_data();
    void testMuted();
    void testIsAvailable();
    void testVideoAvailable_data();
    void testVideoAvailable();
    void testBufferStatus_data();
    void testBufferStatus();
    void testSeekable_data();
    void testSeekable();
    void testPlaybackRate_data();
    void testPlaybackRate();
    void testError_data();
    void testError();
    void testErrorString_data();
    void testErrorString();
    void testService();
    void testPlay_data();
    void testPlay();
    void testPause_data();
    void testPause();
    void testStop_data();
    void testStop();
    void testMediaStatus_data();
    void testMediaStatus();
    void testSetVideoOutput();
    void testSetVideoOutputNoService();
    void testSetVideoOutputNoControl();
    void testSetVideoOutputDestruction();
    void debugEnums();
    void testDestructor();
    void testSupportedMimeTypes();
    void testQrc_data();
    void testQrc();
    void testAudioRole();
    void testCustomAudioRole();

private:
    void setupCommonTestData();

    MockMediaServiceProvider *mockProvider;
    MockMediaPlayerService  *mockService;
    QMediaPlayer *player;
};

void tst_QMediaPlayer::setupCommonTestData()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QMediaPlayer::State>("state");
    QTest::addColumn<QMediaPlayer::MediaStatus>("status");
    QTest::addColumn<QUrl>("mediaContent");
    QTest::addColumn<qint64>("duration");
    QTest::addColumn<qint64>("position");
    QTest::addColumn<bool>("seekable");
    QTest::addColumn<int>("volume");
    QTest::addColumn<bool>("muted");
    QTest::addColumn<bool>("videoAvailable");
    QTest::addColumn<int>("bufferStatus");
    QTest::addColumn<qreal>("playbackRate");
    QTest::addColumn<QMediaPlayer::Error>("error");
    QTest::addColumn<QString>("errorString");

    QTest::newRow("invalid") << false << QMediaPlayer::StoppedState << QMediaPlayer::UnknownMediaStatus <<
                                QUrl() << qint64(0) << qint64(0) << false << 0 << false << false << 0 <<
                                qreal(0) << QMediaPlayer::NoError << QString();
    QTest::newRow("valid+null") << true << QMediaPlayer::StoppedState << QMediaPlayer::UnknownMediaStatus <<
                                QUrl() << qint64(0) << qint64(0) << false << 0 << false << false << 50 <<
                                qreal(0) << QMediaPlayer::NoError << QString();
    QTest::newRow("valid+content+stopped") << true << QMediaPlayer::StoppedState << QMediaPlayer::UnknownMediaStatus <<
                                QUrl(QUrl("file:///some.mp3")) << qint64(0) << qint64(0) << false << 50 << false << false << 0 <<
                                qreal(1) << QMediaPlayer::NoError << QString();
    QTest::newRow("valid+content+playing") << true << QMediaPlayer::PlayingState << QMediaPlayer::LoadedMedia <<
                                QUrl(QUrl("file:///some.mp3")) << qint64(10000) << qint64(10) << true << 50 << true << false << 0 <<
                                qreal(1) << QMediaPlayer::NoError << QString();
    QTest::newRow("valid+content+paused") << true << QMediaPlayer::PausedState << QMediaPlayer::LoadedMedia <<
                                QUrl(QUrl("file:///some.mp3")) << qint64(10000) << qint64(10) << true << 50 << true << false << 0 <<
                                qreal(1)  << QMediaPlayer::NoError << QString();
    QTest::newRow("valud+streaming") << true << QMediaPlayer::PlayingState << QMediaPlayer::LoadedMedia <<
                                QUrl(QUrl("http://example.com/stream")) << qint64(10000) << qint64(10000) << false << 50 << false << true << 0 <<
                                qreal(1)  << QMediaPlayer::NoError << QString();
    QTest::newRow("valid+error") << true << QMediaPlayer::StoppedState << QMediaPlayer::UnknownMediaStatus <<
                                QUrl(QUrl("http://example.com/stream")) << qint64(0) << qint64(0) << false << 50 << false << false << 0 <<
                                qreal(0) << QMediaPlayer::ResourceError << QString("Resource unavailable");
}

void tst_QMediaPlayer::initTestCase()
{
    qRegisterMetaType<QMediaPlayer::State>("QMediaPlayer::State");
    qRegisterMetaType<QMediaPlayer::Error>("QMediaPlayer::Error");
    qRegisterMetaType<QMediaPlayer::MediaStatus>("QMediaPlayer::MediaStatus");
    qRegisterMetaType<QUrl>("QUrl");
}

void tst_QMediaPlayer::cleanupTestCase()
{
}

void tst_QMediaPlayer::init()
{
    mockService = new MockMediaPlayerService;
    mockProvider = new MockMediaServiceProvider(mockService);
    QMediaServiceProvider::setDefaultServiceProvider(mockProvider);

    player = new QMediaPlayer;
}

void tst_QMediaPlayer::cleanup()
{
    delete player;
    delete mockProvider;
    delete mockService;
}

void tst_QMediaPlayer::testNullService_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testNullService()
{
    mockProvider->service = nullptr;
    QMediaPlayer player;

    const QIODevice *nullDevice = nullptr;

    QCOMPARE(player.media(), QUrl());
    QCOMPARE(player.mediaStream(), nullDevice);
    QCOMPARE(player.state(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::UnknownMediaStatus);
    QCOMPARE(player.duration(), qint64(-1));
    QCOMPARE(player.position(), qint64(0));
    QCOMPARE(player.volume(), 0);
    QCOMPARE(player.isMuted(), false);
    QCOMPARE(player.isVideoAvailable(), false);
    QCOMPARE(player.bufferStatus(), 0);
    QCOMPARE(player.isSeekable(), false);
    QCOMPARE(player.playbackRate(), qreal(0));
    QCOMPARE(player.error(), QMediaPlayer::ServiceMissingError);
    QCOMPARE(player.isAvailable(), false);
    QCOMPARE(player.availability(), QMultimedia::ServiceMissing);

    {
        QFETCH(QUrl, mediaContent);

        QSignalSpy spy(&player, SIGNAL(currentMediaChanged(QUrl)));
        QFile file;

        player.setMedia(mediaContent, &file);
        QCOMPARE(player.media(), mediaContent);
        QCOMPARE(player.mediaStream(), nullDevice);
        QCOMPARE(spy.count(), 0);
    } {
        QSignalSpy stateSpy(&player, SIGNAL(stateChanged(QMediaPlayer::State)));
        QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));

        player.play();
        QCOMPARE(player.state(), QMediaPlayer::StoppedState);
        QCOMPARE(player.mediaStatus(), QMediaPlayer::UnknownMediaStatus);
        QCOMPARE(stateSpy.count(), 0);
        QCOMPARE(statusSpy.count(), 0);

        player.pause();
        QCOMPARE(player.state(), QMediaPlayer::StoppedState);
        QCOMPARE(player.mediaStatus(), QMediaPlayer::UnknownMediaStatus);
        QCOMPARE(stateSpy.count(), 0);
        QCOMPARE(statusSpy.count(), 0);

        player.stop();
        QCOMPARE(player.state(), QMediaPlayer::StoppedState);
        QCOMPARE(player.mediaStatus(), QMediaPlayer::UnknownMediaStatus);
        QCOMPARE(stateSpy.count(), 0);
        QCOMPARE(statusSpy.count(), 0);
    } {
        QFETCH(int, volume);
        QFETCH(bool, muted);

        QSignalSpy volumeSpy(&player, SIGNAL(volumeChanged(int)));
        QSignalSpy mutingSpy(&player, SIGNAL(mutedChanged(bool)));

        player.setVolume(volume);
        QCOMPARE(player.volume(), 0);
        QCOMPARE(volumeSpy.count(), 0);

        player.setMuted(muted);
        QCOMPARE(player.isMuted(), false);
        QCOMPARE(mutingSpy.count(), 0);
    } {
        QFETCH(qint64, position);

        QSignalSpy spy(&player, SIGNAL(positionChanged(qint64)));

        player.setPosition(position);
        QCOMPARE(player.position(), qint64(0));
        QCOMPARE(spy.count(), 0);
    } {
        QFETCH(qreal, playbackRate);

        QSignalSpy spy(&player, SIGNAL(playbackRateChanged(qreal)));

        player.setPlaybackRate(playbackRate);
        QCOMPARE(player.playbackRate(), qreal(0));
        QCOMPARE(spy.count(), 0);
    }
}

void tst_QMediaPlayer::testValid()
{
    /*
    QFETCH(bool, valid);

    mockService->setIsValid(valid);
    QCOMPARE(player->isValid(), valid);
    */
}

void tst_QMediaPlayer::testMedia_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testMedia()
{
    QFETCH(QUrl, mediaContent);

    mockService->setMedia(mediaContent);
    QCOMPARE(player->media(), mediaContent);

    QBuffer stream;
    player->setMedia(mediaContent, &stream);
    QCOMPARE(player->media(), mediaContent);
    QCOMPARE((QBuffer*)player->mediaStream(), &stream);
}

void tst_QMediaPlayer::testDuration_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testDuration()
{
    QFETCH(qint64, duration);

    mockService->setDuration(duration);
    QVERIFY(player->duration() == duration);
}

void tst_QMediaPlayer::testPosition_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testPosition()
{
    QFETCH(bool, valid);
    QFETCH(bool, seekable);
    QFETCH(qint64, position);
    QFETCH(qint64, duration);

    mockService->setIsValid(valid);
    mockService->setSeekable(seekable);
    mockService->setPosition(position);
    mockService->setDuration(duration);
    QVERIFY(player->isSeekable() == seekable);
    QVERIFY(player->position() == position);
    QVERIFY(player->duration() == duration);

    if (seekable) {
        { QSignalSpy spy(player, SIGNAL(positionChanged(qint64)));
        player->setPosition(position);
        QCOMPARE(player->position(), position);
        QCOMPARE(spy.count(), 0); }

        mockService->setPosition(position);
        { QSignalSpy spy(player, SIGNAL(positionChanged(qint64)));
        player->setPosition(0);
        QCOMPARE(player->position(), qint64(0));
        QCOMPARE(spy.count(), position == 0 ? 0 : 1); }

        mockService->setPosition(position);
        { QSignalSpy spy(player, SIGNAL(positionChanged(qint64)));
        player->setPosition(duration);
        QCOMPARE(player->position(), duration);
        QCOMPARE(spy.count(), position == duration ? 0 : 1); }

        mockService->setPosition(position);
        { QSignalSpy spy(player, SIGNAL(positionChanged(qint64)));
        player->setPosition(-1);
        QCOMPARE(player->position(), qint64(0));
        QCOMPARE(spy.count(), position == 0 ? 0 : 1); }

    }
    else {
        QSignalSpy spy(player, SIGNAL(positionChanged(qint64)));
        player->setPosition(position);

        QCOMPARE(player->position(), position);
        QCOMPARE(spy.count(), 0);
    }
}

void tst_QMediaPlayer::testVolume_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testVolume()
{
    QFETCH(bool, valid);
    QFETCH(int, volume);

    mockService->setVolume(volume);
    QVERIFY(player->volume() == volume);

    if (valid) {
        { QSignalSpy spy(player, SIGNAL(volumeChanged(int)));
        player->setVolume(10);
        QCOMPARE(player->volume(), 10);
        QCOMPARE(spy.count(), 1); }

        { QSignalSpy spy(player, SIGNAL(volumeChanged(int)));
        player->setVolume(-1000);
        QCOMPARE(player->volume(), 0);
        QCOMPARE(spy.count(), 1); }

        { QSignalSpy spy(player, SIGNAL(volumeChanged(int)));
        player->setVolume(100);
        QCOMPARE(player->volume(), 100);
        QCOMPARE(spy.count(), 1); }

        { QSignalSpy spy(player, SIGNAL(volumeChanged(int)));
        player->setVolume(1000);
        QCOMPARE(player->volume(), 100);
        QCOMPARE(spy.count(), 0); }
    }
}

void tst_QMediaPlayer::testMuted_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testMuted()
{
    QFETCH(bool, valid);
    QFETCH(bool, muted);
    QFETCH(int, volume);

    if (valid) {
        mockService->setMuted(muted);
        mockService->setVolume(volume);
        QVERIFY(player->isMuted() == muted);

        QSignalSpy spy(player, SIGNAL(mutedChanged(bool)));
        player->setMuted(!muted);
        QCOMPARE(player->isMuted(), !muted);
        QCOMPARE(player->volume(), volume);
        QCOMPARE(spy.count(), 1);
    }
}

void tst_QMediaPlayer::testVideoAvailable_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testVideoAvailable()
{
    QFETCH(bool, videoAvailable);

    mockService->setVideoAvailable(videoAvailable);
    QVERIFY(player->isVideoAvailable() == videoAvailable);
}

void tst_QMediaPlayer::testBufferStatus_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testBufferStatus()
{
    QFETCH(int, bufferStatus);

    mockService->setBufferStatus(bufferStatus);
    QVERIFY(player->bufferStatus() == bufferStatus);
}

void tst_QMediaPlayer::testSeekable_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testSeekable()
{
    QFETCH(bool, seekable);

    mockService->setSeekable(seekable);
    QVERIFY(player->isSeekable() == seekable);
}

void tst_QMediaPlayer::testPlaybackRate_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testPlaybackRate()
{
    QFETCH(bool, valid);
    QFETCH(qreal, playbackRate);

    if (valid) {
        mockService->setPlaybackRate(playbackRate);
        QVERIFY(player->playbackRate() == playbackRate);

        QSignalSpy spy(player, SIGNAL(playbackRateChanged(qreal)));
        player->setPlaybackRate(playbackRate + 0.5f);
        QCOMPARE(player->playbackRate(), playbackRate + 0.5f);
        QCOMPARE(spy.count(), 1);
    }
}

void tst_QMediaPlayer::testError_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testError()
{
    QFETCH(QMediaPlayer::Error, error);

    mockService->setError(error);
    QVERIFY(player->error() == error);
}

void tst_QMediaPlayer::testErrorString_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testErrorString()
{
    QFETCH(QString, errorString);

    mockService->setErrorString(errorString);
    QVERIFY(player->errorString() == errorString);
}

void tst_QMediaPlayer::testIsAvailable()
{
    QCOMPARE(player->isAvailable(), true);
    QCOMPARE(player->availability(), QMultimedia::Available);
}

void tst_QMediaPlayer::testService()
{
    /*
    QFETCH(bool, valid);

    mockService->setIsValid(valid);

    if (valid)
        QVERIFY(player->service() != 0);
    else
        QVERIFY(player->service() == 0);
        */
}

void tst_QMediaPlayer::testPlay_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testPlay()
{
    QFETCH(bool, valid);
    QFETCH(QUrl, mediaContent);
    QFETCH(QMediaPlayer::State, state);

    mockService->setIsValid(valid);
    mockService->setState(state);
    mockService->setMedia(mediaContent);
    QVERIFY(player->state() == state);
    QVERIFY(player->media() == mediaContent);

    QSignalSpy spy(player, SIGNAL(stateChanged(QMediaPlayer::State)));

    player->play();

    if (!valid || mediaContent.isEmpty())  {
        QCOMPARE(player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.count(), 0);
    }
    else {
        QCOMPARE(player->state(), QMediaPlayer::PlayingState);
        QCOMPARE(spy.count(), state == QMediaPlayer::PlayingState ? 0 : 1);
    }
}

void tst_QMediaPlayer::testPause_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testPause()
{
    QFETCH(bool, valid);
    QFETCH(QUrl, mediaContent);
    QFETCH(QMediaPlayer::State, state);

    mockService->setIsValid(valid);
    mockService->setState(state);
    mockService->setMedia(mediaContent);
    QVERIFY(player->state() == state);
    QVERIFY(player->media() == mediaContent);

    QSignalSpy spy(player, SIGNAL(stateChanged(QMediaPlayer::State)));

    player->pause();

    if (!valid || mediaContent.isEmpty()) {
        QCOMPARE(player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.count(), 0);
    }
    else {
        QCOMPARE(player->state(), QMediaPlayer::PausedState);
        QCOMPARE(spy.count(), state == QMediaPlayer::PausedState ? 0 : 1);
    }
}

void tst_QMediaPlayer::testStop_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testStop()
{
    QFETCH(QUrl, mediaContent);
    QFETCH(QMediaPlayer::State, state);

    mockService->setState(state);
    mockService->setMedia(mediaContent);
    QVERIFY(player->state() == state);
    QVERIFY(player->media() == mediaContent);

    QSignalSpy spy(player, SIGNAL(stateChanged(QMediaPlayer::State)));

    player->stop();

    if (mediaContent.isEmpty() || state == QMediaPlayer::StoppedState) {
        QCOMPARE(player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.count(), 0);
    }
    else {
        QCOMPARE(player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.count(), 1);
    }
}

void tst_QMediaPlayer::testMediaStatus_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testMediaStatus()
{
    QFETCH(int, bufferStatus);
    int bufferSignals = 0;

    player->setNotifyInterval(10);

    mockService->setMediaStatus(QMediaPlayer::NoMedia);
    mockService->setBufferStatus(bufferStatus);

    AutoConnection connection(
            player, SIGNAL(bufferStatusChanged(int)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));

    QSignalSpy statusSpy(player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy bufferSpy(player, SIGNAL(bufferStatusChanged(int)));

    QCOMPARE(player->mediaStatus(), QMediaPlayer::NoMedia);

    mockService->setMediaStatus(QMediaPlayer::LoadingMedia);
    QCOMPARE(player->mediaStatus(), QMediaPlayer::LoadingMedia);
    QCOMPARE(statusSpy.count(), 1);

    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)),
             QMediaPlayer::LoadingMedia);

    mockService->setMediaStatus(QMediaPlayer::LoadedMedia);
    QCOMPARE(player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(statusSpy.count(), 2);

    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)),
             QMediaPlayer::LoadedMedia);

    // Verify the bufferStatusChanged() signal isn't being emitted.
    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(bufferSpy.count(), 0);

    mockService->setMediaStatus(QMediaPlayer::StalledMedia);
    QCOMPARE(player->mediaStatus(), QMediaPlayer::StalledMedia);
    QCOMPARE(statusSpy.count(), 3);

    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)),
             QMediaPlayer::StalledMedia);

    // Verify the bufferStatusChanged() signal is being emitted.
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(bufferSpy.count() > bufferSignals);
    QCOMPARE(bufferSpy.last().value(0).toInt(), bufferStatus);
    bufferSignals = bufferSpy.count();

    mockService->setMediaStatus(QMediaPlayer::BufferingMedia);
    QCOMPARE(player->mediaStatus(), QMediaPlayer::BufferingMedia);
    QCOMPARE(statusSpy.count(), 4);

    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)),
             QMediaPlayer::BufferingMedia);

    // Verify the bufferStatusChanged() signal is being emitted.
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(bufferSpy.count() > bufferSignals);
    QCOMPARE(bufferSpy.last().value(0).toInt(), bufferStatus);
    bufferSignals = bufferSpy.count();

    mockService->setMediaStatus(QMediaPlayer::BufferedMedia);
    QCOMPARE(player->mediaStatus(), QMediaPlayer::BufferedMedia);
    QCOMPARE(statusSpy.count(), 5);

    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)),
             QMediaPlayer::BufferedMedia);

    // Verify the bufferStatusChanged() signal isn't being emitted.
    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(bufferSpy.count(), bufferSignals);

    mockService->setMediaStatus(QMediaPlayer::EndOfMedia);
    QCOMPARE(player->mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(statusSpy.count(), 6);

    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)),
             QMediaPlayer::EndOfMedia);
}

void tst_QMediaPlayer::testDestructor()
{
    //don't use the same service as tst_QMediaPlayer::player
    mockProvider->service = new MockMediaPlayerService;
    mockProvider->deleteServiceOnRelease = true;

    /* create an object for player */
    QMediaPlayer *victim = new QMediaPlayer;

    /* check whether the object is created */
    QVERIFY(victim);

    /* delete the instance (a crash is a failure :) */
    delete victim;

    //service is released
    QVERIFY(mockProvider->service == nullptr);

    mockProvider->deleteServiceOnRelease = false;
}

void tst_QMediaPlayer::testSetVideoOutput()
{
    MockVideoSurface surface;

    player->setVideoOutput(static_cast<QMediaSink *>(nullptr));

    QCOMPARE(mockService->rendererRef, 0);

    player->setVideoOutput(&surface);
    QVERIFY(mockService->rendererControl->surface() == &surface);
    QCOMPARE(mockService->rendererRef, 1);

    player->setVideoOutput(reinterpret_cast<QAbstractVideoSurface *>(0));
    QVERIFY(mockService->rendererControl->surface() == nullptr);

    //rendererControl is released
    QCOMPARE(mockService->rendererRef, 0);

    player->setVideoOutput(&surface);
    QVERIFY(mockService->rendererControl->surface() == &surface);
    QCOMPARE(mockService->rendererRef, 1);

    player->setVideoOutput(static_cast<QMediaSink *>(nullptr));
    QVERIFY(mockService->rendererControl->surface() == nullptr);
    //rendererControl is released
    QCOMPARE(mockService->rendererRef, 0);

    player->setVideoOutput(&surface);
    QVERIFY(mockService->rendererControl->surface() == &surface);
    QCOMPARE(mockService->rendererRef, 1);
}


void tst_QMediaPlayer::testSetVideoOutputNoService()
{
    MockVideoSurface surface;

    MockMediaServiceProvider provider(nullptr, true);
    QMediaServiceProvider::setDefaultServiceProvider(&provider);
    QMediaPlayer player;

    player.setVideoOutput(&surface);
    // Nothing we can verify here other than it doesn't assert.
}

void tst_QMediaPlayer::testSetVideoOutputNoControl()
{
    MockVideoSurface surface;

    MockMediaPlayerService service;
    service.rendererRef = 1;

    MockMediaServiceProvider provider(&service);
    QMediaServiceProvider::setDefaultServiceProvider(&provider);
    QMediaPlayer player;

    player.setVideoOutput(&surface);
    QVERIFY(service.rendererControl->surface() == nullptr);
}

void tst_QMediaPlayer::testSetVideoOutputDestruction()
{
    MockVideoSurface surface;
    {
        QMediaPlayer player;
        player.setVideoOutput(&surface);
        QVERIFY(mockService->rendererControl->surface() == &surface);
        QCOMPARE(mockService->rendererRef, 1);
    }
    QVERIFY(mockService->rendererControl->surface() == nullptr);
    QCOMPARE(mockService->rendererRef, 0);
}

void tst_QMediaPlayer::debugEnums()
{
    QTest::ignoreMessage(QtDebugMsg, "QMediaPlayer::PlayingState");
    qDebug() << QMediaPlayer::PlayingState;
    QTest::ignoreMessage(QtDebugMsg, "QMediaPlayer::NoMedia");
    qDebug() << QMediaPlayer::NoMedia;
    QTest::ignoreMessage(QtDebugMsg, "QMediaPlayer::NetworkError");
    qDebug() << QMediaPlayer::NetworkError;
}

void tst_QMediaPlayer::testSupportedMimeTypes()
{
    QStringList mimeList = QMediaPlayer::supportedMimeTypes(QMediaPlayer::LowLatency);

    // This is empty on some platforms, and not on others, so can't test something here at the moment.
}

void tst_QMediaPlayer::testQrc_data()
{
    QTest::addColumn<QUrl>("mediaContent");
    QTest::addColumn<QMediaPlayer::MediaStatus>("status");
    QTest::addColumn<QMediaPlayer::Error>("error");
    QTest::addColumn<int>("errorCount");
    QTest::addColumn<bool>("hasStreamFeature");
    QTest::addColumn<QString>("backendMediaContentScheme");
    QTest::addColumn<bool>("backendHasStream");

    QTest::newRow("invalid") << QUrl(QUrl(QLatin1String("qrc:/invalid.mp3")))
                             << QMediaPlayer::InvalidMedia
                             << QMediaPlayer::ResourceError
                             << 1 // error count
                             << false // No StreamPlayback support
                             << QString() // backend should not have got any media (empty URL scheme)
                             << false; // backend should not have got any stream

    QTest::newRow("valid+nostream") << QUrl(QUrl(QLatin1String("qrc:/testdata/nokia-tune.mp3")))
                                    << QMediaPlayer::LoadingMedia
                                    << QMediaPlayer::NoError
                                    << 0 // error count
                                    << false // No StreamPlayback support
                                    << QStringLiteral("file") // backend should have a got a temporary file
                                    << false; // backend should not have got any stream

    QTest::newRow("valid+stream") << QUrl(QUrl(QLatin1String("qrc:/testdata/nokia-tune.mp3")))
                                  << QMediaPlayer::LoadingMedia
                                  << QMediaPlayer::NoError
                                  << 0 // error count
                                  << true // StreamPlayback support
                                  << QStringLiteral("qrc")
                                  << true; // backend should have got a stream (QFile opened from the resource)
}

void tst_QMediaPlayer::testQrc()
{
    QFETCH(QUrl, mediaContent);
    QFETCH(QMediaPlayer::MediaStatus, status);
    QFETCH(QMediaPlayer::Error, error);
    QFETCH(int, errorCount);
    QFETCH(bool, hasStreamFeature);
    QFETCH(QString, backendMediaContentScheme);
    QFETCH(bool, backendHasStream);

    if (hasStreamFeature)
        mockProvider->setSupportedFeatures(QMediaServiceFeaturesInterface::StreamPlayback);

    QMediaPlayer player;

    mockService->setState(QMediaPlayer::PlayingState, QMediaPlayer::NoMedia);

    QSignalSpy mediaSpy(&player, SIGNAL(currentMediaChanged(QUrl)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy errorSpy(&player, SIGNAL(error(QMediaPlayer::Error)));

    player.setMedia(mediaContent);

    QTRY_COMPARE(player.mediaStatus(), status);
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)), status);

    QCOMPARE(player.media(), mediaContent);
    QCOMPARE(mediaSpy.count(), 1);
    QCOMPARE(qvariant_cast<QUrl>(mediaSpy.last().value(0)), mediaContent);

    QCOMPARE(player.error(), error);
    QCOMPARE(errorSpy.count(), errorCount);
    if (errorCount > 0) {
        QCOMPARE(qvariant_cast<QMediaPlayer::Error>(errorSpy.last().value(0)), error);
        QVERIFY(!player.errorString().isEmpty());
    }

    // Check the media actually passed to the backend
    QCOMPARE(mockService->mockControl->media().scheme(), backendMediaContentScheme);
    QCOMPARE(bool(mockService->mockControl->mediaStream()), backendHasStream);
}

void tst_QMediaPlayer::testAudioRole()
{
    {
        mockService->mockControl->hasAudioRole = false;
        QMediaPlayer player;

        QCOMPARE(player.audioRole(), QAudio::UnknownRole);
        QVERIFY(player.supportedAudioRoles().isEmpty());

        QSignalSpy spy(&player, SIGNAL(audioRoleChanged(QAudio::Role)));
        player.setAudioRole(QAudio::MusicRole);
        QCOMPARE(player.audioRole(), QAudio::MusicRole);
        QCOMPARE(spy.count(), 1);
    }

    {
        mockService->reset();
        mockService->mockControl->hasAudioRole = true;
        QMediaPlayer player;
        QSignalSpy spy(&player, SIGNAL(audioRoleChanged(QAudio::Role)));

        QCOMPARE(player.audioRole(), QAudio::UnknownRole);
        QVERIFY(!player.supportedAudioRoles().isEmpty());

        player.setAudioRole(QAudio::MusicRole);
        QCOMPARE(player.audioRole(), QAudio::MusicRole);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(qvariant_cast<QAudio::Role>(spy.last().value(0)), QAudio::MusicRole);

        spy.clear();

        player.setProperty("audioRole", QVariant::fromValue(QAudio::AlarmRole));
        QCOMPARE(qvariant_cast<QAudio::Role>(player.property("audioRole")), QAudio::AlarmRole);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(qvariant_cast<QAudio::Role>(spy.last().value(0)), QAudio::AlarmRole);
    }
}

void tst_QMediaPlayer::testCustomAudioRole()
{
    {
        mockService->mockControl->hasCustomAudioRole = false;
        QMediaPlayer player;

        QVERIFY(player.customAudioRole().isEmpty());
        QVERIFY(player.supportedCustomAudioRoles().isEmpty());

        QSignalSpy spyRole(&player, SIGNAL(audioRoleChanged(QAudio::Role)));
        QSignalSpy spyCustomRole(&player, SIGNAL(customAudioRoleChanged(const QString &)));
        player.setCustomAudioRole(QStringLiteral("customRole"));
        QCOMPARE(player.audioRole(), QAudio::CustomRole);
        QCOMPARE(player.customAudioRole(), QStringLiteral("customRole"));
        QCOMPARE(spyRole.count(), 1);
        QCOMPARE(spyCustomRole.count(), 1);
    }

    {
        mockService->reset();
        mockService->mockControl->hasAudioRole = false;
        QMediaPlayer player;

        QVERIFY(player.customAudioRole().isEmpty());
        QVERIFY(!player.supportedCustomAudioRoles().isEmpty());

        QSignalSpy spyRole(&player, SIGNAL(audioRoleChanged(QAudio::Role)));
        QSignalSpy spyCustomRole(&player, SIGNAL(customAudioRoleChanged(const QString &)));
        player.setCustomAudioRole(QStringLiteral("customRole"));
        QCOMPARE(player.audioRole(), QAudio::CustomRole);
        QCOMPARE(player.customAudioRole(), QStringLiteral("customRole"));
        QCOMPARE(spyRole.count(), 1);
        QCOMPARE(spyCustomRole.count(), 1);
    }

    {
        mockService->reset();
        QMediaPlayer player;
        QSignalSpy spyRole(&player, SIGNAL(audioRoleChanged(QAudio::Role)));
        QSignalSpy spyCustomRole(&player, SIGNAL(customAudioRoleChanged(const QString &)));

        QCOMPARE(player.audioRole(), QAudio::UnknownRole);
        QVERIFY(player.customAudioRole().isEmpty());
        QVERIFY(!player.supportedCustomAudioRoles().isEmpty());

        QString customRole(QStringLiteral("customRole"));
        player.setCustomAudioRole(customRole);
        QCOMPARE(player.audioRole(), QAudio::CustomRole);
        QCOMPARE(player.customAudioRole(), customRole);
        QCOMPARE(spyRole.count(), 1);
        QCOMPARE(qvariant_cast<QAudio::Role>(spyRole.last().value(0)), QAudio::CustomRole);
        QCOMPARE(spyCustomRole.count(), 1);
        QCOMPARE(qvariant_cast<QString>(spyCustomRole.last().value(0)), customRole);

        spyRole.clear();
        spyCustomRole.clear();

        QString customRole2(QStringLiteral("customRole2"));
        player.setProperty("customAudioRole", QVariant::fromValue(customRole2));
        QCOMPARE(qvariant_cast<QString>(player.property("customAudioRole")), customRole2);
        QCOMPARE(player.customAudioRole(), customRole2);
        QCOMPARE(spyRole.count(), 0);
        QCOMPARE(spyCustomRole.count(), 1);
        QCOMPARE(qvariant_cast<QString>(spyCustomRole.last().value(0)), customRole2);

        spyRole.clear();
        spyCustomRole.clear();

        player.setAudioRole(QAudio::MusicRole);
        QCOMPARE(player.audioRole(), QAudio::MusicRole);
        QVERIFY(player.customAudioRole().isEmpty());
        QCOMPARE(spyRole.count(), 1);
        QCOMPARE(qvariant_cast<QAudio::Role>(spyRole.last().value(0)), QAudio::MusicRole);
        QCOMPARE(spyCustomRole.count(), 1);
        QVERIFY(qvariant_cast<QString>(spyCustomRole.last().value(0)).isEmpty());

        spyRole.clear();
        spyCustomRole.clear();

        player.setAudioRole(QAudio::CustomRole);
        QCOMPARE(player.audioRole(), QAudio::CustomRole);
        QVERIFY(player.customAudioRole().isEmpty());
        QCOMPARE(spyRole.count(), 1);
        QCOMPARE(qvariant_cast<QAudio::Role>(spyRole.last().value(0)), QAudio::CustomRole);
        QCOMPARE(spyCustomRole.count(), 0);
    }
}

QTEST_GUILESS_MAIN(tst_QMediaPlayer)
#include "tst_qmediaplayer.moc"
