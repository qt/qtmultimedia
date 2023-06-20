// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QDebug>

#include <private/qplatformcamera_p.h>
#include <private/qplatformimagecapture_p.h>
#include <qmediacapturesession.h>
#include <qcamera.h>
#include <qimagecapture.h>
#include <qgraphicsvideoitem.h>
#include <qobject.h>
#include <qvideowidget.h>
#include <qvideosink.h>

#include "qmockmediacapturesession.h"
#include "qmockintegration.h"

QT_USE_NAMESPACE


class tst_QCameraWidgets: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void testCameraEncodingProperyChange();
    void testSetVideoOutput();

private:
    QMockIntegrationFactory mockIntegrationFactory;
};

void tst_QCameraWidgets::initTestCase()
{
}


void tst_QCameraWidgets::cleanupTestCase()
{
}

void tst_QCameraWidgets::testCameraEncodingProperyChange()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy activeChangedSignal(&camera, SIGNAL(activeChanged(bool)));

    camera.start();
    QCOMPARE(camera.isActive(), true);

    QCOMPARE(activeChangedSignal.size(), 1);
}

void tst_QCameraWidgets::testSetVideoOutput()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    QVideoSink surface;
    QMediaCaptureSession session;

    session.setVideoOutput(&widget);
    QVERIFY(session.videoSink() == widget.videoSink());
    QVERIFY(session.videoOutput() == &widget);

    session.setVideoOutput(&item);
    QVERIFY(session.videoSink() == item.videoSink());
    QVERIFY(session.videoOutput() == &item);

    session.setVideoOutput(nullptr);
    QVERIFY(session.videoSink() == nullptr);
    QVERIFY(session.videoOutput() == nullptr);

    session.setVideoOutput(&widget);
    QVERIFY(session.videoSink() == widget.videoSink());
    QVERIFY(session.videoOutput() == &widget);

    session.setVideoOutput(nullptr);
    QVERIFY(session.videoOutput() == nullptr);

    session.setVideoOutput(&surface);
    QVERIFY(session.videoSink() == &surface);
    QVERIFY(session.videoOutput() == &surface);

    session.setVideoSink(nullptr);
    QVERIFY(session.videoSink() == nullptr);
    QVERIFY(session.videoOutput() == nullptr);

    session.setVideoOutput(&surface);
    QVERIFY(session.videoSink() == &surface);
    QVERIFY(session.videoOutput() == &surface);

    session.setVideoSink(&surface);
    QVERIFY(session.videoSink() == &surface);
    QVERIFY(session.videoOutput() == nullptr);

    session.setVideoOutput(&widget);
    QVERIFY(session.videoSink() == widget.videoSink());
    QVERIFY(session.videoOutput() == &widget);

    session.setVideoOutput(&surface);
    QVERIFY(session.videoOutput() == &surface);
}

QTEST_MAIN(tst_QCameraWidgets)

#include "tst_qcamerawidgets.moc"
