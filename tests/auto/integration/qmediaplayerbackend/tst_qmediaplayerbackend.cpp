// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QDebug>
#include "qmediaplayer.h"
#include <qmediametadata.h>
#include <qaudiobuffer.h>
#include <qvideosink.h>
#include <qvideoframe.h>
#include <qaudiooutput.h>

#include <private/qplatformvideosink_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlproperty.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickloader_p.h>

#include "../shared/mediafileselector.h"
//TESTED_COMPONENT=src/multimedia

#include <QtMultimedia/private/qtmultimedia-config_p.h>
#include "private/qquickvideooutput_p.h"

#include <array>

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
    void loadInvalidMedia();
    void loadInvalidMediaWhilePlayingAndRestore();
    void loadMedia();
    void unloadMedia();
    void loadMediaInLoadingState();
    void loadMediaWhilePlaying();
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
    void surfaceTest();
//    void multipleSurfaces();
    void metadata();
    void playerStateAtEOS();
    void playFromBuffer();
    void audioVideoAvailable();
    void isSeekable();
    void positionAfterSeek();
    void videoDimensions();
    void position();
    void multipleMediaPlayback();
    void multiplePlaybackRateChangingStressTest();
    void multipleSeekStressTest();
    void playbackRateChanging();
    void durationDetectionIssues_data();
    void durationDetectionIssues();
    void finiteLoops();
    void infiniteLoops();
    void seekOnLoops();
    void changeLoopsOnTheFly();
    void changeVideoOutputNoFramesLost();
    void cleanSinkAndNoMoreFramesAfterStop();
    void lazyLoadVideo();
    void videoSinkSignals();
    void nonAsciiFileName();
    void setMedia_setsVideoSinkSize_beforePlaying();

private:
    QUrl selectVideoFile(const QStringList& mediaCandidates);
    bool isWavSupported();

    //one second local wav file
    QUrl localWavFile;
    QUrl localWavFile2;
    QUrl localVideoFile;
    QUrl localVideoFile2;
    QUrl videoDimensionTestFile;
    QUrl localCompressedSoundFile;
    QUrl localFileWithMetadata;
    QUrl localVideoFile3ColorsWithSound;

    const std::array<QRgb, 3> video3Colors = { { 0xFF0000, 0x00FF00, 0x0000FF } };

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
        connect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::addVideoFrame);
        connect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::videoFrameChangedSync);
    }

    QVideoFrame waitForFrame()
    {
        QSignalSpy spy(this, &TestVideoSink::videoFrameChangedSync);
        return spy.wait() ? spy.at(0).at(0).value<QVideoFrame>() : QVideoFrame{};
    }

private Q_SLOTS:
    void addVideoFrame(const QVideoFrame &frame) {
        // qDebug() << m_elapsedTimer.elapsed() << frame.startTime() << frame.endTime() <<
        // frame.toImage().pixelColor(1, 1);

        if (!m_elapsedTimer.isValid())
            m_elapsedTimer.start();
        else
            m_elapsedTimer.restart();

        if (m_storeFrames)
            m_frameList.append(frame);
        ++m_totalFrames;
    }

signals:
    void videoFrameChangedSync(const QVideoFrame &frame);

public:
    QList<QVideoFrame> m_frameList;
    int m_totalFrames = 0; // used instead of the list when frames are not stored
    QElapsedTimer m_elapsedTimer;

private:
    bool m_storeFrames;
};

static void setVideoSinkAsyncFramesCounter(QVideoSink &sink, std::atomic_int &counter)
{
    QObject::connect(&sink, &QVideoSink::videoFrameChanged,
                     &sink, [&counter]() { ++counter; }, Qt::DirectConnection);
}

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
        player.setSource(media);
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

    localWavFile = MediaFileSelector::selectMediaFile(QStringList() << "qrc:/testdata/test.wav");
    localWavFile2 = MediaFileSelector::selectMediaFile(QStringList() << "qrc:/testdata/_test.wav");

    QStringList mediaCandidates;
    mediaCandidates << "qrc:/testdata/colors.mp4";
    mediaCandidates << "qrc:/testdata/colors.ogv";
    localVideoFile = MediaFileSelector::selectMediaFile(mediaCandidates);

    mediaCandidates.clear();
    mediaCandidates << "qrc:/testdata/3colors_with_sound_1s.mp4";
    localVideoFile3ColorsWithSound = MediaFileSelector::selectMediaFile(mediaCandidates);

    mediaCandidates.clear();
    mediaCandidates << "qrc:/testdata/BigBuckBunny.mp4";
    mediaCandidates << "qrc:/testdata/busMpeg4.mp4";
    localVideoFile2 = MediaFileSelector::selectMediaFile(mediaCandidates);

    mediaCandidates.clear();
    mediaCandidates << "qrc:/testdata/BigBuckBunny.mp4";
    videoDimensionTestFile = MediaFileSelector::selectMediaFile(mediaCandidates);

    mediaCandidates.clear();
    mediaCandidates << "qrc:/testdata/nokia-tune.mp3";
    mediaCandidates << "qrc:/testdata/nokia-tune.mkv";
    localCompressedSoundFile = MediaFileSelector::selectMediaFile(mediaCandidates);

    localFileWithMetadata =
            MediaFileSelector::selectMediaFile(QStringList() << "qrc:/testdata/nokia-tune.mp3");

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

void tst_QMediaPlayerBackend::loadInvalidMedia()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioOutput output;
    TestVideoSink surface(false);
    QMediaPlayer player;

    QSignalSpy stateSpy(&player, &QMediaPlayer::playbackStateChanged);
    QSignalSpy errorSpy(&player, &QMediaPlayer::errorOccurred);

    player.setAudioOutput(&output);
    player.setVideoOutput(&surface);

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);

    player.setSource(QUrl("Some not existing media"));

    QCOMPARE(player.source(), QUrl("Some not existing media"));
    QTRY_COMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::InvalidMedia);
    QTRY_COMPARE(player.error(), QMediaPlayer::ResourceError);

    QCOMPARE(errorSpy.size(), 1);
    QCOMPARE(stateSpy.size(), 0);

    player.pause();
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);

    player.play();
    QTRY_COMPARE(player.playbackState(), QMediaPlayer::StoppedState);

    QTest::qWait(150); // wait a bit and check position is not changed

    QCOMPARE(player.position(), 0);
    QCOMPARE(surface.m_totalFrames, 0);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::InvalidMedia);
}

void tst_QMediaPlayerBackend::loadInvalidMediaWhilePlayingAndRestore()
{
    QMediaPlayer player;
    QAudioOutput output;
    QVideoSink surface;
    std::atomic_int framesCount = 0;
    setVideoSinkAsyncFramesCounter(surface, framesCount);

    QSignalSpy stateSpy(&player, &QMediaPlayer::playbackStateChanged);
    QSignalSpy errorSpy(&player, &QMediaPlayer::errorOccurred);

    player.setAudioOutput(&output);
    player.setVideoOutput(&surface);

    player.setSource(localVideoFile3ColorsWithSound);
    player.play();

    QTRY_VERIFY(framesCount > 0);
    QCOMPARE(errorSpy.size(), 0);

    player.setSource(QUrl("Some not existing media"));
    const int savedFramesCount = framesCount;

    QCOMPARE(player.source(), QUrl("Some not existing media"));

    QTRY_COMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::InvalidMedia);
    QTRY_COMPARE(player.error(), QMediaPlayer::ResourceError);

    QVERIFY(!surface.videoFrame().isValid());

    QCOMPARE(errorSpy.size(), 1);

    QTest::qWait(20);
    QCOMPARE(framesCount, savedFramesCount);

    // restore playing
    player.setSource(localVideoFile3ColorsWithSound);
    player.play();
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);
    QCOMPARE(player.error(), QMediaPlayer::NoError);
}

