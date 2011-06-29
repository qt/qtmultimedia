/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#include "tst_QMediaPlayer_xa.h"

QT_USE_NAMESPACE

#define QTEST_MAIN_XA(TestObject) \
    int main(int argc, char *argv[]) { \
        char *new_argv[3]; \
        QApplication app(argc, argv); \
        \
        QString str = "C:\\data\\" + QFileInfo(QCoreApplication::applicationFilePath()).baseName() + ".log"; \
        QByteArray   bytes  = str.toAscii(); \
        \
        char arg1[] = "-o"; \
        \
        new_argv[0] = argv[0]; \
        new_argv[1] = arg1; \
        new_argv[2] = bytes.data(); \
        \
        TestObject tc; \
        return QTest::qExec(&tc, 3, new_argv); \
    }

#define WAIT_FOR_CONDITION(a,e)            \
    for (int _i = 0; _i < 500; _i += 1) {  \
        if ((a) == (e)) break;             \
        QTest::qWait(10);}

#define WAIT_FOR_EITHER_CONDITION(a,e,f)            \
    for (int _i = 0; _i < 500; _i += 1) {  \
        if (((a) == (e)) || ((a) == (f))) break;             \
        QTest::qWait(10);}

#define WAIT_FOR_CONDITION1(a)            \
    for (int _i = 0; _i < 500; _i += 1) {  \
        if (a) break;             \
        QTest::qWait(10);}


#define WAIT_LONG_FOR_CONDITION(a,e)        \
    for (int _i = 0; _i < 1800; _i += 1) {  \
        if ((a) == (e)) break;              \
        QTest::qWait(10);}

#define WAIT_LONG_FOR_CONDITION1(a)        \
    for (int _i = 0; _i < 1800; _i += 1) {  \
        if (a) break;              \
        QTest::qWait(100);}

tst_QMediaPlayer_xa::tst_QMediaPlayer_xa(): m_player(NULL), m_widget(NULL), m_windowWidget(NULL)
{
    audioOnlyContent        = new QMediaContent(QUrl("file:///C:/data/testfiles/test.mp3"));
    videoOnlyContent        = new QMediaContent(QUrl("file:///C:/data/testfiles/test_video.3gp"));
    audioVideoContent       = new QMediaContent(QUrl("file:///C:/data/testfiles/test.3gp"));
    audioVideoAltContent    = new QMediaContent(QUrl("file:///C:/data/testfiles/test_alt.3gp"));
    //streamingContent        = new QMediaContent(QUrl("rtsp://10.48.2.51/Copyright_Free_Test_Content/Clips/Video/3GP/176x144/h263/h263_176x144_15fps_384kbps_AAC-LC_128kbps_mono_44.1kHz.3gp"));
    streamingContent3gp     = new QMediaContent(QUrl("http://www.mobileplayground.co.uk/video/Crazy Frog.3gp"));
    audioStreamingContent   = new QMediaContent(QUrl("http://myopusradio.com:8000/easy"));
    mediaContent            = audioVideoContent;
}

tst_QMediaPlayer_xa::~tst_QMediaPlayer_xa()
{
    delete audioOnlyContent;
    delete videoOnlyContent;
    delete audioVideoContent;
    delete audioVideoAltContent;
}

void tst_QMediaPlayer_xa::initTestCase_data()
{
}

void tst_QMediaPlayer_xa::initTestCase()
{
   m_player = new QMediaPlayer();

   // Symbian back end needs coecontrol for creation.
   m_widget = new QVideoWidget();
   m_widget->setGeometry ( 0, 100, 350, 250 );
   m_player->setVideoOutput(m_widget);
   m_widget->showNormal();
}

void tst_QMediaPlayer_xa::cleanupTestCase()
{
    delete m_player;
    delete m_widget;
    delete m_windowWidget;
}

void tst_QMediaPlayer_xa::resetPlayer()
{
    delete m_player;
    m_player = new QMediaPlayer();
    m_player->setVideoOutput(m_widget);
}

void tst_QMediaPlayer_xa::resetPlayer_WindowControl()
{
    delete m_player;
    m_player = new QMediaPlayer();

    if(!m_windowWidget) {
        m_windowWidget = new QWidget();
        m_windowWidget->showMaximized();
    }

    QVideoWindowControl* windowControl = (QVideoWindowControl*)(m_player->service()->requestControl(QVideoWindowControl_iid));
    if (windowControl)
        windowControl->setWinId(m_windowWidget->winId());
}

void tst_QMediaPlayer_xa::init()
{
    qRegisterMetaType<QMediaPlayer::State>("QMediaPlayer::State");
    qRegisterMetaType<QMediaPlayer::Error>("QMediaPlayer::Error");
    qRegisterMetaType<QMediaPlayer::MediaStatus>("QMediaPlayer::MediaStatus");
    qRegisterMetaType<QMediaContent>("QMediaContent");
    updateLog("QT MediaPlayer Auto Test Cases", true);
}

