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

#include <qvideosink.h>
#include <private/qplatformcamera_p.h>
#include <private/qplatformcameraexposure_p.h>
#include <private/qplatformcamerafocus_p.h>
#include <private/qplatformcameraimagecapture_p.h>
#include <private/qplatformcameraimageprocessing_p.h>
#include <qcamera.h>
#include <qcamerainfo.h>
#include <qcameraimagecapture.h>
#include <qmediacapturesession.h>
#include <qobject.h>
#include <qmediadevices.h>

#include "qmockintegration_p.h"
#include "qmockmediacapturesession.h"
#include "qmockcamerafocus.h"

QT_USE_NAMESPACE


class tst_QCamera: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void init();
    void cleanup();

private slots:
    void testSimpleCamera();
    void testSimpleCameraWhiteBalance();
    void testSimpleCameraExposure();
    void testSimpleCameraFocus();
    void testSimpleCameraCapture();

    void testCameraWhiteBalance();
    void testCameraExposure();
    void testCameraFocus();
    void testCameraCapture();
    void testCameraCaptureMetadata();
    void testImageSettings();
    void testCameraEncodingProperyChange();

    void testConstructor();
    void testQCameraIsAvailable();
    void testErrorSignal();
    void testError();
    void testErrorString();
    void testStatus();


    // Test cases to for focus handling
    void testFocusMode();
    void testZoomChanged();
    void testMaxZoomChangedSignal();

    // Test cases for QPlatformCamera class.
    void testCameraControl();

    // Test case for image processing
    void testContrast();
    void testSaturation();

    void testSetVideoOutput();
    void testSetVideoOutputDestruction();

    void testEnumDebug();

    // Signals test cases for exposure related properties
    void testSignalApertureChanged();
    void testSignalExposureCompensationChanged();
    void testSignalIsoSensitivityChanged();
    void testSignalShutterSpeedChanged();
    void testSignalFlashReady();

private:
    QMockIntegration integration;
};

void tst_QCamera::initTestCase()
{
}

void tst_QCamera::init()
{
}

void tst_QCamera::cleanup()
{
}

void tst_QCamera::testSimpleCamera()
{
    QMockCamera::Simple simple;
    QCamera camera;

    QCOMPARE(camera.isActive(), false);
    camera.start();
    QCOMPARE(camera.isActive(), true);
    camera.stop();
    QCOMPARE(camera.isActive(), false);
}

void tst_QCamera::testSimpleCameraWhiteBalance()
{
    QMockCamera::Simple simple;
    QCamera camera;

    //only WhiteBalanceAuto is supported
    QVERIFY(!camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceAuto));
    QVERIFY(!camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceCloudy));
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceAuto);
    camera.setWhiteBalanceMode(QCamera::WhiteBalanceCloudy);
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceAuto);
    QCOMPARE(camera.manualWhiteBalance()+1.0, 1.0);
    camera.setManualWhiteBalance(5000);
    QCOMPARE(camera.manualWhiteBalance()+1.0, 1.0);
}

void tst_QCamera::testSimpleCameraExposure()
{
    QMockCamera::Simple simple;

    QCamera camera;
    QVERIFY(!camera.isExposureModeSupported(QCamera::ExposureAuto));
    QCOMPARE(camera.exposureMode(), QCamera::ExposureAuto);
    camera.setExposureMode(QCamera::ExposureManual);//should be ignored
    QCOMPARE(camera.exposureMode(), QCamera::ExposureAuto);

    QVERIFY(!camera.isFlashModeSupported(QCamera::FlashOn));
    QCOMPARE(camera.flashMode(), QCamera::FlashOff);
    QCOMPARE(camera.isFlashReady(), false);
    camera.setFlashMode(QCamera::FlashOn);
    QCOMPARE(camera.flashMode(), QCamera::FlashOff);

    QCOMPARE(camera.exposureCompensation(), 0.0);
    camera.setExposureCompensation(2.0);
    QCOMPARE(camera.exposureCompensation(), 0.0);

    QCOMPARE(camera.isoSensitivity(), -1);
    QVERIFY(camera.supportedIsoSensitivities().isEmpty());
    camera.setManualIsoSensitivity(100);
    QCOMPARE(camera.isoSensitivity(), -1);
    camera.setAutoIsoSensitivity();
    QCOMPARE(camera.isoSensitivity(), -1);

    QVERIFY(camera.aperture() < 0);
    QVERIFY(camera.supportedApertures().isEmpty());
    camera.setAutoAperture();
    QVERIFY(camera.aperture() < 0);
    camera.setManualAperture(5.6);
    QVERIFY(camera.aperture() < 0);

    QVERIFY(camera.shutterSpeed() < 0);
    QVERIFY(camera.supportedShutterSpeeds().isEmpty());
    camera.setAutoShutterSpeed();
    QVERIFY(camera.shutterSpeed() < 0);
    camera.setManualShutterSpeed(1/128.0);
    QVERIFY(camera.shutterSpeed() < 0);
}

