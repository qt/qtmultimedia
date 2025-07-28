// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtCore/qdebug.h>
#include <QtCore/qbuffer.h>

#include <qvideosink.h>
#include <qmediaplayer.h>
#include <private/qplatformmediaplayer_p.h>
#include <qobject.h>

#include "qmockintegration.h"
#include "qmockmediaplayer.h"
#include "qmockaudiooutput.h"
#include "qvideosink.h"
#include "qaudiooutput.h"
#include <QtMultimedia/qplaybackoptions.h>

using namespace std::chrono_literals;

QT_USE_NAMESPACE

Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN

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
    void play_doesNotResetError();
    void setSource_resetsError();
    void setSourceDevice_resetsError();
    void testValid();
    void testMedia_data();
    void testMedia();
    void testMultipleMedia_data();
    void testMultipleMedia();
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
    void testSetVideoOutputDestruction();
    void debugEnums();
    void testDestructor();
    void testQrc_data();
    void testQrc();
    void playbackOptions_returnsDefaultAfterConstruction();
    void setPlaybackOptions_setsPlaybackOptions();
    void setPlaybackOptions_doesNotEmitChangeSignal_whenOptionsDidNotChange();
    void resetPlaybackOptions_resetsPlaybackOptionsToDefault();

private:
    void setupCommonTestData();

    QMockMediaPlayer *mockPlayer;
    QAudioOutput *audioOutput = nullptr;
    QMediaPlayer *player;
};

class QtTestMediaPlayer : public QMediaPlayer
{
    Q_OBJECT
    Q_PROPERTY(int a READ a WRITE setA NOTIFY aChanged)
    Q_PROPERTY(int b READ b WRITE setB NOTIFY bChanged)
    Q_PROPERTY(int c READ c WRITE setC NOTIFY cChanged)
    Q_PROPERTY(int d READ d WRITE setD)
public:
    QtTestMediaPlayer() : QMediaPlayer() {}

    [[nodiscard]] int a() const { return m_a; }
    void setA(int a) { m_a = a; }

    [[nodiscard]] int b() const { return m_b; }
    void setB(int b) { m_b = b; }

    [[nodiscard]] int c() const { return m_c; }
    void setC(int c) { m_c = c; }

    [[nodiscard]] int d() const { return m_d; }
    void setD(int d) { m_d = d; }

Q_SIGNALS:
    void aChanged(int a);
    void bChanged(int b);
    void cChanged(int c);

private:
    int m_a = 0;
    int m_b = 0;
    int m_c = 0;
    int m_d = 0;
};

void tst_QMediaPlayer::setupCommonTestData()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QMediaPlayer::PlaybackState>("state");
    QTest::addColumn<QMediaPlayer::MediaStatus>("status");
    QTest::addColumn<QUrl>("mediaContent");
    QTest::addColumn<qint64>("duration");
    QTest::addColumn<qint64>("position");
    QTest::addColumn<bool>("seekable");
    QTest::addColumn<int>("volume");
    QTest::addColumn<bool>("muted");
    QTest::addColumn<bool>("videoAvailable");
    QTest::addColumn<int>("bufferProgress");
    QTest::addColumn<qreal>("playbackRate");
    QTest::addColumn<QMediaPlayer::Error>("error");
    QTest::addColumn<QString>("errorString");

    QTest::newRow("invalid") << false << QMediaPlayer::StoppedState << QMediaPlayer::InvalidMedia <<
                                QUrl() << qint64(0) << qint64(0) << false << 0 << false << false << 0 <<
                                qreal(0) << QMediaPlayer::NoError << QString();
    QTest::newRow("valid+null") << true << QMediaPlayer::StoppedState << QMediaPlayer::InvalidMedia <<
                                QUrl() << qint64(0) << qint64(0) << false << 0 << false << false << 50 <<
                                qreal(0) << QMediaPlayer::NoError << QString();
    QTest::newRow("valid+content+stopped") << true << QMediaPlayer::StoppedState << QMediaPlayer::InvalidMedia <<
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
    QTest::newRow("valid+error") << true << QMediaPlayer::StoppedState << QMediaPlayer::InvalidMedia <<
                                QUrl(QUrl("http://example.com/stream")) << qint64(0) << qint64(0) << false << 50 << false << false << 0 <<
                                qreal(0) << QMediaPlayer::ResourceError << QStringLiteral("Resource unavailable");
}

