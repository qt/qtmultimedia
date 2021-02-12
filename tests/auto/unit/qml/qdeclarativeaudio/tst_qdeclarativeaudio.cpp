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

//TESTED_COMPONENT=plugins/declarative/multimedia

#include <QtTest/QtTest>

#include "qdeclarativeaudio_p.h"
#include "qdeclarativemediametadata_p.h"

#include "mockmediaplayer.h"
#include "qmockintegration_p.h"

#include <QtMultimedia/qmediametadata.h>
#include <private/qplatformmediaplayer_p.h>
#include <private/qdeclarativevideooutput_p.h>
#include <QAbstractVideoSurface>

#include <QtGui/qguiapplication.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>

class tst_QDeclarativeAudio : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();

private slots:
    void nullService();

    void source();
    void autoLoad();
    void playing();
    void paused();
    void duration();
    void position();
    void volume();
    void muted();
    void bufferProgress();
    void seekable();
    void playbackRate();
    void status();
    void metaData_data();
    void metaData();
    void error();
    void loops();
    void audioRole();
    void customAudioRole();
    void videoOutput();

private:
    void enumerator(const QMetaObject *object, const char *name, QMetaEnum *result);
    QMetaEnum enumerator(const QMetaObject *object, const char *name);
    void keyToValue(const QMetaEnum &enumeration, const char *key, int *result);
    int keyToValue(const QMetaEnum &enumeration, const char *key);

    QMockIntegration *mockIntegration;
};

Q_DECLARE_METATYPE(QDeclarativeAudio::Error);
Q_DECLARE_METATYPE(QDeclarativeAudio::AudioRole);

void tst_QDeclarativeAudio::initTestCase()
{
    qRegisterMetaType<QDeclarativeAudio::Error>();
    qRegisterMetaType<QDeclarativeAudio::AudioRole>();
    mockIntegration = new QMockIntegration;
}

void tst_QDeclarativeAudio::nullService()
{
    mockIntegration->setFlags(QMockIntegration::NoPlayerInterface);
    QDeclarativeAudio audio;
    audio.classBegin();

    QCOMPARE(audio.source(), QUrl());
    audio.setSource(QUrl("http://example.com"));
    QCOMPARE(audio.source(), QUrl("http://example.com"));

    QCOMPARE(audio.playbackState(), audio.StoppedState);
    audio.play();
    QCOMPARE(audio.playbackState(), audio.StoppedState);
    audio.pause();
    QCOMPARE(audio.playbackState(), audio.StoppedState);

    QCOMPARE(audio.duration(), 0);

    QCOMPARE(audio.position(), 0);
    audio.seek(10000);
    QCOMPARE(audio.position(), 10000);

    QCOMPARE(audio.volume(), qreal(1.0));
    audio.setVolume(0.5);
    QCOMPARE(audio.volume(), qreal(0.5));

    QCOMPARE(audio.isMuted(), false);
    audio.setMuted(true);
    QCOMPARE(audio.isMuted(), true);

    QCOMPARE(audio.bufferProgress(), qreal(0));

    QCOMPARE(audio.isSeekable(), false);

    QCOMPARE(audio.playbackRate(), qreal(1.0));

    QCOMPARE(audio.status(), QDeclarativeAudio::NoMedia);

    QCOMPARE(audio.error(), QDeclarativeAudio::ServiceMissing);

    QVERIFY(audio.metaData());
    mockIntegration->setFlags({});
}

void tst_QDeclarativeAudio::source()
{
    const QUrl url1("http://example.com");
    const QUrl url2("file:///local/path");
    const QUrl url3;

    QDeclarativeAudio audio;
    auto *player = mockIntegration->lastPlayer();
    audio.classBegin();
    audio.componentComplete();

    QSignalSpy spy(&audio, SIGNAL(sourceChanged()));

    audio.setSource(url1);
    QCOMPARE(audio.source(), url1);
    QCOMPARE(player->media(), url1);
    QCOMPARE(spy.count(), 1);

    audio.setSource(url2);
    QCOMPARE(audio.source(), url2);
    QCOMPARE(player->media(), url2);
    QCOMPARE(spy.count(), 2);

    audio.setSource(url3);
    QCOMPARE(audio.source(), url3);
    QCOMPARE(player->media(), url3);
    QCOMPARE(spy.count(), 3);
}