void tst_QMediaPlayer_xa::cleanup()
{
}


void tst_QMediaPlayer_xa::testMedia()
{
    updateLog("*****testMedia");

    setAudioVideoContent();

    QTest::qWait(500);
    QCOMPARE(m_player->media(), *mediaContent);

    updateLog("*****testMedia: PASSED");
}


void tst_QMediaPlayer_xa::testDuration()
{
    updateLog("*****testDuration");

    resetPlayer();

    QSignalSpy spy(m_player, SIGNAL(durationChanged(qint64)));
    setAudioVideoContent();

    WAIT_FOR_CONDITION1(spy.count()>0);

    QVERIFY(m_player->duration() == duration);

    updateLog("*****testDuration: PASSED");
}

void tst_QMediaPlayer_xa::testPosition()
{
    updateLog("*****testPosition");
    resetPlayer();

    qint64 position = 60000;
    setAudioVideoContent();

    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

    QCOMPARE(m_player->isSeekable(), true);

     // preset position
    {
        QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->setPosition(position);
        WAIT_FOR_CONDITION(spy.count(), 1);
        QCOMPARE(m_player->position(), position);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toLongLong(), position);
    }

    // same pos second time
    {
        QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->setPosition(position);
        QCOMPARE(m_player->position(), position);
        QCOMPARE(spy.count(), 0);
    }

    //zero pos
    {
        QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->setPosition(0);
        QCOMPARE(m_player->position(), qint64(0));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toLongLong(),  qint64(0));
    }

    //end pos
    {
        QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->setPosition(duration);
        QCOMPARE(m_player->position(), duration);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toLongLong(),  duration);
    }

    //negative pos
    {
        QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->setPosition(qint64(-1));
        QCOMPARE(m_player->position(), qint64(0));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toLongLong(),  qint64(0));
    }

    //over duration
    {
        QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->setPosition(duration+1);
        QCOMPARE(m_player->position(), duration);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toLongLong(),  duration);
    }

}


void tst_QMediaPlayer_xa::testPositionWhilePlaying()
{
    updateLog("*****testPositionWhilePlaying");
    resetPlayer();

    qint64 position = 60000;

    setAudioVideoContent();

    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

    // preset position
    {
        QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        m_player->setPosition(position);
        WAIT_FOR_CONDITION1(spy.count()>0);
        QVERIFY(m_player->mediaStatus() == QMediaPlayer::BufferingMedia || m_player->mediaStatus() == QMediaPlayer::BufferedMedia);

        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QVERIFY(m_player->position()>=position);
        QVERIFY(spy.count()!=0);
    }

    //reset position
    m_player->stop();
    m_player->setPosition(position);
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

     //zero pos
     {  QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        m_player->setPosition(0);
        WAIT_FOR_CONDITION1(spy.count()>0);
        QVERIFY(m_player->mediaStatus() == QMediaPlayer::BufferingMedia ||
                m_player->mediaStatus() == QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QVERIFY(m_player->position() >= qint64(0));
        QVERIFY(spy.count()!=0);
    }

    //reset position
    m_player->stop();
    m_player->setPosition(position);
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

    updateLog("*****testPositionWhilePlaying: PASSED");
}


void tst_QMediaPlayer_xa::testVolume()
{
    updateLog("*****testVolume");

    int volume = 20;

    // preset volume
    {
        QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(volume);
        QCOMPARE(m_player->volume(), volume);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);
    }

    // same volume
    {
        QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        int currentVolume = m_player->volume();
        m_player->setVolume(currentVolume);
        QCOMPARE(m_player->volume(), currentVolume);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 0);
    }

    // zero volume
    {
        QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(0);
        QCOMPARE(m_player->volume(), 0);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);
    }

    // max volume
    {
        QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(100);
        QCOMPARE(m_player->volume(), 100);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);
    }

    // negative volume
    {
        QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(int(-1));
        QCOMPARE(m_player->volume(), 0);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);
    }

    // over max volume
    {
        QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(1000);
        QCOMPARE(m_player->volume(), 100);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);
    }

    updateLog("*****testVolume: PASSED");
}

void tst_QMediaPlayer_xa::testVolumeWhilePlaying()
{
    updateLog("*****testVideoAndAudioAvailability");
    resetPlayer();

    int volume = 20;

    setAudioVideoContent();

    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);

    m_player->play();
    QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);

    // preset volume
    {
        QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(volume);
        QCOMPARE(m_player->volume(), volume);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);
    }

    // same volume
    {
        QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        int currentVolume = m_player->volume();
        m_player->setVolume(currentVolume);
        QCOMPARE(m_player->volume(), currentVolume);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 0);
    }

    // zero volume
    {
        QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(0);
        QCOMPARE(m_player->volume(), 0);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);
    }

    // max volume
    {
        QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(100);
        QCOMPARE(m_player->volume(), 100);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);
    }

    // negative volume
    {
        QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(int(-1));
        QCOMPARE(m_player->volume(), 0);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);
    }

    // over max volume
    {
        QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(1000);
        QCOMPARE(m_player->volume(), 100);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);
    }

    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

    updateLog("*****testVideoAndAudioAvailability: PASSED");
}


