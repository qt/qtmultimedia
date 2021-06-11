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

#include <QtTest/QtTest>
#include <QDebug>

#include <qcamera.h>
#include <qcameradevice.h>
#include <qmediadevices.h>

#include "qmockintegration_p.h"
#include "qmockmediacapturesession.h"

QT_USE_NAMESPACE

class tst_QCameraDevice: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void init();
    void cleanup();

private slots:
    void constructor();
    void defaultCamera();
    void availableCameras();
    void equality_operators();

private:
    QMockIntegration integration;
};

void tst_QCameraDevice::initTestCase()
{
}

void tst_QCameraDevice::init()
{
}

void tst_QCameraDevice::cleanup()
{
}

void tst_QCameraDevice::constructor()
{
    {
        // default camera
        QCamera camera;
        QCameraDevice info(camera.cameraDevice());
        QVERIFY(!info.isNull());
        QCOMPARE(info.id(), QStringLiteral("default"));
        QCOMPARE(info.description(), QStringLiteral("defaultCamera"));
        QCOMPARE(info.position(), QCameraDevice::UnspecifiedPosition);
    }

    auto cameras = QMediaDevices::videoInputs();
    QCameraDevice info;
    for (const auto &c : cameras) {
        if (c.position() == QCameraDevice::BackFace)
            info = c;
    }
    QVERIFY(!info.isNull());

    QCamera camera(info);
    QCOMPARE(info, camera.cameraDevice());
    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), QStringLiteral("back"));
    QCOMPARE(info.description(), QStringLiteral("backCamera"));
    QCOMPARE(info.position(), QCameraDevice::BackFace);

    QCameraDevice info2(info);
    QVERIFY(!info2.isNull());
    QCOMPARE(info2.id(), QStringLiteral("back"));
    QCOMPARE(info2.description(), QStringLiteral("backCamera"));
    QCOMPARE(info2.position(), QCameraDevice::BackFace);
}

void tst_QCameraDevice::defaultCamera()
{
    QCameraDevice info = QMediaDevices::defaultVideoInput();

    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), QStringLiteral("default"));
    QCOMPARE(info.description(), QStringLiteral("defaultCamera"));
    QCOMPARE(info.position(), QCameraDevice::UnspecifiedPosition);

    QCamera camera(info);
    QCOMPARE(camera.cameraDevice(), info);
}

void tst_QCameraDevice::availableCameras()
{
    QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    QCOMPARE(cameras.count(), 3);

    QCameraDevice info = cameras.at(0);
    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), QStringLiteral("default"));
    QCOMPARE(info.description(), QStringLiteral("defaultCamera"));
    QCOMPARE(info.position(), QCameraDevice::UnspecifiedPosition);

    info = cameras.at(1);
    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), QStringLiteral("front"));
    QCOMPARE(info.description(), QStringLiteral("frontCamera"));
    QCOMPARE(info.position(), QCameraDevice::FrontFace);

    QCOMPARE(cameras.count(), 3);
    info = cameras.at(2);
    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), QStringLiteral("back"));
    QCOMPARE(info.description(), QStringLiteral("backCamera"));
    QCOMPARE(info.position(), QCameraDevice::BackFace);
}

void tst_QCameraDevice::equality_operators()
{
    QCameraDevice defaultCamera = QMediaDevices::defaultVideoInput();
    QList<QCameraDevice> cameras = QMediaDevices::videoInputs();

    QVERIFY(defaultCamera == cameras.at(0));
    QVERIFY(defaultCamera != cameras.at(1));
    QVERIFY(cameras.at(0) != cameras.at(1));

    {
        QCamera camera(defaultCamera);
        QVERIFY(camera.cameraDevice() == defaultCamera);
        QVERIFY(camera.cameraDevice() == cameras.at(0));
    }

    {
        QCamera camera(cameras.at(1));
        QVERIFY(camera.cameraDevice() == cameras.at(1));
    }
}


QTEST_MAIN(tst_QCameraDevice)

#include "tst_qcameradevice.moc"