void tst_QMediaPlayerBackend::loadMedia()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioOutput output;
    QMediaPlayer player;
    player.setAudioOutput(&output);

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);

    QSignalSpy stateSpy(&player, SIGNAL(playbackStateChanged(QMediaPlayer::PlaybackState)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy mediaSpy(&player, SIGNAL(sourceChanged(QUrl)));

    player.setSource(localWavFile);

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);

    QVERIFY(player.mediaStatus() != QMediaPlayer::NoMedia);
    QVERIFY(player.mediaStatus() != QMediaPlayer::InvalidMedia);
    QVERIFY(player.source() == localWavFile);

    QCOMPARE(stateSpy.size(), 0);
    QVERIFY(statusSpy.size() > 0);
    QCOMPARE(mediaSpy.size(), 1);
    QCOMPARE(mediaSpy.last()[0].value<QUrl>(), localWavFile);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QVERIFY(player.hasAudio());
    QVERIFY(!player.hasVideo());
}

void tst_QMediaPlayerBackend::unloadMedia()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioOutput output;
    QMediaPlayer player;
    player.setAudioOutput(&output);

    QSignalSpy stateSpy(&player, SIGNAL(playbackStateChanged(QMediaPlayer::PlaybackState)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy mediaSpy(&player, SIGNAL(sourceChanged(QUrl)));
    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));
    QSignalSpy durationSpy(&player, SIGNAL(durationChanged(qint64)));

    player.setSource(localWavFile);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QVERIFY(player.position() == 0);
#ifdef Q_OS_QNX
    // QNX mm-renderer only updates the duration when 'play' is triggered
    QVERIFY(player.duration() == 0);
#else
    QVERIFY(player.duration() > 0);
#endif

    player.play();

    QTRY_VERIFY(player.position() > 0);
    QVERIFY(player.duration() > 0);

    stateSpy.clear();
    statusSpy.clear();
    mediaSpy.clear();
    positionSpy.clear();
    durationSpy.clear();

    player.setSource(QUrl());

    QVERIFY(player.position() <= 0);
    QVERIFY(player.duration() <= 0);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(player.source(), QUrl());

    QVERIFY(!statusSpy.isEmpty());
    QVERIFY(!mediaSpy.isEmpty());
}

void tst_QMediaPlayerBackend::loadMediaInLoadingState()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output);
    player.setSource(localWavFile2);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadingMedia);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    // Sets new media while old has not been finished.
    player.setSource(localWavFile);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadingMedia);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    player.play();
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    player.setSource(localWavFile2);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadingMedia);
}

void tst_QMediaPlayerBackend::loadMediaWhilePlaying()
{
    QMediaPlayer player;
    QAudioOutput output;
    TestVideoSink surface(false);

    player.setAudioOutput(&output);
    player.setVideoOutput(&surface);

    player.setSource(localVideoFile3ColorsWithSound);
    player.play();
    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QVERIFY(surface.waitForFrame().isValid());
    QVERIFY(player.hasAudio());
    QVERIFY(player.hasVideo());

    QSignalSpy stateSpy(&player, &QMediaPlayer::playbackStateChanged);
    QSignalSpy errorSpy(&player, &QMediaPlayer::errorChanged);

    player.setSource(localWavFile2);
    QCOMPARE(player.source(), localWavFile2);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(errorSpy.size(), 0);
    QVERIFY(player.hasAudio());
    QVERIFY(!player.hasVideo());
    QVERIFY(!surface.videoFrame().isValid());

    player.play();

    player.setSource(localVideoFile2);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QVERIFY(player.hasVideo());
    QVERIFY(!player.hasAudio());
    QCOMPARE(errorSpy.size(), 0);
}

void tst_QMediaPlayerBackend::playPauseStop()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioOutput output;
    QMediaPlayer player;
    player.setAudioOutput(&output);

    QSignalSpy stateSpy(&player, SIGNAL(playbackStateChanged(QMediaPlayer::PlaybackState)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));
    QSignalSpy errorSpy(&player, SIGNAL(errorOccurred(QMediaPlayer::Error, const QString&)));

    // Check play() without a media
    player.play();

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QCOMPARE(player.position(), 0);
    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(statusSpy.size(), 0);
    QCOMPARE(positionSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 0);

    // Check pause() without a media
    player.pause();

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QCOMPARE(player.position(), 0);
    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(statusSpy.size(), 0);
    QCOMPARE(positionSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 0);

    // The rest is with a valid media

    player.setSource(localWavFile);

    QCOMPARE(player.position(), qint64(0));

    player.play();

    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(statusSpy.size() > 0 &&
                statusSpy.last()[0].value<QMediaPlayer::MediaStatus>() == QMediaPlayer::BufferedMedia);

    QTRY_VERIFY(player.position() > 100);
    QVERIFY(player.duration() > 0);
    QTRY_VERIFY(positionSpy.size() > 0);
    QTRY_VERIFY(positionSpy.last()[0].value<qint64>() > 0);

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    qint64 positionBeforePause = player.position();
    player.pause();

    QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PausedState);

    QTest::qWait(500);

    QTRY_VERIFY(qAbs(player.position() - positionBeforePause) < 150);

    stateSpy.clear();
    statusSpy.clear();

    player.stop();

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);
    //it's allowed to emit statusChanged() signal async
    QTRY_COMPARE(statusSpy.size(), 1);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::LoadedMedia);

    //ensure the position is reset to 0 at stop and positionChanged(0) is emitted
    QTRY_COMPARE(player.position(), qint64(0));
    QTRY_VERIFY(positionSpy.size() > 0);
    QCOMPARE(positionSpy.last()[0].value<qint64>(), qint64(0));
    QVERIFY(player.duration() > 0);

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    player.play();

    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);
    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PlayingState);
    QCOMPARE(statusSpy.size(), 1); // Should not go through Loading again when play -> stop -> play
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::BufferedMedia);

    player.stop();
    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    player.setSource(localWavFile2);

    QTRY_VERIFY(statusSpy.size() > 0);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::LoadedMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.size(), 0);

    player.play();
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QTRY_VERIFY(positionSpy.size() > 0 && positionSpy.last()[0].value<qint64>() > 100);
    QVERIFY(player.position() > 100);
    positionSpy.clear();

    player.setSource(localWavFile);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QTRY_VERIFY(statusSpy.size() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::LoadedMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_VERIFY(stateSpy.size() > 0);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);
    QTRY_VERIFY(positionSpy.size() > 0 && positionSpy.last()[0].value<qint64>() == 0);
    QCOMPARE(player.position(), 0);

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    player.play();

    QTRY_VERIFY(player.position() > 100);

    player.setSource(QUrl());

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);
    QTRY_VERIFY(statusSpy.size() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::NoMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_VERIFY(stateSpy.size() > 0);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);
    QTRY_VERIFY(positionSpy.size() > 0);
    QCOMPARE(player.position(), 0);
    QCOMPARE(positionSpy.last()[0].value<qint64>(), 0);
    QCOMPARE(player.duration(), 0);
}


