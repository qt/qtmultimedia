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
#include <private/qplatformimagecapture_p.h>
#include <qmediacapturesession.h>
#include <qcamera.h>
#include <qimagecapture.h>
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
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy activeChangedSignal(&camera, SIGNAL(activeChanged(bool)));

    camera.start();
    QCOMPARE(camera.isActive(), true);

    QCOMPARE(activeChangedSignal.count(), 1);
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