void tst_QMediaPlayer_xa::testMuted()
{
    updateLog("*****testMuted");

    int volume = 20;

    //reset mute & volume
    m_player->setMuted(false);
    m_player->setVolume(0);
    QVERIFY(m_player->isMuted() == false);
    QCOMPARE(m_player->volume(), 0);

    // set muted
    {
        QSignalSpy spy(m_player, SIGNAL(mutedChanged(bool)));
        m_player->setMuted(true);
        QCOMPARE(spy.count(), 1);
        QVERIFY(m_player->isMuted() == true);
    }

    // set muted again
    {
        QSignalSpy spy(m_player, SIGNAL(mutedChanged(bool)));
        m_player->setMuted(true);
        QCOMPARE(spy.count(), 0);
        QVERIFY(m_player->isMuted() == true);
    }

    // unmute
    {
        QSignalSpy spy(m_player, SIGNAL(mutedChanged(bool)));
        m_player->setMuted(false);
        QCOMPARE(spy.count(), 1);
        QVERIFY(m_player->isMuted() == false);
    }

    // set volume while muted
    {
        QSignalSpy muteSpy(m_player, SIGNAL(mutedChanged(bool)));
        QSignalSpy volumeSpy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setMuted(true);
        m_player->setVolume(volume);
        QCOMPARE(m_player->volume(), volume);
        QCOMPARE(muteSpy.count(), 1);
        QCOMPARE(volumeSpy.count(), 1);
        QVERIFY(m_player->isMuted() == true);
    }

    updateLog("*****testMuted: PASSED");
}

void tst_QMediaPlayer_xa::testMutedWhilePlaying()
{
    updateLog("*****testMutedWhilePlaying");
    resetPlayer();

    int volume = 20;

    setAudioVideoContent();

    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);

    //reset mute & volume
    m_player->setMuted(false);
    m_player->setVolume(65);
    QVERIFY(m_player->isMuted() == false);
    QCOMPARE(m_player->volume(), 65);

    m_player->play();
    QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);

    // set muted
    {
        QSignalSpy spy(m_player, SIGNAL(mutedChanged(bool)));
        m_player->setMuted(true);
        QCOMPARE(spy.count(), 1);
        QVERIFY(m_player->isMuted() == true);
    }

    // set muted again
    {
        QSignalSpy spy(m_player, SIGNAL(mutedChanged(bool)));
        m_player->setMuted(true);
        QCOMPARE(spy.count(), 0);
        QVERIFY(m_player->isMuted() == true);
    }

    // unmute
    {
        QSignalSpy spy(m_player, SIGNAL(mutedChanged(bool)));
        m_player->setMuted(false);
        QCOMPARE(spy.count(), 1);
        QVERIFY(m_player->isMuted() == false);
    }

    // set volume while muted
    {
        QSignalSpy muteSpy(m_player, SIGNAL(mutedChanged(bool)));
        QSignalSpy volumeSpy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setMuted(true);
        m_player->setVolume(volume);
        QCOMPARE(m_player->volume(), volume);
        QCOMPARE(muteSpy.count(), 1);
        QCOMPARE(volumeSpy.count(), 1);
        QVERIFY(m_player->isMuted() == true);
    }

    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);


    updateLog("*****testMutedWhilePlaying: PASSED");
}

void tst_QMediaPlayer_xa::testVideoAndAudioAvailability()
{
    updateLog("*****testVideoAndAudioAvailability");
    resetPlayer();

    QList<QVariant> arguments;


    setVideoOnlyContent();


    QSignalSpy audioAvailableSpy(m_player, SIGNAL(audioAvailableChanged(bool)));
    QSignalSpy videoAvailableSpy(m_player, SIGNAL(videoAvailableChanged(bool)));

    setAudioOnlyContent();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    updateLog("SetMedia: audioOnlyContent");
    WAIT_FOR_CONDITION(m_player->isAudioAvailable(), true);
    updateLog("\t isAudioAvailable() == true");
    QVERIFY(m_player->isVideoAvailable() == false);
    updateLog("\t isVideoAvailable() == false");
    QCOMPARE(audioAvailableSpy.count(), 1);
    arguments = audioAvailableSpy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);
    updateLog("\t audioAvailableChanged(true)");
    QCOMPARE(videoAvailableSpy.count(), 1);
    arguments = videoAvailableSpy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == false);
    updateLog("\t videoAvailableChanged(false)");

    setVideoOnlyContent();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    updateLog("SetMedia: videoOnlyContent");
    WAIT_FOR_CONDITION(m_player->isVideoAvailable(), true);
    updateLog("\t isVideoAvailable() == true");
    QVERIFY(m_player->isAudioAvailable() == false);
    updateLog("\t isAudioAvailable() == false");
    QCOMPARE(audioAvailableSpy.count(), 1);
    arguments = audioAvailableSpy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == false);
    updateLog("\t audioAvailableChanged(false)");
    QCOMPARE(videoAvailableSpy.count(), 1);
    arguments = videoAvailableSpy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);
    updateLog("\t videoAvailableChanged(true)");

    setAudioVideoContent();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    updateLog("SetMedia: audioVideoContent");
    WAIT_FOR_CONDITION(m_player->isAudioAvailable(), true);
    updateLog("\t isAudioAvailable() == true");
    QCOMPARE(audioAvailableSpy.count(), 1);
    arguments = audioAvailableSpy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);
    updateLog("\t audioAvailableChanged(true)");
    QCOMPARE(videoAvailableSpy.count(), 0);

    updateLog("*****testVideoAndAudioAvailability: PASSED");

}

