/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest/QtTest>
#include <QtCore/qdebug.h>
#include <QtCore/qbuffer.h>
#include <QtNetwork/qnetworkconfiguration.h>

#include <qgraphicsvideoitem.h>
#include <qabstractvideosurface.h>
#include <qmediaplayer.h>
#include <qmediaplayercontrol.h>

#include "mockmediaserviceprovider.h"
#include "mockmediaplayerservice.h"
#include "mockvideosurface.h"

QT_USE_NAMESPACE

class tst_QMediaPlayerWidgets: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void testSetVideoOutput();
    void testSetVideoOutputNoService();
    void testSetVideoOutputNoControl();

private:
    MockMediaServiceProvider *mockProvider;
    MockMediaPlayerService  *mockService;
    QMediaPlayer *player;
};

void tst_QMediaPlayerWidgets::initTestCase()
{
    qRegisterMetaType<QMediaPlayer::State>("QMediaPlayer::State");
    qRegisterMetaType<QMediaPlayer::Error>("QMediaPlayer::Error");
    qRegisterMetaType<QMediaPlayer::MediaStatus>("QMediaPlayer::MediaStatus");
    qRegisterMetaType<QMediaContent>("QMediaContent");

    mockService = new MockMediaPlayerService;
    mockProvider = new MockMediaServiceProvider(mockService, true);
    player = new QMediaPlayer(0, 0, mockProvider);
}

void tst_QMediaPlayerWidgets::cleanupTestCase()
{
    delete player;
}

void tst_QMediaPlayerWidgets::init()
{
    mockService->reset();
}

void tst_QMediaPlayerWidgets::cleanup()
{
}

void tst_QMediaPlayerWidgets::testSetVideoOutput()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    MockVideoSurface surface;

    MockMediaPlayerService service;
    MockMediaServiceProvider provider(&service);
    QMediaPlayer player(0, 0, &provider);

    player.setVideoOutput(&widget);
    QVERIFY(widget.mediaObject() == &player);

    player.setVideoOutput(&item);
    QVERIFY(widget.mediaObject() == 0);
    QVERIFY(item.mediaObject() == &player);

    player.setVideoOutput(reinterpret_cast<QVideoWidget *>(0));
    QVERIFY(item.mediaObject() == 0);

    player.setVideoOutput(&widget);
    QVERIFY(widget.mediaObject() == &player);

    player.setVideoOutput(reinterpret_cast<QGraphicsVideoItem *>(0));
    QVERIFY(widget.mediaObject() == 0);

    player.setVideoOutput(&surface);
    QVERIFY(service.rendererControl->surface() == &surface);

    player.setVideoOutput(reinterpret_cast<QAbstractVideoSurface *>(0));
    QVERIFY(service.rendererControl->surface() == 0);

    player.setVideoOutput(&surface);
    QVERIFY(service.rendererControl->surface() == &surface);

    player.setVideoOutput(&widget);
    QVERIFY(service.rendererControl->surface() == 0);
    QVERIFY(widget.mediaObject() == &player);

    player.setVideoOutput(&surface);
    QVERIFY(service.rendererControl->surface() == &surface);
    QVERIFY(widget.mediaObject() == 0);
}


void tst_QMediaPlayerWidgets::testSetVideoOutputNoService()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    MockVideoSurface surface;

    MockMediaServiceProvider provider(0, true);
    QMediaPlayer player(0, 0, &provider);

    player.setVideoOutput(&widget);
    QVERIFY(widget.mediaObject() == 0);

    player.setVideoOutput(&item);
    QVERIFY(item.mediaObject() == 0);

    player.setVideoOutput(&surface);
    // Nothing we can verify here other than it doesn't assert.
}

void tst_QMediaPlayerWidgets::testSetVideoOutputNoControl()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    MockVideoSurface surface;

    MockMediaPlayerService service;
    service.rendererRef = 1;
    service.windowRef = 1;

    MockMediaServiceProvider provider(&service);
    QMediaPlayer player(0, 0, &provider);

    player.setVideoOutput(&widget);
    QVERIFY(widget.mediaObject() == 0);

    player.setVideoOutput(&item);
    QVERIFY(item.mediaObject() == 0);

    player.setVideoOutput(&surface);
    QVERIFY(service.rendererControl->surface() == 0);
}

QTEST_MAIN(tst_QMediaPlayerWidgets)
#include "tst_qmediaplayerwidgets.moc"
