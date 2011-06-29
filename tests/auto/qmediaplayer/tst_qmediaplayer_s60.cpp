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

//TESTED_COMPONENT=src/multimedia

#include "tst_qmediaplayer_s60.h"
#include <qmediastreamscontrol.h>

static const QString TestFilePath = QLatin1String("c:/data/testfiles/");

// Comment out the following line if the test device does not have network
// access.
#define TEST_DEVICE_HAS_NETWORK_ACCESS

QT_USE_NAMESPACE

void tst_QMediaPlayer_s60::initTestCase_data()
{
#ifdef __WINSCW__
    initTestCase_data_default_winscw();
#else
    initTestCase_data_default_armv5();
#endif
}

void tst_QMediaPlayer_s60::initTestCase_data_default_armv5()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<bool>("streamable");
    QTest::addColumn<QMediaPlayer::State>("state");
    QTest::addColumn<QMediaPlayer::MediaStatus>("status");
    QTest::addColumn<QMediaContent>("mediaContent");
    QTest::addColumn<qint64>("duration");
    QTest::addColumn<qint64>("position");
    QTest::addColumn<bool>("seekable");
    QTest::addColumn<bool>("seekableWhilePlaying");
    QTest::addColumn<int>("volume");
    QTest::addColumn<bool>("videoAvailable");
    QTest::addColumn<bool>("audioAvailable");
    QTest::addColumn<qreal>("playbackRate");
    QTest::addColumn<QMediaPlayer::Error>("error");
    QTest::addColumn<QString>("errorString");

    QTest::newRow("TestDataNull")
    << false //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::NoMedia // status
    << QMediaContent() // mediaContent
    << qint64(-1) // duration
    << qint64(0) // position
    << false // seekable
    << false // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << false //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_3gp.3gp")
    << true //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status LoadedMedia
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_3gp.3gp")) // mediaContent
    << qint64(46860) // duration
    << qint64(23430) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << true //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_mp4.mp4")
    << true //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_mp4.mp4")) // mediaContent
    << qint64(2701) // duration
    << qint64(1351) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << true //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_wmv9.wmv")
    << true //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_wmv9.wmv")) // mediaContent
    << qint64(169389) // duration
    << qint64(84695) // position (duration/2)
    << true // seekable
    << false // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << true //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_AAC.dat")
    << false //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::InvalidMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_AAC.dat")) // mediaContent
    << qint64(-1) // duration
    << qint64(0) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << false //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::ResourceError // error
    << QString(); // errorString

    QTest::newRow("test_amr.amr")
    << true // valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_amr.amr")) // mediaContent
    << qint64(14400) // duration
    << qint64(7200) // position
    << true // seekable
    << false // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << true // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_mp3.mp3")
    << true // valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_mp3.mp3")) // mediaContent
    << qint64(102044) //qint64(102044)  duration
    << qint64(51022) // position
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << true // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

#ifdef TEST_DEVICE_HAS_NETWORK_ACCESS
    QTest::newRow("test_stream 3gp")
    << false // valid
    << true //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl("rtsp://v3.cache4.c.youtube.com/CkgLENy73wIaPwlU2rm7yu8PFhMYESARFEIJbXYtZ29vZ2xlSARSB3JlbGF0ZWRaDkNsaWNrVGh1bWJuYWlsYPi6_IXT2rvpSgw=/0/0/0/video.3gp")) // mediaContent
    << qint64(103000) // duration
    << qint64(0) // position
    << false // seekable
    << false // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << true // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("RTSP link to H.263/AMR")
    << false // valid
    << true //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl("rtsp://v7.cache8.c.youtube.com/CiILENy73wIaGQnNUH6lxLd0MRMYDSANFEgGUgZ2aWRlb3MM/0/0/0/video.3gp")) // mediaContent
    << qint64(249000) // duration
    << qint64(0) // position
    << false // seekable
    << false // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << true // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("RTSP link to MPEG-4/AAC")
    << false // valid
    << true //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl("rtsp://v8.cache1.c.youtube.com/CiILENy73wIaGQnNUH6lxLd0MRMYESARFEgGUgZ2aWRlb3MM/0/0/0/video.3gp")) // mediaContent
    << qint64(249000) // duration
    << qint64(0) // position
    << false // seekable
    << false // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << true // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString
#endif // TEST_DEVICE_HAS_NETWORK_ACCESS
}

