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

void tst_QMediaPlayerBackend::loadMedia()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QMediaPlayer player;
    QAudioOutput output;
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

    QCOMPARE(stateSpy.count(), 0);
    QVERIFY(statusSpy.count() > 0);
    QCOMPARE(mediaSpy.count(), 1);
    QCOMPARE(mediaSpy.last()[0].value<QUrl>(), localWavFile);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QVERIFY(player.hasAudio());
    QVERIFY(!player.hasVideo());
}

void tst_QMediaPlayerBackend::unloadMedia()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QMediaPlayer player;
    QAudioOutput output;
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

    QMediaPlayer player;
    QAudioOutput output;
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

void tst_QMediaPlayerBackend::playPauseStop()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QMediaPlayer player;
    QAudioOutput output;
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
    QCOMPARE(stateSpy.count(), 0);
    QCOMPARE(statusSpy.count(), 0);
    QCOMPARE(positionSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);

    // Check pause() without a media
    player.pause();

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QCOMPARE(player.position(), 0);
    QCOMPARE(stateSpy.count(), 0);
    QCOMPARE(statusSpy.count(), 0);
    QCOMPARE(positionSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);

    // The rest is with a valid media

    player.setSource(localWavFile);

    QCOMPARE(player.position(), qint64(0));

    player.play();

    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(statusSpy.count() > 0 &&
                statusSpy.last()[0].value<QMediaPlayer::MediaStatus>() == QMediaPlayer::BufferedMedia);

    QTRY_VERIFY(player.position() > 100);
    QVERIFY(player.duration() > 0);
    QTRY_VERIFY(positionSpy.count() > 0);
    QTRY_VERIFY(positionSpy.last()[0].value<qint64>() > 0);

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    qint64 positionBeforePause = player.position();
    player.pause();

    QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PausedState);

    QTest::qWait(500);

    QTRY_VERIFY(qAbs(player.position() - positionBeforePause) < 150);

    stateSpy.clear();
    statusSpy.clear();

    player.stop();

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);
    //it's allowed to emit statusChanged() signal async
    QTRY_COMPARE(statusSpy.count(), 1);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::LoadedMedia);

    //ensure the position is reset to 0 at stop and positionChanged(0) is emitted
    QTRY_COMPARE(player.position(), qint64(0));
    QTRY_VERIFY(positionSpy.count() > 0);
    QCOMPARE(positionSpy.last()[0].value<qint64>(), qint64(0));
    QVERIFY(player.duration() > 0);

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    player.play();

    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PlayingState);
    QCOMPARE(statusSpy.count(), 1); // Should not go through Loading again when play -> stop -> play
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::BufferedMedia);

    player.stop();
    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    player.setSource(localWavFile2);

    QTRY_VERIFY(statusSpy.count() > 0);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::LoadedMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.count(), 0);

    player.play();

    QTRY_VERIFY(player.position() > 100);

    player.setSource(localWavFile);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QTRY_VERIFY(statusSpy.count() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::LoadedMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_VERIFY(stateSpy.count() > 0);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);
    QTRY_VERIFY(positionSpy.count() > 0);
    QCOMPARE(player.position(), 0);
    QCOMPARE(positionSpy.last()[0].value<qint64>(), 0);

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    player.play();

    QTRY_VERIFY(player.position() > 100);

    player.setSource(QUrl());

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::NoMedia);
    QTRY_VERIFY(statusSpy.count() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::NoMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_VERIFY(stateSpy.count() > 0);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);
    QTRY_VERIFY(positionSpy.count() > 0);
    QCOMPARE(player.position(), 0);
    QCOMPARE(positionSpy.last()[0].value<qint64>(), 0);
    QCOMPARE(player.duration(), 0);
}


