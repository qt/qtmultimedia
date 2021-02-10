/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QDebug>

#include <qabstractvideosurface.h>
#include <qcameracontrol.h>
#include <qcameraexposurecontrol.h>
#include <qcamerafocuscontrol.h>
#include <qcameraimagecapturecontrol.h>
#include <qcameraimageprocessingcontrol.h>
#include <qmediaservice.h>
#include <qcamera.h>
#include <qcameraimagecapture.h>
#include <qgraphicsvideoitem.h>
#include <qvideorenderercontrol.h>
#include <qvideowidget.h>
#include <qvideowindowcontrol.h>

#include "mockvideosurface.h"
#include "mockvideorenderercontrol.h"
#include "mockvideowindowcontrol.h"
#include "mockmediarecorderservice.h"
#include "qmockintegration_p.h"

QT_USE_NAMESPACE


class tst_QCameraWidgets: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

private slots:
    void testCameraEncodingProperyChange();
    void testSetVideoOutput();
    void testSetVideoOutputNoService();
    void testSetVideoOutputNoControl();

private:
    QMockIntegration *mockIntegration;
};

void tst_QCameraWidgets::initTestCase()
{
}

void tst_QCameraWidgets::init()
{
    mockIntegration = new QMockIntegration;
}

void tst_QCameraWidgets::cleanup()
{
    delete mockIntegration;
}


void tst_QCameraWidgets::cleanupTestCase()
{
}

void tst_QCameraWidgets::testCameraEncodingProperyChange()
{
    QCamera camera;
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
}

void tst_QCameraWidgets::testSetVideoOutput()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    MockVideoSurface surface;
    QCamera camera;
    auto *mockCameraService = mockIntegration->lastCaptureService();

    camera.setViewfinder(&widget);
    qDebug() << widget.mediaSource();
    QVERIFY(widget.mediaSource() == &camera);

    camera.setViewfinder(&item);
    QVERIFY(widget.mediaSource() == nullptr);
    QVERIFY(item.mediaSource() == &camera);

    camera.setViewfinder(reinterpret_cast<QVideoWidget *>(0));
    QVERIFY(item.mediaSource() == nullptr);

    camera.setViewfinder(&widget);
    QVERIFY(widget.mediaSource() == &camera);

    camera.setViewfinder(reinterpret_cast<QGraphicsVideoItem *>(0));
    QVERIFY(widget.mediaSource() == nullptr);

    camera.setViewfinder(&surface);
    QVERIFY(mockCameraService->rendererControl->surface() == &surface);

    camera.setViewfinder(reinterpret_cast<QAbstractVideoSurface *>(0));
    QVERIFY(mockCameraService->rendererControl->surface() == nullptr);

    camera.setViewfinder(&surface);
    QVERIFY(mockCameraService->rendererControl->surface() == &surface);

    camera.setViewfinder(&widget);
    QVERIFY(mockCameraService->rendererControl->surface() == nullptr);
    QVERIFY(widget.mediaSource() == &camera);

    camera.setViewfinder(&surface);
    QVERIFY(mockCameraService->rendererControl->surface() == &surface);
    QVERIFY(widget.mediaSource() == nullptr);
}


void tst_QCameraWidgets::testSetVideoOutputNoService()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    MockVideoSurface surface;

    mockIntegration->setFlags(QMockIntegration::NoCaptureInterface);
    QCamera camera;
    mockIntegration->setFlags({});

    camera.setViewfinder(&widget);
    QVERIFY(widget.mediaSource() == nullptr);

    camera.setViewfinder(&item);
    QVERIFY(item.mediaSource() == nullptr);

    camera.setViewfinder(&surface);
    // Nothing we can verify here other than it doesn't assert.

}


void tst_QCameraWidgets::testSetVideoOutputNoControl()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    MockVideoSurface surface;


    QCamera camera;
    auto *mockCameraService = mockIntegration->lastCaptureService();
    mockCameraService->rendererRef = 1;
    mockCameraService->windowRef = 1;

    camera.setViewfinder(&widget);
    QVERIFY(widget.mediaSource() == nullptr);

    camera.setViewfinder(&item);
    QVERIFY(item.mediaSource() == nullptr);

    camera.setViewfinder(&surface);
    QVERIFY(mockCameraService->rendererControl->surface() == nullptr);
}

QTEST_MAIN(tst_QCameraWidgets)

#include "tst_qcamerawidgets.moc"