void tst_QCamera::testSimpleCameraFocus()
{
    QMockCamera::Simple simple;

    QCamera camera;

    QVERIFY(!camera.isFocusModeSupported(QCamera::FocusModeAuto));
    QVERIFY(!camera.isFocusModeSupported(QCamera::FocusModeInfinity));

    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);
    camera.setFocusMode(QCamera::FocusModeAuto);
    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);

    QCOMPARE(camera.maximumZoomFactor(), 1.0);
    QCOMPARE(camera.minimumZoomFactor(), 1.0);
    QCOMPARE(camera.zoomFactor(), 1.0);

    camera.zoomTo(100.0, 100.0);
    QCOMPARE(camera.zoomFactor(), 1.0);

    QCOMPARE(camera.customFocusPoint(), QPointF(-1., -1.));
    camera.setCustomFocusPoint(QPointF(1.0, 1.0));
    QCOMPARE(camera.customFocusPoint(), QPointF(-1, -1));
}

void tst_QCamera::testSimpleCameraCapture()
{
    QMockCamera::Simple simple;

    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(!imageCapture.isReadyForCapture());
    QVERIFY(imageCapture.isAvailable());

    QCOMPARE(imageCapture.error(), QCameraImageCapture::NoError);
    QVERIFY(imageCapture.errorString().isEmpty());

    QSignalSpy errorSignal(&imageCapture, SIGNAL(errorOccurred(int,QCameraImageCapture::Error,QString)));
    imageCapture.captureToFile(QString::fromLatin1("/dev/null"));
    QCOMPARE(errorSignal.size(), 1);
    QCOMPARE(imageCapture.error(), QCameraImageCapture::NotReadyError);
    QVERIFY(!imageCapture.errorString().isEmpty());

    camera.start();
    imageCapture.captureToFile(QString::fromLatin1("/dev/null"));
    QCOMPARE(errorSignal.size(), 1);
    QCOMPARE(imageCapture.error(), QCameraImageCapture::NoError);
    QVERIFY(imageCapture.errorString().isEmpty());
}

void tst_QCamera::testCameraCapture()
{
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(!imageCapture.isReadyForCapture());

    QSignalSpy capturedSignal(&imageCapture, SIGNAL(imageCaptured(int,QImage)));
    QSignalSpy errorSignal(&imageCapture, SIGNAL(errorOccurred(int,QCameraImageCapture::Error,QString)));

    imageCapture.captureToFile(QString::fromLatin1("/dev/null"));
    QCOMPARE(capturedSignal.size(), 0);
    QCOMPARE(errorSignal.size(), 1);
    QCOMPARE(imageCapture.error(), QCameraImageCapture::NotReadyError);

    errorSignal.clear();

    camera.start();
    QVERIFY(imageCapture.isReadyForCapture());
    QCOMPARE(errorSignal.size(), 0);

    imageCapture.captureToFile(QString::fromLatin1("/dev/null"));

    QTRY_COMPARE(capturedSignal.size(), 1);
    QCOMPARE(errorSignal.size(), 0);
    QCOMPARE(imageCapture.error(), QCameraImageCapture::NoError);
}