void tst_QMediaPlayer_s60::initTestCase_data_default_winscw()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<bool>("streamable");
    QTest::addColumn<QMediaPlayer::State>("state");
    QTest::addColumn<QMediaPlayer::MediaStatus>("status");
    QTest::addColumn<QMediaContent>("mediaContent");
    QTest::addColumn<qint64>("duration");
    QTest::addColumn<qint64>("position");
    QTest::addColumn<bool>("seekable");
    QTest::addColumn<bool>("seekableWhilePlaying");
    QTest::addColumn<int>("volume");
    QTest::addColumn<bool>("videoAvailable");
    QTest::addColumn<bool>("audioAvailable");
    QTest::addColumn<qreal>("playbackRate");
    QTest::addColumn<QMediaPlayer::Error>("error");
    QTest::addColumn<QString>("errorString");

    QTest::newRow("TestDataNull")
    << false //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::NoMedia // status
    << QMediaContent() // mediaContent
    << qint64(-1) // duration
    << qint64(0) // position
    << false // seekable
    << false // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << false //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_3GPP.dat")
    << true //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_3GPP.dat")) // mediaContent
    << qint64(7200) // duration
    << qint64(3600) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << true //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_3gp.3gp")
    << true //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_3gp.3gp")) // mediaContent
    << qint64(46860) // duration
    << qint64(23430) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << true //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_mp4.mp4")
    << true //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_mp4.mp4")) // mediaContent
    << qint64(2701) // duration
    << qint64(1351) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << false //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::FormatError // error
    << QString(); // errorString

    QTest::newRow("test_MP4.dat")
    << false //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::InvalidMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_MP4.dat")) // mediaContent
    << qint64(-1) // duration
    << qint64(0) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << false //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::AccessDeniedError// error
    << QString(); // errorString

    QTest::newRow("test_wmv9.wmv")
    << true //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_wmv9.wmv")) // mediaContent
    << qint64(169389) // duration
    << qint64(84695) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << false //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::FormatError // error
    << QString(); // errorString

    QTest::newRow("test_h264_qcif.264")
    << false //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::InvalidMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_h264_qcif.264")) // mediaContent
    << qint64(-1) // duration
    << qint64(0) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << false //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::ResourceError // error
    << QString(); // errorString

    QTest::newRow("test_RM.dat")
    << true //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_RM.dat")) // mediaContent
    << qint64(20245) // duration
    << qint64(10123) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << true //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_SWF.dat")
    << false //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::InvalidMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_SWF.dat")) // mediaContent
    << qint64(-1) // duration
    << qint64(0) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << false //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::ResourceError // error
    << QString(); // errorString

    QTest::newRow("test_WMV.dat")
    << true //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_WMV.dat")) // mediaContent
    << qint64(3098) // duration
    << qint64(1549) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying, on emulator codec leaks memory and causes alloc panic on dtor
    << 50 // volume
    << true // videoAvailable
    << false //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::FormatError // error
    << QString(); // errorString

    QTest::newRow("test_WMA.dat")
    << false //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::InvalidMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_WMA.dat")) // mediaContent
    << qint64(-1) // duration
    << qint64(0) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << false //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::AccessDeniedError // error
    << QString(); // errorString

    QTest::newRow("test_flash_video.flv")
    << false // valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::InvalidMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_flash_video.flv")) // mediaContent
    << qint64(-1) // duration
    << qint64(0) // position
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << false //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::ResourceError // error
    << QString(); // errorString

    QTest::newRow("test_MXMF.dat")
    << true //valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_MXMF.dat")) // mediaContent
    << qint64(31980) // duration
    << qint64(15990) // position (duration/2)
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << true //audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_amr.amr")
    << true // valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_amr.amr")) // mediaContent
    << qint64(14402) // duration
    << qint64(7200) // position
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << true // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_AMR.dat")
    << true // valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_AMR.dat")) // mediaContent
    << qint64(38509) // duration
    << qint64(19255) // position
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << true // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_mp3.mp3")
    << false // valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_mp3.mp3")) // mediaContent
    << qint64(-1) //qint64(102044)  duration
    << qint64(0) // position
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << false // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::ResourceError // error
    << QString(); // errorString

    QTest::newRow("test_MP3.dat")
    << false // valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::InvalidMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_MP3.dat")) // mediaContent
    << qint64(-1) //qint64(102044)  duration
    << qint64(0) // position
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << false // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::ResourceError // error
    << QString(); // errorString

    QTest::newRow("test_MIDI.dat")
    << true // valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_MIDI.dat")) // mediaContent
    << qint64(32782) // duration
    << qint64(16391) // position
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << true // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

    QTest::newRow("test_ACC.dat")
    << false // valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::InvalidMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_ACC.dat")) // mediaContent
    << qint64(-1) // duration
    << qint64(0) // position
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << false // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::ResourceError // error
    << QString(); // errorString

    QTest::newRow("test_WAV.dat")
    << true // valid
    << false //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl::fromLocalFile(TestFilePath + "test_WAV.dat")) // mediaContent
    << qint64(2864) // duration
    << qint64(1432) // position
    << true // seekable
    << true // seekableWhilePlaying
    << 50 // volume
    << false // videoAvailable
    << true // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::NoError // error
    << QString(); // errorString

#ifdef TEST_DEVICE_HAS_NETWORK_ACCESS
    QTest::newRow("test_stream")
    << false // valid
    << true //streamable
    << QMediaPlayer::StoppedState // state
    << QMediaPlayer::LoadedMedia // status
    << QMediaContent(QUrl("rtsp://v3.cache4.c.youtube.com/CkgLENy73wIaPwlU2rm7yu8PFhMYESARFEIJbXYtZ29vZ2xlSARSB3JlbGF0ZWRaDkNsaWNrVGh1bWJuYWlsYPi6_IXT2rvpSgw=/0/0/0/video.3gp")) // mediaContent
    << qint64(-1) // duration
    << qint64(0) // position
    << false // seekable
    << false // seekableWhilePlaying
    << 50 // volume
    << true // videoAvailable
    << false // audioAvailable
    << qreal(0) // playbackRate
    << QMediaPlayer::AccessDeniedError // error
    << QString(); // errorString
#endif
}

void tst_QMediaPlayer_s60::initTestCase()
{
    m_player = new QMediaPlayer();

    // Symbian back end needs coecontrol for creation.
    m_widget = new QVideoWidget();
    m_player->setVideoOutput(m_widget);
    m_widget->show();
    runonce = false;
}

void tst_QMediaPlayer_s60::cleanupTestCase()
{
    delete m_player;
    delete m_widget;
}

