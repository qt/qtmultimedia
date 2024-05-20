// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QDebug>

#include <qcamera.h>
#include <qcameradevice.h>
#include <qmediadevices.h>

#include "qmockintegration.h"

QT_USE_NAMESPACE

Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN

using namespace Qt::Literals;

class tst_QCameraDevice: public QObject
{
    Q_OBJECT

private slots:
    void constructor();
    void defaultCamera();
    void availableCameras();
    void equality_operators();
    void qDebug_operator();
};

void tst_QCameraDevice::constructor()
{
    {
        // default camera
        QCamera camera;
        QCameraDevice info(camera.cameraDevice());
        QVERIFY(!info.isNull());
        QCOMPARE(info.id(), u"default"_s);
        QCOMPARE(info.description(), u"defaultCamera"_s);
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
    QCOMPARE(info.id(), u"back"_s);
    QCOMPARE(info.description(), u"backCamera"_s);
    QCOMPARE(info.position(), QCameraDevice::BackFace);

    QCameraDevice info2(info);
    QVERIFY(!info2.isNull());
    QCOMPARE(info2.id(), u"back"_s);
    QCOMPARE(info2.description(), u"backCamera"_s);
    QCOMPARE(info2.position(), QCameraDevice::BackFace);
}

void tst_QCameraDevice::defaultCamera()
{
    QCameraDevice info = QMediaDevices::defaultVideoInput();

    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), u"default"_s);
    QCOMPARE(info.description(), u"defaultCamera"_s);
    QCOMPARE(info.position(), QCameraDevice::UnspecifiedPosition);

    QCamera camera(info);
    QCOMPARE(camera.cameraDevice(), info);
}

void tst_QCameraDevice::availableCameras()
{
    QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    QCOMPARE(cameras.size(), 3);

    QCameraDevice info = cameras.at(0);
    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), u"default"_s);
    QCOMPARE(info.description(), u"defaultCamera"_s);
    QCOMPARE(info.position(), QCameraDevice::UnspecifiedPosition);

    info = cameras.at(1);
    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), QStringLiteral("front"));
    QCOMPARE(info.description(), QStringLiteral("frontCamera"));
    QCOMPARE(info.position(), QCameraDevice::FrontFace);

    QCOMPARE(cameras.size(), 3);
    info = cameras.at(2);
    QVERIFY(!info.isNull());
    QCOMPARE(info.id(), u"back"_s);
    QCOMPARE(info.description(), u"backCamera"_s);
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

void tst_QCameraDevice::qDebug_operator()
{
    QString outputString;
    QDebug debug(&outputString);
    debug.nospace();

    QCameraDevice defaultCamera = QMediaDevices::defaultVideoInput();
    debug << defaultCamera;

    QCOMPARE(outputString,
             u"\"QCameraDevice(name=defaultCamera, id=default, position=UnspecifiedPosition)\" "_s);
}

QTEST_MAIN(tst_QCameraDevice)

#include "tst_qcameradevice.moc"