void tst_QMediaPlayerBackend::processEOS()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QMediaPlayer player;
    QAudioOutput output;
    player.setAudioOutput(&output);

    QSignalSpy stateSpy(&player, SIGNAL(playbackStateChanged(QMediaPlayer::PlaybackState)));
    QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    player.setSource(localWavFile);

    player.play();
    player.setPosition(900);

    //wait up to 5 seconds for EOS
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);

    QVERIFY(statusSpy.count() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::EndOfMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.count(), 2);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);

    //at EOS the position stays at the end of file
    QCOMPARE(player.position(), player.duration());
    QTRY_VERIFY(positionSpy.count() > 0);
    QTRY_COMPARE(positionSpy.last()[0].value<qint64>(), player.duration());

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    player.play();

    //position is reset to start
    QTRY_VERIFY(player.position() < 100);
    QTRY_VERIFY(positionSpy.count() > 0);
    QCOMPARE(positionSpy.first()[0].value<qint64>(), 0);

    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PlayingState);
    QVERIFY(statusSpy.count() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::BufferedMedia);

    positionSpy.clear();
    QTRY_VERIFY(player.position() > 100);
    QTRY_VERIFY(positionSpy.count() > 0);
    QVERIFY(positionSpy.last()[0].value<qint64>() > 100);
    player.setPosition(900);
    //wait up to 5 seconds for EOS
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QVERIFY(statusSpy.count() > 0);
    QCOMPARE(statusSpy.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::EndOfMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.count(), 2);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);

    //position stays at the end of file
    QCOMPARE(player.position(), player.duration());
    QTRY_VERIFY(positionSpy.count() > 0);
    QTRY_COMPARE(positionSpy.last()[0].value<qint64>(), player.duration());

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
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(player.position(), player.duration());

    stateSpy.clear();
    statusSpy.clear();
    positionSpy.clear();

    // pause() should reset position to beginning and status to Buffered
    player.pause();

    QTRY_COMPARE(player.position(), 0);
    QTRY_VERIFY(positionSpy.count() > 0);
    QTRY_COMPARE(positionSpy.first()[0].value<qint64>(), 0);

    QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PausedState);
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
    //volume and muted properties should be independent
    QMediaPlayer player;
    QAudioOutput output;
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
    QCOMPARE(volumeSpy.count(), 1);
    QCOMPARE(volumeSpy.last()[0].toFloat(), output.volume());
    QCOMPARE(mutedSpy.count(), 0);

    output.setVolume(0.5);
    QTRY_COMPARE(output.volume(), 0.5);
    QVERIFY(!output.isMuted());
    QCOMPARE(volumeSpy.count(), 2);
    QCOMPARE(volumeSpy.last()[0].toFloat(), output.volume());
    QCOMPARE(mutedSpy.count(), 0);

    output.setMuted(true);
    QTRY_VERIFY(output.isMuted());
    QVERIFY(output.volume() > 0);
    QCOMPARE(volumeSpy.count(), 2);
    QCOMPARE(mutedSpy.count(), 1);
    QCOMPARE(mutedSpy.last()[0].toBool(), output.isMuted());

    output.setMuted(false);
    QTRY_VERIFY(!output.isMuted());
    QVERIFY(output.volume() > 0);
    QCOMPARE(volumeSpy.count(), 2);
    QCOMPARE(mutedSpy.count(), 2);
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

    QMediaPlayer player;
    QAudioOutput output;
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
        QMediaPlayer player;
        QAudioOutput output;
        player.setAudioOutput(&output);
        output.setVolume(1);
        player.setSource(localWavFile);
        QCOMPARE(output.volume(), 1);
        player.play();
        QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(output.volume(), 1);
    }

    {
        QMediaPlayer player;
        QAudioOutput output;
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
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    QMediaPlayer player;
    QAudioOutput output;
    player.setAudioOutput(&output);

    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    TestVideoSink *surface = new TestVideoSink;
    player.setVideoOutput(surface);

    player.setSource(localVideoFile);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(surface->m_frameList.isEmpty()); // frame must not appear until we call pause() or play()

    positionSpy.clear();
    qint64 position = 7000;
    player.setPosition(position);
    QTRY_VERIFY(!positionSpy.isEmpty());
    QTRY_COMPARE(player.position(), position);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTest::qWait(250); // wait a bit to ensure the frame is not rendered
    QVERIFY(surface->m_frameList.isEmpty()); // still no frame, we must call pause() or play() to see a frame

    player.pause();
    QTRY_COMPARE(player.playbackState(), QMediaPlayer::PausedState); // it might take some time for the operation to be completed
    QTRY_VERIFY(!surface->m_frameList.isEmpty()); // we must see a frame at position 7000 here

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
        QImage image = frame.toImage();
        QVERIFY(!image.isNull());
        QVERIFY(qRed(image.pixel(0, 0)) >= 230); // conversion from YUV => RGB, that's why it's not 255
        QVERIFY(qGreen(image.pixel(0, 0)) < 20);
        QVERIFY(qBlue(image.pixel(0, 0)) < 20);
    }

    surface->m_frameList.clear();
    positionSpy.clear();
    position = 12000;
    player.setPosition(position);
    QTRY_VERIFY(!positionSpy.isEmpty() && qAbs(player.position() - position) < (qint64)500);
    QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    QTRY_VERIFY(!surface->m_frameList.isEmpty());

    {
        QVideoFrame frame = surface->m_frameList.back();
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

    QMediaPlayer player;
    QAudioOutput output;
    player.setAudioOutput(&output);
    TestVideoSink surface(false);
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
    QTRY_VERIFY(positionSpy.count() > 0);
    QVERIFY(qAbs(positionSpy.last()[0].value<qint64>() - position) < qint64(200));

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.count(), 0);

    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    positionSpy.clear();

    player.play();

    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(player.position() > position);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QTest::qWait(100);
    // Check that it never played from the beginning
    QVERIFY(player.position() > position);
    for (int i = 0; i < positionSpy.count(); ++i)
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
    QTRY_VERIFY(positionSpy.count() > 0);
    QVERIFY(qAbs(positionSpy.last()[0].value<qint64>() - position) < qint64(200));

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.count(), 0);

    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    positionSpy.clear();

    player.play();

    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);
    QVERIFY(qAbs(player.position() - position) < qint64(200));

    QTest::qWait(500);
    // Check that it never played from the beginning
    QVERIFY(player.position() > (position - 200));
    for (int i = 0; i < positionSpy.count(); ++i)
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
    QTRY_VERIFY(positionSpy.count() > 0);
    QVERIFY(qAbs(positionSpy.last()[0].value<qint64>() - position) < qint64(200));

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.count(), 0);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    player.play();
    QTRY_COMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    positionSpy.clear();
    QTRY_VERIFY(player.position() > (position - 200));

    QTest::qWait(500);
    // Check that it never played from the beginning
    QVERIFY(player.position() > (position - 200));
    for (int i = 0; i < positionSpy.count(); ++i)
        QVERIFY(positionSpy.at(i)[0].value<qint64>() > (position - 200));
}