void tst_QMediaPlayer_s60::init()
{
	qRegisterMetaType<QMediaPlayer::State>("QMediaPlayer::State");
	qRegisterMetaType<QMediaPlayer::Error>("QMediaPlayer::Error");
	qRegisterMetaType<QMediaPlayer::MediaStatus>("QMediaPlayer::MediaStatus");
	qRegisterMetaType<QMediaContent>("QMediaContent");
}

void tst_QMediaPlayer_s60::cleanup()
{
}

void tst_QMediaPlayer_s60::testNullService()
{
    if(runonce)
        return;
    MockProvider_s60 provider(0);
    QMediaPlayer player(0, 0, &provider);

    const QIODevice *nullDevice = 0;

    QCOMPARE(player.media(), QMediaContent());
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

    {
        QFETCH_GLOBAL(QMediaContent, mediaContent);

        QSignalSpy spy(&player, SIGNAL(mediaChanged(QMediaContent)));
        QFile file;

        player.setMedia(mediaContent, &file);
        QCOMPARE(player.media(), QMediaContent());
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
        QFETCH_GLOBAL(int, volume);

        QSignalSpy volumeSpy(&player, SIGNAL(volumeChanged(int)));
        QSignalSpy mutingSpy(&player, SIGNAL(mutedChanged(bool)));

        player.setVolume(volume);
        QCOMPARE(player.volume(), 0);
        QCOMPARE(volumeSpy.count(), 0);

        player.setMuted(false);
        QCOMPARE(player.isMuted(), false);
        QCOMPARE(mutingSpy.count(), 0);
    } {
        QFETCH_GLOBAL(qint64, position);

        QSignalSpy spy(&player, SIGNAL(positionChanged(qint64)));

        player.setPosition(position);
        QCOMPARE(player.position(), qint64(0));
        QCOMPARE(spy.count(), 0);
    } {
        QFETCH_GLOBAL(qreal, playbackRate);

        QSignalSpy spy(&player, SIGNAL(playbackRateChanged(qreal)));

        player.setPlaybackRate(playbackRate);
        QCOMPARE(player.playbackRate(), qreal(0));
        QCOMPARE(spy.count(), 0);
    } {
        QMediaPlaylist playlist;
        player.setPlaylist(&playlist);

        QSignalSpy mediaSpy(&player, SIGNAL(mediaChanged(QMediaContent)));
        QSignalSpy statusSpy(&player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));

        playlist.addMedia(QUrl("http://example.com/stream"));
        playlist.addMedia(QUrl("file:///some.mp3"));

        playlist.setCurrentIndex(0);
        QCOMPARE(playlist.currentIndex(), 0);
        QCOMPARE(player.media(), QMediaContent());
        QCOMPARE(mediaSpy.count(), 0);
        QCOMPARE(statusSpy.count(), 0);

        playlist.next();
        QCOMPARE(playlist.currentIndex(), 1);
        QCOMPARE(player.media(), QMediaContent());
        QCOMPARE(mediaSpy.count(), 0);
        QCOMPARE(statusSpy.count(), 0);
    }
    runonce = true;
}

void tst_QMediaPlayer_s60::testMedia()
{
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    m_player->setMedia(mediaContent);
    QTest::qWait(500);
    QCOMPARE(m_player->media(), mediaContent);
}


void tst_QMediaPlayer_s60::testDuration()
{
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(qint64, duration);
    QFETCH_GLOBAL(bool, valid);

    QSignalSpy spy(m_player, SIGNAL(durationChanged(qint64)));
    m_player->setMedia(mediaContent);

    if(valid) {
        WAIT_FOR_CONDITION(spy.count(), 1);
    } else {
        WAIT_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia));
    }

    QVERIFY(m_player->duration() == duration);
}

void tst_QMediaPlayer_s60::testPosition()
{
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(qint64, duration);
    QFETCH_GLOBAL(bool, valid);
    QFETCH_GLOBAL(bool, seekable);
    QFETCH_GLOBAL(qint64, position);
    QFETCH_GLOBAL(QMediaPlayer::MediaStatus, status);

    m_player->setMedia(mediaContent);

    if (valid) {
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

        QCOMPARE(m_player->isSeekable(), seekable);
        //QCOMPARE(m_player->mediaStatus(), status);
        QVERIFY(m_player->mediaStatus() == status);

        // preset position
        { QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->setPosition(position);
        QCOMPARE(m_player->position(), position);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toLongLong(), position);}

        // same pos second time
        { QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->setPosition(position);
        QCOMPARE(m_player->position(), position);
        QCOMPARE(spy.count(), 0);}

        //zero pos
        { QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->setPosition(0);
        QCOMPARE(m_player->position(), qint64(0));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toLongLong(),  qint64(0));}

        //end pos
        { QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->setPosition(duration);
        QCOMPARE(m_player->position(), duration);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toLongLong(),  duration);}

        //negative pos
        { QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->setPosition(qint64(-1));
        QCOMPARE(m_player->position(), qint64(0));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toLongLong(),  qint64(0));}

        //over duration
        { QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->setPosition(duration+1);
        QCOMPARE(m_player->position(), duration);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toLongLong(),  duration);}

    } else {
        if (valid)
            WAIT_LONG_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia)
        else
            WAIT_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia))
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(m_player->isSeekable(), seekable);
        QCOMPARE(m_player->mediaStatus(), status);
    }
}