void tst_QCamera::testCameraCaptureMetadata()
{
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy metadataSignal(&imageCapture, SIGNAL(imageMetadataAvailable(int,const QMediaMetaData&)));
    QSignalSpy savedSignal(&imageCapture, SIGNAL(imageSaved(int,QString)));

    camera.start();
    int id = imageCapture.captureToFile(QString::fromLatin1("/dev/null"));

    QTRY_COMPARE(savedSignal.size(), 1);

    QCOMPARE(metadataSignal.size(), 1);

    QVariantList metadata = metadataSignal[0];
    QCOMPARE(metadata[0].toInt(), id);
    QMediaMetaData data = metadata[1].value<QMediaMetaData>();
    QCOMPARE(data.keys().length(), 2);
    QCOMPARE(data[QMediaMetaData::Author].toString(), "Author");
    QCOMPARE(data[QMediaMetaData::Date].toDateTime().date().year(), 2021);
}


void tst_QCamera::testCameraWhiteBalance()
{
    QSet<QCamera::WhiteBalanceMode> whiteBalanceModes;
    whiteBalanceModes << QCamera::WhiteBalanceAuto;
    whiteBalanceModes << QCamera::WhiteBalanceFlash;
    whiteBalanceModes << QCamera::WhiteBalanceTungsten;
    whiteBalanceModes << QCamera::WhiteBalanceManual;

    QCamera camera;
    QMockCamera *mockCamera = integration.lastCamera();
    mockCamera->mockImageProcessing->setSupportedWhiteBalanceModes(whiteBalanceModes);

    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceAuto);
    camera.setWhiteBalanceMode(QCamera::WhiteBalanceFlash);
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceFlash);
    QVERIFY(camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceAuto));
    QVERIFY(camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceFlash));
    QVERIFY(camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceTungsten));
    QVERIFY(!camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceCloudy));

    camera.setWhiteBalanceMode(QCamera::WhiteBalanceTungsten);
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceTungsten);

    camera.setWhiteBalanceMode(QCamera::WhiteBalanceManual);
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceManual);
    camera.setManualWhiteBalance(34);
    QCOMPARE(camera.manualWhiteBalance(), 34.0);

    camera.setManualWhiteBalance(432.0);
    QCOMPARE(camera.manualWhiteBalance(), 432.0);
}