void tst_QMediaPlayerBackend::processEOS()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output);

    QSignalSpy stateSpy(&player, SIGNAL(playbackStateChanged(QMediaPlayer::PlaybackState)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    player.setSource(localWavFile);

    player.play();
    player.setPosition(900);

    //wait up to 5 seconds for EOS
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);

    QVERIFY(statusSpy.size() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::EndOfMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.size(), 2);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);

    //at EOS the position stays at the end of file
    QCOMPARE(player.position(), player.duration());
    QTRY_VERIFY(positionSpy.size() > 0);
    QTRY_COMPARE(positionSpy.last()[0].value<qint64>(), player.duration());

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    player.play();

    //position is reset to start
    QTRY_VERIFY(player.position() < 100);
    QTRY_VERIFY(positionSpy.size() > 0);
    QCOMPARE(positionSpy.first()[0].value<qint64>(), 0);

    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PlayingState);
    QVERIFY(statusSpy.size() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::BufferedMedia);

    positionSpy.clear();
    QTRY_VERIFY(player.position() > 100);
    QTRY_VERIFY(positionSpy.size() > 0 && positionSpy.last()[0].value<qint64>() > 100);
    player.setPosition(900);
    //wait up to 5 seconds for EOS
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QVERIFY(statusSpy.size() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::EndOfMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.size(), 2);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);

    //position stays at the end of file
    QCOMPARE(player.position(), player.duration());
    QTRY_VERIFY(positionSpy.size() > 0);
    QTRY_COMPARE(positionSpy.last()[0].value<qint64>(), player.duration());

    //after setPosition EndOfMedia status should be reset to Loaded
    stateSpy.clear();
    statusSpy.clear();
    player.setPosition(500);

    //this transition can be async, so allow backend to perform it
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QCOMPARE(stateSpy.size(), 0);
    QTRY_VERIFY(statusSpy.size() > 0 &&
        statusSpy.last()[0].value<QMediaPlayer::MediaStatus>() == QMediaPlayer::LoadedMedia);

    player.play();
    player.setPosition(900);
    //wait up to 5 seconds for EOS
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(player.position(), player.duration());

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    // pause() should reset position to beginning and status to Buffered
    player.pause();

    QTRY_COMPARE(player.position(), 0);
    QTRY_VERIFY(positionSpy.size() > 0);
    QTRY_COMPARE(positionSpy.first()[0].value<qint64>(), 0);

    QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PausedState);
    QVERIFY(statusSpy.size() > 0);
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
    QAudioOutput output;
    player->setAudioOutput(&output);
    player->setPosition(800); // don't wait as long for EOS
    DeleteLaterAtEos deleter(player);
    player->setSource(localWavFile);

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
    // volume and muted properties should be independent
    QAudioOutput output;
    QMediaPlayer player;
    player.setAudioOutput(&output);
    QCOMPARE(output.volume(), 1.);
    QVERIFY(!output.isMuted());

    player.setSource(localWavFile);
    player.pause();

    QCOMPARE(output.volume(), 1.);
    QVERIFY(!output.isMuted());

    QSignalSpy volumeSpy(&output, SIGNAL(volumeChanged(float)));
    QSignalSpy mutedSpy(&output, SIGNAL(mutedChanged(bool)));

    //setting volume to 0 should not trigger muted
    output.setVolume(0);
    QTRY_COMPARE(output.volume(), 0);
    QVERIFY(!output.isMuted());
    QCOMPARE(volumeSpy.size(), 1);
    QCOMPARE(volumeSpy.last()[0].toFloat(), output.volume());
    QCOMPARE(mutedSpy.size(), 0);

    output.setVolume(0.5);
    QTRY_COMPARE(output.volume(), 0.5);
    QVERIFY(!output.isMuted());
    QCOMPARE(volumeSpy.size(), 2);
    QCOMPARE(volumeSpy.last()[0].toFloat(), output.volume());
    QCOMPARE(mutedSpy.size(), 0);

    output.setMuted(true);
    QTRY_VERIFY(output.isMuted());
    QVERIFY(output.volume() > 0);
    QCOMPARE(volumeSpy.size(), 2);
    QCOMPARE(mutedSpy.size(), 1);
    QCOMPARE(mutedSpy.last()[0].toBool(), output.isMuted());

    output.setMuted(false);
    QTRY_VERIFY(!output.isMuted());
    QVERIFY(output.volume() > 0);
    QCOMPARE(volumeSpy.size(), 2);
    QCOMPARE(mutedSpy.size(), 2);
    QCOMPARE(mutedSpy.last()[0].toBool(), output.isMuted());

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
    float vol = volume/100.;

    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output);

    //volume and muted should not be preserved between player instances
    QVERIFY(output.volume() > 0);
    QVERIFY(!output.isMuted());

    output.setVolume(vol);
    output.setMuted(muted);

    QTRY_COMPARE(output.volume(), vol);
    QTRY_COMPARE(output.isMuted(), muted);

    player.setSource(localWavFile);
    QCOMPARE(output.volume(), vol);
    QCOMPARE(output.isMuted(), muted);

    player.pause();

    //to ensure the backend doesn't change volume/muted
    //async during file loading.

    QTRY_COMPARE(output.volume(), vol);
    QCOMPARE(output.isMuted(), muted);

    player.setSource(QUrl());
    QTRY_COMPARE(output.volume(), vol);
    QCOMPARE(output.isMuted(), muted);

    player.setSource(localWavFile);
    player.pause();

    QTRY_COMPARE(output.volume(), vol);
    QCOMPARE(output.isMuted(), muted);
}

void tst_QMediaPlayerBackend::initialVolume()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    {
        QAudioOutput output;
        QMediaPlayer player;
        player.setAudioOutput(&output);
        output.setVolume(1);
        player.setSource(localWavFile);
        QCOMPARE(output.volume(), 1);
        player.play();
        QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(output.volume(), 1);
    }

    {
        QAudioOutput output;
        QMediaPlayer player;
        player.setAudioOutput(&output);
        player.setSource(localWavFile);
        QCOMPARE(output.volume(), 1);
        player.play();
        QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(output.volume(), 1);
    }
}