void tst_QMediaPlayer_s60::testPositionWhilePlaying()
{
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(qint64, duration);
    QFETCH_GLOBAL(bool, seekableWhilePlaying);
    QFETCH_GLOBAL(bool, seekable);
    QFETCH_GLOBAL(qint64, position);
    QFETCH_GLOBAL(bool, valid);
    QFETCH_GLOBAL(bool, streamable);
    QFETCH_GLOBAL(QMediaPlayer::MediaStatus, status);

    m_player->setMedia(mediaContent);

    if (streamable) {
        WAIT_LONG_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia || QMediaPlayer::LoadedMedia));
    } else {
        WAIT_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia || QMediaPlayer::LoadedMedia));
    }

    //qDebug()<<"";
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
    QCOMPARE(m_player->isSeekable(), seekable);
    QCOMPARE(m_player->mediaStatus(), status);

    if (seekableWhilePlaying && valid) {
        QCOMPARE(m_player->isSeekable(), seekableWhilePlaying);

        // preset position
        //qDebug()<<"preset";
        { QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        m_player->setPosition(position);
        WAIT_FOR_CONDITION(spy.count(), 2);
        QVERIFY(m_player->mediaStatus() == QMediaPlayer::BufferingMedia ||
                m_player->mediaStatus() == QMediaPlayer::BufferedMedia ||
                m_player->mediaStatus() == QMediaPlayer::EndOfMedia ||
                m_player->mediaStatus() == QMediaPlayer::LoadedMedia);

        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QVERIFY(m_player->position()>=position);
        QVERIFY(spy.count()!=0);}

        //reset position
        m_player->stop();
        m_player->setPosition(position);
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

        //zero pos
        //qDebug()<<"zero";
        { QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        m_player->setPosition(0);
        WAIT_FOR_CONDITION(spy.count(), 2);
        QVERIFY(m_player->mediaStatus() == QMediaPlayer::BufferingMedia ||
                m_player->mediaStatus() == QMediaPlayer::BufferedMedia ||
                m_player->mediaStatus() == QMediaPlayer::EndOfMedia ||
                m_player->mediaStatus() == QMediaPlayer::LoadedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QVERIFY(m_player->position() >= qint64(0));
        QVERIFY(spy.count()!=0);}

        //reset position
        m_player->stop();
        m_player->setPosition(position);
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

        //end pos
        //qDebug()<<"dur";
        { QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        m_player->setPosition(duration);
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(m_player->position(), qint64(0));
        QVERIFY(spy.count()!=0);}

        //reset position
        m_player->stop();
        m_player->setPosition(position);
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

        //negative pos
        //qDebug()<<"negavite";
        { QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        m_player->setPosition(qint64(-1));
        WAIT_FOR_CONDITION(spy.count(), 2);
        QVERIFY(m_player->mediaStatus() == QMediaPlayer::BufferingMedia ||
                m_player->mediaStatus() == QMediaPlayer::BufferedMedia ||
                m_player->mediaStatus() == QMediaPlayer::EndOfMedia ||
                m_player->mediaStatus() == QMediaPlayer::LoadedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QVERIFY(m_player->position() >= qint64(0));
        QVERIFY(spy.count()!=0);}

        //reset position
        m_player->stop();
        m_player->setPosition(position);
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

        //over duration
        //qDebug()<<"over";
        { QSignalSpy spy(m_player, SIGNAL(positionChanged(qint64)));
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        m_player->setPosition(duration+1);
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);
        // s60 MP3 controller has a weard feature to reduce position by ~200 when position
        // is set over the duration, therefore we skip it here
        if (!mediaContent.canonicalUrl().path().endsWith("mp3")) {
            QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
            QCOMPARE(m_player->position(), qint64(0));
            QVERIFY(spy.count()!=0);}
        }
    } else
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);

    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
    //qDebug()<<"end";
}


void tst_QMediaPlayer_s60::testVolume()
{
    QFETCH_GLOBAL(int, volume);

    // preset volume
    { QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
    m_player->setVolume(volume);
    QCOMPARE(m_player->volume(), volume);
    QCOMPARE(m_player->isMuted(), false);
    QCOMPARE(spy.count(), 1);}

    // same volume
    { QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
    int currentVolume = m_player->volume();
    m_player->setVolume(currentVolume);
    QCOMPARE(m_player->volume(), currentVolume);
    QCOMPARE(m_player->isMuted(), false);
    QCOMPARE(spy.count(), 0);}

    // zero volume
    { QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
    m_player->setVolume(0);
    QCOMPARE(m_player->volume(), 0);
    QCOMPARE(m_player->isMuted(), false);
    QCOMPARE(spy.count(), 1);}

    // max volume
    { QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
    m_player->setVolume(100);
    QCOMPARE(m_player->volume(), 100);
    QCOMPARE(m_player->isMuted(), false);
    QCOMPARE(spy.count(), 1);}

    // negative volume
    { QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
    m_player->setVolume(int(-1));
    QCOMPARE(m_player->volume(), 0);
    QCOMPARE(m_player->isMuted(), false);
    QCOMPARE(spy.count(), 1);}

    // over max volume
    { QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
    m_player->setVolume(1000);
    QCOMPARE(m_player->volume(), 100);
    QCOMPARE(m_player->isMuted(), false);
    QCOMPARE(spy.count(), 1);}
}

