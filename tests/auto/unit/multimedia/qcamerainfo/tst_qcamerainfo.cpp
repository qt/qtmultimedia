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
#include <qcamerainfo.h>
#include <qmediadevicemanager.h>

#include "qmockintegration_p.h"
#include "qmockmediacapturesession.h"

QT_USE_NAMESPACE

class tst_QCameraInfo: public QObject
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

void tst_QCameraInfo::initTestCase()
{
}

void tst_QCameraInfo::init()
{
}

void tst_QCameraInfo::cleanup()
{
}

void tst_QCameraInfo::constructor()
{
    {
        // default camera
        QCamera camera;
        QCameraInfo info(camera.cameraInfo());
        QVERIFY(!info.isNull());
        QCOMPARE(info.id(), QStringLiteral("default"));
        QCOMPARE(info.description(), QStringLiteral("defaultCamera"));
        QCOMPARE(info.position(), QCameraInfo::UnspecifiedPosition);
    }

    auto cameras = QMediaDeviceManager::videoInputs();
    QCameraInfo info;
    for (const auto &c : cameras) {
        if (c.position() == QCameraInfo::BackFace)
            info = c;
    }
    QVERIFY(!info.isNull());

    QCamera camera(info);
    QCOMPARE(info, camera.cameraInfo());
    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), QStringLiteral("back"));
    QCOMPARE(info.description(), QStringLiteral("backCamera"));
    QCOMPARE(info.position(), QCameraInfo::BackFace);

    QCameraInfo info2(info);
    QVERIFY(!info2.isNull());
    QCOMPARE(info2.id(), QStringLiteral("back"));
    QCOMPARE(info2.description(), QStringLiteral("backCamera"));
    QCOMPARE(info2.position(), QCameraInfo::BackFace);
}

void tst_QCameraInfo::defaultCamera()
{
    QCameraInfo info = QMediaDeviceManager::defaultVideoInput();

    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), QStringLiteral("default"));
    QCOMPARE(info.description(), QStringLiteral("defaultCamera"));
    QCOMPARE(info.position(), QCameraInfo::UnspecifiedPosition);

    QCamera camera(info);
    QCOMPARE(camera.cameraInfo(), info);
}

void tst_QCameraInfo::availableCameras()
{
    QList<QCameraInfo> cameras = QMediaDeviceManager::videoInputs();
    QCOMPARE(cameras.count(), 3);

    QCameraInfo info = cameras.at(0);
    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), QStringLiteral("default"));
    QCOMPARE(info.description(), QStringLiteral("defaultCamera"));
    QCOMPARE(info.position(), QCameraInfo::UnspecifiedPosition);

    info = cameras.at(1);
    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), QStringLiteral("front"));
    QCOMPARE(info.description(), QStringLiteral("frontCamera"));
    QCOMPARE(info.position(), QCameraInfo::FrontFace);

    QCOMPARE(cameras.count(), 3);
    info = cameras.at(2);
    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), QStringLiteral("back"));
    QCOMPARE(info.description(), QStringLiteral("backCamera"));
    QCOMPARE(info.position(), QCameraInfo::BackFace);
}

void tst_QCameraInfo::equality_operators()
{
    QCameraInfo defaultCamera = QMediaDeviceManager::defaultVideoInput();
    QList<QCameraInfo> cameras = QMediaDeviceManager::videoInputs();

    QVERIFY(defaultCamera == cameras.at(0));
    QVERIFY(defaultCamera != cameras.at(1));
    QVERIFY(cameras.at(0) != cameras.at(1));

    {
        QCamera camera(defaultCamera);
        QVERIFY(camera.cameraInfo() == defaultCamera);
        QVERIFY(camera.cameraInfo() == cameras.at(0));
    }

    {
        QCamera camera(cameras.at(1));
        QVERIFY(camera.cameraInfo() == cameras.at(1));
    }
}


QTEST_MAIN(tst_QCameraInfo)

#include "tst_qcamerainfo.moc"
