/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
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
#include <QDebug>

#include <qabstractvideosurface.h>
#include <qcameracontrol.h>
#include <qcameralockscontrol.h>
#include <qcameraexposurecontrol.h>
#include <qcameraflashcontrol.h>
#include <qcamerafocuscontrol.h>
#include <qcameraimagecapturecontrol.h>
#include <qimageencodercontrol.h>
#include <qcameraimageprocessingcontrol.h>
#include <qcameracapturebufferformatcontrol.h>
#include <qcameracapturedestinationcontrol.h>
#include <qmediaservice.h>
#include <qcamera.h>
#include <qcameraimagecapture.h>
#include <qgraphicsvideoitem.h>
#include <qvideorenderercontrol.h>
#include <qvideowidget.h>
#include <qvideowindowcontrol.h>

#include "mockcameraservice.h"

#include "mockmediaserviceprovider.h"
#include "mockvideosurface.h"
#include "mockvideorenderercontrol.h"
#include "mockvideowindowcontrol.h"

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
    void testSetVideoOutputNoService();
    void testSetVideoOutputNoControl();

private:
    MockSimpleCameraService  *mockSimpleCameraService;
    MockMediaServiceProvider *provider;
};

void tst_QCameraWidgets::initTestCase()
{
    provider = new MockMediaServiceProvider;
    mockSimpleCameraService = new MockSimpleCameraService;
    provider->service = mockSimpleCameraService;
}

void tst_QCameraWidgets::cleanupTestCase()
{
    delete mockSimpleCameraService;
    delete provider;
}

void tst_QCameraWidgets::testCameraEncodingProperyChange()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);

    QSignalSpy stateChangedSignal(&camera, SIGNAL(stateChanged(QCamera::State)));
    QSignalSpy statusChangedSignal(&camera, SIGNAL(statusChanged(QCamera::Status)));

    camera.start();
    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::ActiveStatus);

    QCOMPARE(stateChangedSignal.count(), 1);
    QCOMPARE(statusChangedSignal.count(), 1);
    stateChangedSignal.clear();
    statusChangedSignal.clear();


    camera.setCaptureMode(QCamera::CaptureVideo);
    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::LoadedStatus);

    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    QTest::qWait(10);

    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::ActiveStatus);
    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    //backens should not be stopped since the capture mode is Video
    imageCapture.setEncodingSettings(QImageEncoderSettings());
    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 0);

    camera.setCaptureMode(QCamera::CaptureStillImage);
    QTest::qWait(10);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    //the settings change should trigger camera stop/start
    imageCapture.setEncodingSettings(QImageEncoderSettings());
    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::LoadedStatus);

    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    QTest::qWait(10);

    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::ActiveStatus);
    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    //the settings change should trigger camera stop/start only once
    camera.setCaptureMode(QCamera::CaptureVideo);
    camera.setCaptureMode(QCamera::CaptureStillImage);
    imageCapture.setEncodingSettings(QImageEncoderSettings());
    imageCapture.setEncodingSettings(QImageEncoderSettings());

    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::LoadedStatus);

    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    QTest::qWait(10);

    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::ActiveStatus);
    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    //setting the viewfinder should also trigger backend to be restarted:
    camera.setViewfinder(new QGraphicsVideoItem());
    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::LoadedStatus);

    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);

    QTest::qWait(10);

    service.mockControl->m_propertyChangesSupported = true;
    //the changes to encoding settings,
    //capture mode and encoding parameters should not trigger service restart
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    camera.setCaptureMode(QCamera::CaptureVideo);
    camera.setCaptureMode(QCamera::CaptureStillImage);
    imageCapture.setEncodingSettings(QImageEncoderSettings());
    imageCapture.setEncodingSettings(QImageEncoderSettings());
    camera.setViewfinder(new QGraphicsVideoItem());

    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 0);
}

void tst_QCameraWidgets::testSetVideoOutput()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    MockVideoSurface surface;

    MockCameraService service;
    MockMediaServiceProvider provider;
    provider.service = &service;
    QCamera camera(0, &provider);

    camera.setViewfinder(&widget);
    qDebug() << widget.mediaObject();
    QVERIFY(widget.mediaObject() == &camera);

    camera.setViewfinder(&item);
    QVERIFY(widget.mediaObject() == 0);
    QVERIFY(item.mediaObject() == &camera);

    camera.setViewfinder(reinterpret_cast<QVideoWidget *>(0));
    QVERIFY(item.mediaObject() == 0);

    camera.setViewfinder(&widget);
    QVERIFY(widget.mediaObject() == &camera);

    camera.setViewfinder(reinterpret_cast<QGraphicsVideoItem *>(0));
    QVERIFY(widget.mediaObject() == 0);

    camera.setViewfinder(&surface);
    QVERIFY(service.rendererControl->surface() == &surface);

    camera.setViewfinder(reinterpret_cast<QAbstractVideoSurface *>(0));
    QVERIFY(service.rendererControl->surface() == 0);

    camera.setViewfinder(&surface);
    QVERIFY(service.rendererControl->surface() == &surface);

    camera.setViewfinder(&widget);
    QVERIFY(service.rendererControl->surface() == 0);
    QVERIFY(widget.mediaObject() == &camera);

    camera.setViewfinder(&surface);
    QVERIFY(service.rendererControl->surface() == &surface);
    QVERIFY(widget.mediaObject() == 0);
}


void tst_QCameraWidgets::testSetVideoOutputNoService()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    MockVideoSurface surface;

    MockMediaServiceProvider provider;
    provider.service = 0;
    QCamera camera(0, &provider);

    camera.setViewfinder(&widget);
    QVERIFY(widget.mediaObject() == 0);

    camera.setViewfinder(&item);
    QVERIFY(item.mediaObject() == 0);

    camera.setViewfinder(&surface);
    // Nothing we can verify here other than it doesn't assert.
}

void tst_QCameraWidgets::testSetVideoOutputNoControl()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    MockVideoSurface surface;

    MockCameraService service;
    service.rendererRef = 1;
    service.windowRef = 1;

    MockMediaServiceProvider provider;
    provider.service = &service;
    QCamera camera(0, &provider);

    camera.setViewfinder(&widget);
    QVERIFY(widget.mediaObject() == 0);

    camera.setViewfinder(&item);
    QVERIFY(item.mediaObject() == 0);

    camera.setViewfinder(&surface);
    QVERIFY(service.rendererControl->surface() == 0);
}

QTEST_MAIN(tst_QCameraWidgets)

#include "tst_qcamerawidgets.moc"