void tst_QMediaPlayer_s60::testVolumeWhilePlaying()
{
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(int, volume);
    QFETCH_GLOBAL(bool, valid);

    if (valid) {
        m_player->setMedia(mediaContent);
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);

        m_player->play();
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);

        // preset volume
        { QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(volume);
        QCOMPARE(m_player->volume(), volume);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);}

        // same volume
        { QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        int currentVolume = m_player->volume();
        m_player->setVolume(currentVolume);
        QCOMPARE(m_player->volume(), currentVolume);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 0);}

        // zero volume
        { QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(0);
        QCOMPARE(m_player->volume(), 0);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);}

        // max volume
        { QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(100);
        QCOMPARE(m_player->volume(), 100);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);}

        // negative volume
        { QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(int(-1));
        QCOMPARE(m_player->volume(), 0);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);}

        // over max volume
        { QSignalSpy spy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setVolume(1000);
        QCOMPARE(m_player->volume(), 100);
        QCOMPARE(m_player->isMuted(), false);
        QCOMPARE(spy.count(), 1);}

        m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
    }
}


void tst_QMediaPlayer_s60::testMuted()
{
    QFETCH_GLOBAL(int, volume);

    //reset mute & volume
    m_player->setMuted(false);
    m_player->setVolume(0);
    QVERIFY(m_player->isMuted() == false);
    QCOMPARE(m_player->volume(), 0);

    // set muted
    {QSignalSpy spy(m_player, SIGNAL(mutedChanged(bool)));
    m_player->setMuted(true);
    QCOMPARE(spy.count(), 1);
    QVERIFY(m_player->isMuted() == true);}

    // set muted again
    {QSignalSpy spy(m_player, SIGNAL(mutedChanged(bool)));
    m_player->setMuted(true);
    QCOMPARE(spy.count(), 0);
    QVERIFY(m_player->isMuted() == true);}

    // unmute
    {QSignalSpy spy(m_player, SIGNAL(mutedChanged(bool)));
    m_player->setMuted(false);
    QCOMPARE(spy.count(), 1);
    QVERIFY(m_player->isMuted() == false);}

    // set volume while muted
    {QSignalSpy muteSpy(m_player, SIGNAL(mutedChanged(bool)));
    QSignalSpy volumeSpy(m_player, SIGNAL(volumeChanged(int)));
    m_player->setMuted(true);
    m_player->setVolume(volume);
    QCOMPARE(m_player->volume(), volume);
    QCOMPARE(muteSpy.count(), 1);
    QCOMPARE(volumeSpy.count(), 1);
    QVERIFY(m_player->isMuted() == true);}
}

void tst_QMediaPlayer_s60::testMutedWhilePlaying()
{
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(int, volume);
    QFETCH_GLOBAL(bool, valid);

    if (valid) {
        m_player->setMedia(mediaContent);
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
        {QSignalSpy spy(m_player, SIGNAL(mutedChanged(bool)));
        m_player->setMuted(true);
        QCOMPARE(spy.count(), 1);
        QVERIFY(m_player->isMuted() == true);}

        // set muted again
        {QSignalSpy spy(m_player, SIGNAL(mutedChanged(bool)));
        m_player->setMuted(true);
        QCOMPARE(spy.count(), 0);
        QVERIFY(m_player->isMuted() == true);}

        // unmute
        {QSignalSpy spy(m_player, SIGNAL(mutedChanged(bool)));
        m_player->setMuted(false);
        QCOMPARE(spy.count(), 1);
        QVERIFY(m_player->isMuted() == false);}

        // set volume while muted
        {QSignalSpy muteSpy(m_player, SIGNAL(mutedChanged(bool)));
        QSignalSpy volumeSpy(m_player, SIGNAL(volumeChanged(int)));
        m_player->setMuted(true);
        m_player->setVolume(volume);
        QCOMPARE(m_player->volume(), volume);
        QCOMPARE(muteSpy.count(), 1);
        QCOMPARE(volumeSpy.count(), 1);
        QVERIFY(m_player->isMuted() == true);}

        m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
    }
}


void tst_QMediaPlayer_s60::testVideoAndAudioAvailability()
{
    QFETCH_GLOBAL(bool, videoAvailable);
    QFETCH_GLOBAL(bool, audioAvailable);
    QFETCH_GLOBAL(bool, valid);
    QFETCH_GLOBAL(QMediaContent, mediaContent);

    if(valid) {
        QSignalSpy audioAvailableSpy(m_player, SIGNAL(audioAvailableChanged(bool)));
        QSignalSpy videoAvailableSpy(m_player, SIGNAL(videoAvailableChanged(bool)));

        m_player->setMedia(mediaContent);
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);

        QVERIFY(m_player->isVideoAvailable() == videoAvailable);
        QVERIFY(m_player->isAudioAvailable() == audioAvailable);

        QCOMPARE(audioAvailableSpy.count(), 1);
        QCOMPARE(videoAvailableSpy.count(), 1);
    }
}

void tst_QMediaPlayer_s60::testError()
{
    QFETCH_GLOBAL(QMediaPlayer::Error, error);
    QFETCH_GLOBAL(bool, videoAvailable);
    QFETCH_GLOBAL(bool, audioAvailable);
    QFETCH_GLOBAL(bool, valid);
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(QMediaPlayer::State, state);
    QFETCH_GLOBAL(QMediaPlayer::MediaStatus, status);

    QSignalSpy errorSpy(m_player, SIGNAL(error(QMediaPlayer::Error)));
    m_player->setMedia(mediaContent);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia || QMediaPlayer::LoadedMedia));
    QVERIFY(m_player->mediaStatus() == status);
    QVERIFY(m_player->state() == state);
    QVERIFY(m_player->error() == error);
    if (error != QMediaPlayer::NoError) {
        QVERIFY(errorSpy.count()!=0);
    }
}