void tst_QMediaPlayer_xa::testStreamInformation()
{
    updateLog("*****testStreamInformation");
    resetPlayer();
    QMediaStreamsControl* m_streamControl = (QMediaStreamsControl*)(m_player->service()->requestControl(QMediaStreamsControl_iid));
    setVideoOnlyContent();

    if(m_streamControl)
    {
        {
            QSignalSpy streamInfoSpy(m_streamControl, SIGNAL(streamsChanged()));
            setAudioOnlyContent();
            WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
            QVERIFY(m_streamControl->streamCount() == 1);
            QCOMPARE(streamInfoSpy.count(), 1);
        }

        {
            QSignalSpy streamInfoSpy(m_streamControl, SIGNAL(streamsChanged()));
            setAudioVideoContent();
            WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
            QVERIFY(m_streamControl->streamCount() == 2);
            QCOMPARE(streamInfoSpy.count(), 1);
        }

        {
            QSignalSpy streamInfoSpy(m_streamControl, SIGNAL(streamsChanged()));
            setAudioVideoContent(); //set alternate content
            WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
            QVERIFY(m_streamControl->streamCount() == 2);
            QCOMPARE(streamInfoSpy.count(), 1);
        }

        {
            QSignalSpy streamInfoSpy(m_streamControl, SIGNAL(streamsChanged()));
            setVideoOnlyContent();
            WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
            QVERIFY(m_streamControl->streamCount() == 1);
            QCOMPARE(streamInfoSpy.count(), 1);
        }

        updateLog("*****testStreamInformation: PASSED");
    }
}

void tst_QMediaPlayer_xa::testPlay()
{
    updateLog("*****testPlay");
    resetPlayer();

    setAudioVideoContent();

    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(m_player->media() == *mediaContent);
    QSignalSpy spy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
    m_player->play();

    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);

    //Play->Play
    {
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->play();
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QCOMPARE(stateSpy.count(), 0);
    }

    //Play->Pause
    {
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->pause();
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QCOMPARE(stateSpy.count(), 1);
    }

    //Play->Stop
    {
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(stateSpy.count(), 1);
    }

    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

    updateLog("*****testPlay: PASSED");
}

void tst_QMediaPlayer_xa::testPause()
{
    updateLog("*****testPause");
    resetPlayer();

    setAudioVideoContent();

    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(m_player->media() == *mediaContent);
    QSignalSpy spy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
    m_player->pause();
    // at present there is no support for stop->pause state transition. TODO: uncomment when support added
    //QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
    //QCOMPARE(spy.count(), 1);

    //Pause->Play
    {
        //QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->play();
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QCOMPARE(stateSpy.count(), 1);
    }

    //Pause->Pause
    {
        m_player->pause();
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->pause();
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QCOMPARE(stateSpy.count(), 0);
    }

    //Pause->Stop
    {
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(stateSpy.count(), 1);
    }

    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

    updateLog("*****testPause: PASSED");

}

void tst_QMediaPlayer_xa::testStop()
{
    updateLog("*****testStop");
    resetPlayer();
    setAudioVideoContent();

    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(m_player->media() == *mediaContent);

    QSignalSpy spy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
    m_player->stop();

    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
    QCOMPARE(spy.count(), 0);

    //Stop->Play
    {
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->play();
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QCOMPARE(stateSpy.count(), 1);
    }
    // at present there is no support for stop->pause state transition. TODO: uncomment when support added
    //Stop->Pause
/*    {
        m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->pause();
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QCOMPARE(stateSpy.count(), 1);
    }
*/
    //Stop->Stop
    {
        m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(stateSpy.count(), 0);
    }

    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

    updateLog("*****testStop: PASSED");
}