void tst_QMediaPlayerBackend::seekPauseSeek()
{
#ifdef Q_OS_ANDROID
    QSKIP("frame.toImage will return null image because of QTBUG-108446");
#endif
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    TestVideoSink surface;
    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output);

    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    player.setVideoOutput(&surface);

    player.setSource(localVideoFile);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(surface.m_frameList.isEmpty()); // frame must not appear until we call pause() or play()

    positionSpy.clear();
    qint64 position = 7000;
    player.setPosition(position);
    QTRY_VERIFY(!positionSpy.isEmpty());
    QTRY_COMPARE(player.position(), position);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTest::qWait(250); // wait a bit to ensure the frame is not rendered
    QVERIFY(surface.m_frameList
                    .isEmpty()); // still no frame, we must call pause() or play() to see a frame

    player.pause();
    QTRY_COMPARE(player.playbackState(), QMediaPlayer::PausedState); // it might take some time for the operation to be completed
    QTRY_VERIFY(!surface.m_frameList.isEmpty()); // we must see a frame at position 7000 here

    // Make sure that the frame has a timestamp before testing - not all backends provides this
    if (!surface.m_frameList.back().isValid() || surface.m_frameList.back().startTime() < 0)
        QSKIP("No timestamp");

    {
        QVideoFrame frame = surface.m_frameList.back();
        const qint64 elapsed = (frame.startTime() / 1000) - position; // frame.startTime() is microsecond, position is milliseconds.
        QVERIFY2(qAbs(elapsed) < (qint64)500, QByteArray::number(elapsed).constData());
        QCOMPARE(frame.width(), 160);
        QCOMPARE(frame.height(), 120);

        // create QImage for QVideoFrame to verify RGB pixel colors
        QImage image = frame.toImage();
        QVERIFY(!image.isNull());
        QVERIFY(qRed(image.pixel(0, 0)) >= 230); // conversion from YUV => RGB, that's why it's not 255
        QVERIFY(qGreen(image.pixel(0, 0)) < 20);
        QVERIFY(qBlue(image.pixel(0, 0)) < 20);
    }

    surface.m_frameList.clear();
    positionSpy.clear();
    position = 12000;
    player.setPosition(position);
    QTRY_VERIFY(!positionSpy.isEmpty() && qAbs(player.position() - position) < (qint64)500);
    QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    QTRY_VERIFY(!surface.m_frameList.isEmpty());

    {
        QVideoFrame frame = surface.m_frameList.back();
        const qint64 elapsed = (frame.startTime() / 1000) - position;
        QVERIFY2(qAbs(elapsed) < (qint64)500, QByteArray::number(elapsed).constData());
        QCOMPARE(frame.width(), 160);
        QCOMPARE(frame.height(), 120);

        QImage image = frame.toImage();
        QVERIFY(!image.isNull());
        QVERIFY(qRed(image.pixel(0, 0)) < 20);
        QVERIFY(qGreen(image.pixel(0, 0)) >= 230);
        QVERIFY(qBlue(image.pixel(0, 0)) < 20);
    }
}

void tst_QMediaPlayerBackend::seekInStoppedState()
{
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    TestVideoSink surface(false);
    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output);
    player.setVideoOutput(&surface);

    QSignalSpy stateSpy(&player, SIGNAL(playbackStateChanged(QMediaPlayer::PlaybackState)));
    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    player.setSource(localVideoFile);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(player.position(), 0);
    QVERIFY(player.isSeekable());

    stateSpy.clear();
    positionSpy.clear();

    qint64 position = 5000;
    player.setPosition(position);

    QTRY_VERIFY(qAbs(player.position() - position) < qint64(200));
    QTRY_VERIFY(positionSpy.size() > 0);
    QVERIFY(qAbs(positionSpy.last()[0].value<qint64>() - position) < qint64(200));

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.size(), 0);

    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    positionSpy.clear();

    player.play();

    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(player.position() > position);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QTest::qWait(100);
    // Check that it never played from the beginning
    QVERIFY(player.position() > position);
    for (int i = 0; i < positionSpy.size(); ++i)
        QVERIFY(positionSpy.at(i)[0].value<qint64>() > (position - 200));

    // ------
    // Same tests but after play() --> stop()

    player.stop();
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QTRY_COMPARE(player.position(), 0);

    stateSpy.clear();
    positionSpy.clear();

    player.setPosition(position);

    QTRY_VERIFY(qAbs(player.position() - position) < qint64(200));
    QTRY_VERIFY(positionSpy.size() > 0);
    QVERIFY(qAbs(positionSpy.last()[0].value<qint64>() - position) < qint64(200));

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.size(), 0);

    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    positionSpy.clear();

    player.play();

    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);
    QVERIFY(qAbs(player.position() - position) < qint64(200));

    QTest::qWait(500);
    // Check that it never played from the beginning
    QVERIFY(player.position() > (position - 200));
    for (int i = 0; i < positionSpy.size(); ++i)
        QVERIFY(positionSpy.at(i)[0].value<qint64>() > (position - 200));

    // ------
    // Same tests but after reaching the end of the media

    player.setPosition(player.duration() - 100);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QVERIFY(qAbs(player.position() - player.duration()) < 10);

    stateSpy.clear();
    positionSpy.clear();

    player.setPosition(position);

    QTRY_VERIFY(qAbs(player.position() - position) < qint64(200));
    QTRY_VERIFY(positionSpy.size() > 0);
    QVERIFY(qAbs(positionSpy.last()[0].value<qint64>() - position) < qint64(200));

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    player.play();
    QTRY_COMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    positionSpy.clear();
    QTRY_VERIFY(player.position() > (position - 200));

    QTest::qWait(500);
    // Check that it never played from the beginning
    QVERIFY(player.position() > (position - 200));
    for (int i = 0; i < positionSpy.size(); ++i)
        QVERIFY(positionSpy.at(i)[0].value<qint64>() > (position - 200));
}

void tst_QMediaPlayerBackend::subsequentPlayback()
{
    if (localCompressedSoundFile.isEmpty())
        QSKIP("Sound format is not supported");

    QAudioOutput output;
    QMediaPlayer player;
    player.setAudioOutput(&output);
    player.setSource(localCompressedSoundFile);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QTRY_VERIFY(player.isSeekable());
    player.setPosition(5000);
    player.play();

    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QTRY_COMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    // Could differ by up to 1 compressed frame length
    QVERIFY(qAbs(player.position() - player.duration()) < 100);
    QVERIFY(player.position() > 0);

    player.play();
    QTRY_COMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(player.position() > 1000);
    player.pause();
    QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    // make sure position does not "jump" closer to the end of the file
    QVERIFY(player.position() > 1000);
    // try to seek back to zero
    player.setPosition(0);
    QTRY_COMPARE(player.position(), qint64(0));
    player.play();
    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(player.position() > 1000);
    player.pause();
    QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    QVERIFY(player.position() > 1000);
}

void tst_QMediaPlayerBackend::multipleMediaPlayback()
{
    if (localVideoFile.isEmpty() || localVideoFile2.isEmpty())
        QSKIP("Video format is not supported");

    QAudioOutput output;
    TestVideoSink surface(false);
    QMediaPlayer player;

    player.setVideoOutput(&surface);
    player.setAudioOutput(&output);
    player.setSource(localVideoFile);

    QCOMPARE(player.source(), localVideoFile);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    player.setPosition(0);
    player.play();

    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(player.position() > 0);
    QCOMPARE(player.source(), localVideoFile);

    player.stop();

    player.setSource(localVideoFile2);

    QCOMPARE(player.source(), localVideoFile2);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QTRY_VERIFY(player.isSeekable());

    player.setPosition(0);
    player.play();

    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(player.position() > 0);
    QCOMPARE(player.source(), localVideoFile2);

    player.stop();

    QTRY_COMPARE(player.playbackState(), QMediaPlayer::StoppedState);
}

void tst_QMediaPlayerBackend::multiplePlaybackRateChangingStressTest()
{
    if (localVideoFile3ColorsWithSound.isEmpty())
        QSKIP("Video format is not supported");

#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("SKIP on macOS CI since multiple fake drawing on macOS CI platform causes UB. To be "
              "investigated.");