void tst_QMediaPlayer_s60::testPlay()
{
    QFETCH_GLOBAL(bool, valid);
    QFETCH_GLOBAL(bool, streamable);
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(QMediaPlayer::State, state);

    m_player->setMedia(mediaContent);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia || QMediaPlayer::LoadedMedia));
    QVERIFY(m_player->state() == state);
    QVERIFY(m_player->media() == mediaContent);
    QSignalSpy spy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
    m_player->play();

    if(valid) {
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QCOMPARE(spy.count(), state == QMediaPlayer::PlayingState ? 0 : 1);

        //Play->Play
        {QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->play();
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QCOMPARE(stateSpy.count(), 0);}

        //Play->Pause
        {QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->pause();
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QCOMPARE(stateSpy.count(), 1);}

        //Play->Stop
        {m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        if (m_player->mediaStatus() != QMediaPlayer::EndOfMedia) {
            QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
            QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
            m_player->stop();
            QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
            QCOMPARE(stateSpy.count(), 1);
        } else {
            QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
            QCOMPARE(spy.count(), 4);
        }}
    } else if (streamable) {
        m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.count(), 2);
    } else {
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.count(), 0);
    }

    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
}

void tst_QMediaPlayer_s60::testPause()
{
    QFETCH_GLOBAL(bool, valid);
    QFETCH_GLOBAL(bool, streamable);
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(QMediaPlayer::State, state);

    m_player->setMedia(mediaContent);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia || QMediaPlayer::LoadedMedia));
    QVERIFY(m_player->state() == state);
    QVERIFY(m_player->media() == mediaContent);
    QSignalSpy spy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
    m_player->pause();

    if(valid) {
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QCOMPARE(spy.count(), 1);

        //Pause->Play
        {QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->play();
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QCOMPARE(stateSpy.count(), 1);}

        //Pause->Pause
        {m_player->pause();
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->pause();
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QCOMPARE(stateSpy.count(), 0);}

        //Pause->Stop
        {QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(stateSpy.count(), 1);}
    } else if (streamable) {
        m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.count(), 2);
    } else {
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.count(), 0);
    }
}

void tst_QMediaPlayer_s60::testStop()
{
    QFETCH_GLOBAL(bool, valid);
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(QMediaPlayer::State, state);

    m_player->setMedia(mediaContent);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia || QMediaPlayer::LoadedMedia));
    QVERIFY(m_player->state() == state);
    QVERIFY(m_player->media() == mediaContent);

    QSignalSpy spy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
    m_player->stop();

    if(valid) {
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.count(), 0);

        //Stop->Play
        {QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->play();
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QCOMPARE(stateSpy.count(), 1);}

        //Stop->Pause
        {m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->pause();
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QCOMPARE(stateSpy.count(), 1);}

        //Stop->Stop
        {m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        m_player->stop();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(stateSpy.count(), 0);}
    } else {
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(spy.count(), 0);
    }
}

void tst_QMediaPlayer_s60::testMediaStatus()
{
    QFETCH_GLOBAL(bool, videoAvailable);
    QFETCH_GLOBAL(bool, valid);
    QFETCH_GLOBAL(bool, streamable);
    QFETCH_GLOBAL(QMediaContent, mediaContent);

    QSignalSpy statusSpy(m_player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    mediaStatusList list(m_player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    m_player->setMedia(mediaContent);
    WAIT_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia || QMediaPlayer::LoadedMedia));

    if(valid) {
        if (videoAvailable) {
            QCOMPARE(statusSpy.count(), 3);
            QCOMPARE(list.count(), 3);
            QCOMPARE(list.at(0), QMediaPlayer::UnknownMediaStatus);
            QCOMPARE(list.at(1), QMediaPlayer::LoadingMedia);
            QCOMPARE(list.at(2), QMediaPlayer::LoadedMedia);
            QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);

            m_player->play();
            {WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);}
            QCOMPARE(statusSpy.count(), 5);
            QCOMPARE(list.count(), 5);
            QCOMPARE(list.at(3), QMediaPlayer::BufferingMedia);
            QCOMPARE(list.at(4), QMediaPlayer::BufferedMedia);
            QCOMPARE(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);

            {WAIT_LONG_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);}
            QVERIFY(statusSpy.count() == 6);
            QVERIFY(list.count() == 6);
            QCOMPARE(list.last(), QMediaPlayer::EndOfMedia);
            QCOMPARE(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);
        }else {
            QCOMPARE(statusSpy.count(), 3);
            QCOMPARE(list.count(), 3);
            QCOMPARE(list.at(0), QMediaPlayer::UnknownMediaStatus);
            QCOMPARE(list.at(1), QMediaPlayer::LoadingMedia);
            QCOMPARE(list.at(2), QMediaPlayer::LoadedMedia);
            QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);

            m_player->play();
            {WAIT_LONG_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);}
            QVERIFY(statusSpy.count() == 4);
            QVERIFY(list.count() == 4);
            QCOMPARE(list.last(), QMediaPlayer::EndOfMedia);
            QCOMPARE(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);
        }
    } else if (mediaContent.isNull()) {
        QCOMPARE(statusSpy.count(), 2);
        QCOMPARE(list.count(), 2);
        QCOMPARE(list.at(0), QMediaPlayer::UnknownMediaStatus);
        QCOMPARE(list.at(1), QMediaPlayer::NoMedia);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::NoMedia);
        m_player->play();
        QCOMPARE(statusSpy.count(), 2);
        QCOMPARE(list.count(), 2);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::NoMedia);
    } else if (streamable) {
        QCOMPARE(statusSpy.count(), 3);
        QCOMPARE(list.count(), 3);
        QCOMPARE(list.at(0), QMediaPlayer::UnknownMediaStatus);
        QCOMPARE(list.at(1), QMediaPlayer::LoadingMedia);
        QCOMPARE(list.at(2), QMediaPlayer::LoadedMedia);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
        m_player->play();
        QCOMPARE(statusSpy.count(), 3);
        QCOMPARE(list.count(), 3);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
    } else {
        QCOMPARE(statusSpy.count(), 3);
        QCOMPARE(list.count(), 3);
        QCOMPARE(list.at(0), QMediaPlayer::UnknownMediaStatus);
        QCOMPARE(list.at(1), QMediaPlayer::LoadingMedia);
        QCOMPARE(list.at(2), QMediaPlayer::InvalidMedia);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::InvalidMedia);
        m_player->play();
        QCOMPARE(statusSpy.count(), 3);
        QCOMPARE(list.count(), 3);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::InvalidMedia);
    }

    m_player->stop();
    QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
}