void tst_QMediaPlayer_xa::testMediaStatus()
{
    updateLog("*****testMediaStatus");
    resetPlayer();

    QSignalSpy statusSpy(m_player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));

    setAudioVideoContent();

    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(statusSpy.count()>0);

    {
        QSignalSpy statusSpy(m_player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QVERIFY(statusSpy.count()>0);
    }

    {
        QSignalSpy statusSpy(m_player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
        m_player->setPosition(duration - 10);
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);
        QVERIFY(statusSpy.count()>0);
    }

    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

    updateLog("*****testMediaStatus: PASSED");
}

void tst_QMediaPlayer_xa::testBufferStatus()
{
    updateLog("*****testBufferStatus");
    resetPlayer();
    m_player->setNotifyInterval(50); //Since default interval is 1 sec,could not receive any bufferStatusChanged SIGNAL,hence checking for 50milliseconds
    QSignalSpy spy(m_player, SIGNAL(bufferStatusChanged(int)));
  //    setStreamingContent();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    m_player->play();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
    QCOMPARE(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
    WAIT_FOR_CONDITION(m_player->bufferStatus(), 100);
    QVERIFY(spy.count()>0);
    updateLog("*****testBufferStatus: PASSED");
}

void tst_QMediaPlayer_xa::testPlaybackRate()
{
    updateLog("*****testPlaybackRate");

    resetPlayer();

    qreal playbackRate = 1.5;

    setAudioVideoContent();

    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);

    m_player->setPlaybackRate(playbackRate);
    QVERIFY(m_player->playbackRate() == playbackRate);

    QSignalSpy spy(m_player, SIGNAL(playbackRateChanged(qreal)));
    m_player->setPlaybackRate(playbackRate + 0.5f);
    QCOMPARE(m_player->playbackRate(), playbackRate + 0.5f);
    QCOMPARE(spy.count(), 1);

    updateLog("*****testPlaybackRate: PASSED");
}

void tst_QMediaPlayer_xa::testPlaybackRateWhilePlaying()
{
    updateLog("*****testPlaybackRateWhilePlaying");
    resetPlayer();

    qreal playbackRate = 1.5;

    setAudioVideoContent();

    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);

    m_player->play();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);

    m_player->setPlaybackRate(playbackRate);
    QVERIFY(m_player->playbackRate() == playbackRate);

    QSignalSpy spy(m_player, SIGNAL(playbackRateChanged(qreal)));
    m_player->setPlaybackRate(playbackRate + 0.5f);
    QCOMPARE(m_player->playbackRate(), playbackRate + 0.5f);
    QCOMPARE(spy.count(), 1);

    updateLog("*****testPlaybackRateWhilePlaying: PASSED");
}

void tst_QMediaPlayer_xa::testSeekable()
{
    updateLog("*****testBufferStatus");
    resetPlayer();
    QSignalSpy spy(m_player, SIGNAL(seekableChanged(bool)));
    setAudioVideoContent();
    qint64 position = 1000;
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(m_player->isSeekable()==true);
    m_player->setPosition(position);
    QCOMPARE(spy.count(), 0);
    QVERIFY(m_player->isSeekable()==true);

    updateLog("*****testBufferStatus: PASSED");

}

void tst_QMediaPlayer_xa::testAspectRatioMode()
{
    updateLog("*****testBufferStatus");
    resetPlayer();
    setAudioVideoContent();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QVideoWidgetControl* m_videoWidgetControl = (QVideoWidgetControl*)(m_player->service()->requestControl(QVideoWidgetControl_iid));

    m_player->play();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(m_videoWidgetControl->aspectRatioMode(), Qt::KeepAspectRatio); //default

    QTest::qWait(5000); //wait for 5 seconds

    {
        m_videoWidgetControl->setAspectRatioMode(Qt::IgnoreAspectRatio);
        QCOMPARE(m_videoWidgetControl->aspectRatioMode(), Qt::IgnoreAspectRatio);
        QTest::qWait(5000); //wait for 5 seconds
    }

    {
        m_videoWidgetControl->setAspectRatioMode(Qt::KeepAspectRatioByExpanding);
        QCOMPARE(m_videoWidgetControl->aspectRatioMode(), Qt::KeepAspectRatioByExpanding);
        QTest::qWait(5000); //wait for 5 seconds
    }
    {
        m_videoWidgetControl->setAspectRatioMode(Qt::KeepAspectRatio);
        QCOMPARE(m_videoWidgetControl->aspectRatioMode(), Qt::KeepAspectRatio);
        QTest::qWait(5000); //wait for 5 seconds
    }

    updateLog("*****testBufferStatus: PASSED");

}

