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
#ifndef TST_QMEDIAPLAYER_XA_H
#define TST_QMEDIAPLAYER_XA_H

#include <QtTest/QtTest>
#include <QtCore>
#include <QtGui>
#include <QFile>

#include <QMediaPlayer>
#include <QMediaPlayerControl>
#include <QMediaService>
#include <QMediaStreamsControl>
#include <QVideoWidget>
#include <QVideoWidgetControl>
#include <QVideoWindowControl>
#include <QMetadataReaderControl>
#include <QtNetwork/qnetworkconfiguration.h>
#include <QtNetwork/qnetworkconfigmanager.h>
#include <qmedianetworkaccesscontrol.h>

QT_USE_NAMESPACE
class mediaStatusList;
/*
class mediaStatusList : public QObject, public QList<QMediaPlayer::MediaStatus>
{
    Q_OBJECT
public slots:
    void mediaStatus(QMediaPlayer::MediaStatus status) {
        append(status);
    }

public:
    mediaStatusList(QObject *obj, const char *aSignal)
    : QObject()
    {
        connect(obj, aSignal, this, SLOT(mediaStatus(QMediaPlayer::MediaStatus)));
    }
};
*/
class tst_QMediaPlayer_xa: public QObject
{
    Q_OBJECT

public:
    tst_QMediaPlayer_xa();
    ~tst_QMediaPlayer_xa();

private slots:
    void initTestCase_data();
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testMediaStatus();
    void testBufferStatus();
    void testPositionWhilePlaying();
    void testFullScreen();
    void testWindowControl_FullScreen();
    void testSeekable();
    void testStreamInformation();
    void testAspectRatioMode();
    void testMedia();
    void testDuration();
    void testPosition();
    void testVolume();
    void testVolumeWhilePlaying();
    void testMuted();
    void testMutedWhilePlaying();
    void testVideoAndAudioAvailability();
    void testPlay();
    void testPause();
    void testStop();
    void testPlaybackRate();
    void testPlaybackRateWhilePlaying();
    void testWindowControl_NativeSize();
    void testWindowControl_AspectRatioMode();
    void testSetconfigurationsAP();
    void testSetAccesspoint();
    void testGetAccesspoint();
    void testDiffmediacontentAP();
    void testInvalidaddressAP();
    void testMultipleAccesspoints();
    void testReconnectAPWhilestreaming();
    void teststreampausestream();
    void testStressAccessPoint();

private:

    void resetPlayer();
    void resetPlayer_WindowControl();

    void updateLog(QString str, bool delFile=false);
    void setAudioOnlyContent()
    {
        mediaContent = audioOnlyContent;

        m_player->setMedia(*mediaContent);
    }

    void setVideoOnlyContent()
    {
        mediaContent = videoOnlyContent;
        duration = 60000;

        m_player->setMedia(*mediaContent);
    }

    void setAudioVideoContent()
    {
        if (mediaContent == audioVideoContent) {
            mediaContent = audioVideoAltContent;
            duration = 101840;
        } else {
            mediaContent = audioVideoContent;
            duration = 141000;
        }
        m_player->setMedia(*mediaContent);
    }

    void setStreamingContent3gp()
    {
        mediaContent = streamingContent3gp;

        m_player->setMedia(*mediaContent);
    }
   
    void setAudioStreamingContent()
    {
        mediaContent = audioStreamingContent;

        m_player->setMedia(*mediaContent);
    }


    QMediaContent* audioOnlyContent;
    QMediaContent* videoOnlyContent;
    QMediaContent* audioVideoContent;
    QMediaContent* audioVideoAltContent;
    QMediaContent* mediaContent;
    QMediaContent* streamingContent3gp;
    QMediaContent* audioStreamingContent;
    //for access-point requirement
    QList<QNetworkConfiguration> accesspointlist;
    QList<QNetworkConfiguration> secaccesspoint;
    QNetworkConfigurationManager manager;

    qint64 duration;
    QMediaPlayer *m_player;
    QVideoWidget *m_widget;
    QWidget *m_windowWidget;
};

#endif // TST_QMEDIAPLAYER_XA_H