void tst_QMediaPlayer_s60::testPlaylist()
{
    if(!runonce) {
        QMediaContent content0(QUrl::fromLocalFile(TestFilePath + "test_mp4.mp4"));
        QMediaContent content1(QUrl::fromLocalFile(TestFilePath + "test_WAV.dat"));
        QMediaContent content2(QUrl::fromLocalFile(TestFilePath + "test_mp4.mp4"));
        QMediaContent content3(QUrl::fromLocalFile(TestFilePath + "test_WAV.dat"));
        QMediaContent content4(QUrl::fromLocalFile(TestFilePath + "test_MIDI.dat"));

        QMediaPlaylist *playlist = new QMediaPlaylist(m_player);
        m_player->setPlaylist(playlist);

        QSignalSpy stateSpy(m_player, SIGNAL(stateChanged(QMediaPlayer::State)));
        QSignalSpy mediaSpy(m_player, SIGNAL(mediaChanged(QMediaContent)));

        // Test the player does nothing with an empty playlist attached.
        m_player->play();
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(m_player->media(), QMediaContent());
        QCOMPARE(stateSpy.count(), 0);
        QCOMPARE(mediaSpy.count(), 0);

        playlist->addMedia(content0);
        playlist->addMedia(content1);
        playlist->addMedia(content2);
        playlist->addMedia(content3);

        // Test changing the playlist position, changes the current media, but not the playing state.
        playlist->setCurrentIndex(1);
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
        QCOMPARE(m_player->media(), content1);
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
        QCOMPARE(stateSpy.count(), 0);
        QCOMPARE(mediaSpy.count(), 1);

        // Test playing starts with the current media.
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->media(), content1);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QVERIFY(m_player->mediaStatus() == (QMediaPlayer::BufferedMedia||QMediaPlayer::BufferingMedia));
        QCOMPARE(stateSpy.count(), 1);
        QCOMPARE(mediaSpy.count(), 1);

        // Test pausing doesn't change the current media.
        m_player->pause();
        QCOMPARE(m_player->media(), content1);
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QVERIFY(m_player->mediaStatus() == (QMediaPlayer::BufferedMedia||QMediaPlayer::BufferingMedia));
        QCOMPARE(stateSpy.count(), 2);
        QCOMPARE(mediaSpy.count(), 1);

        // Test stopping doesn't change the current media.
        m_player->stop();
        QCOMPARE(m_player->media(), content1);
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
        QCOMPARE(stateSpy.count(), 3);
        QCOMPARE(mediaSpy.count(), 1);

        // Test when the player service reaches the end of the current media, the player moves onto
        // the next item without stopping.
        m_player->play();
        QCOMPARE(m_player->media(), content1);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QCOMPARE(stateSpy.count(), 4);
        QCOMPARE(mediaSpy.count(), 1);

        WAIT_FOR_CONDITION(mediaSpy.count(), 2);
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->media(), content2);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QVERIFY(m_player->mediaStatus() == (QMediaPlayer::BufferedMedia||QMediaPlayer::BufferingMedia));
        QCOMPARE(stateSpy.count(), 6);
        QCOMPARE(mediaSpy.count(), 2);

        // Test skipping the current media doesn't change the state.
        playlist->next();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->media(), content3);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QVERIFY(m_player->mediaStatus() == (QMediaPlayer::BufferedMedia||QMediaPlayer::BufferingMedia));
        QCOMPARE(stateSpy.count(), 6);
        QCOMPARE(mediaSpy.count(), 3);

        // Test changing the current media while paused doesn't change the state.
        m_player->pause();
        QCOMPARE(m_player->media(), content3);
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QVERIFY(m_player->mediaStatus() == (QMediaPlayer::BufferedMedia||QMediaPlayer::BufferingMedia));
        QCOMPARE(stateSpy.count(), 7);
        QCOMPARE(mediaSpy.count(), 3);

        playlist->previous();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
        QCOMPARE(m_player->media(), content2);
        QCOMPARE(m_player->state(), QMediaPlayer::PausedState);
        QVERIFY(m_player->mediaStatus() == (QMediaPlayer::BufferedMedia||QMediaPlayer::BufferingMedia));
        QCOMPARE(stateSpy.count(), 7);
        QCOMPARE(mediaSpy.count(), 4);

        // Test changing the current media while stopped doesn't change the state.
        m_player->stop();
        QCOMPARE(m_player->media(), content2);
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
        QCOMPARE(stateSpy.count(), 8);
        QCOMPARE(mediaSpy.count(), 4);

        playlist->next();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
        QCOMPARE(m_player->media(), content3);
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::LoadedMedia);
        QCOMPARE(stateSpy.count(), 8);
        QCOMPARE(mediaSpy.count(), 5);

        // Test the player is stopped and the current media cleared when it reaches the end of the last
        // item in the playlist.
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->media(), content3);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QVERIFY(m_player->mediaStatus() == (QMediaPlayer::BufferedMedia||QMediaPlayer::BufferingMedia));
        QCOMPARE(stateSpy.count(), 9);
        QCOMPARE(mediaSpy.count(), 5);

        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(m_player->media(), QMediaContent());
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::NoMedia);
        QCOMPARE(stateSpy.count(), 10);
        QCOMPARE(mediaSpy.count(), 6);

        // Test starts playing from the start of the playlist if there is no current media selected.
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia);
        QCOMPARE(m_player->media(), content0);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QVERIFY(m_player->mediaStatus() == (QMediaPlayer::BufferedMedia||QMediaPlayer::BufferingMedia));
        QCOMPARE(stateSpy.count(), 11);
        QCOMPARE(mediaSpy.count(), 7);

        // Test deleting the playlist stops the player and clears the media it set.
        delete playlist;
        QCOMPARE(m_player->media(), QMediaContent());
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::NoMedia);
        QCOMPARE(stateSpy.count(), 12);
        QCOMPARE(mediaSpy.count(), 8);

        // Test the player works as normal with the playlist removed.
        m_player->play();
        QCOMPARE(m_player->media(), QMediaContent());
        QCOMPARE(m_player->state(), QMediaPlayer::StoppedState);
        QCOMPARE(m_player->mediaStatus(), QMediaPlayer::NoMedia);
        QCOMPARE(stateSpy.count(), 12);
        QCOMPARE(mediaSpy.count(), 8);

        m_player->setMedia(content1);
        m_player->play();
        WAIT_FOR_CONDITION(m_player->mediaStatus(), QMediaPlayer::BufferedMedia)
        QCOMPARE(m_player->media(), content1);
        QCOMPARE(m_player->state(), QMediaPlayer::PlayingState);
        QVERIFY(m_player->mediaStatus() == (QMediaPlayer::BufferedMedia||QMediaPlayer::BufferingMedia));
        QCOMPARE(stateSpy.count(), 13);
        QCOMPARE(mediaSpy.count(), 9);
        m_player->stop();
        runonce = true;
    }
}