void tst_QMediaPlayer_xa::testFullScreen()
{
    updateLog("*****testFullScreen");
    resetPlayer();
    setAudioVideoContent();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QVideoWidgetControl* m_videoWidgetControl = (QVideoWidgetControl*)(m_player->service()->requestControl(QVideoWidgetControl_iid));

    m_player->play();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);

    //m_widget->resize(50, 50);
    m_widget->showNormal();
    QTest::qWait(10000); //wait for 5 seconds


    if(m_videoWidgetControl)
    {
        QSignalSpy spy(m_videoWidgetControl, SIGNAL(fullScreenChanged(bool)));
        m_videoWidgetControl->setFullScreen(true);
        QTest::qWait(10000); //wait for 5 seconds
        QCOMPARE(m_videoWidgetControl->isFullScreen(), true);
        QCOMPARE(spy.count(), 1);
    }


    if(m_videoWidgetControl)
    {
        QSignalSpy spy(m_videoWidgetControl, SIGNAL(fullScreenChanged(bool)));
        m_videoWidgetControl->setFullScreen(false);
        QCOMPARE(m_videoWidgetControl->isFullScreen(), false);
        QTest::qWait(10000); //wait for 5 seconds
        QCOMPARE(spy.count(), 1);
    }

    updateLog("*****testFullScreen: PASSED");
}

void tst_QMediaPlayer_xa::testWindowControl_NativeSize()
{
    updateLog("*****testWindowControl_NativeSize");
    resetPlayer_WindowControl();
    setAudioVideoContent();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(m_player->media() == *mediaContent);
    m_player->play();
    WAIT_LONG_FOR_CONDITION(m_player->state(), QMediaPlayer::PlayingState);
    QVideoWindowControl* windowControl = (QVideoWindowControl*)(m_player->service()->requestControl(QVideoWindowControl_iid));
    if (windowControl)
    {
        QSize size = windowControl->nativeSize();
    }

    WAIT_LONG_FOR_CONDITION(m_player->state(), QMediaPlayer::StoppedState);
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

    updateLog("*****testWindowControl_NativeSize: PASSED");
}

void tst_QMediaPlayer_xa::testWindowControl_AspectRatioMode()
{
    updateLog("*****testWindowControl_AspectRatioMode");
    resetPlayer_WindowControl();
    setAudioVideoContent();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QVideoWindowControl* windowControl = (QVideoWindowControl*)(m_player->service()->requestControl(QVideoWindowControl_iid));

    m_player->play();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(windowControl->aspectRatioMode(), Qt::KeepAspectRatio); //default

    QTest::qWait(5000); //wait for 5 seconds

    {
        windowControl->setAspectRatioMode(Qt::IgnoreAspectRatio);
        QCOMPARE(windowControl->aspectRatioMode(), Qt::IgnoreAspectRatio);
        QTest::qWait(5000); //wait for 5 seconds
    }

    {
        windowControl->setAspectRatioMode(Qt::KeepAspectRatioByExpanding);
        QCOMPARE(windowControl->aspectRatioMode(), Qt::KeepAspectRatioByExpanding);
        QTest::qWait(5000); //wait for 5 seconds
    }
    {
        windowControl->setAspectRatioMode(Qt::KeepAspectRatio);
        QCOMPARE(windowControl->aspectRatioMode(), Qt::KeepAspectRatio);
        QTest::qWait(5000); //wait for 5 seconds
    }

    updateLog("*****testWindowControl_AspectRatioMode: PASSED");

}

void tst_QMediaPlayer_xa::testWindowControl_FullScreen()
{
    updateLog("*****testWindowControl_FullScreen");
    resetPlayer_WindowControl();
    setAudioVideoContent();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QVideoWindowControl* windowControl = (QVideoWindowControl*)(m_player->service()->requestControl(QVideoWindowControl_iid));

    m_player->play();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);

    //m_windowWidget->resize(250, 350);
    m_windowWidget->showNormal();
    QTest::qWait(10000); //wait for 5 seconds

    if(windowControl)
    {
        QSignalSpy spy(windowControl, SIGNAL(fullScreenChanged(bool)));
        windowControl->setFullScreen(true);
        QTest::qWait(10000); //wait for 5 seconds
        QCOMPARE(windowControl->isFullScreen(), true);
        QCOMPARE(spy.count(), 1);
    }


    if(windowControl)
    {
        QSignalSpy spy(windowControl, SIGNAL(fullScreenChanged(bool)));
        windowControl->setFullScreen(false);
        QCOMPARE(windowControl->isFullScreen(), false);
        QTest::qWait(10000); //wait for 5 seconds
        QCOMPARE(spy.count(), 1);
    }

    updateLog("*****testWindowControl_FullScreen: PASSED");
}


//adding access-point testcase


void tst_QMediaPlayer_xa::testSetconfigurationsAP()
{
    updateLog("*****testSetconfigurationsAP");
    resetPlayer();

    //Passing only valid Accesspoint in QList
    QList<QNetworkConfiguration> configs;
    accesspointlist = manager.allConfigurations();
    for (int i=0; i<=accesspointlist.size()-1;i++)
    qDebug()<<"accesspointlist"<< accesspointlist.at(i).name();
    configs<<accesspointlist.at(8);
    QSignalSpy spy(m_player, SIGNAL(networkConfigurationChanged(const QNetworkConfiguration&)));
    m_player->setNetworkConfigurations(configs);
    setStreamingContent3gp();
    m_player->play();
    QTest::qWait(100000);
    WAIT_LONG_FOR_CONDITION(m_player->state(), QMediaPlayer::PlayingState);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
    QCOMPARE((m_player->currentNetworkConfiguration().name()), accesspointlist.at(8).name());
    updateLog("*****testSetconfigurationsAP: PASSED");

}


