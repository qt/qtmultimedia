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
#include <private/qplatformcamera_p.h>
#include <private/qplatformcameraexposure_p.h>
#include <private/qplatformcamerafocus_p.h>
#include <private/qplatformcameraimagecapture_p.h>
#include <private/qplatformcameraimageprocessing_p.h>
#include <qmediacapturesession.h>
#include <qcamera.h>
#include <qcameraimagecapture.h>
#include <qgraphicsvideoitem.h>
#include <qobject.h>
#include <qvideowidget.h>

#include "mockvideosurface.h"
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
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy stateChangedSignal(&camera, SIGNAL(stateChanged(QCamera::State)));
    QSignalSpy statusChangedSignal(&camera, SIGNAL(statusChanged(QCamera::Status)));

    camera.start();
    QCOMPARE(camera.isActive(), true);
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
    QMediaCaptureSession session;

    session.setVideoPreview(&widget);
//    qDebug() << widget.mediaSource();
//    QVERIFY(widget.mediaSource() == &session);

    session.setVideoPreview(&item);
//    QVERIFY(widget.mediaSource() == nullptr);
//    QVERIFY(item.mediaSource() == &session);

    session.setVideoPreview(reinterpret_cast<QVideoWidget *>(0));
//    QVERIFY(item.mediaSource() == nullptr);

    session.setVideoPreview(&widget);
//    QVERIFY(widget.mediaSource() == &session);

    session.setVideoPreview(reinterpret_cast<QGraphicsVideoItem *>(0));
//    QVERIFY(widget.mediaSource() == nullptr);

    session.setVideoPreview(&surface);
//    QVERIFY(mocksessionService->rendererControl->surface() == &surface);

    session.setVideoPreview(reinterpret_cast<QAbstractVideoSurface *>(0));
//    QVERIFY(mocksessionService->rendererControl->surface() == nullptr);

    session.setVideoPreview(&surface);
//    QVERIFY(mocksessionService->rendererControl->surface() == &surface);

    session.setVideoPreview(&widget);
//    QVERIFY(mocksessionService->rendererControl->surface() == nullptr);
//    QVERIFY(widget.mediaSource() == &session);

    session.setVideoPreview(&surface);
//    QVERIFY(mockCameraService->rendererControl->surface() == &surface);
//    QVERIFY(widget.mediaSource() == nullptr);
}

QTEST_MAIN(tst_QCameraWidgets)

#include "tst_qcamerawidgets.moc"