void tst_QDeclarativeAudio::autoLoad()
{
    QDeclarativeAudio audio;
    audio.classBegin();
    audio.componentComplete();

    QSignalSpy spy(&audio, SIGNAL(autoLoadChanged()));

    QCOMPARE(audio.isAutoLoad(), true);

    audio.setAutoLoad(false);
    QCOMPARE(audio.isAutoLoad(), false);
    QCOMPARE(spy.count(), 1);

    audio.setSource(QUrl("http://example.com"));
    QCOMPARE(audio.source(), QUrl("http://example.com"));
    audio.play();
    QCOMPARE(audio.playbackState(), audio.PlayingState);
    audio.stop();

    audio.setAutoLoad(true);
    audio.setSource(QUrl("http://example.com"));
    audio.pause();
    QCOMPARE(spy.count(), 2);
    QCOMPARE(audio.playbackState(), audio.PausedState);
}

void tst_QDeclarativeAudio::playing()
{
    QDeclarativeAudio audio;
    auto *mockPlayer = mockIntegration->lastPlayer();
    audio.classBegin();

    QSignalSpy stateChangedSpy(&audio, SIGNAL(playbackStateChanged()));
    QSignalSpy playingSpy(&audio, SIGNAL(playing()));
    QSignalSpy stoppedSpy(&audio, SIGNAL(stopped()));

    int stateChanged = 0;
    int playing = 0;
    int stopped = 0;

    audio.componentComplete();
    audio.setSource(QUrl("http://example.com"));

    QCOMPARE(audio.playbackState(), audio.StoppedState);

    // play() when stopped.
    audio.play();
    QCOMPARE(audio.playbackState(), audio.PlayingState);
    QCOMPARE(mockPlayer->state(), QMediaPlayer::PlayingState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(playingSpy.count(),        ++playing);
    QCOMPARE(stoppedSpy.count(),          stopped);

    // stop() when playing.
    audio.stop();
    QCOMPARE(audio.playbackState(), audio.StoppedState);
    QCOMPARE(mockPlayer->state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(playingSpy.count(),          playing);
    QCOMPARE(stoppedSpy.count(),        ++stopped);

    // stop() when stopped.
    audio.stop();
    QCOMPARE(audio.playbackState(), audio.StoppedState);
    QCOMPARE(mockPlayer->state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateChangedSpy.count(),   stateChanged);
    QCOMPARE(playingSpy.count(),          playing);
    QCOMPARE(stoppedSpy.count(),          stopped);

    audio.play();
    QCOMPARE(audio.playbackState(), audio.PlayingState);
    QCOMPARE(mockPlayer->state(), QMediaPlayer::PlayingState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(playingSpy.count(),        ++playing);
    QCOMPARE(stoppedSpy.count(),          stopped);

    // play() when playing.
    audio.play();
    QCOMPARE(audio.playbackState(), audio.PlayingState);
    QCOMPARE(mockPlayer->state(), QMediaPlayer::PlayingState);
    QCOMPARE(stateChangedSpy.count(),   stateChanged);
    QCOMPARE(playingSpy.count(),          playing);
    QCOMPARE(stoppedSpy.count(),          stopped);
}

void tst_QDeclarativeAudio::paused()
{
    QDeclarativeAudio audio;
    auto *player = mockIntegration->lastPlayer();
    audio.classBegin();

    QSignalSpy stateChangedSpy(&audio, SIGNAL(playbackStateChanged()));
    QSignalSpy pausedSpy(&audio, SIGNAL(paused()));
    int stateChanged = 0;
    int pausedCount = 0;

    audio.componentComplete();
    audio.setSource(QUrl("http://example.com"));

    QCOMPARE(audio.playbackState(), audio.StoppedState);

    // play() when stopped.
    audio.play();
    QCOMPARE(audio.playbackState(), audio.PlayingState);
    QCOMPARE(player->state(), QMediaPlayer::PlayingState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(pausedSpy.count(), pausedCount);

    // pause() when playing.
    audio.pause();
    QCOMPARE(audio.playbackState(), audio.PausedState);
    QCOMPARE(player->state(), QMediaPlayer::PausedState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(pausedSpy.count(), ++pausedCount);

    // pause() when paused.
    audio.pause();
    QCOMPARE(audio.playbackState(), audio.PausedState);
    QCOMPARE(player->state(), QMediaPlayer::PausedState);
    QCOMPARE(stateChangedSpy.count(),   stateChanged);
    QCOMPARE(pausedSpy.count(), pausedCount);

    // stop() when paused.
    audio.stop();
    QCOMPARE(audio.playbackState(), audio.StoppedState);
    QCOMPARE(player->state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(pausedSpy.count(), pausedCount);

    // pause() when stopped.
    audio.pause();
    QCOMPARE(audio.playbackState(), audio.PausedState);
    QCOMPARE(player->state(), QMediaPlayer::PausedState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(pausedSpy.count(), ++pausedCount);

    // play() when paused.
    audio.play();
    QCOMPARE(audio.playbackState(), audio.PlayingState);
    QCOMPARE(player->state(), QMediaPlayer::PlayingState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(pausedSpy.count(), pausedCount);
}

void tst_QDeclarativeAudio::duration()
{
    QDeclarativeAudio audio;
    auto *player = mockIntegration->lastPlayer();
    audio.classBegin();
    audio.componentComplete();

    QSignalSpy spy(&audio, SIGNAL(durationChanged()));

    QCOMPARE(audio.duration(), 0);

    player->setDuration(4040);
    QCOMPARE(audio.duration(), 4040);
    QCOMPARE(spy.count(), 1);

    player->setDuration(-129);
    QCOMPARE(audio.duration(), -129);
    QCOMPARE(spy.count(), 2);

    player->setDuration(0);
    QCOMPARE(audio.duration(), 0);
    QCOMPARE(spy.count(), 3);

    // Unnecessary duration changed signals aren't filtered.
    player->setDuration(0);
    QCOMPARE(audio.duration(), 0);
    QCOMPARE(spy.count(), 4);
}

void tst_QDeclarativeAudio::position()
{
    QDeclarativeAudio audio;
    auto *player = mockIntegration->lastPlayer();
    audio.classBegin();
    audio.componentComplete();

    QSignalSpy spy(&audio, SIGNAL(positionChanged()));

    QCOMPARE(audio.position(), 0);

    // QDeclarativeAudio won't bound set positions to the duration.  A media service may though.
    QCOMPARE(audio.duration(), 0);

    audio.seek(450);
    QCOMPARE(audio.position(), 450);
    QCOMPARE(player->position(), qint64(450));
    QCOMPARE(spy.count(), 1);

    audio.seek(-5403);
    QCOMPARE(audio.position(), 0);
    QCOMPARE(player->position(), qint64(0));
    QCOMPARE(spy.count(), 2);

    audio.seek(-5403);
    QCOMPARE(audio.position(), 0);
    QCOMPARE(player->position(), qint64(0));
    QCOMPARE(spy.count(), 2);

    // Check the signal change signal is emitted if the change originates from the media service.
    player->setPosition(450);
    QCOMPARE(audio.position(), 450);
    QCOMPARE(spy.count(), 3);

    connect(&audio, SIGNAL(positionChanged()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    player->updateState(QMediaPlayer::PlayingState);
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(spy.count() > 3 && spy.count() < 6); // 4 or 5

    player->updateState(QMediaPlayer::PausedState);
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(spy.count() < 6);
}

void tst_QDeclarativeAudio::volume()
{
    QDeclarativeAudio audio;
    auto *player = mockIntegration->lastPlayer();
    audio.classBegin();
    audio.componentComplete();

    QSignalSpy spy(&audio, SIGNAL(volumeChanged()));

    QCOMPARE(audio.volume(), qreal(1.0));

    audio.setVolume(0.7);
    QCOMPARE(audio.volume(), qreal(0.7));
    QCOMPARE(player->volume(), 70);
    QCOMPARE(spy.count(), 1);

    audio.setVolume(0.7);
    QCOMPARE(audio.volume(), qreal(0.7));
    QCOMPARE(player->volume(), 70);
    QCOMPARE(spy.count(), 1);

    player->setVolume(30);
    QCOMPARE(audio.volume(), qreal(0.3));
    QCOMPARE(spy.count(), 2);
}

void tst_QDeclarativeAudio::muted()
{
    QDeclarativeAudio audio;
    auto *player = mockIntegration->lastPlayer();
    audio.classBegin();
    audio.componentComplete();

    QSignalSpy spy(&audio, SIGNAL(mutedChanged()));

    QCOMPARE(audio.isMuted(), false);

    audio.setMuted(true);
    QCOMPARE(audio.isMuted(), true);
    QCOMPARE(player->isMuted(), true);
    QCOMPARE(spy.count(), 1);

    player->setMuted(false);
    QCOMPARE(audio.isMuted(), false);
    QCOMPARE(spy.count(), 2);

    audio.setMuted(false);
    QCOMPARE(audio.isMuted(), false);
    QCOMPARE(player->isMuted(), false);
    QCOMPARE(spy.count(), 2);
}

void tst_QDeclarativeAudio::bufferProgress()
{
    QDeclarativeAudio audio;
    auto *player = mockIntegration->lastPlayer();
    audio.classBegin();
    audio.componentComplete();

    QSignalSpy spy(&audio, SIGNAL(bufferProgressChanged()));

    QCOMPARE(audio.bufferProgress(), qreal(0.0));

    player->setBufferStatus(20);
    QCOMPARE(audio.bufferProgress(), qreal(0.2));
    QCOMPARE(spy.count(), 1);

    player->setBufferStatus(20);
    QCOMPARE(audio.bufferProgress(), qreal(0.2));
    QCOMPARE(spy.count(), 2);

    player->setBufferStatus(40);
    QCOMPARE(audio.bufferProgress(), qreal(0.4));
    QCOMPARE(spy.count(), 3);

    connect(&audio, SIGNAL(positionChanged()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    player->updateMediaStatus(
            QMediaPlayer::BufferingMedia, QMediaPlayer::PlayingState);
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(spy.count() > 3 && spy.count() < 6); // 4 or 5

    player->updateMediaStatus(QMediaPlayer::BufferedMedia);
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(spy.count() < 6);
}

void tst_QDeclarativeAudio::seekable()
{
    QDeclarativeAudio audio;
    auto *player = mockIntegration->lastPlayer();
    audio.classBegin();
    audio.componentComplete();

    QSignalSpy spy(&audio, SIGNAL(seekableChanged()));

    QCOMPARE(audio.isSeekable(), false);

    player->setSeekable(true);
    QCOMPARE(audio.isSeekable(), true);
    QCOMPARE(spy.count(), 1);

    player->setSeekable(true);
    QCOMPARE(audio.isSeekable(), true);
    QCOMPARE(spy.count(), 2);

    player->setSeekable(false);
    QCOMPARE(audio.isSeekable(), false);
    QCOMPARE(spy.count(), 3);
}

void tst_QDeclarativeAudio::playbackRate()
{
    QDeclarativeAudio audio;
    auto *player = mockIntegration->lastPlayer();
    audio.classBegin();
    audio.componentComplete();

    QSignalSpy spy(&audio, SIGNAL(playbackRateChanged()));

    QCOMPARE(audio.playbackRate(), qreal(1.0));

    audio.setPlaybackRate(0.5);
    QCOMPARE(audio.playbackRate(), qreal(0.5));
    QCOMPARE(player->playbackRate(), qreal(0.5));
    QCOMPARE(spy.count(), 1);

    player->setPlaybackRate(2.0);
    QCOMPARE(player->playbackRate(), qreal(2.0));
    QCOMPARE(spy.count(), 2);

    audio.setPlaybackRate(2.0);
    QCOMPARE(audio.playbackRate(), qreal(2.0));
    QCOMPARE(player->playbackRate(), qreal(2.0));
    QCOMPARE(spy.count(), 2);
}

void tst_QDeclarativeAudio::status()
{
    QDeclarativeAudio audio;
    auto *player = mockIntegration->lastPlayer();
    audio.classBegin();
    audio.componentComplete();

    QSignalSpy statusChangedSpy(&audio, SIGNAL(statusChanged()));

    QCOMPARE(audio.status(), QDeclarativeAudio::NoMedia);

    // Set media, start loading.
    player->updateMediaStatus(QMediaPlayer::LoadingMedia);
    QCOMPARE(audio.status(), QDeclarativeAudio::Loading);
    QCOMPARE(statusChangedSpy.count(), 1);

    // Finish loading.
    player->updateMediaStatus(QMediaPlayer::LoadedMedia);
    QCOMPARE(audio.status(), QDeclarativeAudio::Loaded);
    QCOMPARE(statusChangedSpy.count(), 2);

    // Play, start buffering.
    player->updateMediaStatus(
            QMediaPlayer::StalledMedia, QMediaPlayer::PlayingState);
    QCOMPARE(audio.status(), QDeclarativeAudio::Stalled);
    QCOMPARE(statusChangedSpy.count(), 3);

    // Enough data buffered to proceed.
    player->updateMediaStatus(QMediaPlayer::BufferingMedia);
    QCOMPARE(audio.status(), QDeclarativeAudio::Buffering);
    QCOMPARE(statusChangedSpy.count(), 4);

    // Errant second buffering status changed.
    player->updateMediaStatus(QMediaPlayer::BufferingMedia);
    QCOMPARE(audio.status(), QDeclarativeAudio::Buffering);
    QCOMPARE(statusChangedSpy.count(), 4);

    // Buffer full.
    player->updateMediaStatus(QMediaPlayer::BufferedMedia);
    QCOMPARE(audio.status(), QDeclarativeAudio::Buffered);
    QCOMPARE(statusChangedSpy.count(), 5);

    // Buffer getting low.
    player->updateMediaStatus(QMediaPlayer::BufferingMedia);
    QCOMPARE(audio.status(), QDeclarativeAudio::Buffering);
    QCOMPARE(statusChangedSpy.count(), 6);

    // Buffer full.
    player->updateMediaStatus(QMediaPlayer::BufferedMedia);
    QCOMPARE(audio.status(), QDeclarativeAudio::Buffered);
    QCOMPARE(statusChangedSpy.count(), 7);

    // Finished.
    player->updateMediaStatus(
            QMediaPlayer::EndOfMedia, QMediaPlayer::StoppedState);
    QCOMPARE(audio.status(), QDeclarativeAudio::EndOfMedia);
    QCOMPARE(statusChangedSpy.count(), 8);
}

void tst_QDeclarativeAudio::metaData_data()
{
    QTest::addColumn<QByteArray>("propertyName");
    QTest::addColumn<QString>("propertyKey");
    QTest::addColumn<QVariant>("value");

    QTest::newRow("title")
            << QByteArray("title")
            << QMediaMetaData::Title
            << QVariant(QString::fromLatin1("This is a title"));

    QTest::newRow("genre")
            << QByteArray("genre")
            << QMediaMetaData::Genre
            << QVariant(QString::fromLatin1("rock"));

    QTest::newRow("trackNumber")
            << QByteArray("trackNumber")
            << QMediaMetaData::TrackNumber
            << QVariant(8);
}

void tst_QDeclarativeAudio::metaData()
{
    QFETCH(QByteArray, propertyName);
    QFETCH(QString, propertyKey);
    QFETCH(QVariant, value);

    QDeclarativeAudio audio;
    audio.classBegin();
    audio.componentComplete();

    QSignalSpy spy(audio.metaData(), SIGNAL(metaDataChanged()));

    const int index = audio.metaData()->metaObject()->indexOfProperty(propertyName.constData());
    QVERIFY(index != -1);

    QMetaProperty property = audio.metaData()->metaObject()->property(index);
    QCOMPARE(property.read(&audio), QVariant());

    property.write(audio.metaData(), value);
    QCOMPARE(property.read(audio.metaData()), QVariant());
    QCOMPARE(spy.count(), 0);
}

void tst_QDeclarativeAudio::error()
{
    const QString errorString = QLatin1String("Failed to open device.");

    QDeclarativeAudio audio;
    auto *player = mockIntegration->lastPlayer();
    audio.classBegin();
    audio.componentComplete();

    QSignalSpy errorSpy(&audio, SIGNAL(error(QDeclarativeAudio::Error,QString)));
    QSignalSpy errorChangedSpy(&audio, SIGNAL(errorChanged()));

    QCOMPARE(audio.error(), QDeclarativeAudio::NoError);
    QCOMPARE(audio.errorString(), QString());

    player->emitError(QMediaPlayer::ResourceError, errorString);

    QCOMPARE(audio.error(), QDeclarativeAudio::ResourceError);
    QCOMPARE(audio.errorString(), errorString);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorChangedSpy.count(), 1);

    // Changing the source resets the error properties.
    audio.setSource(QUrl("http://example.com"));
    QCOMPARE(audio.error(), QDeclarativeAudio::NoError);
    QCOMPARE(audio.errorString(), QString());
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorChangedSpy.count(), 2);

    // But isn't noisy.
    audio.setSource(QUrl("file:///file/path"));
    QCOMPARE(audio.error(), QDeclarativeAudio::NoError);
    QCOMPARE(audio.errorString(), QString());
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorChangedSpy.count(), 2);
}

void tst_QDeclarativeAudio::loops()
{
    QDeclarativeAudio audio;
    auto *player = mockIntegration->lastPlayer();

    QSignalSpy loopsChangedSpy(&audio, SIGNAL(loopCountChanged()));
    QSignalSpy stateChangedSpy(&audio, SIGNAL(playbackStateChanged()));
    QSignalSpy stoppedSpy(&audio, SIGNAL(stopped()));

    int stateChanged = 0;
    int loopsChanged = 0;
    int stoppedCount = 0;

    audio.classBegin();
    audio.componentComplete();

    QCOMPARE(audio.playbackState(), audio.StoppedState);

    //setLoopCount(3) when stopped.
    audio.setLoopCount(3);
    QCOMPARE(audio.loopCount(), 3);
    QCOMPARE(loopsChangedSpy.count(), ++loopsChanged);

    //play till end
    audio.play();
    QCOMPARE(audio.playbackState(), audio.PlayingState);
    QCOMPARE(player->state(), QMediaPlayer::PlayingState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(stoppedSpy.count(), stoppedCount);

    // play() when playing.
    audio.play();
    QCOMPARE(audio.playbackState(), audio.PlayingState);
    QCOMPARE(player->state(), QMediaPlayer::PlayingState);
    QCOMPARE(stateChangedSpy.count(),   stateChanged);
    QCOMPARE(stoppedSpy.count(), stoppedCount);

    player->updateMediaStatus(QMediaPlayer::EndOfMedia, QMediaPlayer::StoppedState);
    QCOMPARE(audio.playbackState(), audio.PlayingState);
    QCOMPARE(player->state(), QMediaPlayer::PlayingState);
    QCOMPARE(stateChangedSpy.count(),   stateChanged);
    QCOMPARE(stoppedSpy.count(), stoppedCount);

    //play to end
    player->updateMediaStatus(QMediaPlayer::EndOfMedia, QMediaPlayer::StoppedState);
    QCOMPARE(stoppedSpy.count(), stoppedCount);
    //play to end
    player->updateMediaStatus(QMediaPlayer::EndOfMedia, QMediaPlayer::StoppedState);
    QCOMPARE(audio.playbackState(), audio.StoppedState);
    QCOMPARE(player->state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(stoppedSpy.count(), ++stoppedCount);

    // stop when playing
    audio.play();
    QCOMPARE(player->state(), QMediaPlayer::PlayingState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(stoppedSpy.count(), stoppedCount);
    player->updateMediaStatus(QMediaPlayer::EndOfMedia, QMediaPlayer::StoppedState);
    audio.stop();
    QCOMPARE(audio.playbackState(), audio.StoppedState);
    QCOMPARE(player->state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(stoppedSpy.count(), ++stoppedCount);

    //play() with infinite loop
    audio.setLoopCount(-1);
    QCOMPARE(audio.loopCount(), -1);
    QCOMPARE(loopsChangedSpy.count(), ++loopsChanged);
    audio.play();
    QCOMPARE(audio.playbackState(), audio.PlayingState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(player->state(), QMediaPlayer::PlayingState);
    player->updateMediaStatus(QMediaPlayer::EndOfMedia, QMediaPlayer::StoppedState);
    QCOMPARE(audio.playbackState(), audio.PlayingState);
    QCOMPARE(player->state(), QMediaPlayer::PlayingState);
    QCOMPARE(stateChangedSpy.count(), stateChanged);

    // stop() when playing in infinite loop.
    audio.stop();
    QCOMPARE(audio.playbackState(), audio.StoppedState);
    QCOMPARE(player->state(), QMediaPlayer::StoppedState);
    QCOMPARE(stateChangedSpy.count(), ++stateChanged);
    QCOMPARE(stoppedSpy.count(), ++stoppedCount);

    qDebug() << "Testing version 5";
}

void tst_QDeclarativeAudio::audioRole()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0 \n import QtMultimedia 5.6 \n Audio { }", QUrl());

    {
        QDeclarativeAudio *audio = static_cast<QDeclarativeAudio*>(component.create());
        auto *player = mockIntegration->lastPlayer();
        player->hasAudioRole = false;

        QCOMPARE(audio->audioRole(), QDeclarativeAudio::UnknownRole);
        QVERIFY(audio->supportedAudioRoles().isArray());
        QVERIFY(audio->supportedAudioRoles().toVariant().toList().isEmpty());

        QSignalSpy spy(audio, SIGNAL(audioRoleChanged()));
        audio->setAudioRole(QDeclarativeAudio::MusicRole);
        QCOMPARE(audio->audioRole(), QDeclarativeAudio::MusicRole);
        QCOMPARE(spy.count(), 1);
    }

    {
        QDeclarativeAudio *audio = static_cast<QDeclarativeAudio*>(component.create());
        auto *player = mockIntegration->lastPlayer();
        player->hasAudioRole = false;
        QSignalSpy spy(audio, SIGNAL(audioRoleChanged()));

        QCOMPARE(audio->audioRole(), QDeclarativeAudio::UnknownRole);
        QVERIFY(audio->supportedAudioRoles().isArray());
        QVERIFY(!audio->supportedAudioRoles().toVariant().toList().isEmpty());

        audio->setAudioRole(QDeclarativeAudio::MusicRole);
        QCOMPARE(audio->audioRole(), QDeclarativeAudio::MusicRole);
        QCOMPARE(spy.count(), 1);
    }
}

void tst_QDeclarativeAudio::customAudioRole()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0 \n import QtMultimedia 5.11 \n Audio { }", QUrl());
    auto *player = mockIntegration->lastPlayer();

    {
        player->hasCustomAudioRole = false;
        QObject *audio = component.create();
        QVERIFY(audio);

        QMetaEnum audioRoleEnum = enumerator(audio->metaObject(), "AudioRole");
        int AudioRole_CustomRoleValue = keyToValue(audioRoleEnum, "CustomRole");
        QVERIFY(!QTest::currentTestFailed());

        QVERIFY(audio->property("customAudioRole").toString().isEmpty());

        QSignalSpy spyRole(audio, SIGNAL(audioRoleChanged()));
        QSignalSpy spyCustomRole(audio, SIGNAL(customAudioRoleChanged()));
        audio->setProperty("customAudioRole", QStringLiteral("customRole"));
        QCOMPARE(audio->property("audioRole").toInt(), AudioRole_CustomRoleValue);
        QCOMPARE(audio->property("customAudioRole").toString(), QStringLiteral("customRole"));
        QCOMPARE(spyRole.count(), 1);
        QCOMPARE(spyCustomRole.count(), 1);
    }

    {
        player->hasAudioRole = false;

        QObject *audio = component.create();
        QVERIFY(audio);

        QMetaEnum audioRoleEnum = enumerator(audio->metaObject(), "AudioRole");
        int AudioRole_CustomRoleValue = keyToValue(audioRoleEnum, "CustomRole");
        QVERIFY(!QTest::currentTestFailed());

        QVERIFY(audio->property("customAudioRole").toString().isEmpty());

        QSignalSpy spyRole(audio, SIGNAL(audioRoleChanged()));
        QSignalSpy spyCustomRole(audio, SIGNAL(customAudioRoleChanged()));
        audio->setProperty("customAudioRole", QStringLiteral("customRole"));
        QCOMPARE(audio->property("audioRole").toInt(), AudioRole_CustomRoleValue);
        QCOMPARE(audio->property("customAudioRole").toString(), QStringLiteral("customRole"));
        QCOMPARE(spyRole.count(), 1);
        QCOMPARE(spyCustomRole.count(), 1);
    }

    {
        QObject *audio = component.create();
        QVERIFY(audio);

        QMetaEnum audioRoleEnum = enumerator(audio->metaObject(), "AudioRole");
        int AudioRole_UnknownRoleValue = keyToValue(audioRoleEnum, "UnknownRole");
        int AudioRole_CustomRoleValue = keyToValue(audioRoleEnum, "CustomRole");
        int AudioRole_MusicRoleValue = keyToValue(audioRoleEnum, "MusicRole");
        QVERIFY(!QTest::currentTestFailed());

        QSignalSpy spyRole(audio, SIGNAL(audioRoleChanged()));
        QSignalSpy spyCustomRole(audio, SIGNAL(customAudioRoleChanged()));

        QCOMPARE(audio->property("audioRole").toInt(), AudioRole_UnknownRoleValue);
        QVERIFY(audio->property("customAudioRole").toString().isEmpty());

        QString customRole(QStringLiteral("customRole"));
        audio->setProperty("customAudioRole", customRole);
        QCOMPARE(audio->property("audioRole").toInt(), AudioRole_CustomRoleValue);
        QCOMPARE(audio->property("customAudioRole").toString(), customRole);
        QCOMPARE(spyRole.count(), 1);
        QCOMPARE(spyCustomRole.count(), 1);

        spyRole.clear();
        spyCustomRole.clear();

        QString customRole2(QStringLiteral("customRole2"));
        audio->setProperty("customAudioRole", customRole2);
        QCOMPARE(audio->property("customAudioRole").toString(), customRole2);
        QCOMPARE(spyRole.count(), 0);
        QCOMPARE(spyCustomRole.count(), 1);

        spyRole.clear();
        spyCustomRole.clear();

        audio->setProperty("audioRole", AudioRole_MusicRoleValue);
        QCOMPARE(audio->property("audioRole").toInt(), AudioRole_MusicRoleValue);
        QVERIFY(audio->property("customAudioRole").toString().isEmpty());
        QCOMPARE(spyRole.count(), 1);
        QCOMPARE(spyCustomRole.count(), 1);

        spyRole.clear();
        spyCustomRole.clear();

        audio->setProperty("audioRole", AudioRole_CustomRoleValue);
        QCOMPARE(audio->property("audioRole").toInt(), AudioRole_CustomRoleValue);
        QVERIFY(audio->property("customAudioRole").toString().isEmpty());
        QCOMPARE(spyRole.count(), 1);
        QCOMPARE(spyCustomRole.count(), 0);
    }
}

void tst_QDeclarativeAudio::enumerator(const QMetaObject *object,
                                       const char *name,
                                       QMetaEnum *result)
{
    int index = object->indexOfEnumerator(name);
    QVERIFY(index >= 0);
    *result = object->enumerator(index);
}

QMetaEnum tst_QDeclarativeAudio::enumerator(const QMetaObject *object, const char *name)
{
    QMetaEnum result;
    enumerator(object, name, &result);
    return result;
}

void tst_QDeclarativeAudio::keyToValue(const QMetaEnum &enumeration, const char *key, int *result)
{
    bool ok = false;
    *result = enumeration.keyToValue(key, &ok);
    QVERIFY(ok);
}

int tst_QDeclarativeAudio::keyToValue(const QMetaEnum &enumeration, const char *key)
{
    int result;
    keyToValue(enumeration, key, &result);
    return result;
}

struct Surface : QAbstractVideoSurface
{
    Surface(QObject *parent = nullptr) : QAbstractVideoSurface(parent) { }
    [[nodiscard]] QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType) const override
    {
        return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32;
    }

    bool present(const QVideoFrame &) override { return true; }
};

void tst_QDeclarativeAudio::videoOutput()
{
    QDeclarativeAudio audio;
    QSignalSpy spy(&audio, &QDeclarativeAudio::videoOutputChanged);

    audio.classBegin();
    audio.componentComplete();

    QVERIFY(audio.videoOutput().isNull());

    QVariant surface;
    surface.setValue(new Surface(this));
    audio.setVideoOutput(surface);
    QCOMPARE(audio.videoOutput(), surface);
    QCOMPARE(spy.count(), 1);

    QQmlEngine engine;
    QJSValue jsArray = engine.newArray(5);
    jsArray.setProperty(0, engine.newQObject(new Surface(this)));
    jsArray.setProperty(1, engine.newQObject(new Surface(this)));
    QDeclarativeVideoOutput output;
    jsArray.setProperty(2, engine.newQObject(&output));
    jsArray.setProperty(3, 123);
    jsArray.setProperty(4, QLatin1String("ignore this"));

    QVariant surfaces;
    surfaces.setValue(jsArray);
    audio.setVideoOutput(surfaces);

    auto arr = audio.videoOutput().value<QJSValue>();
    QVERIFY(arr.equals(jsArray));
    QCOMPARE(spy.count(), 2);
}

QTEST_MAIN(tst_QDeclarativeAudio)

#include "tst_qdeclarativeaudio.moc"