void tst_QMediaPlayer_xa::testSetAccesspoint()
{
    updateLog("*****testSetAccesspoint");
    resetPlayer();
    QList<QNetworkConfiguration> configs;
    accesspointlist = manager.allConfigurations();
    configs<<accesspointlist.at(0);
    configs<<accesspointlist.at(3);
    configs<<accesspointlist.at(10);
    configs<<accesspointlist.at(8);
    //Passing only valid Accesspoint in QList
    QSignalSpy spy(m_player, SIGNAL(networkConfigurationChanged(const QNetworkConfiguration&)));
    m_player->setNetworkConfigurations(configs);

    setStreamingContent3gp();
    QTest::qWait(200000);
    m_player->play();
    QTest::qWait(10000);
    WAIT_LONG_FOR_CONDITION(m_player->state(), QMediaPlayer::PlayingState);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
    QCOMPARE((m_player->currentNetworkConfiguration().name()), accesspointlist.at(8).name());

    updateLog("*****testSetAccesspoint: PASSED");
}


void tst_QMediaPlayer_xa::testGetAccesspoint()
{
    updateLog("*****testGetAccesspoint");
    resetPlayer();
    //getting information about the current configured accesspoint without setting any configurations
    QNetworkConfiguration getaccespoint;
    getaccespoint = m_player->currentNetworkConfiguration();
    QCOMPARE(getaccespoint.name(),QString(""));
    updateLog("*****testGetAccesspoint:  ");
}


void tst_QMediaPlayer_xa::testDiffmediacontentAP()
{
    updateLog("*****streaming Different mediacontent files via AP");
    resetPlayer();
    QList<QNetworkConfiguration> configs;
    accesspointlist = manager.allConfigurations();
    configs<<accesspointlist.at(8);
    QSignalSpy spy(m_player, SIGNAL(networkConfigurationChanged(const QNetworkConfiguration&)));
    m_player->setNetworkConfigurations(configs);
    //first mediacontent file
    setAudioStreamingContent();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    m_player->play();
    QTest::qWait(30000);
    QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);
    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

    //second mediacontent file
    setStreamingContent3gp();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    m_player->play();
    QTest::qWait(20000);
    QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);
    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

    QCOMPARE((m_player->currentNetworkConfiguration().name()), accesspointlist.at(8).name());
    QNetworkConfiguration getaccespoint;
    getaccespoint = m_player->currentNetworkConfiguration();
    QCOMPARE(getaccespoint.name(), QString("MMMW"));

    updateLog("*****testDiffmediacontentAP: PASSED");
}


void tst_QMediaPlayer_xa::testInvalidaddressAP()
{
    updateLog("*****testInvalidaddressAP");
    resetPlayer();
    //setting all invalid accesspoint
    QList<QNetworkConfiguration> configs;
    accesspointlist = manager.allConfigurations();
    configs<<accesspointlist.at(0);
    configs<<accesspointlist.at(2);
    configs<<accesspointlist.at(3);
    QSignalSpy spy(m_player, SIGNAL(networkConfigurationChanged(const QNetworkConfiguration&)));
    m_player->setNetworkConfigurations(configs);
    QNetworkConfiguration getaccespoint;
    getaccespoint = m_player->currentNetworkConfiguration();
    QCOMPARE(getaccespoint.name(), QString(""));

    updateLog("*****testInvalidaddressAP: PASSED");
}



void tst_QMediaPlayer_xa::testMultipleAccesspoints()
{
    updateLog("*****testMultipleAccesspoints");
    resetPlayer();
    QList<QNetworkConfiguration> configs;
    accesspointlist = manager.allConfigurations();
    configs<<accesspointlist.at(8);
    QSignalSpy spy(m_player, SIGNAL(networkConfigurationChanged(const QNetworkConfiguration&)));
    m_player->setNetworkConfigurations(configs);
    setStreamingContent3gp();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    m_player->play();
    WAIT_LONG_FOR_CONDITION(m_player->state(), QMediaPlayer::PlayingState);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
    QTest::qWait(20000);
    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
    QCOMPARE((m_player->currentNetworkConfiguration().name()), accesspointlist.at(8).name());
    //Second configuration list
    QList<QNetworkConfiguration> secconfigs;
    secaccesspoint = manager.allConfigurations();
    secconfigs<<secaccesspoint.at(5);
    QSignalSpy spy1(m_player, SIGNAL(networkConfigurationChanged(const QNetworkConfiguration&)));
    m_player->setNetworkConfigurations(secconfigs);
    setStreamingContent3gp();
    // setAudioStreamingContent();
    QTest::qWait(30000);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    m_player->play();
    WAIT_LONG_FOR_CONDITION(m_player->state(), QMediaPlayer::PlayingState);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
    QTest::qWait(10000);
    updateLog("*****testSetAccesspoint: PASSED");
    QCOMPARE((m_player->currentNetworkConfiguration().name()), accesspointlist.at(5).name());
    QNetworkConfiguration getaccespoint;
    getaccespoint = m_player->currentNetworkConfiguration();
    QCOMPARE(getaccespoint.name(), QString("Mobile Office"));

    updateLog("*****testMultipleAccesspoints: PASSED");
}