void tst_QMediaPlayer::initTestCase()
{
}

void tst_QMediaPlayer::cleanupTestCase()
{
}

void tst_QMediaPlayer::init()
{
    player = new QMediaPlayer;
    mockPlayer = QMockIntegration::instance()->lastPlayer();
    QTEST_ASSERT(mockPlayer);
    audioOutput = new QAudioOutput;
    player->setAudioOutput(audioOutput);
    QTEST_ASSERT(mockPlayer->m_audioOutput != nullptr);
}

void tst_QMediaPlayer::cleanup()
{
    delete player;
}

void tst_QMediaPlayer::play_doesNotResetError()
{
    player->setSource({ "https://dummy/dummy.mp3" });

    mockPlayer->setError(QMediaPlayer::AccessDeniedError);
    mockPlayer->setErrorString(QStringLiteral(u"Error"));
    QSignalSpy errorSpy{ player, &QMediaPlayer::errorChanged };

    player->play();

    QCOMPARE_EQ(player->error(), QMediaPlayer::AccessDeniedError);
    QCOMPARE_EQ(player->errorString(), QStringLiteral(u"Error"));
    QCOMPARE_EQ(errorSpy.size(), 0);
}

void tst_QMediaPlayer::setSource_resetsError()
{
    mockPlayer->setError(QMediaPlayer::AccessDeniedError);
    mockPlayer->setErrorString(QStringLiteral(u"Error"));
    QSignalSpy errorSpy{ player, &QMediaPlayer::errorChanged };

    player->setSource({ "https://dummy/dummy.mp3" });

    QCOMPARE_EQ(player->error(), QMediaPlayer::NoError);
    QVERIFY(player->errorString().isEmpty());
    QCOMPARE_EQ(errorSpy.size(), 1);
}

void tst_QMediaPlayer::setSourceDevice_resetsError()
{
    mockPlayer->setError(QMediaPlayer::AccessDeniedError);
    mockPlayer->setErrorString(QStringLiteral(u"Error"));
    QSignalSpy errorSpy{ player, &QMediaPlayer::errorChanged };

    player->setSourceDevice(nullptr, { "https://dummy/dummy.mp3" });

    QCOMPARE_EQ(player->error(), QMediaPlayer::NoError);
    QVERIFY(player->errorString().isEmpty());
    QCOMPARE_EQ(errorSpy.size(), 1);
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

    player->setSource(mediaContent);
    QCOMPARE(player->source(), mediaContent);

    QBuffer stream;
    player->setSourceDevice(&stream, mediaContent);
    QCOMPARE(player->source(), mediaContent);
    QCOMPARE(player->sourceDevice(), &stream);
}

void tst_QMediaPlayer::testMultipleMedia_data()
{
    QTest::addColumn<QUrl>("mediaContent");
    QTest::addColumn<QUrl>("anotherMediaContent");

    QTest::newRow("multipleSources")
            << QUrl(QUrl("file:///some.mp3")) << QUrl(QUrl("file:///someother.mp3"));
}

void tst_QMediaPlayer::testMultipleMedia()
{
    QFETCH(QUrl, mediaContent);

    player->setSource(mediaContent);
    QCOMPARE(player->source(), mediaContent);

    QBuffer stream;
    player->setSourceDevice(&stream, mediaContent);
    QCOMPARE(player->source(), mediaContent);
    QCOMPARE(player->sourceDevice(), &stream);

    QFETCH(QUrl, anotherMediaContent);

    player->setSource(anotherMediaContent);
    QCOMPARE(player->source(), anotherMediaContent);

    QBuffer anotherStream;
    player->setSourceDevice(&anotherStream, anotherMediaContent);
    QCOMPARE(player->source(), anotherMediaContent);
    QCOMPARE(player->sourceDevice(), &anotherStream);
}