void tst_QCamera::testCameraExposure()
{
    QCamera camera;

    QVERIFY(camera.isExposureModeSupported(QCamera::ExposureAuto));
    QCOMPARE(camera.exposureMode(), QCamera::ExposureAuto);

    QVERIFY(camera.isExposureModeSupported(QCamera::ExposureManual));
    camera.setExposureMode(QCamera::ExposureManual);
    QCOMPARE(camera.exposureMode(), QCamera::ExposureManual);

    QVERIFY(camera.isExposureModeSupported(QCamera::ExposureNight));
    camera.setExposureMode(QCamera::ExposureNight);
    QCOMPARE(camera.exposureMode(), QCamera::ExposureNight);

    QVERIFY(camera.isExposureModeSupported(QCamera::ExposureSports ));
    camera.setExposureMode(QCamera::ExposureSports);
    QCOMPARE(camera.exposureMode(), QCamera::ExposureSports);

    QVERIFY(camera.isExposureModeSupported(QCamera::ExposureSnow ));
    camera.setExposureMode(QCamera::ExposureSnow);
    QCOMPARE(camera.exposureMode(), QCamera::ExposureSnow);

    QVERIFY(camera.isExposureModeSupported(QCamera::ExposureBeach ));
    camera.setExposureMode(QCamera::ExposureBeach);
    QCOMPARE(camera.exposureMode(), QCamera::ExposureBeach);

    QVERIFY(camera.isExposureModeSupported(QCamera::ExposurePortrait ));
    camera.setExposureMode(QCamera::ExposurePortrait);
    QCOMPARE(camera.exposureMode(), QCamera::ExposurePortrait);


    camera.setFlashMode(QCamera::FlashAuto);
    QCOMPARE(camera.flashMode(), QCamera::FlashAuto);
    QCOMPARE(camera.isFlashReady(), true);
    camera.setFlashMode(QCamera::FlashOn);
    QCOMPARE(camera.flashMode(), QCamera::FlashOn);

    QCOMPARE(camera.exposureCompensation(), 0.0);
    camera.setExposureCompensation(2.0);
    QCOMPARE(camera.exposureCompensation(), 2.0);

    int minIso = camera.supportedIsoSensitivities().first();
    int maxIso = camera.supportedIsoSensitivities().last();
    QVERIFY(camera.isoSensitivity() > 0);
    QCOMPARE(camera.requestedIsoSensitivity(), -1);
    QVERIFY(minIso > 0);
    QVERIFY(maxIso > 0);
    camera.setManualIsoSensitivity(minIso);
    QCOMPARE(camera.isoSensitivity(), minIso);
    camera.setManualIsoSensitivity(maxIso*10);
    QCOMPARE(camera.isoSensitivity(), maxIso);
    QCOMPARE(camera.requestedIsoSensitivity(), maxIso*10);

    camera.setManualIsoSensitivity(-10);
    QCOMPARE(camera.isoSensitivity(), minIso);
    QCOMPARE(camera.requestedIsoSensitivity(), -10);
    camera.setAutoIsoSensitivity();
    QCOMPARE(camera.isoSensitivity(), 100);
    QCOMPARE(camera.requestedIsoSensitivity(), -1);

    QCOMPARE(camera.requestedAperture(), -1.0);
    qreal minAperture = camera.supportedApertures().first();
    qreal maxAperture = camera.supportedApertures().last();
    QVERIFY(minAperture > 0);
    QVERIFY(maxAperture > 0);
    QVERIFY(camera.aperture() >= minAperture);
    QVERIFY(camera.aperture() <= maxAperture);

    camera.setAutoAperture();
    QVERIFY(camera.aperture() >= minAperture);
    QVERIFY(camera.aperture() <= maxAperture);
    QCOMPARE(camera.requestedAperture(), -1.0);

    camera.setManualAperture(0);
    QCOMPARE(camera.aperture(), minAperture);
    QCOMPARE(camera.requestedAperture()+1.0, 1.0);

    camera.setManualAperture(10000);
    QCOMPARE(camera.aperture(), maxAperture);
    QCOMPARE(camera.requestedAperture(), 10000.0);

    camera.setAutoAperture();
    QCOMPARE(camera.requestedAperture(), -1.0);

    QCOMPARE(camera.requestedShutterSpeed(), -1.0);
    qreal minShutterSpeed = camera.supportedShutterSpeeds().first();
    qreal maxShutterSpeed = camera.supportedShutterSpeeds().last();
    QVERIFY(minShutterSpeed > 0);
    QVERIFY(maxShutterSpeed > 0);
    QVERIFY(camera.shutterSpeed() >= minShutterSpeed);
    QVERIFY(camera.shutterSpeed() <= maxShutterSpeed);

    camera.setAutoShutterSpeed();
    QVERIFY(camera.shutterSpeed() >= minShutterSpeed);
    QVERIFY(camera.shutterSpeed() <= maxShutterSpeed);
    QCOMPARE(camera.requestedShutterSpeed(), -1.0);

    camera.setManualShutterSpeed(0);
    QCOMPARE(camera.shutterSpeed(), minShutterSpeed);
    QCOMPARE(camera.requestedShutterSpeed()+1.0, 1.0);

    camera.setManualShutterSpeed(10000);
    QCOMPARE(camera.shutterSpeed(), maxShutterSpeed);
    QCOMPARE(camera.requestedShutterSpeed(), 10000.0);

    camera.setAutoShutterSpeed();
    QCOMPARE(camera.requestedShutterSpeed(), -1.0);
}

void tst_QCamera::testCameraFocus()
{
    QCamera camera;

    QVERIFY(camera.isFocusModeSupported(QCamera::FocusModeAuto));
    QVERIFY(camera.isFocusModeSupported(QCamera::FocusModeAuto));
    QVERIFY(!camera.isFocusModeSupported(QCamera::FocusModeInfinity));

    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);
    camera.setFocusMode(QCamera::FocusModeManual);
    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);
    camera.setFocusMode(QCamera::FocusModeAuto);
    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);

    QVERIFY(camera.maximumZoomFactor() == 4.0);
    QVERIFY(camera.minimumZoomFactor() == 1.0);
    QCOMPARE(camera.zoomFactor(), 1.0);
    camera.zoomTo(0.5, 1.0);
    QCOMPARE(camera.zoomFactor(), 1.0);
    camera.zoomTo(2.0, 0.5);
    QCOMPARE(camera.zoomFactor(), 2.0);
    camera.zoomTo(2.5, 1);
    QCOMPARE(camera.zoomFactor(), 2.5);
    camera.zoomTo(2000000.0, 0);
    QCOMPARE(camera.zoomFactor(), camera.maximumZoomFactor());

    QCOMPARE(camera.customFocusPoint(), QPointF(-1, -1));
    camera.setCustomFocusPoint(QPointF(1.0, 1.0));
    QCOMPARE(camera.customFocusPoint(), QPointF(1.0, 1.0));
}