#endif

    TestVideoSink surface(false);
    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output);
    player.setVideoOutput(&surface);

    player.setSource(localVideoFile3ColorsWithSound);

    player.play();

    surface.waitForFrame();

    QSignalSpy spy(&player, &QMediaPlayer::playbackStateChanged);

    constexpr qint64 expectedVideoDuration = 3000;
    constexpr int waitingInterval = 200;
    constexpr qint64 maxDuration = expectedVideoDuration + 2000;
    constexpr qint64 minDuration = expectedVideoDuration - 100;
    constexpr qint64 maxFrameDelay = 2000;

    surface.m_elapsedTimer.start();

    qint64 duration = 0;

    for (int i = 0; !spy.wait(waitingInterval); ++i) {
        duration += waitingInterval * player.playbackRate();

        player.setPlaybackRate(0.5 * (i % 4 + 1));

        QCOMPARE_LE(duration, maxDuration);

        QVERIFY2(surface.m_elapsedTimer.elapsed() < maxFrameDelay,
                 "If the delay is more than 2s, we consider the video playing is hanging.");

        /* Some debug code for windows. Use the code instead of the check above to debug the bug.
         * https://bugreports.qt.io/browse/QTBUG-105940.
         * TODO: fix hanging on windows and remove.
        if ( surface.m_elapsedTimer.elapsed() > maxFrameDelay ) {
            qDebug() << "pause/play";
            player.pause();
            player.play();
            surface.m_elapsedTimer.restart();
            spy.clear();
        }*/
    }

    duration += waitingInterval * player.playbackRate();

    QCOMPARE_GT(duration, minDuration);

    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).size(), 1);
    QCOMPARE(spy.at(0).at(0).value<QMediaPlayer::PlaybackState>(),
             QMediaPlayer::PlaybackState::StoppedState);

    QCOMPARE(player.playbackState(), QMediaPlayer::PlaybackState::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::MediaStatus::EndOfMedia);
}

static qreal colorDifference(QRgb first, QRgb second)
{
    const auto diffVector = QVector3D(qRed(first), qGreen(first), qBlue(first)) - QVector3D(qRed(second), qGreen(second), qBlue(second));
    static const auto normalizationFactor = 1. / ( 255 * qSqrt(3.) );
    return diffVector.length() * normalizationFactor;
}

template<typename It>
It findSimilarColor(It it, It end, QRgb color)
{
    return std::min_element(it, end, [color](QRgb first, QRgb second) {
        return colorDifference(first, color) < colorDifference(second, color);
    });
}

void tst_QMediaPlayerBackend::multipleSeekStressTest()
{
#ifdef Q_OS_ANDROID
    QSKIP("frame.toImage will return null image because of QTBUG-108446");
#endif
    if (localVideoFile3ColorsWithSound.isEmpty())
        QSKIP("Video format is not supported");

    TestVideoSink surface(false);
    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output);
    player.setVideoOutput(&surface);

    player.setSource(localVideoFile3ColorsWithSound);

    player.play();

    auto waitAndCheckFrame = [&](qint64 pos, QString checkInfo) {
        auto errorPrintingGuard = qScopeGuard([&]() {
            qDebug() << "Error:" << checkInfo;
            qDebug() << "Position:" << pos;
        });

        auto frame = surface.waitForFrame();
        QVERIFY(frame.isValid());

        const auto trackTime = pos * 1000;

        // in theory, previous frame might be received, in this case we wait for a new one that is
        // expected to be relevant
        if (frame.endTime() < trackTime || frame.startTime() > trackTime) {
            frame = surface.waitForFrame();
            QVERIFY(frame.isValid());
        }

        QCOMPARE_GE(frame.startTime(), trackTime - 200'000);
        QCOMPARE_LE(frame.endTime(), trackTime + 200'000);

        auto frameImage = frame.toImage();
        const auto actualColor = frameImage.pixel(1, 1);

        const auto actualColorIndex = std::distance(
                video3Colors.begin(),
                findSimilarColor(video3Colors.begin(), video3Colors.end(), actualColor));

        const auto expectedColorIndex = pos / 1000;

        QCOMPARE(actualColorIndex, expectedColorIndex);

        errorPrintingGuard.dismiss();
    };

    auto seekAndCheck = [&](qint64 pos) {
        QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));
        player.setPosition(pos);

        QTRY_VERIFY(positionSpy.size() >= 1);
        int setPosition = positionSpy.first().first().toInt();
        QCOMPARE_GT(setPosition, pos - 100);
        QCOMPARE_LT(setPosition, pos + 100);
    };

    constexpr qint64 posInterval = 10;

    {
        for (qint64 pos = posInterval; pos <= 2200; pos += posInterval)
            seekAndCheck(pos);

        waitAndCheckFrame(2200, "emulate fast moving of a seek slider forward");

        QCOMPARE_NE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    }

    {
        for (qint64 pos = 2100; pos >= 800; pos -= posInterval)
            seekAndCheck(pos);

        waitAndCheckFrame(800, "emulate fast moving of a seek slider backward");

        QCOMPARE_NE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    }

    {
        player.pause();

        for (qint64 pos = 500; pos <= 1100; pos += posInterval)
            seekAndCheck(pos);

        waitAndCheckFrame(1100, "emulate fast moving of a seek slider forward on paused state");

        QCOMPARE_NE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    }
}

void tst_QMediaPlayerBackend::playbackRateChanging()
{
#ifdef Q_OS_ANDROID
    QSKIP("frame.toImage will return null image because of QTBUG-108446");
#endif
    if (localVideoFile3ColorsWithSound.isEmpty())
        QSKIP("Video format is not supported");

#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("SKIP on macOS CI since multiple fake drawing on macOS CI platform causes UB. To be "
              "investigated: QTBUG-111744");
#endif

    TestVideoSink surface(false);
    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output); // TODO: mock audio output and check sound by frequency
    player.setVideoOutput(&surface);
    player.setSource(localVideoFile3ColorsWithSound);

    std::optional<QRgb> color;
    connect(&surface, &QVideoSink::videoFrameChanged, this, [&](const QVideoFrame& frame) {
        auto image = frame.toImage();
        color = image.isNull() ? std::optional<QRgb>{} : image.pixel(1, 1);
    }, Qt::DirectConnection);

    auto checkColorAndPosition = [&](int colorIndex, QString errorTag) {
        QVERIFY(color);
        const auto expectedColor = video3Colors[colorIndex];
        const auto actualColor = *color;

        auto errorPrintingGuard = qScopeGuard([&]() {
            qDebug() << "Error Tag:" << errorTag;
            qDebug() << "Actual Color:" << QColor(actualColor)
                     << "Expected Color:" << QColor(expectedColor);
            qDebug() << "Most probable actual color index:"
                     << std::distance(video3Colors.begin(),
                                      findSimilarColor(video3Colors.begin(), video3Colors.end(),
                                                       actualColor))
                     << " Expected color index:" << colorIndex;
            qDebug() << "Actual position:" << player.position();
        });

        constexpr qint64 intervalTime = 1000;

        // TODO: investigate why frames sometimes are not delivered in time on windows
        constexpr qreal maxColorDifference = 0.18;
        QCOMPARE_LE(colorDifference(actualColor, expectedColor), maxColorDifference);
        QCOMPARE_GT(player.position(), intervalTime * colorIndex);
        QCOMPARE_LT(player.position(), intervalTime * (colorIndex + 1));

        errorPrintingGuard.dismiss();
    };

    player.play();

    auto waitUntil = [&](quint64 t) {
        surface.waitForFrame();

        QTest::qWait((t - player.position()) / player.playbackRate());
    };

    waitUntil(400);
    checkColorAndPosition(0, "Check default playback rate");

    player.setPlaybackRate(2.);

    waitUntil(1400);
    checkColorAndPosition(1, "Check 2.0 playback rate");

    player.setPlaybackRate(0.5);

    waitUntil(1900);
    checkColorAndPosition(1, "Check 0.5 playback rate");

    player.stop();
}

