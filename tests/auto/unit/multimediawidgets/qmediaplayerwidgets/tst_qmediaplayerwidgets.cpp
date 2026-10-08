// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qdebug.h>
#include <QtCore/qbuffer.h>

#include <qgraphicsvideoitem.h>
#include <qvideosink.h>
#include <qmediaplayer.h>
#include <private/qplatformmediaplayer_p.h>

#include "qvideosink.h"
#include "qmockintegration.h"

QT_USE_NAMESPACE

Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN

class tst_QMediaPlayerWidgets: public QObject
{
    Q_OBJECT

private slots:
    void testSetVideoOutput();
    void testSetVideoOutputNoService();
    void testSetVideoOutputNoControl();
};

void tst_QMediaPlayerWidgets::testSetVideoOutput()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    QVideoSink surface;

    QMediaPlayer player;

    player.setVideoOutput(&widget);
//    QVERIFY(widget.mediaSource() == &player);

    player.setVideoOutput(&item);
//    QVERIFY(widget.mediaSource() == nullptr);
//    QVERIFY(item.mediaSource() == &player);

    player.setVideoOutput(reinterpret_cast<QVideoWidget *>(0));
//    QVERIFY(item.mediaSource() == nullptr);

    player.setVideoOutput(&widget);
//    QVERIFY(widget.mediaSource() == &player);

    player.setVideoOutput(reinterpret_cast<QGraphicsVideoItem *>(0));
//    QVERIFY(widget.mediaSource() == nullptr);

    player.setVideoOutput(&surface);
//    QVERIFY(mockService->rendererControl->surface() == &surface);

    player.setVideoOutput(reinterpret_cast<QVideoSink *>(0));
//    QVERIFY(mockService->rendererControl->surface() == nullptr);

    player.setVideoOutput(&surface);
//    QVERIFY(mockService->rendererControl->surface() == &surface);

    player.setVideoOutput(&widget);
//    QVERIFY(mockService->rendererControl->surface() == nullptr);
//    QVERIFY(widget.mediaSource() == &player);

    player.setVideoOutput(&surface);
//    QVERIFY(mockService->rendererControl->surface() == &surface);
//    QVERIFY(widget.mediaSource() == nullptr);
}


void tst_QMediaPlayerWidgets::testSetVideoOutputNoService()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    QVideoSink surface;

    QMockIntegration::instance()->setFlags(QMockIntegration::NoPlayerInterface);
    QMediaPlayer player;
    QMockIntegration::instance()->setFlags({});

    player.setVideoOutput(&widget);

    player.setVideoOutput(&item);

    player.setVideoOutput(&surface);
    // Nothing we can verify here other than it doesn't assert.
}

void tst_QMediaPlayerWidgets::testSetVideoOutputNoControl()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    QVideoSink surface;

    QMediaPlayer player;

    player.setVideoOutput(&widget);

    player.setVideoOutput(&item);

    player.setVideoOutput(&surface);
}

QTEST_MAIN(tst_QMediaPlayerWidgets)
#include "tst_qmediaplayerwidgets.moc"