void tst_QCamera::testImageSettings()
{
    QImageEncoderSettings settings;
    QVERIFY(settings.isNull());
    QVERIFY(settings == QImageEncoderSettings());

    QCOMPARE(settings.format(), QImageEncoderSettings::UnspecifiedFormat);
    settings.setFormat(QImageEncoderSettings::Tiff);
    QCOMPARE(settings.format(), QImageEncoderSettings::Tiff);
    QVERIFY(!settings.isNull());
    QVERIFY(settings != QImageEncoderSettings());

    settings = QImageEncoderSettings();
    QCOMPARE(settings.quality(), QImageEncoderSettings::NormalQuality);
    settings.setQuality(QImageEncoderSettings::HighQuality);
    QCOMPARE(settings.quality(), QImageEncoderSettings::HighQuality);
    QVERIFY(!settings.isNull());

    settings = QImageEncoderSettings();
    QCOMPARE(settings.resolution(), QSize());
    settings.setResolution(QSize(320,240));
    QCOMPARE(settings.resolution(), QSize(320,240));
    settings.setResolution(800,600);
    QCOMPARE(settings.resolution(), QSize(800,600));
    QVERIFY(!settings.isNull());

    settings = QImageEncoderSettings();
    QVERIFY(settings.isNull());
    QCOMPARE(settings.format(), QImageEncoderSettings::UnspecifiedFormat);
    QCOMPARE(settings.quality(), QImageEncoderSettings::NormalQuality);
    QCOMPARE(settings.resolution(), QSize());

    {
        QImageEncoderSettings settings1;
        QImageEncoderSettings settings2;
        QCOMPARE(settings2, settings1);

        settings2 = settings1;
        QCOMPARE(settings2, settings1);
        QVERIFY(settings2.isNull());

        settings1.setQuality(QImageEncoderSettings::HighQuality);

        QVERIFY(settings2.isNull());
        QVERIFY(!settings1.isNull());
        QVERIFY(settings1 != settings2);
    }

    {
        QImageEncoderSettings settings1;
        QImageEncoderSettings settings2(settings1);
        QCOMPARE(settings2, settings1);

        settings2 = settings1;
        QCOMPARE(settings2, settings1);
        QVERIFY(settings2.isNull());

        settings1.setQuality(QImageEncoderSettings::HighQuality);

        QVERIFY(settings2.isNull());
        QVERIFY(!settings1.isNull());
        QVERIFY(settings1 != settings2);
    }

    QImageEncoderSettings settings1;
    QImageEncoderSettings settings2;

    settings1 = QImageEncoderSettings();
    settings1.setResolution(800,600);
    settings2 = QImageEncoderSettings();
    settings2.setResolution(QSize(800,600));
    QVERIFY(settings1 == settings2);
    settings2.setResolution(QSize(400,300));
    QVERIFY(settings1 != settings2);

    settings1 = QImageEncoderSettings();
    settings1.setFormat(QImageEncoderSettings::PNG);
    settings2 = QImageEncoderSettings();
    settings2.setFormat(QImageEncoderSettings::PNG);
    QVERIFY(settings1 == settings2);
    settings2.setFormat(QImageEncoderSettings::Tiff);
    QVERIFY(settings1 != settings2);

    settings1 = QImageEncoderSettings();
    settings1.setQuality(QImageEncoderSettings::NormalQuality);
    settings2 = QImageEncoderSettings();
    settings2.setQuality(QImageEncoderSettings::NormalQuality);
    QVERIFY(settings1 == settings2);
    settings2.setQuality(QImageEncoderSettings::LowQuality);
    QVERIFY(settings1 != settings2);
}

void tst_QCamera::testCameraEncodingProperyChange()
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
    statusChangedSignal.clear();
}

void tst_QCamera::testSetVideoOutput()
{
    QMediaCaptureSession session;
    QVideoSink surface;
    QCamera camera;
    session.setCamera(&camera);

    QVERIFY(session.videoOutput().isNull());
    session.setVideoOutput(&surface);

    QVERIFY(session.videoOutput().value<QVideoSink *>() == &surface);

    session.setVideoOutput(static_cast<QVideoSink *>(nullptr));
    QVERIFY(session.videoOutput().isNull());
}