void tst_QMediaPlayerBackend::surfaceTest()
{
    // 25 fps video file
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    QAudioOutput output;
    TestVideoSink surface(false);
    QMediaPlayer player;
    player.setAudioOutput(&output);
    player.setVideoOutput(&surface);
    player.setSource(localVideoFile);
    player.play();
    QTRY_VERIFY(player.position() >= 1000);
    QVERIFY2(surface.m_totalFrames >= 25, qPrintable(QString("Expected >= 25, got %1").arg(surface.m_totalFrames)));
}

#if 0
void tst_QMediaPlayerBackend::multipleSurfaces()
{
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    QVideoSink surface1;
    QVideoSink surface2;

    QMediaPlayer player;
    player.setVideoOutput(QList<QVideoSink *>() << &surface1 << &surface2);
    player.setSource(localVideoFile);
    player.play();
    QTRY_VERIFY(player.position() >= 1000);
//    QVERIFY2(surface1.m_totalFrames >= 25, qPrintable(QString("Expected >= 25, got %1").arg(surface1.m_totalFrames)));
//    QVERIFY2(surface2.m_totalFrames >= 25, qPrintable(QString("Expected >= 25, got %1").arg(surface2.m_totalFrames)));
//    QCOMPARE(surface1.m_totalFrames, surface2.m_totalFrames);
}
#endif

void tst_QMediaPlayerBackend::metadata()
{
    if (localFileWithMetadata.isEmpty())
        QSKIP("No supported media file");

    QAudioOutput output;
    QMediaPlayer player;
    player.setAudioOutput(&output);

    QSignalSpy metadataChangedSpy(&player, SIGNAL(metaDataChanged()));

    player.setSource(localFileWithMetadata);

    QTRY_VERIFY(metadataChangedSpy.size() > 0);

    QCOMPARE(player.metaData().value(QMediaMetaData::Title).toString(), QStringLiteral("Nokia Tune"));
    QCOMPARE(player.metaData().value(QMediaMetaData::ContributingArtist).toString(), QStringLiteral("TestArtist"));
    QCOMPARE(player.metaData().value(QMediaMetaData::AlbumTitle).toString(), QStringLiteral("TestAlbum"));
    QCOMPARE(player.metaData().value(QMediaMetaData::Duration), QVariant(7696));

    metadataChangedSpy.clear();

    player.setSource(QUrl());

    QCOMPARE(metadataChangedSpy.size(), 1);
    QVERIFY(player.metaData().isEmpty());
}

void tst_QMediaPlayerBackend::playerStateAtEOS()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioOutput output;
    QMediaPlayer player;
    player.setAudioOutput(&output);

    bool endOfMediaReceived = false;
    connect(&player, &QMediaPlayer::mediaStatusChanged,
            this, [&](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
            endOfMediaReceived = true;
        }
    });

    player.setSource(localWavFile);
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
    player.setSourceDevice(&file, localVideoFile);
    player.play();
    QTRY_VERIFY(player.position() >= 1000);
    QVERIFY2(surface.m_totalFrames >= 25, qPrintable(QString("Expected >= 25, got %1").arg(surface.m_totalFrames)));
}

void tst_QMediaPlayerBackend::audioVideoAvailable()
{
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    TestVideoSink surface(false);
    QAudioOutput output;
    QMediaPlayer player;
    QSignalSpy hasVideoSpy(&player, SIGNAL(hasVideoChanged(bool)));
    QSignalSpy hasAudioSpy(&player, SIGNAL(hasAudioChanged(bool)));
    player.setVideoOutput(&surface);
    player.setAudioOutput(&output);
    player.setSource(localVideoFile);
    QTRY_VERIFY(player.hasVideo());
    QTRY_VERIFY(player.hasAudio());
    QCOMPARE(hasVideoSpy.size(), 1);
    QCOMPARE(hasAudioSpy.size(), 1);
    player.setSource(QUrl());
    QTRY_VERIFY(!player.hasVideo());
    QTRY_VERIFY(!player.hasAudio());
    QCOMPARE(hasVideoSpy.size(), 2);
    QCOMPARE(hasAudioSpy.size(), 2);
}

void tst_QMediaPlayerBackend::isSeekable()
{
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    TestVideoSink surface(false);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
    QVERIFY(!player.isSeekable());
    player.setSource(localVideoFile);
    QTRY_VERIFY(player.isSeekable());
}

void tst_QMediaPlayerBackend::positionAfterSeek()
{
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    TestVideoSink surface(false);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
    QVERIFY(!player.isSeekable());
    player.setSource(localVideoFile);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    player.pause();
    player.setPosition(500);
    QTRY_VERIFY(player.position() == 500);
    player.setPosition(700);
    QVERIFY(player.position() != 0);
    QTRY_VERIFY(player.position() == 700);
    player.play();
    QTRY_VERIFY(player.position() > 700);
    player.setPosition(200);
    QVERIFY(player.position() != 0);
    QTRY_VERIFY(player.position() < 700);
}

void tst_QMediaPlayerBackend::videoDimensions()
{
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    TestVideoSink surface(true);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
    QVERIFY(!player.isSeekable());
    player.setSource(videoDimensionTestFile);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    player.pause();
    QTRY_COMPARE(surface.m_totalFrames, 1);
    QCOMPARE(surface.m_frameList.last().width(), 540);
    QCOMPARE(surface.videoSize().width(), 540);
    QCOMPARE(surface.m_frameList.last().height(), 320);
    QCOMPARE(surface.videoSize().height(), 320);
}

void tst_QMediaPlayerBackend::position()
{
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    TestVideoSink surface(true);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
    QVERIFY(!player.isSeekable());
    player.setSource(localVideoFile);
    QTRY_VERIFY(player.isSeekable());

    player.play();
    player.setPosition(1000);
    QVERIFY(player.position() > 950);
    QVERIFY(player.position() < 1050);
    QTRY_VERIFY(player.position() > 1050);

    player.pause();
    player.setPosition(500);
    QVERIFY(player.position() > 450);
    QVERIFY(player.position() < 550);
    QTest::qWait(200);
    QVERIFY(player.position() > 450);
    QVERIFY(player.position() < 550);
}

void tst_QMediaPlayerBackend::durationDetectionIssues_data()
{
    QTest::addColumn<QString>("mediaFile");
    QTest::addColumn<qint64>("expectedDuration");
    QTest::addColumn<int>("expectedVideoTrackCount");
    QTest::addColumn<qint64>("expectedVideoTrackDuration");
    QTest::addColumn<int>("expectedAudioTrackCount");
    QTest::addColumn<QVariant>("expectedAudioTrackDuration");

    // clang-format off

    QTest::newRow("stream-duration-in-metadata")
            << QString{ "qrc:/testdata/duration_issues.webm" }
            << 400ll        // Total media duration
            << 1            // Number of video tracks in file
            << 400ll        // Video stream duration
            << 0            // Number of audio tracks in file
            << QVariant{};  // Audio stream duration (unused)

    QTest::newRow("no-stream-duration-in-metadata")
            << QString{ "qrc:/testdata/nokia-tune.mkv" }
            << 7531ll       // Total media duration
            << 0            // Number of video tracks in file
            << 0ll          // Video stream duration (unused)
            << 1            // Number of audio tracks in file
            << QVariant{};  // Audio stream duration (not present on file)

    // clang-format on
}