void tst_QMediaPlayerBackend::subsequentPlayback()
{
    if (localCompressedSoundFile.isEmpty())
        QSKIP("Sound format is not supported");

    QMediaPlayer player;
    QAudioOutput output;
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

    TestVideoSink surface(false);
    QMediaPlayer player;
    QAudioOutput output;

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

void tst_QMediaPlayerBackend::surfaceTest()
{
    // 25 fps video file
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    TestVideoSink surface(false);
    QMediaPlayer player;
    QAudioOutput output;
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

    QMediaPlayer player;
    QAudioOutput output;
    player.setAudioOutput(&output);

    QSignalSpy metadataChangedSpy(&player, SIGNAL(metaDataChanged()));

    player.setSource(localFileWithMetadata);

    QTRY_VERIFY(metadataChangedSpy.count() > 0);

    QCOMPARE(player.metaData().value(QMediaMetaData::Title).toString(), QStringLiteral("Nokia Tune"));
    QCOMPARE(player.metaData().value(QMediaMetaData::ContributingArtist).toString(), QStringLiteral("TestArtist"));
    QCOMPARE(player.metaData().value(QMediaMetaData::AlbumTitle).toString(), QStringLiteral("TestAlbum"));

    metadataChangedSpy.clear();

    player.setSource(QUrl());

    QCOMPARE(metadataChangedSpy.count(), 1);
    QVERIFY(player.metaData().isEmpty());
}

void tst_QMediaPlayerBackend::playerStateAtEOS()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QMediaPlayer player;
    QAudioOutput output;
    player.setAudioOutput(&output);

    bool endOfMediaReceived = false;
    connect(&player, &QMediaPlayer::mediaStatusChanged, [&](QMediaPlayer::MediaStatus status) {
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
    QCOMPARE(hasVideoSpy.count(), 1);
    QCOMPARE(hasAudioSpy.count(), 1);
    player.setSource(QUrl());
    QTRY_VERIFY(!player.hasVideo());
    QTRY_VERIFY(!player.hasAudio());
    QCOMPARE(hasVideoSpy.count(), 2);
    QCOMPARE(hasAudioSpy.count(), 2);
}

void tst_QMediaPlayerBackend::isSeekable()
{
    if (localVideoFile.isEmpty())
        QSKIP("No supported video file");

    TestVideoSink surface(false);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
#ifdef Q_OS_ANDROID
    QEXPECT_FAIL("", "On Android isSeekable() is always set to true due to QTBUG-96952", Continue);
#endif
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
#ifdef Q_OS_ANDROID
    QEXPECT_FAIL("", "On Android isSeekable() is always set to true due to QTBUG-96952", Continue);
#endif
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
#ifdef Q_OS_ANDROID
    QEXPECT_FAIL("", "On Android isSeekable() is always set to true due to QTBUG-96952", Continue);
#endif
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
#ifdef Q_OS_ANDROID
    QEXPECT_FAIL("", "On Android isSeekable() is always set to true due to QTBUG-96952", Continue);
#endif
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

QTEST_MAIN(tst_QMediaPlayerBackend)
#include "tst_qmediaplayerbackend.moc"