void tst_QMediaPlayer_s60::testStreamControl()
{
    QFETCH_GLOBAL(bool, valid);
    QFETCH_GLOBAL(bool, streamable);
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(QMediaPlayer::State, state);
    QFETCH_GLOBAL(bool, videoAvailable);
    QFETCH_GLOBAL(bool, audioAvailable);

    QMediaStreamsControl* m_streamControl = (QMediaStreamsControl*)(m_player->service()->requestControl(QMediaStreamsControl_iid));
    if(m_streamControl) {
        if (valid || streamable) {
            if (audioAvailable && !videoAvailable) {
                QSignalSpy streamInfoSpy(m_streamControl, SIGNAL(streamsChanged()));
                m_player->setMedia(mediaContent);
                WAIT_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia || QMediaPlayer::LoadedMedia));
                QVERIFY(m_streamControl->streamCount() == 1);
                QVERIFY(m_streamControl->streamType(1) == QMediaStreamsControl::AudioStream);
                QVERIFY(m_streamControl->isActive(1) == true);
                QCOMPARE(streamInfoSpy.count(), 1);
            }else if (audioAvailable && videoAvailable){
                QSignalSpy streamInfoSpy(m_streamControl, SIGNAL(streamsChanged()));
                m_player->setMedia(mediaContent);
                WAIT_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia || QMediaPlayer::LoadedMedia));
                QVERIFY(m_streamControl->streamCount() == 2);
                QVERIFY(m_streamControl->streamType(1) == QMediaStreamsControl::VideoStream);
                QVERIFY(m_streamControl->isActive(2) == true);
                QCOMPARE(streamInfoSpy.count(), 1);
            }else if (!audioAvailable && videoAvailable) {
                QSignalSpy streamInfoSpy(m_streamControl, SIGNAL(streamsChanged()));
                m_player->setMedia(mediaContent);
                WAIT_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia || QMediaPlayer::LoadedMedia));
                QVERIFY(m_streamControl->streamCount() == 1);
                QVERIFY(m_streamControl->streamType(1) == QMediaStreamsControl::VideoStream);
                QVERIFY(m_streamControl->isActive(2) == true);
                QCOMPARE(streamInfoSpy.count(), 1);
            }

        // Test that streamsChanged signal do not get emitted when adding same media
        QSignalSpy streamInfoSpy(m_streamControl, SIGNAL(streamsChanged()));
        m_player->setMedia(mediaContent);
        WAIT_FOR_CONDITION(m_player->mediaStatus(), (QMediaPlayer::NoMedia || QMediaPlayer::InvalidMedia || QMediaPlayer::LoadedMedia));
        QCOMPARE(streamInfoSpy.count(), 0);
        }
    }
}