void tst_QMediaPlayerBackend::durationDetectionIssues()
{
    QFETCH(QString, mediaFile);
    QFETCH(qint64, expectedDuration);
    QFETCH(int, expectedVideoTrackCount);
    QFETCH(qint64, expectedVideoTrackDuration);
    QFETCH(int, expectedAudioTrackCount);
    QFETCH(QVariant, expectedAudioTrackDuration);

    // ffmpeg detects stream an incorrect stream duration, so we take
    // the correct duration from the metadata
    const QUrl videoWithDurationIssues =
            MediaFileSelector::selectMediaFile({ mediaFile });

    TestVideoSink surface(false);
    QAudioOutput output;
    QMediaPlayer player;

    QSignalSpy durationSpy(&player, &QMediaPlayer::durationChanged);

    player.setVideoOutput(&surface);
    player.setAudioOutput(&output);
    player.setSource(videoWithDurationIssues);

    // Duration event received
    QCOMPARE(durationSpy.size(), 1);
    QCOMPARE(durationSpy.front().front(), expectedDuration);

    // Duration property
    QCOMPARE(player.duration(), expectedDuration);
    QCOMPARE(player.metaData().value(QMediaMetaData::Duration), expectedDuration);

    // Track duration properties
    const auto videoTracks = player.videoTracks();
    QCOMPARE(videoTracks.size(), expectedVideoTrackCount);

    if (expectedVideoTrackCount != 0)
        QCOMPARE(videoTracks.front().value(QMediaMetaData::Duration), expectedVideoTrackDuration);

    const auto audioTracks = player.audioTracks();
    QCOMPARE(audioTracks.size(), expectedAudioTrackCount);

    if (expectedAudioTrackCount != 0)
        QCOMPARE(audioTracks.front().value(QMediaMetaData::Duration), expectedAudioTrackDuration);
}

static std::vector<std::pair<qint64, qint64>>
positionChangingIntervals(const QSignalSpy &positionSpy)
{
    std::vector<std::pair<qint64, qint64>> result;
    for (auto &params : positionSpy) {
        const auto pos = params.front().value<qint64>();

        if (result.empty() || pos < result.back().second)
            result.emplace_back(pos, pos);
        else
            result.back().second = pos;
    }

    return result;
}

void tst_QMediaPlayerBackend::finiteLoops()
{
    if (localVideoFile3ColorsWithSound.isEmpty())
        QSKIP("Video format is not supported");

#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("The test accidently gets crashed on macOS CI, not reproduced locally. To be "
              "investigated: QTBUG-111744");
#endif

    TestVideoSink surface(false);
    QMediaPlayer player;

    QSignalSpy positionSpy(&player, &QMediaPlayer::positionChanged);

    player.setVideoOutput(&surface);

    QCOMPARE(player.loops(), 1);
    player.setLoops(3);
    QCOMPARE(player.loops(), 3);

    player.setSource(localVideoFile3ColorsWithSound);
    player.setPlaybackRate(5);

    player.play();
    surface.waitForFrame();

    // check pause doesn't affect looping
    {
        QTest::qWait(static_cast<int>(player.duration() * 3
                                      * 0.6 /*relative pos*/ / player.playbackRate()));
        player.pause();
        player.play();
    }

    QTRY_COMPARE(player.playbackState(), QMediaPlayer::StoppedState);

    auto intervals = positionChangingIntervals(positionSpy);

    QCOMPARE(intervals.size(), 3u);
    QCOMPARE_GT(intervals[0].first, 0);
    QCOMPARE(intervals[0].second, player.duration());
    QCOMPARE(intervals[1], std::make_pair(qint64(0), player.duration()));
    QCOMPARE(intervals[2], std::make_pair(qint64(0), player.duration()));

    QCOMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);

    // be sure that counter is reset if repeat the same
    {
        positionSpy.clear();
        player.play();
        player.setPlaybackRate(10);
        surface.waitForFrame();

        QTRY_COMPARE(player.playbackState(), QMediaPlayer::StoppedState);
        QCOMPARE(positionChangingIntervals(positionSpy).size(), 3);
        QCOMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    }
}

void tst_QMediaPlayerBackend::infiniteLoops()
{
    if (localVideoFile2.isEmpty())
        QSKIP("Video format is not supported");

#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("The test accidently gets crashed on macOS CI, not reproduced locally. To be "
              "investigated: QTBUG-111744");
#endif

    TestVideoSink surface(false);
    QMediaPlayer player;

    player.setVideoOutput(&surface);

    QCOMPARE(player.loops(), 1);
    player.setLoops(QMediaPlayer::Infinite);
    QCOMPARE(player.loops(), QMediaPlayer::Infinite);

    // select some small file
    player.setSource(localVideoFile2);
    player.setPlaybackRate(20);

    player.play();
    surface.waitForFrame();

    for (int i = 0; i < 2; ++i) {
        QSignalSpy positionSpy(&player, &QMediaPlayer::positionChanged);

        QTest::qWait(
                std::max(static_cast<int>(player.duration() / player.playbackRate() * 4),
                         300 /*ensure some minimum waiting time to reduce threading flakiness*/));
        QCOMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);

        const auto intervals = positionChangingIntervals(positionSpy);
        QVERIFY(!intervals.empty());
        QCOMPARE(intervals.front().second, player.duration());
    }

    player.stop(); // QMediaPlayer::stop stops whether or not looping is infinite
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
}

void tst_QMediaPlayerBackend::seekOnLoops()
{
    if (localVideoFile3ColorsWithSound.isEmpty())
        QSKIP("Video format is not supported");

#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("The test accidently gets crashed on macOS CI, not reproduced locally. To be "
              "investigated: QTBUG-111744");
#endif

    TestVideoSink surface(false);
    QMediaPlayer player;

    QSignalSpy positionSpy(&player, &QMediaPlayer::positionChanged);

    player.setVideoOutput(&surface);
    player.setLoops(3);
    player.setPlaybackRate(2);

    player.setSource(localVideoFile3ColorsWithSound);

    player.play();
    surface.waitForFrame();

    // seek in the 1st loop
    player.setPosition(player.duration() * 4 / 5);

    // wait for the 2nd loop and seek
    surface.waitForFrame();
    QTRY_VERIFY(player.position() < player.duration() / 2);
    player.setPosition(player.duration() * 8 / 9);

    // wait for the 3rd loop and seek
    surface.waitForFrame();
    QTRY_VERIFY(player.position() < player.duration() / 2);
    player.setPosition(player.duration() * 4 / 5);

    QTRY_COMPARE(player.playbackState(), QMediaPlayer::StoppedState);

    auto intervals = positionChangingIntervals(positionSpy);

    QCOMPARE(intervals.size(), 3);
    QCOMPARE_GT(intervals[0].first, 0);
    QCOMPARE(intervals[0].second, player.duration());
    QCOMPARE(intervals[1], std::make_pair(qint64(0), player.duration()));
    QCOMPARE(intervals[2], std::make_pair(qint64(0), player.duration()));

    QCOMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
}

void tst_QMediaPlayerBackend::changeLoopsOnTheFly()
{
    if (localVideoFile3ColorsWithSound.isEmpty())
        QSKIP("Video format is not supported");

#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("The test accidently gets crashed on macOS CI, not reproduced locally. To be "
              "investigated: QTBUG-111744");