void tst_QMediaPlayer::testDuration_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testDuration()
{
    QFETCH(qint64, duration);

    mockPlayer->setDuration(duration);
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

    mockPlayer->setIsValid(valid);
    mockPlayer->setSeekable(seekable);
    mockPlayer->setPosition(position);
    mockPlayer->setDuration(duration);
    QVERIFY(player->isSeekable() == seekable);
    QVERIFY(player->position() == position);
    QVERIFY(player->duration() == duration);

    if (seekable) {
        {
            QSignalSpy spy(player, &QMediaPlayer::positionChanged);
            player->setPosition(position);
            QCOMPARE(player->position(), position);
            QCOMPARE(spy.size(), 0);
        }

        mockPlayer->setPosition(position);
        {
            QSignalSpy spy(player, &QMediaPlayer::positionChanged);
            player->setPosition(0);
            QCOMPARE(player->position(), qint64(0));
            QCOMPARE(spy.size(), position == 0 ? 0 : 1);
        }

        mockPlayer->setPosition(position);
        {
            QSignalSpy spy(player, &QMediaPlayer::positionChanged);
            player->setPosition(duration);
            QCOMPARE(player->position(), duration);
            QCOMPARE(spy.size(), position == duration ? 0 : 1);
        }

        mockPlayer->setPosition(position);
        {
            QSignalSpy spy(player, &QMediaPlayer::positionChanged);
            player->setPosition(-1);
            QCOMPARE(player->position(), qint64(0));
            QCOMPARE(spy.size(), position == 0 ? 0 : 1);
        }

    }
    else {
        QSignalSpy spy(player, &QMediaPlayer::positionChanged);
        player->setPosition(position);

        QCOMPARE(player->position(), position);
        QCOMPARE(spy.size(), 0);
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
    float vol = volume/100.;

    audioOutput->setVolume(vol);
    QVERIFY(audioOutput->volume() == vol);

    if (valid) {
        {
            QSignalSpy spy(audioOutput, &QAudioOutput::volumeChanged);
            audioOutput->setVolume(.1f);
            QCOMPARE(audioOutput->volume(), .1f);
            QCOMPARE(spy.size(), 1);
        }

        {
            QSignalSpy spy(audioOutput, &QAudioOutput::volumeChanged);
            audioOutput->setVolume(-1000.f);
            QCOMPARE(audioOutput->volume(), 0.f);
            QCOMPARE(spy.size(), 1);
        }

        {
            QSignalSpy spy(audioOutput, &QAudioOutput::volumeChanged);
            audioOutput->setVolume(1.f);
            QCOMPARE(audioOutput->volume(), 1.f);
            QCOMPARE(spy.size(), 1);
        }

        {
            QSignalSpy spy(audioOutput, &QAudioOutput::volumeChanged);
            audioOutput->setVolume(1000.f);
            QCOMPARE(audioOutput->volume(), 1.f);
            QCOMPARE(spy.size(), 0);
        }
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
    float vol = volume/100.;

    if (valid) {
        audioOutput->setMuted(muted);
        audioOutput->setVolume(vol);
        QVERIFY(audioOutput->isMuted() == muted);

        QSignalSpy spy(audioOutput, &QAudioOutput::mutedChanged);
        audioOutput->setMuted(!muted);
        QCOMPARE(audioOutput->isMuted(), !muted);
        QCOMPARE(audioOutput->volume(), vol);
        QCOMPARE(spy.size(), 1);
    }
}

void tst_QMediaPlayer::testVideoAvailable_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testVideoAvailable()
{
    QFETCH(bool, videoAvailable);

    mockPlayer->setVideoAvailable(videoAvailable);
    QVERIFY(player->hasVideo() == videoAvailable);
}

void tst_QMediaPlayer::testBufferStatus_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testBufferStatus()
{
    QFETCH(int, bufferProgress);

    mockPlayer->setBufferStatus(bufferProgress);
    QVERIFY(player->bufferProgress() == bufferProgress);
}

void tst_QMediaPlayer::testSeekable_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testSeekable()
{
    QFETCH(bool, seekable);

    mockPlayer->setSeekable(seekable);
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
        mockPlayer->setPlaybackRate(playbackRate);
        QVERIFY(player->playbackRate() == playbackRate);

        QSignalSpy spy(player, &QMediaPlayer::playbackRateChanged);
        player->setPlaybackRate(playbackRate + 0.5f);
        QCOMPARE(player->playbackRate(), playbackRate + 0.5f);
        QCOMPARE(spy.size(), 1);
    }
}

void tst_QMediaPlayer::testError_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testError()
{
    QFETCH(QMediaPlayer::Error, error);

    mockPlayer->setError(error);
    QVERIFY(player->error() == error);
}

void tst_QMediaPlayer::testErrorString_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testErrorString()
{
    QFETCH(QString, errorString);

    mockPlayer->setErrorString(errorString);
    QVERIFY(player->errorString() == errorString);
}

void tst_QMediaPlayer::testIsAvailable()
{
    QCOMPARE(player->isAvailable(), true);
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
    QFETCH(QMediaPlayer::PlaybackState, state);

    mockPlayer->setIsValid(valid);
    player->setSource(mediaContent);
    mockPlayer->setState(state);
    QCOMPARE(player->playbackState(), state);
    QCOMPARE(player->isPlaying(), state == QMediaPlayer::PlayingState);
    QCOMPARE(player->source(), mediaContent);

    QSignalSpy spy(player, &QMediaPlayer::playbackStateChanged);
    QSignalSpy playingChanged(player, &QMediaPlayer::playingChanged);

    player->play();

    if (!valid || mediaContent.isEmpty())  {
        QCOMPARE(player->playbackState(), QMediaPlayer::StoppedState);
        QVERIFY(!player->isPlaying());
        QCOMPARE(spy.size(), 0);
        QVERIFY(playingChanged.empty());
    }
    else {
        QCOMPARE(player->playbackState(), QMediaPlayer::PlayingState);
        QVERIFY(player->isPlaying());
        QCOMPARE(spy.size(), state == QMediaPlayer::PlayingState ? 0 : 1);
        QCOMPARE_EQ(playingChanged.size(), state == QMediaPlayer::PlayingState ? 0 : 1);
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
    QFETCH(QMediaPlayer::PlaybackState, state);

    mockPlayer->setIsValid(valid);
    player->setSource(mediaContent);
    mockPlayer->setState(state);
    QVERIFY(player->playbackState() == state);
    QCOMPARE(player->isPlaying(), state == QMediaPlayer::PlayingState);
    QVERIFY(player->source() == mediaContent);

    QSignalSpy spy(player, &QMediaPlayer::playbackStateChanged);
    QSignalSpy playingChanged(player, &QMediaPlayer::playingChanged);

    player->pause();

    QVERIFY(!player->isPlaying());

    if (!valid || mediaContent.isEmpty()) {
        QCOMPARE(player->playbackState(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.size(), 0);
        QCOMPARE(playingChanged.size(), 0);
    }
    else {
        QCOMPARE(player->playbackState(), QMediaPlayer::PausedState);
        QCOMPARE(spy.size(), state == QMediaPlayer::PausedState ? 0 : 1);
        QCOMPARE(playingChanged.size(), state == QMediaPlayer::PlayingState ? 1 : 0);
    }
}

void tst_QMediaPlayer::testStop_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testStop()
{
    QFETCH(QUrl, mediaContent);
    QFETCH(QMediaPlayer::PlaybackState, state);

    player->setSource(mediaContent);
    mockPlayer->setState(state);
    QVERIFY(player->playbackState() == state);
    QCOMPARE(player->isPlaying(), state == QMediaPlayer::PlayingState);
    QVERIFY(player->source() == mediaContent);

    QSignalSpy spy(player, &QMediaPlayer::playbackStateChanged);
    QSignalSpy playingChanged(player, &QMediaPlayer::playingChanged);

    player->stop();

    QVERIFY(!player->isPlaying());

    if (mediaContent.isEmpty() || state == QMediaPlayer::StoppedState) {
        QCOMPARE(player->playbackState(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.size(), 0);
        QCOMPARE(playingChanged.size(), 0);
    }
    else {
        QCOMPARE(player->playbackState(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(playingChanged.size(), state == QMediaPlayer::PlayingState ? 1 : 0);
    }
}

void tst_QMediaPlayer::testMediaStatus_data()
{
    setupCommonTestData();
}

void tst_QMediaPlayer::testMediaStatus()
{
    QFETCH(int, bufferProgress);
    int bufferSignals = 0;

    mockPlayer->setMediaStatus(QMediaPlayer::NoMedia);
    mockPlayer->setBufferStatus(bufferProgress);

    QSignalSpy statusSpy(player, &QMediaPlayer::mediaStatusChanged);
    QSignalSpy bufferSpy(player, &QMediaPlayer::bufferProgressChanged);

    QCOMPARE(player->mediaStatus(), QMediaPlayer::NoMedia);

    mockPlayer->setMediaStatus(QMediaPlayer::LoadingMedia);
    QCOMPARE(player->mediaStatus(), QMediaPlayer::LoadingMedia);
    QCOMPARE(statusSpy.size(), 1);

    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)),
             QMediaPlayer::LoadingMedia);

    mockPlayer->setMediaStatus(QMediaPlayer::LoadedMedia);
    QCOMPARE(player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(statusSpy.size(), 2);

    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)),
             QMediaPlayer::LoadedMedia);

    // Verify the bufferProgressChanged() signal isn't being emitted.
    QCOMPARE(bufferSpy.size(), 0);

    mockPlayer->setMediaStatus(QMediaPlayer::StalledMedia);
    QCOMPARE(player->mediaStatus(), QMediaPlayer::StalledMedia);
    QCOMPARE(statusSpy.size(), 3);

    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)),
             QMediaPlayer::StalledMedia);

    // Verify the bufferProgressChanged() signal is being emitted.
    QVERIFY(bufferSpy.size() > bufferSignals);
    QCOMPARE(bufferSpy.last().value(0).toInt(), bufferProgress);
    bufferSignals = bufferSpy.size();

    mockPlayer->setMediaStatus(QMediaPlayer::BufferingMedia);
    QCOMPARE(player->mediaStatus(), QMediaPlayer::BufferingMedia);
    QCOMPARE(statusSpy.size(), 4);

    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)),
             QMediaPlayer::BufferingMedia);

    // Verify the bufferProgressChanged() signal is being emitted.
    QVERIFY(bufferSpy.size() > bufferSignals);
    QCOMPARE(bufferSpy.last().value(0).toInt(), bufferProgress);
    bufferSignals = bufferSpy.size();

    mockPlayer->setMediaStatus(QMediaPlayer::BufferedMedia);
    QCOMPARE(player->mediaStatus(), QMediaPlayer::BufferedMedia);
    QCOMPARE(statusSpy.size(), 5);

    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)),
             QMediaPlayer::BufferedMedia);

    // Verify the bufferProgressChanged() signal isn't being emitted.
    QCOMPARE(bufferSpy.size(), bufferSignals);

    mockPlayer->setMediaStatus(QMediaPlayer::EndOfMedia);
    QCOMPARE(player->mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(statusSpy.size(), 6);

    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)),
             QMediaPlayer::EndOfMedia);
}