void tst_QCamera::testSetVideoOutputDestruction()
{
    {
        QVideoSink surface;
        {
            QMediaCaptureSession session;
            QCamera camera;
            session.setCamera(&camera);

            QVERIFY(session.videoOutput().isNull());
            session.setVideoOutput(&surface);
        }
    }

    {
        QMediaCaptureSession session;
        {
            QVideoSink surface;
            QCamera camera;
            session.setCamera(&camera);

            QVERIFY(session.videoOutput().isNull());
            session.setVideoOutput(&surface);
        }
    }

    {
        QCamera camera;
        {
            QMediaCaptureSession session;
            QVideoSink surface;
            session.setCamera(&camera);

            QVERIFY(session.videoOutput().isNull());
            session.setVideoOutput(&surface);
        }
    }
    {
        QMediaCaptureSession session;
        {
            QVideoSink surface;
            QCamera camera;
            session.setCamera(&camera);

            QVERIFY(session.videoOutput().isNull());
            session.setVideoOutput(&surface);
        }
    }
}

void tst_QCamera::testEnumDebug()
{
    QTest::ignoreMessage(QtDebugMsg, "QCamera::ActiveStatus");
    qDebug() << QCamera::ActiveStatus;
    QTest::ignoreMessage(QtDebugMsg, "QCamera::CameraError");
    qDebug() << QCamera::CameraError;
//    QTest::ignoreMessage(QtDebugMsg, "QCameraInfo::FrontFace");
//    qDebug() << QCameraInfo::FrontFace;
}

void tst_QCamera::testCameraControl()
{
    QCamera camera;
    QMockCamera *m_cameraControl = new QMockCamera(&camera);
    QVERIFY(m_cameraControl != nullptr);
}

void tst_QCamera::testConstructor()
{
    auto cameras = QMediaDevices::videoInputs();
    QCameraInfo defaultCamera = QMediaDevices::defaultVideoInput();
    QCameraInfo frontCamera, backCamera;
    for (const auto &c : cameras) {
        if (frontCamera.isNull() && c.position() == QCameraInfo::FrontFace)
            frontCamera = c;
        if (backCamera.isNull() && c.position() == QCameraInfo::BackFace)
            backCamera = c;
    }
    QVERIFY(!defaultCamera.isNull());
    QVERIFY(!frontCamera.isNull());
    QVERIFY(!backCamera.isNull());

    {
        QCamera camera;
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraInfo(), defaultCamera);
    }

    {
        QCamera camera(QCameraInfo::FrontFace);
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraInfo(), frontCamera);
    }

    {
        QCamera camera(QMediaDevices::defaultVideoInput());
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraInfo(), defaultCamera);
    }

    {
        QCameraInfo cameraInfo = QMediaDevices::videoInputs().at(0);
        QCamera camera(cameraInfo);
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraInfo(), cameraInfo);
    }

    {
        QCamera camera(QCameraInfo::BackFace);
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraInfo(), backCamera);
    }

    {
        // Should load the default camera when UnspecifiedPosition is requested
        QCamera camera(QCameraInfo::UnspecifiedPosition);
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraInfo(), defaultCamera);
    }
}

/* Test case for isAvailable */
void tst_QCamera::testQCameraIsAvailable()
{
    QCamera camera;
    QVERIFY(camera.isAvailable());
}

/* Test case for verifying if error signal generated correctly */
void tst_QCamera::testErrorSignal()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);
    auto *service = integration.lastCaptureService();
    Q_ASSERT(service);
    Q_ASSERT(service->mockCameraControl);

    QSignalSpy spyError(&camera, SIGNAL(errorOccurred(QCamera::Error,const QString&)));

    /* Set the QPlatformCamera error and verify if the signal is emitted correctly in QCamera */
    service->mockCameraControl->setError(QCamera::CameraError,QString("Camera Error"));

    QVERIFY(spyError.count() == 1);
    QCamera::Error err = qvariant_cast<QCamera::Error >(spyError.at(0).at(0));
    QVERIFY(err == QCamera::CameraError);

    spyError.clear();

    /* Set the QPlatformCamera error and verify if the signal is emitted correctly in QCamera */
    service->mockCameraControl->setError(QCamera::CameraError,QString("InvalidRequestError Error"));
    QVERIFY(spyError.count() == 1);
    err = qvariant_cast<QCamera::Error >(spyError.at(0).at(0));
    QVERIFY(err == QCamera::CameraError);

    spyError.clear();

    /* Set the QPlatformCamera error and verify if the signal is emitted correctly in QCamera */
    service->mockCameraControl->setError(QCamera::CameraError,QString("NotSupportedFeatureError Error"));
    QVERIFY(spyError.count() == 1);
    err = qvariant_cast<QCamera::Error >(spyError.at(0).at(0));
    QVERIFY(err == QCamera::CameraError);

}