#endif

    TestVideoSink surface(false);
    QMediaPlayer player;

    QSignalSpy positionSpy(&player, &QMediaPlayer::positionChanged);

    player.setVideoOutput(&surface);
    player.setLoops(4);
    player.setPlaybackRate(5);

    player.setSource(localVideoFile3ColorsWithSound);

    player.play();
    surface.waitForFrame();

    player.setPosition(player.duration() * 4 / 5);

    // wait for the 2nd loop
    surface.waitForFrame();
    QTRY_VERIFY(player.position() < player.duration() / 2);
    player.setPosition(player.duration() * 8 / 9);

    player.setLoops(1);

    QTRY_COMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);

    auto intervals = positionChangingIntervals(positionSpy);
    QCOMPARE(intervals.size(), 2);

    QCOMPARE(intervals[1], std::make_pair(qint64(0), player.duration()));
}

void tst_QMediaPlayerBackend::changeVideoOutputNoFramesLost()
{
    QVideoSink sinks[4];
    std::atomic_int framesCount[4] = {
        0,
    };
    for (int i = 0; i < 4; ++i)
        setVideoSinkAsyncFramesCounter(sinks[i], framesCount[i]);

    QMediaPlayer player;

    player.setPlaybackRate(10);

    player.setVideoOutput(&sinks[0]);
    player.setSource(localVideoFile3ColorsWithSound);
    player.play();
    QTRY_VERIFY(!player.isPlaying());

    player.setPlaybackRate(4);
    player.setVideoOutput(&sinks[1]);
    player.play();

    QTRY_VERIFY(framesCount[1] >= framesCount[0] / 4);
    player.setVideoOutput(&sinks[2]);
    const int savedFrameNumber1 = framesCount[1];

    QTRY_VERIFY(framesCount[2] >= (framesCount[0] - savedFrameNumber1) / 2);
    player.setVideoOutput(&sinks[3]);
    const int savedFrameNumber2 = framesCount[2];

    QTRY_VERIFY(!player.isPlaying());

    // check if no frames sent to old sinks
    QCOMPARE(framesCount[1], savedFrameNumber1);
    QCOMPARE(framesCount[2], savedFrameNumber2);

    // no frames lost
    QCOMPARE(framesCount[1] + framesCount[2] + framesCount[3], framesCount[0]);
}

void tst_QMediaPlayerBackend::cleanSinkAndNoMoreFramesAfterStop()
{
    QVideoSink sink;
    std::atomic_int framesCount = 0;
    setVideoSinkAsyncFramesCounter(sink, framesCount);
    QMediaPlayer player;

    player.setPlaybackRate(10);
    player.setVideoOutput(&sink);

    player.setSource(localVideoFile3ColorsWithSound);

    // Run a few time to have more chances to detect race conditions
    for (int i = 0; i < 8; ++i) {
        player.play();
        QTRY_VERIFY(framesCount > 0);

        player.stop();

        QVERIFY(!sink.videoFrame().isValid());

        QCOMPARE_NE(framesCount, 0);
        framesCount = 0;

        QTest::qWait(30);

        // check if nothing changed after short waiting
        QCOMPARE(framesCount, 0);
    }
}

void tst_QMediaPlayerBackend::lazyLoadVideo()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(QUrl("qrc:/LazyLoad.qml"));
    QScopedPointer<QObject> root(component.create());
    QQuickItem *rootItem = qobject_cast<QQuickItem *>(root.get());
    QVERIFY(rootItem);

    QQuickView view;
    rootItem->setParentItem(view.contentItem());
    view.resize(600, 800);
    view.show();

    QQuickLoader *loader = qobject_cast<QQuickLoader *>(rootItem->findChild<QQuickItem *>("loader"));
    QVERIFY(loader);
    QCOMPARE(QQmlProperty::read(loader, "active").toBool(), false);
    loader->setProperty("active", true);
    QCOMPARE(QQmlProperty::read(loader, "active").toBool(), true);

    QQuickItem *videoPlayer = qobject_cast<QQuickItem *>(loader->findChild<QQuickItem *>("videoPlayer"));
    QVERIFY(videoPlayer);

    QCOMPARE(QQmlProperty::read(videoPlayer, "playbackState").value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PlayingState);
    QCOMPARE(QQmlProperty::read(videoPlayer, "error").value<QMediaPlayer::Error>(), QMediaPlayer::NoError);

    QVideoSink *videoSink = QQmlProperty::read(videoPlayer, "videoSink").value<QVideoSink *>();
    QVERIFY(videoSink);

    QSignalSpy spy(videoSink, &QVideoSink::videoFrameChanged);
    QVERIFY(spy.wait());

    QVideoFrame frame = spy.at(0).at(0).value<QVideoFrame>();
    QVERIFY(frame.isValid());
}

void tst_QMediaPlayerBackend::videoSinkSignals()
{
    // TODO: come up with custom frames source,
    // create the test target tst_QVideoSinkBackend,
    // and move the test there

    if (localVideoFile2.isEmpty())
        QSKIP("Video format is not supported");

    QVideoSink sink;
    QMediaPlayer player;
    player.setVideoSink(&sink);

    std::atomic<int> videoFrameCounter = 0;
    std::atomic<int> videoSizeCounter = 0;

    player.setSource(localVideoFile2);

    sink.platformVideoSink()->setNativeSize({}); // reset size to be able to check the size update

    connect(&sink, &QVideoSink::videoFrameChanged, this, [&](const QVideoFrame &frame) {
        QCOMPARE(sink.videoFrame(), frame);
        QCOMPARE(sink.videoSize(), frame.size());
        ++videoFrameCounter;
    }, Qt::DirectConnection);

    connect(&sink, &QVideoSink::videoSizeChanged, this, [&]() {
        QCOMPARE(sink.videoSize(), sink.videoFrame().size());
        if (sink.videoSize().isValid()) // filter end frame
            ++videoSizeCounter;
    }, Qt::DirectConnection);

    player.play();

    QTRY_COMPARE_GE(videoFrameCounter, 2);
    QCOMPARE(videoSizeCounter, 1);
}

void tst_QMediaPlayerBackend::nonAsciiFileName()
{
    QFile resourceFile(":/testdata/test.wav");
    if (!resourceFile.open(QIODeviceBase::ReadOnly))
        QSKIP("Could not open a resource file");

    QTemporaryFile temporaryFile("äöüØøÆ中文");
    if (!temporaryFile.open())
        QSKIP("Could not open a temporary file");

    QByteArray bytes = resourceFile.readAll();
    QDataStream stream(&temporaryFile);
    stream.writeRawData(bytes.data(), bytes.length());

    QMediaPlayer player;

    QSignalSpy errorOccurredSpy(&player, &QMediaPlayer::errorOccurred);

    player.setSource(temporaryFile.fileName());
    player.play();

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(errorOccurredSpy.size(), 0);
}

void tst_QMediaPlayerBackend::setMedia_setsVideoSinkSize_beforePlaying()
{
    QVideoSink sink1;
    QVideoSink sink2;
    QMediaPlayer player;

    QSignalSpy spy1(&sink1, &QVideoSink::videoSizeChanged);
    QSignalSpy spy2(&sink2, &QVideoSink::videoSizeChanged);

    player.setVideoOutput(&sink1);
    QCOMPARE(sink1.videoSize(), QSize());

    player.setSource(localVideoFile3ColorsWithSound);
    QCOMPARE(sink1.videoSize(), QSize(684, 384));

    player.setVideoOutput(&sink2);
    QCOMPARE(sink2.videoSize(), QSize(684, 384));

    QCOMPARE(spy1.size(), 1);
    QCOMPARE(spy2.size(), 1);
}

QTEST_MAIN(tst_QMediaPlayerBackend)
#include "tst_qmediaplayerbackend.moc"