void tst_QMediaPlayer::testDestructor()
{
    /* create an object for player */
    QMediaPlayer *victim = new QMediaPlayer;

    /* check whether the object is created */
    QVERIFY(victim);

    /* delete the instance (a crash is a failure :) */
    delete victim;
}

void tst_QMediaPlayer::testSetVideoOutput()
{
    QVideoSink surface;

    player->setVideoOutput(static_cast<QObject *>(nullptr));

//    QCOMPARE(mockService->rendererRef, 0);

    player->setVideoOutput(&surface);
//    QVERIFY(mockService->rendererControl->surface() == &surface);
//    QCOMPARE(mockService->rendererRef, 1);

    player->setVideoOutput(reinterpret_cast<QVideoSink *>(0));
//    QVERIFY(mockService->rendererControl->surface() == nullptr);

    //rendererControl is released
//    QCOMPARE(mockService->rendererRef, 0);

    player->setVideoOutput(&surface);
//    QVERIFY(mockService->rendererControl->surface() == &surface);
//    QCOMPARE(mockService->rendererRef, 1);

    player->setVideoOutput(static_cast<QObject *>(nullptr));
//    QVERIFY(mockService->rendererControl->surface() == nullptr);
//    //rendererControl is released
//    QCOMPARE(mockService->rendererRef, 0);

    player->setVideoOutput(&surface);
//    QVERIFY(mockService->rendererControl->surface() == &surface);
//    QCOMPARE(mockService->rendererRef, 1);
}

