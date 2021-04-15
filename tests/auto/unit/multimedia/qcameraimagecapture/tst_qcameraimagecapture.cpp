/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

/****************************************************************************
Author : Vijay/Avinash

Reviewer Name       Date                Coverage ( Full / Test Case IDs ).
---------------------------------------------------------------------------
                                        Initial review of test cases.
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QDebug>

#include <private/qplatformcamera_p.h>
#include <private/qplatformcameraexposure_p.h>
#include <private/qplatformcamerafocus_p.h>
#include <private/qplatformcameraimagecapture_p.h>
#include <private/qplatformcameraimageprocessing_p.h>
#include <qcamera.h>
#include <qcameraimagecapture.h>
#include <qmediacapturesession.h>

#include "mockmediarecorderservice.h"
#include "qmockintegration_p.h"

QT_USE_NAMESPACE

class tst_QCameraImageCapture: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

private slots:
    void constructor();
    void mediaSource();
    void deleteMediaSource();
    void isReadyForCapture();
    void capture();
    void encodingSettings();
    void errors();
    void error();
    void imageCaptured();
    void imageExposed();
    void imageSaved();
    void readyForCaptureChanged();
    void cameraImageCaptureControl();

private:
    QMockIntegration *mockIntegration;
};

void tst_QCameraImageCapture::initTestCase()
{
    mockIntegration = new QMockIntegration;
}

void tst_QCameraImageCapture::init()
{
}

void tst_QCameraImageCapture::cleanup()
{
}

void tst_QCameraImageCapture::cleanupTestCase()
{
    delete mockIntegration;
}

//MaemoAPI-1823:test QCameraImageCapture Constructor
void tst_QCameraImageCapture::constructor()
{
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(imageCapture.isAvailable() == true);
}

//MaemoAPI-1824:test mediaSource
void tst_QCameraImageCapture::mediaSource()
{
    {
        QMediaCaptureSession session;
        QCamera camera;
        mockIntegration->lastCaptureService()->hasControls = false;
        QCameraImageCapture imageCapture;
        session.setCamera(&camera);
        session.setImageCapture(&imageCapture);

        QVERIFY(!imageCapture.isAvailable());
    }

    {
        QMediaCaptureSession session;
        QCamera camera;
        QCameraImageCapture imageCapture;
        session.setCamera(&camera);
        session.setImageCapture(&imageCapture);

        QVERIFY(imageCapture.isAvailable());
    }
}

void tst_QCameraImageCapture::deleteMediaSource()
{
    QMediaCaptureSession session;
    QCamera *camera = new QCamera;
    QCameraImageCapture *capture = new QCameraImageCapture;
    session.setCamera(camera);
    session.setImageCapture(capture);

    QVERIFY(capture->isAvailable());

    delete camera;

    QVERIFY(session.camera() == nullptr);
    QVERIFY(!capture->isAvailable());

    capture->captureToFile();
    delete capture;
}

//MaemoAPI-1825:test isReadyForCapture
void tst_QCameraImageCapture::isReadyForCapture()
{
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.captureToFile();
    QTRY_VERIFY(imageCapture.isReadyForCapture());
    camera.stop();
}

//MaemoAPI-1826:test capture
void tst_QCameraImageCapture::capture()
{
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    QVERIFY(imageCapture.captureToFile() == -1);
    camera.start();
    QVERIFY(imageCapture.isReadyForCapture() == true);
    QTest::qWait(300);
    QVERIFY(imageCapture.captureToFile() != -1);
    camera.stop();
}

//MaemoAPI-1828:test encodingSettings
//MaemoAPI-1829:test set encodingSettings
void tst_QCameraImageCapture::encodingSettings()
{
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.encodingSettings() == QImageEncoderSettings());
    QImageEncoderSettings settings;
    settings.setFormat(QImageEncoderSettings::JPEG);
    settings.setQuality(QImageEncoderSettings::NormalQuality);
    imageCapture.setEncodingSettings(settings);
    QVERIFY(!imageCapture.encodingSettings().isNull());
    QVERIFY(imageCapture.encodingSettings().format() == QImageEncoderSettings::JPEG);
    QVERIFY(imageCapture.encodingSettings().quality() == QImageEncoderSettings::NormalQuality);
}

//MaemoAPI-1830:test errors
void tst_QCameraImageCapture::errors()
{
    MockMediaRecorderService::simpleCamera = true;

    {
        QMediaCaptureSession session;
        QCamera camera;
        QCameraImageCapture imageCapture;
        session.setCamera(&camera);

        session.setImageCapture(&imageCapture);
        QVERIFY(imageCapture.isAvailable() == false);
        imageCapture.captureToFile(QString::fromLatin1("/dev/null"));
        QVERIFY(imageCapture.error() == QCameraImageCapture::NotSupportedFeatureError);
        QVERIFY2(!imageCapture.errorString().isEmpty(), "Device does not support images capture");
    }

    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.error() == QCameraImageCapture::NoError);
    QVERIFY(imageCapture.errorString().isEmpty());

    imageCapture.captureToFile();
    QVERIFY(imageCapture.error() == QCameraImageCapture::NotReadyError);
    QVERIFY2(!imageCapture.errorString().isEmpty(), "Could not capture in stopped state");

    MockMediaRecorderService::simpleCamera = false;
}

//MaemoAPI-1831:test error
void tst_QCameraImageCapture::error()
{
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy spy(&imageCapture, SIGNAL(error(int,QCameraImageCapture::Error,QString)));
    imageCapture.captureToFile();
    QTest::qWait(30);
    QVERIFY(spy.count() == 1);
    QVERIFY(qvariant_cast<int>(spy.at(0).at(0)) == -1);
    QVERIFY(qvariant_cast<QCameraImageCapture::Error>(spy.at(0).at(1)) == QCameraImageCapture::NotReadyError);
    QVERIFY(qvariant_cast<QString>(spy.at(0).at(2)) == "Could not capture in stopped state");
    spy.clear();
}

//MaemoAPI-1832:test imageCaptured
void tst_QCameraImageCapture::imageCaptured()
{
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy spy(&imageCapture, SIGNAL(imageCaptured(int,QImage)));
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.captureToFile();
    QTRY_VERIFY(imageCapture.isReadyForCapture());

    QVERIFY(spy.count() == 1);
    QVERIFY(qvariant_cast<int>(spy.at(0).at(0)) > 0);
    QImage image = qvariant_cast<QImage>(spy.at(0).at(1));
    QVERIFY(image.isNull() == true);
    spy.clear();
    camera.stop();
}

//MaemoAPI-1833:test imageExposed
void tst_QCameraImageCapture::imageExposed()
{
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy spy(&imageCapture, SIGNAL(imageExposed(int)));
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.captureToFile();
    QTRY_VERIFY(imageCapture.isReadyForCapture());

    QVERIFY(spy.count() == 1);
    QVERIFY(qvariant_cast<int>(spy.at(0).at(0)) > 0);
    spy.clear();
    camera.stop();
}

//MaemoAPI-1834:test imageSaved
void tst_QCameraImageCapture::imageSaved()
{
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy spy(&imageCapture, SIGNAL(imageSaved(int,QString)));
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.captureToFile(QString::fromLatin1("/usr/share"));
    QTRY_VERIFY(imageCapture.isReadyForCapture());

    QVERIFY(spy.count() == 1);
    QVERIFY(qvariant_cast<int>(spy.at(0).at(0)) > 0);
    QVERIFY(qvariant_cast<QString>(spy.at(0).at(1)) == "/usr/share");
    spy.clear();
    camera.stop();
}

//MaemoAPI-1835:test readyForCaptureChanged
void tst_QCameraImageCapture::readyForCaptureChanged()
{
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy spy(&imageCapture, SIGNAL(readyForCaptureChanged(bool)));
    QVERIFY(imageCapture.isReadyForCapture() == false);
    imageCapture.captureToFile();
    QTest::qWait(100);
    QVERIFY(spy.count() == 0);
    QVERIFY2(!imageCapture.errorString().isEmpty(),"Could not capture in stopped state" );
    camera.start();
    QTest::qWait(100);
    imageCapture.captureToFile();
    QTest::qWait(100);
    QVERIFY(spy.count() == 2);
    QVERIFY(spy.at(0).at(0).toBool() == false);
    QVERIFY(spy.at(1).at(0).toBool() == true);
    camera.stop();
    spy.clear();
}

//MaemoAPI-1853:test cameraImageCapture control constructor
void tst_QCameraImageCapture::cameraImageCaptureControl()
{
    MockCameraControl ctrl;
    MockCaptureControl capctrl(&ctrl);
}

QTEST_MAIN(tst_QCameraImageCapture)

#include "tst_qcameraimagecapture.moc"