/* Test case for verifying the QCamera error */
void tst_QCamera::testError()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);
    auto *service = integration.lastCaptureService();

    /* Set the QPlatformCamera error and verify if it is set correctly in QCamera */
    service->mockCameraControl->setError(QCamera::CameraError,QString("Camera Error"));
    QVERIFY(camera.error() == QCamera::CameraError);

    /* Set the QPlatformCamera error and verify if it is set correctly in QCamera */
    service->mockCameraControl->setError(QCamera::CameraError,QString("InvalidRequestError Error"));
    QVERIFY(camera.error() == QCamera::CameraError);

    /* Set the QPlatformCamera error and verify if it is set correctly in QCamera */
    service->mockCameraControl->setError(QCamera::CameraError,QString("CameraError Error"));
    QVERIFY(camera.error() == QCamera::CameraError);

}

/* Test the error strings for QCamera class */
void tst_QCamera::testErrorString()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);
    auto *service = integration.lastCaptureService();

    /* Set the QPlatformCamera error and verify if it is set correctly in QCamera */
    service->mockCameraControl->setError(QCamera::CameraError,QString("Camera Error"));
    QVERIFY(camera.errorString() == QString("Camera Error"));

    /* Set the QPlatformCamera error and verify if it is set correctly in QCamera */
    service->mockCameraControl->setError(QCamera::CameraError,QString("InvalidRequestError Error"));
    QVERIFY(camera.errorString() == QString("InvalidRequestError Error"));

    /* Set the QPlatformCamera error and verify if it is set correctly in QCamera */
    service->mockCameraControl->setError(QCamera::CameraError,QString("CameraError Error"));
    QVERIFY(camera.errorString() == QString("CameraError Error"));
}

/* Test case for verifying Status of QCamera. */
void tst_QCamera::testStatus()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);
    auto *service = integration.lastCaptureService();

    /* Set the QPlatformCamera status and verify if it is set correctly in QCamera */
    service->mockCameraControl->setStatus(QCamera::StartingStatus);
    QVERIFY(camera.status() == QCamera::StartingStatus);

    /* Set the QPlatformCamera status and verify if it is set correctly in QCamera */
    service->mockCameraControl->setStatus(QCamera::StartingStatus);
    QVERIFY(camera.status() == QCamera::StartingStatus);

    /* Set the QPlatformCamera status and verify if it is set correctly in QCamera */
    service->mockCameraControl->setStatus(QCamera::UnavailableStatus);
    QVERIFY(camera.status() == QCamera::UnavailableStatus);
}

void tst_QCamera::testContrast()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QVERIFY(camera.contrast() == 0);

    camera.setContrast(0.123);
    QCOMPARE(camera.contrast(), 0.123);

    camera.setContrast(4.56);
    QCOMPARE(camera.contrast(), 4.56);
}

void tst_QCamera::testSaturation()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QCOMPARE(camera.saturation()+1.0, 1.0);

    camera.setSaturation(0.5);
    QCOMPARE(camera.saturation(), 0.5);

    camera.setSaturation(-0.5);
    QCOMPARE(camera.saturation(), -0.5);
}

//Added this code to cover QCamera::FocusModeHyperfocal and QCamera::FocusModeAutoNear
//As the FocusModeHyperfocal and FocusModeAutoNear are not supported we can not set the focus mode to these Focus Modes
void tst_QCamera::testFocusMode()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QVERIFY(!camera.isFocusModeSupported(QCamera::FocusModeHyperfocal));
    QVERIFY(!camera.isFocusModeSupported(QCamera::FocusModeAutoNear));
    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);
    camera.setFocusMode(QCamera::FocusModeHyperfocal);
    QVERIFY(camera.focusMode()!= QCamera::FocusModeHyperfocal);
    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);
    camera.setFocusMode(QCamera::FocusModeAutoNear);
    QVERIFY(camera.focusMode()!= QCamera::FocusModeAutoNear);
    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);
}

