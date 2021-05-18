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

#include <private/qplatformcamera_p.h>
#include <private/qplatformcameraimagecapture_p.h>
#include <private/qplatformcameraimageprocessing_p.h>
#include <qmediacapturesession.h>
#include <qcamera.h>
#include <qcameraimagecapture.h>
#include <qgraphicsvideoitem.h>
#include <qobject.h>
#include <qvideowidget.h>
#include <qvideosink.h>

#include "qmockmediacapturesession.h"
#include "qmockintegration_p.h"

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
    QMockIntegration mockIntegration;
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
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy statusChangedSignal(&camera, SIGNAL(statusChanged(QCamera::Status)));

    camera.start();
    QCOMPARE(camera.isActive(), true);
    QCOMPARE(camera.status(), QCamera::ActiveStatus);

    QCOMPARE(statusChangedSignal.count(), 1);
}

void tst_QCameraWidgets::testSetVideoOutput()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    QVideoSink surface;
    QMediaCaptureSession session;

    session.setVideoOutput(&widget);
    QVERIFY(session.videoOutput() == QVariant::fromValue(&widget));

    session.setVideoOutput(&item);
    QVERIFY(session.videoOutput() == QVariant::fromValue(&item));

    session.setVideoOutput(static_cast<QVideoWidget *>(nullptr));
    QVERIFY(session.videoOutput() == QVariant());

    session.setVideoOutput(&widget);
    QVERIFY(session.videoOutput() == QVariant::fromValue(&widget));

    session.setVideoOutput(static_cast<QGraphicsVideoItem *>(nullptr));
    QVERIFY(session.videoOutput() == QVariant());

    session.setVideoOutput(&surface);
    QVERIFY(session.videoOutput() == QVariant::fromValue(&surface));

    session.setVideoOutput(static_cast<QVideoSink *>(nullptr));
    QVERIFY(session.videoOutput() == QVariant());

    session.setVideoOutput(&surface);
    QVERIFY(session.videoOutput() == QVariant::fromValue(&surface));

    session.setVideoOutput(&widget);
    QVERIFY(session.videoOutput() == QVariant::fromValue(&widget));

    session.setVideoOutput(&surface);
    QVERIFY(session.videoOutput() == QVariant::fromValue(&surface));
}

QTEST_MAIN(tst_QCameraWidgets)

#include "tst_qcamerawidgets.moc"