void tst_QMediaPlayer::testSetVideoOutputDestruction()
{
    QVideoSink surface;
    {
        QMediaPlayer player;
        player.setVideoOutput(&surface);
    }
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

void tst_QMediaPlayer::testQrc_data()
{
    QTest::addColumn<QUrl>("mediaContent");
    QTest::addColumn<QMediaPlayer::MediaStatus>("status");
    QTest::addColumn<QMediaPlayer::Error>("error");
    QTest::addColumn<int>("errorCount");
    QTest::addColumn<QString>("backendMediaContentScheme");
    QTest::addColumn<bool>("backendHasStream");

    QTest::newRow("invalid") << QUrl(QUrl(QLatin1String("qrc:/invalid.mp3")))
                             << QMediaPlayer::InvalidMedia
                             << QMediaPlayer::ResourceError
                             << 1 // error count
                             << QString() // backend should not have got any media (empty URL scheme)
                             << false; // backend should not have got any stream

    QTest::newRow("valid+nostream") << QUrl(QUrl(QLatin1String("qrc:/testdata/nokia-tune.mp3")))
                                    << QMediaPlayer::LoadingMedia
                                    << QMediaPlayer::NoError
                                    << 0 // error count
                                    << QStringLiteral("file") // backend should have a got a temporary file
                                    << false; // backend should not have got any stream

    QTest::newRow("valid+stream") << QUrl(QUrl(QLatin1String("qrc:/testdata/nokia-tune.mp3")))
                                  << QMediaPlayer::LoadingMedia
                                  << QMediaPlayer::NoError
                                  << 0 // error count
                                  << QStringLiteral("qrc")
                                  << true; // backend should have got a stream (QFile opened from the resource)
}

void tst_QMediaPlayer::testQrc()
{
    QFETCH(QUrl, mediaContent);
    QFETCH(QMediaPlayer::MediaStatus, status);
    QFETCH(QMediaPlayer::Error, error);
    QFETCH(int, errorCount);
    QFETCH(QString, backendMediaContentScheme);
    QFETCH(bool, backendHasStream);

    mockPlayer->setState(QMediaPlayer::PlayingState, QMediaPlayer::NoMedia);
    mockPlayer->setStreamPlaybackSupported(backendHasStream);

    QSignalSpy mediaSpy(player, &QMediaPlayer::sourceChanged);
    QSignalSpy statusSpy(player, &QMediaPlayer::mediaStatusChanged);
    QSignalSpy errorSpy(player, &QMediaPlayer::errorOccurred);

    player->setSource(mediaContent);

    QTRY_COMPARE(player->mediaStatus(), status);
    QCOMPARE(statusSpy.size(), 1);
    QCOMPARE(qvariant_cast<QMediaPlayer::MediaStatus>(statusSpy.last().value(0)), status);

    QCOMPARE(player->source(), mediaContent);
    QCOMPARE(mediaSpy.size(), 1);
    QCOMPARE(qvariant_cast<QUrl>(mediaSpy.last().value(0)), mediaContent);

    QCOMPARE(player->error(), error);
    QCOMPARE(errorSpy.size(), errorCount);
    if (errorCount > 0) {
        QCOMPARE(qvariant_cast<QMediaPlayer::Error>(errorSpy.last().value(0)), error);
        QVERIFY(!player->errorString().isEmpty());
    }

    // Check the media actually passed to the backend
    QCOMPARE(mockPlayer->media().scheme(), backendMediaContentScheme);
    QCOMPARE(bool(mockPlayer->mediaStream()), backendHasStream);
}

void tst_QMediaPlayer::playbackOptions_returnsDefaultAfterConstruction()
{
    QCOMPARE_EQ(player->playbackOptions(), QPlaybackOptions{});
}

void tst_QMediaPlayer::setPlaybackOptions_setsPlaybackOptions()
{
    QSignalSpy spy{ player, &QMediaPlayer::playbackOptionsChanged };

    QPlaybackOptions options;
    const std::chrono::milliseconds defaultNetworkTimeout = options.networkTimeout();
    options.setNetworkTimeout(defaultNetworkTimeout + 1ms);

    player->setPlaybackOptions(options);

    QCOMPARE_NE(player->playbackOptions().networkTimeout(), defaultNetworkTimeout);

    QCOMPARE_EQ(spy.size(), 1);
}

void tst_QMediaPlayer::setPlaybackOptions_doesNotEmitChangeSignal_whenOptionsDidNotChange()
{
    QSignalSpy spy{ player, &QMediaPlayer::playbackOptionsChanged };

    player->setPlaybackOptions(player->playbackOptions());

    QCOMPARE_EQ(spy.size(), 0);
}

void tst_QMediaPlayer::resetPlaybackOptions_resetsPlaybackOptionsToDefault()
{
    auto options = player->playbackOptions();
    options.setNetworkTimeout(options.networkTimeout() + 1ms);
    player->setPlaybackOptions(options);

    player->resetPlaybackOptions();
    QCOMPARE_EQ(player->playbackOptions(), QPlaybackOptions{});
}

QTEST_GUILESS_MAIN(tst_QMediaPlayer)
#include "tst_qmediaplayer.moc"