void tst_QMediaPlayer_xa::testReconnectAPWhilestreaming()
{
    updateLog("*****testReconnectAPWhilestreaming");
    resetPlayer();
    QList<QNetworkConfiguration> configs;
    accesspointlist = manager.allConfigurations();
    configs<<accesspointlist.at(15);
    configs<<accesspointlist.at(12);
    configs<<accesspointlist.at(0);
    configs<<accesspointlist.at(8);
    QSignalSpy spy(m_player, SIGNAL(networkConfigurationChanged(const QNetworkConfiguration&)));
    m_player->setNetworkConfigurations(configs);
    setAudioStreamingContent();
    m_player->play();
    QTest::qWait(200000);
    configs<<accesspointlist.at(5);
    m_player->setNetworkConfigurations(configs);
    m_player->play();
    QTest::qWait(20000);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE((m_player->currentNetworkConfiguration().name()), accesspointlist.at(8).name());
    QNetworkConfiguration getaccespoint;
    getaccespoint = m_player->currentNetworkConfiguration();
    QCOMPARE(getaccespoint.name(), QString("MMMW"));
    updateLog("*****testReconnectAPWhilestreaming: PASSED");
}


void tst_QMediaPlayer_xa::teststreampausestream()
{
    updateLog("*****teststreampausestream");
    resetPlayer();
    QList<QNetworkConfiguration> configs;
    accesspointlist = manager.allConfigurations();
    configs<<accesspointlist.at(8);
    QSignalSpy spy(m_player, SIGNAL(networkConfigurationChanged(const QNetworkConfiguration&)));
    m_player->setNetworkConfigurations(configs);
    setStreamingContent3gp();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    m_player->play();
    WAIT_LONG_FOR_CONDITION(m_player->state(), QMediaPlayer::PlayingState);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
    QTest::qWait(10000);
    m_player->pause();
    WAIT_LONG_FOR_CONDITION(m_player->state(), QMediaPlayer::PausedState);

    //Setting up the accesspoint when the player is in paused state
    QList<QNetworkConfiguration> secconfigs;
    secaccesspoint = manager.allConfigurations();
    secconfigs<<secaccesspoint.at(5);
    m_player->setNetworkConfigurations(secconfigs);
    m_player->play();
    QTest::qWait(20000);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
    QCOMPARE((m_player->currentNetworkConfiguration()).name(), accesspointlist.at(8).name());
    m_player->stop();
    WAIT_LONG_FOR_CONDITION(m_player->state(), QMediaPlayer::StoppedState);
    setStreamingContent3gp();
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    m_player->play();
    QTest::qWait(20000);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
    m_player->stop();
    WAIT_LONG_FOR_CONDITION(m_player->state(), QMediaPlayer::StoppedState);
    QCOMPARE((m_player->currentNetworkConfiguration().name()), accesspointlist.at(5).name());

    updateLog("*****teststreampausestream: PASSED");
}



void tst_QMediaPlayer_xa::testStressAccessPoint()
{
    updateLog("*****testStressAccessPoint");
    resetPlayer();
    for (int i=0; i<=50; i++) {
        QList<QNetworkConfiguration> configs;
        accesspointlist = manager.allConfigurations();
        configs<<accesspointlist.at(8);
        QSignalSpy spy(m_player, SIGNAL(networkConfigurationChanged(const QNetworkConfiguration&)));
        m_player->setNetworkConfigurations(configs);
        setStreamingContent3gp();
        m_player->play();
        QTest::qWait(20000);
        WAIT_LONG_FOR_CONDITION(m_player->state(), QMediaPlayer::PlayingState);
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE((m_player->currentNetworkConfiguration().name()), accesspointlist.at(8).name());
        resetPlayer();
    }

    updateLog("*****testStressAccessPoint: PASSED");
}



void tst_QMediaPlayer_xa::updateLog(QString log, bool delFile)
{
        QString logFileName = "C:\\data\\" + QFileInfo(QCoreApplication::applicationFilePath()).baseName() + "_detailed.log";
        if(delFile)
        {
            QFile::remove(logFileName);
        }

        QFile f(logFileName);

        if( !f.open( QIODevice::WriteOnly | QIODevice::Append ) )
    {
        return;
    }

    QTextStream ts( &f );

    ts<<log<<"\n";

    f.close();
}