void tst_QCamera::testZoomChanged()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy spy(&camera, SIGNAL(zoomFactorChanged(float)));
    QVERIFY(spy.count() == 0);
    camera.setZoomFactor(2.0);
    QVERIFY(spy.count() == 1);
    camera.zoomTo(3.0, 1);
    QVERIFY(spy.count() == 2);
    camera.zoomTo(1.0, 0);
    QVERIFY(spy.count() == 3);
}

void tst_QCamera::testMaxZoomChangedSignal()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);
    QMockCamera *mock = integration.lastCamera();
    QMockCameraFocus *mockFocus = mock->mockFocus;

    // ### change max zoom factor on backend, e.g. by changing camera
    QSignalSpy spy(&camera, SIGNAL(maximumZoomFactorChanged(float)));
    mockFocus->setMaxZoomFactor(55);
    QVERIFY(spy.count() == 1);
    QCOMPARE(camera.maximumZoomFactor(), 55);
}

void tst_QCamera::testSignalApertureChanged()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy spyApertureChanged(&camera, SIGNAL(apertureChanged(qreal)));
    QSignalSpy spyApertureRangeChanged(&camera, SIGNAL(apertureRangeChanged()));


    QVERIFY(spyApertureChanged.count() ==0);
    camera.setManualAperture(10.0);//set the ManualAperture to 10.0

    QTest::qWait(100);
    QVERIFY(spyApertureChanged.count() ==1);
    QVERIFY(spyApertureRangeChanged.count() ==1);
}

void tst_QCamera::testSignalExposureCompensationChanged()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy spyExposureCompensationChanged(&camera, SIGNAL(exposureCompensationChanged(qreal)));

    QVERIFY(spyExposureCompensationChanged.count() ==0);

    QVERIFY(camera.exposureCompensation() != 800);
    camera.setExposureCompensation(2.0);

    QTest::qWait(100);

    QVERIFY(camera.exposureCompensation() == 2.0);

    QCOMPARE(spyExposureCompensationChanged.count(),1);

    // Setting the same should not result in a signal
    camera.setExposureCompensation(2.0);
    QTest::qWait(100);

    QVERIFY(camera.exposureCompensation() == 2.0);
    QCOMPARE(spyExposureCompensationChanged.count(),1);
}

void tst_QCamera::testSignalIsoSensitivityChanged()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy spyisoSensitivityChanged(&camera, SIGNAL(isoSensitivityChanged(int)));

    QVERIFY(spyisoSensitivityChanged.count() ==0);

    camera.setManualIsoSensitivity(800); //set the manualiso sentivity to 800
    QTest::qWait(100);
    QVERIFY(spyisoSensitivityChanged.count() ==1);

}
void tst_QCamera::testSignalShutterSpeedChanged()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy spySignalShutterSpeedChanged(&camera, SIGNAL(shutterSpeedChanged(qreal)));
    QSignalSpy spySignalShutterSpeedRangeChanged(&camera, SIGNAL(shutterSpeedRangeChanged()));

    QVERIFY(spySignalShutterSpeedChanged.count() ==0);

    camera.setManualShutterSpeed(2.0);//set the ManualShutterSpeed to 2.0
    QTest::qWait(100);

    QVERIFY(spySignalShutterSpeedChanged.count() ==1);
    QVERIFY(spySignalShutterSpeedRangeChanged.count() ==1);
}

void tst_QCamera::testSignalFlashReady()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy spyflashReady(&camera,SIGNAL(flashReady(bool)));

    QVERIFY(spyflashReady.count() ==0);

    QVERIFY(camera.flashMode() ==QCamera::FlashAuto);

    camera.setFlashMode(QCamera::FlashOff);//set theFlashMode to QCamera::FlashOff

    QVERIFY(camera.flashMode() ==QCamera::FlashOff);

    QVERIFY(spyflashReady.count() ==1);
}

QTEST_MAIN(tst_QCamera)

#include "tst_qcamera.moc"
