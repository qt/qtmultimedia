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
#include <qcamera.h>
#include <qcamerainfo.h>
#include <qcameraimagecapture.h>
#include <qobject.h>
#include <qmediadevicemanager.h>

#include "mockvideosurface.h"
#include "qmockintegration_p.h"
#include "mockmediarecorderservice.h"

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
    void testSimpleCaptureDestination();

    void testCameraWhiteBalance();
    void testCameraExposure();
    void testCameraFocus();
    void testCameraCapture();
    void testCameraCaptureMetadata();
    void testImageSettings();
    void testCameraEncodingProperyChange();
    void testCaptureDestination();

    void testConstructor();
    void testQCameraIsAvailable();
    void testQCameraIsNotAvailable();
    void testErrorSignal();
    void testError();
    void testErrorString();
    void testStatus();


    // Test cases to for QCameraFocus
    void testCameraFocusIsAvailable();
    void testFocusModes();
    void testOpticalAndDigitalZoomChanged();
    void testMaxOpticalZoomChangedSignal();
    void testMaxDigitalZoomChangedSignal();

    // Test cases for QPlatformCamera class.
    void testCameraControl();
    void testEnumsOfQPlatformCamera();

    // Test case for QCameraImageProcessing class
    void testContrast();
    void testIsAvailable();
    void testSaturation();

    void testSetVideoOutput();
    void testSetVideoOutputNoService();
    void testSetVideoOutputDestruction();

    void testEnumDebug();

    // constructor for QCameraImageProceesing
    void testImageProcessingControl();

    // Signals test cases for QCameraExposure
    void testSignalApertureChanged();
    void testSignalExposureCompensationChanged();
    void testSignalIsoSensitivityChanged();
    void testSignalShutterSpeedChanged();
    void testSignalFlashReady();

    // test constructor
    void testExposureControlConstructor();

private:
    QMockIntegration *integration;
};

void tst_QCamera::initTestCase()
{
    MockMediaRecorderService::simpleCamera = false;
}

void tst_QCamera::init()
{
    integration = new QMockIntegration;
}

void tst_QCamera::cleanup()
{
    delete integration;
}

void tst_QCamera::testSimpleCamera()
{
    MockMediaRecorderService::simpleCamera = true;
    QCamera camera;

    QCOMPARE(camera.state(), QCamera::UnloadedState);
    camera.start();
    QCOMPARE(camera.state(), QCamera::ActiveState);
    camera.stop();
    QCOMPARE(camera.state(), QCamera::LoadedState);
    camera.unload();
    QCOMPARE(camera.state(), QCamera::UnloadedState);
    camera.load();
    QCOMPARE(camera.state(), QCamera::LoadedState);
}

void tst_QCamera::testSimpleCameraWhiteBalance()
{
    MockMediaRecorderService::simpleCamera = true;
    QCamera camera;

    //only WhiteBalanceAuto is supported
    QVERIFY(!camera.imageProcessing()->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceAuto));
    QVERIFY(!camera.imageProcessing()->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceCloudy));
    QCOMPARE(camera.imageProcessing()->whiteBalanceMode(), QCameraImageProcessing::WhiteBalanceAuto);
    camera.imageProcessing()->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceCloudy);
    QCOMPARE(camera.imageProcessing()->whiteBalanceMode(), QCameraImageProcessing::WhiteBalanceAuto);
    QCOMPARE(camera.imageProcessing()->manualWhiteBalance()+1.0, 1.0);
    camera.imageProcessing()->setManualWhiteBalance(5000);
    QCOMPARE(camera.imageProcessing()->manualWhiteBalance()+1.0, 1.0);
}

void tst_QCamera::testSimpleCameraExposure()
{
    MockMediaRecorderService::simpleCamera = true;

    QCamera camera;
    QCameraExposure *cameraExposure = camera.exposure();
    QVERIFY(cameraExposure != nullptr);

    QVERIFY(!cameraExposure->isExposureModeSupported(QCameraExposure::ExposureAuto));
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureAuto);
    cameraExposure->setExposureMode(QCameraExposure::ExposureManual);//should be ignored
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureAuto);

    QVERIFY(!cameraExposure->isFlashModeSupported(QCameraExposure::FlashOff));
    QCOMPARE(cameraExposure->flashMode(), QCameraExposure::FlashOff);
    QCOMPARE(cameraExposure->isFlashReady(), false);
    cameraExposure->setFlashMode(QCameraExposure::FlashOn);
    QCOMPARE(cameraExposure->flashMode(), QCameraExposure::FlashOff);

    QCOMPARE(cameraExposure->exposureCompensation(), 0.0);
    cameraExposure->setExposureCompensation(2.0);
    QCOMPARE(cameraExposure->exposureCompensation(), 0.0);

    QCOMPARE(cameraExposure->isoSensitivity(), -1);
    QVERIFY(cameraExposure->supportedIsoSensitivities().isEmpty());
    cameraExposure->setManualIsoSensitivity(100);
    QCOMPARE(cameraExposure->isoSensitivity(), -1);
    cameraExposure->setAutoIsoSensitivity();
    QCOMPARE(cameraExposure->isoSensitivity(), -1);

    QVERIFY(cameraExposure->aperture() < 0);
    QVERIFY(cameraExposure->supportedApertures().isEmpty());
    cameraExposure->setAutoAperture();
    QVERIFY(cameraExposure->aperture() < 0);
    cameraExposure->setManualAperture(5.6);
    QVERIFY(cameraExposure->aperture() < 0);

    QVERIFY(cameraExposure->shutterSpeed() < 0);
    QVERIFY(cameraExposure->supportedShutterSpeeds().isEmpty());
    cameraExposure->setAutoShutterSpeed();
    QVERIFY(cameraExposure->shutterSpeed() < 0);
    cameraExposure->setManualShutterSpeed(1/128.0);
    QVERIFY(cameraExposure->shutterSpeed() < 0);
}

void tst_QCamera::testSimpleCameraFocus()
{
    MockMediaRecorderService::simpleCamera = true;

    QCamera camera;

    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != nullptr);

    QVERIFY(!cameraFocus->isFocusModeSupported(QCameraFocus::AutoFocus));
    QVERIFY(!cameraFocus->isFocusModeSupported(QCameraFocus::ContinuousFocus));
    QVERIFY(!cameraFocus->isFocusModeSupported(QCameraFocus::InfinityFocus));

    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::AutoFocus);
    QTest::ignoreMessage(QtWarningMsg, "Focus mode selection is not supported");
    cameraFocus->setFocusMode(QCameraFocus::ContinuousFocus);
    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::AutoFocus);

    QCOMPARE(cameraFocus->maximumZoomFactor(), 1.0);
    QCOMPARE(cameraFocus->minimumZoomFactor(), 1.0);
    QCOMPARE(cameraFocus->zoomFactor(), 1.0);

    QTest::ignoreMessage(QtWarningMsg, "The camera doesn't support zooming.");
    cameraFocus->zoomTo(100.0, 100.0);
    QCOMPARE(cameraFocus->zoomFactor(), 1.0);

    QVERIFY(!cameraFocus->isFocusPointModeSupported(QCameraFocus::FocusPointAuto));
    QCOMPARE(cameraFocus->focusPointMode(), QCameraFocus::FocusPointAuto);

    QTest::ignoreMessage(QtWarningMsg, "Focus points mode selection is not supported");
    cameraFocus->setFocusPointMode( QCameraFocus::FocusPointCenter );
    QCOMPARE(cameraFocus->focusPointMode(), QCameraFocus::FocusPointAuto);

    QCOMPARE(cameraFocus->customFocusPoint(), QPointF(0.5, 0.5));
    QTest::ignoreMessage(QtWarningMsg, "Focus points selection is not supported");
    cameraFocus->setCustomFocusPoint(QPointF(1.0, 1.0));
    QCOMPARE(cameraFocus->customFocusPoint(), QPointF(0.5, 0.5));
}

void tst_QCamera::testSimpleCameraCapture()
{
    MockMediaRecorderService::simpleCamera = true;

    QCamera camera;
    QCameraImageCapture imageCapture(&camera);

    QVERIFY(!imageCapture.isReadyForCapture());
    QVERIFY(!imageCapture.isAvailable());

    QCOMPARE(imageCapture.error(), QCameraImageCapture::NoError);
    QVERIFY(imageCapture.errorString().isEmpty());

    QSignalSpy errorSignal(&imageCapture, SIGNAL(error(int,QCameraImageCapture::Error,QString)));
    imageCapture.capture(QString::fromLatin1("/dev/null"));
    QCOMPARE(errorSignal.size(), 1);
    QCOMPARE(imageCapture.error(), QCameraImageCapture::NotSupportedFeatureError);
    QVERIFY(!imageCapture.errorString().isEmpty());
}

void tst_QCamera::testSimpleCaptureDestination()
{
    MockMediaRecorderService::simpleCamera = true;

    QCamera camera;
    QCameraImageCapture imageCapture(&camera);

    QCOMPARE(imageCapture.captureDestination(), QCameraImageCapture::CaptureToFile);
    imageCapture.setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    QCOMPARE(imageCapture.captureDestination(), QCameraImageCapture::CaptureToFile);
}

void tst_QCamera::testCaptureDestination()
{
    QCamera camera;
    QCameraImageCapture imageCapture(&camera);

    QSignalSpy destinationChangedSignal(&imageCapture, SIGNAL(captureDestinationChanged(QCameraImageCapture::CaptureDestinations)));

    QCOMPARE(imageCapture.captureDestination(), QCameraImageCapture::CaptureToFile);
    imageCapture.setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    QCOMPARE(imageCapture.captureDestination(), QCameraImageCapture::CaptureToBuffer);
    QCOMPARE(destinationChangedSignal.size(), 1);
    QCOMPARE(destinationChangedSignal.first().first().value<QCameraImageCapture::CaptureDestinations>(),
             QCameraImageCapture::CaptureToBuffer);

    imageCapture.setCaptureDestination(QCameraImageCapture::CaptureToBuffer | QCameraImageCapture::CaptureToFile);
    QCOMPARE(imageCapture.captureDestination(), QCameraImageCapture::CaptureToBuffer | QCameraImageCapture::CaptureToFile);
    QCOMPARE(destinationChangedSignal.size(), 2);
}

void tst_QCamera::testCameraCapture()
{
    QCamera camera;
    QCameraImageCapture imageCapture(&camera);

    QVERIFY(!imageCapture.isReadyForCapture());

    QSignalSpy capturedSignal(&imageCapture, SIGNAL(imageCaptured(int,QImage)));
    QSignalSpy errorSignal(&imageCapture, SIGNAL(error(int,QCameraImageCapture::Error,QString)));

    imageCapture.capture(QString::fromLatin1("/dev/null"));
    QCOMPARE(capturedSignal.size(), 0);
    QCOMPARE(errorSignal.size(), 1);
    QCOMPARE(imageCapture.error(), QCameraImageCapture::NotReadyError);

    errorSignal.clear();

    camera.start();
    QVERIFY(imageCapture.isReadyForCapture());
    QCOMPARE(errorSignal.size(), 0);

    imageCapture.capture(QString::fromLatin1("/dev/null"));

    QTRY_COMPARE(capturedSignal.size(), 1);
    QCOMPARE(errorSignal.size(), 0);
    QCOMPARE(imageCapture.error(), QCameraImageCapture::NoError);
}

void tst_QCamera::testCameraCaptureMetadata()
{
    QCamera camera;
    QCameraImageCapture imageCapture(&camera);

    QSignalSpy metadataSignal(&imageCapture, SIGNAL(imageMetadataAvailable(int,const QMediaMetaData&)));
    QSignalSpy savedSignal(&imageCapture, SIGNAL(imageSaved(int,QString)));

    camera.start();
    int id = imageCapture.capture(QString::fromLatin1("/dev/null"));

    QTRY_COMPARE(savedSignal.size(), 1);

    QCOMPARE(metadataSignal.size(), 1);

    QVariantList metadata = metadataSignal[0];
    QCOMPARE(metadata[0].toInt(), id);
    QMediaMetaData data = metadata[1].value<QMediaMetaData>();
    QCOMPARE(data.keys().length(), 2);
    QCOMPARE(data[QMediaMetaData::Author].toString(), "Author");
    QCOMPARE(data[QMediaMetaData::Year].toInt(), 2021);
}


void tst_QCamera::testCameraWhiteBalance()
{
    QSet<QCameraImageProcessing::WhiteBalanceMode> whiteBalanceModes;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceAuto;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceFlash;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceTungsten;

    QCamera camera;
    auto *service = integration->lastCaptureService();
    service->mockImageProcessingControl->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceFlash);
    service->mockImageProcessingControl->setSupportedWhiteBalanceModes(whiteBalanceModes);
    service->mockImageProcessingControl->setParameter(
                QPlatformCameraImageProcessing::ColorTemperature,
                QVariant(34));

    QCameraImageProcessing *cameraImageProcessing = camera.imageProcessing();

    QCOMPARE(cameraImageProcessing->whiteBalanceMode(), QCameraImageProcessing::WhiteBalanceFlash);
    QVERIFY(camera.imageProcessing()->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceAuto));
    QVERIFY(camera.imageProcessing()->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceFlash));
    QVERIFY(camera.imageProcessing()->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceTungsten));
    QVERIFY(!camera.imageProcessing()->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceCloudy));

    cameraImageProcessing->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceTungsten);
    QCOMPARE(cameraImageProcessing->whiteBalanceMode(), QCameraImageProcessing::WhiteBalanceTungsten);

    cameraImageProcessing->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceManual);
    QCOMPARE(cameraImageProcessing->whiteBalanceMode(), QCameraImageProcessing::WhiteBalanceManual);
    QCOMPARE(cameraImageProcessing->manualWhiteBalance(), 34.0);

    cameraImageProcessing->setManualWhiteBalance(432.0);
    QCOMPARE(cameraImageProcessing->manualWhiteBalance(), 432.0);
}

void tst_QCamera::testCameraExposure()
{
    QCamera camera;

    QCameraExposure *cameraExposure = camera.exposure();
    QVERIFY(cameraExposure != nullptr);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureAuto));
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureAuto);

    // Test Cases For QCameraExposure
    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureManual));
    cameraExposure->setExposureMode(QCameraExposure::ExposureManual);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureManual);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureNight));
    cameraExposure->setExposureMode(QCameraExposure::ExposureNight);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureNight);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureSports ));
    cameraExposure->setExposureMode(QCameraExposure::ExposureSports);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureSports);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureSnow ));
    cameraExposure->setExposureMode(QCameraExposure::ExposureSnow);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureSnow);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureBeach ));
    cameraExposure->setExposureMode(QCameraExposure::ExposureBeach);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureBeach);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposurePortrait ));
    cameraExposure->setExposureMode(QCameraExposure::ExposurePortrait);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposurePortrait);


    cameraExposure->setFlashMode(QCameraExposure::FlashAuto);
    QCOMPARE(cameraExposure->flashMode(), QCameraExposure::FlashAuto);
    QCOMPARE(cameraExposure->isFlashReady(), true);
    cameraExposure->setFlashMode(QCameraExposure::FlashOn);
    QCOMPARE(cameraExposure->flashMode(), QCameraExposure::FlashOn);

    QCOMPARE(cameraExposure->exposureCompensation(), 0.0);
    cameraExposure->setExposureCompensation(2.0);
    QCOMPARE(cameraExposure->exposureCompensation(), 2.0);

    int minIso = cameraExposure->supportedIsoSensitivities().first();
    int maxIso = cameraExposure->supportedIsoSensitivities().last();
    QVERIFY(cameraExposure->isoSensitivity() > 0);
    QCOMPARE(cameraExposure->requestedIsoSensitivity(), -1);
    QVERIFY(minIso > 0);
    QVERIFY(maxIso > 0);
    cameraExposure->setManualIsoSensitivity(minIso);
    QCOMPARE(cameraExposure->isoSensitivity(), minIso);
    cameraExposure->setManualIsoSensitivity(maxIso*10);
    QCOMPARE(cameraExposure->isoSensitivity(), maxIso);
    QCOMPARE(cameraExposure->requestedIsoSensitivity(), maxIso*10);

    cameraExposure->setManualIsoSensitivity(-10);
    QCOMPARE(cameraExposure->isoSensitivity(), minIso);
    QCOMPARE(cameraExposure->requestedIsoSensitivity(), -10);
    cameraExposure->setAutoIsoSensitivity();
    QCOMPARE(cameraExposure->isoSensitivity(), 100);
    QCOMPARE(cameraExposure->requestedIsoSensitivity(), -1);

    QCOMPARE(cameraExposure->requestedAperture(), -1.0);
    qreal minAperture = cameraExposure->supportedApertures().first();
    qreal maxAperture = cameraExposure->supportedApertures().last();
    QVERIFY(minAperture > 0);
    QVERIFY(maxAperture > 0);
    QVERIFY(cameraExposure->aperture() >= minAperture);
    QVERIFY(cameraExposure->aperture() <= maxAperture);

    cameraExposure->setAutoAperture();
    QVERIFY(cameraExposure->aperture() >= minAperture);
    QVERIFY(cameraExposure->aperture() <= maxAperture);
    QCOMPARE(cameraExposure->requestedAperture(), -1.0);

    cameraExposure->setManualAperture(0);
    QCOMPARE(cameraExposure->aperture(), minAperture);
    QCOMPARE(cameraExposure->requestedAperture()+1.0, 1.0);

    cameraExposure->setManualAperture(10000);
    QCOMPARE(cameraExposure->aperture(), maxAperture);
    QCOMPARE(cameraExposure->requestedAperture(), 10000.0);

    cameraExposure->setAutoAperture();
    QCOMPARE(cameraExposure->requestedAperture(), -1.0);

    QCOMPARE(cameraExposure->requestedShutterSpeed(), -1.0);
    qreal minShutterSpeed = cameraExposure->supportedShutterSpeeds().first();
    qreal maxShutterSpeed = cameraExposure->supportedShutterSpeeds().last();
    QVERIFY(minShutterSpeed > 0);
    QVERIFY(maxShutterSpeed > 0);
    QVERIFY(cameraExposure->shutterSpeed() >= minShutterSpeed);
    QVERIFY(cameraExposure->shutterSpeed() <= maxShutterSpeed);

    cameraExposure->setAutoShutterSpeed();
    QVERIFY(cameraExposure->shutterSpeed() >= minShutterSpeed);
    QVERIFY(cameraExposure->shutterSpeed() <= maxShutterSpeed);
    QCOMPARE(cameraExposure->requestedShutterSpeed(), -1.0);

    cameraExposure->setManualShutterSpeed(0);
    QCOMPARE(cameraExposure->shutterSpeed(), minShutterSpeed);
    QCOMPARE(cameraExposure->requestedShutterSpeed()+1.0, 1.0);

    cameraExposure->setManualShutterSpeed(10000);
    QCOMPARE(cameraExposure->shutterSpeed(), maxShutterSpeed);
    QCOMPARE(cameraExposure->requestedShutterSpeed(), 10000.0);

    cameraExposure->setAutoShutterSpeed();
    QCOMPARE(cameraExposure->requestedShutterSpeed(), -1.0);
}

void tst_QCamera::testCameraFocus()
{
    QCamera camera;

    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != nullptr);

    QVERIFY(cameraFocus->isFocusModeSupported(QCameraFocus::AutoFocus));
    QVERIFY(cameraFocus->isFocusModeSupported(QCameraFocus::ContinuousFocus));
    QVERIFY(!cameraFocus->isFocusModeSupported(QCameraFocus::InfinityFocus));

    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::AutoFocus);
    cameraFocus->setFocusMode(QCameraFocus::ManualFocus);
    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::AutoFocus);
    cameraFocus->setFocusMode(QCameraFocus::ContinuousFocus);
    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::ContinuousFocus);

    QVERIFY(cameraFocus->maximumZoomFactor() >= 1.0);
    QVERIFY(cameraFocus->minimumZoomFactor() == 1.0);
    QCOMPARE(cameraFocus->zoomFactor(), 1.0);
    cameraFocus->zoomTo(0.5, 1.0);
    QCOMPARE(cameraFocus->zoomFactor(), 1.0);
    cameraFocus->zoomTo(2.0, 0.5);
    QCOMPARE(cameraFocus->zoomFactor(), 1.0);
    cameraFocus->zoomTo(2.0, 2.5);
    QCOMPARE(cameraFocus->zoomFactor(), 2.5);
    cameraFocus->zoomTo(2000000.0, -1);
    QVERIFY(qFuzzyCompare(cameraFocus->zoomFactor(), cameraFocus->maximumZoomFactor()));

    QVERIFY(cameraFocus->isFocusPointModeSupported(QCameraFocus::FocusPointAuto));
    QVERIFY(cameraFocus->isFocusPointModeSupported(QCameraFocus::FocusPointCenter));
    QVERIFY(cameraFocus->isFocusPointModeSupported(QCameraFocus::FocusPointCustom));
    QCOMPARE(cameraFocus->focusPointMode(), QCameraFocus::FocusPointAuto);

    cameraFocus->setFocusPointMode( QCameraFocus::FocusPointCenter );
    QCOMPARE(cameraFocus->focusPointMode(), QCameraFocus::FocusPointCenter);

    cameraFocus->setFocusPointMode( QCameraFocus::FocusPointFaceDetection );
    QCOMPARE(cameraFocus->focusPointMode(), QCameraFocus::FocusPointCenter);

    QCOMPARE(cameraFocus->customFocusPoint(), QPointF(0.5, 0.5));
    cameraFocus->setCustomFocusPoint(QPointF(1.0, 1.0));
    QCOMPARE(cameraFocus->customFocusPoint(), QPointF(1.0, 1.0));
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
    QCOMPARE(settings.quality(), QMultimedia::NormalQuality);
    settings.setQuality(QMultimedia::HighQuality);
    QCOMPARE(settings.quality(), QMultimedia::HighQuality);
    QVERIFY(!settings.isNull());

    settings = QImageEncoderSettings();
    QCOMPARE(settings.resolution(), QSize());
    settings.setResolution(QSize(320,240));
    QCOMPARE(settings.resolution(), QSize(320,240));
    settings.setResolution(800,600);
    QCOMPARE(settings.resolution(), QSize(800,600));
    QVERIFY(!settings.isNull());

    settings = QImageEncoderSettings();
    settings.setEncodingOption(QLatin1String("encoderOption"), QVariant(1));
    QCOMPARE(settings.encodingOption(QLatin1String("encoderOption")), QVariant(1));
    QVariantMap options;
    options.insert(QLatin1String("encoderOption"), QVariant(1));
    QCOMPARE(settings.encodingOptions(), options);
    options.insert(QLatin1String("encoderOption2"), QVariant(2));
    options.remove(QLatin1String("encoderOption"));
    settings.setEncodingOptions(options);
    QCOMPARE(settings.encodingOption(QLatin1String("encoderOption")), QVariant());
    QCOMPARE(settings.encodingOption(QLatin1String("encoderOption2")), QVariant(2));
    QVERIFY(!settings.isNull());
    QVERIFY(settings != QImageEncoderSettings());

    settings = QImageEncoderSettings();
    QVERIFY(settings.isNull());
    QCOMPARE(settings.format(), QImageEncoderSettings::UnspecifiedFormat);
    QCOMPARE(settings.quality(), QMultimedia::NormalQuality);
    QCOMPARE(settings.resolution(), QSize());
    QVERIFY(settings.encodingOptions().isEmpty());

    {
        QImageEncoderSettings settings1;
        QImageEncoderSettings settings2;
        QCOMPARE(settings2, settings1);

        settings2 = settings1;
        QCOMPARE(settings2, settings1);
        QVERIFY(settings2.isNull());

        settings1.setQuality(QMultimedia::HighQuality);

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

        settings1.setQuality(QMultimedia::HighQuality);

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
    settings1.setQuality(QMultimedia::NormalQuality);
    settings2 = QImageEncoderSettings();
    settings2.setQuality(QMultimedia::NormalQuality);
    QVERIFY(settings1 == settings2);
    settings2.setQuality(QMultimedia::LowQuality);
    QVERIFY(settings1 != settings2);

    settings1 = QImageEncoderSettings();
    settings1.setEncodingOption(QLatin1String("encoderOption"), QVariant(1));
    settings2 = QImageEncoderSettings();
    settings2.setEncodingOption(QLatin1String("encoderOption"), QVariant(1));
    QVERIFY(settings1 == settings2);
    settings2.setEncodingOption(QLatin1String("encoderOption"), QVariant(2));
    QVERIFY(settings1 != settings2);
}

void tst_QCamera::testCameraEncodingProperyChange()
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

void tst_QCamera::testSetVideoOutput()
{
    MockVideoSurface surface;
    QCamera camera;

    camera.setViewfinder(static_cast<QAbstractVideoSurface *>(nullptr));

//    QCOMPARE(mockCameraService->rendererRef, 0);

//    camera.setViewfinder(&surface);
//    QVERIFY(mockCameraService->rendererControl->surface() == &surface);
//    QCOMPARE(mockCameraService->rendererRef, 1);

//    camera.setViewfinder(static_cast<QAbstractVideoSurface *>(nullptr));
//    QVERIFY(mockCameraService->rendererControl->surface() == nullptr);

//    //rendererControl is released
//    QCOMPARE(mockCameraService->rendererRef, 0);

//    camera.setViewfinder(&surface);
//    QVERIFY(mockCameraService->rendererControl->surface() == &surface);
//    QCOMPARE(mockCameraService->rendererRef, 1);

//    camera.setViewfinder(static_cast<QAbstractVideoSurface *>(nullptr));
//    QVERIFY(mockCameraService->rendererControl->surface() == nullptr);
//    //rendererControl is released
//    QCOMPARE(mockCameraService->rendererRef, 0);

//    camera.setViewfinder(&surface);
//    QVERIFY(mockCameraService->rendererControl->surface() == &surface);
//    QCOMPARE(mockCameraService->rendererRef, 1);
}


void tst_QCamera::testSetVideoOutputNoService()
{
    MockVideoSurface surface;

    integration->setFlags(QMockIntegration::NoCaptureInterface);
    QCamera camera;

    camera.setViewfinder(&surface);
    // Nothing we can verify here other than it doesn't assert.
}

void tst_QCamera::testSetVideoOutputDestruction()
{
    MockVideoSurface surface;

    {
        QCamera camera;
        camera.setViewfinder(&surface);
    }
}

void tst_QCamera::testEnumDebug()
{
    QTest::ignoreMessage(QtDebugMsg, "QCamera::ActiveState");
    qDebug() << QCamera::ActiveState;
    QTest::ignoreMessage(QtDebugMsg, "QCamera::ActiveStatus");
    qDebug() << QCamera::ActiveStatus;
    QTest::ignoreMessage(QtDebugMsg, "QCamera::CameraError");
    qDebug() << QCamera::CameraError;
    QTest::ignoreMessage(QtDebugMsg, "QCameraInfo::FrontFace ");
    qDebug() << QCameraInfo::FrontFace;
}

void tst_QCamera::testCameraControl()
{
    MockCameraControl *m_cameraControl=new MockCameraControl(this);
    QVERIFY(m_cameraControl != nullptr);
}

void tst_QCamera::testConstructor()
{
    auto cameras = QMediaDeviceManager::videoInputs();
    QCameraInfo defaultCamera = QMediaDeviceManager::defaultVideoInput();
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
        QCamera camera(QMediaDeviceManager::defaultVideoInput());
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraInfo(), defaultCamera);
    }

    {
        QCameraInfo cameraInfo = QMediaDeviceManager::videoInputs().at(0);
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

void tst_QCamera::testQCameraIsNotAvailable()
{
    integration->setFlags(QMockIntegration::NoCaptureInterface);
    QCamera camera;

    QCOMPARE(camera.error(), QCamera::CameraError);
    QVERIFY(!camera.isAvailable());
    integration->setFlags({});
}

/* Test case for verifying if error signal generated correctly */
void tst_QCamera::testErrorSignal()
{
    QCamera camera;
    auto *service = integration->lastCaptureService();

    QSignalSpy spyError(&camera, SIGNAL(errorOccurred(QCamera::Error)));

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
    QCamera camera;
    auto *service = integration->lastCaptureService();

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
    QCamera camera;
    auto *service = integration->lastCaptureService();

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
    QCamera camera;
    auto *service = integration->lastCaptureService();

    /* Set the QPlatformCamera status and verify if it is set correctly in QCamera */
    service->mockCameraControl->setStatus(QCamera::StartingStatus);
    QVERIFY(camera.status() == QCamera::StartingStatus);

    /* Set the QPlatformCamera status and verify if it is set correctly in QCamera */
    service->mockCameraControl->setStatus(QCamera::LoadingStatus);
    QVERIFY(camera.status() == QCamera::LoadingStatus);

    /* Set the QPlatformCamera status and verify if it is set correctly in QCamera */
    service->mockCameraControl->setStatus(QCamera::UnavailableStatus);
    QVERIFY(camera.status() == QCamera::UnavailableStatus);
}

/* All the enums test case for QPlatformCamera class*/
void tst_QCamera::testEnumsOfQPlatformCamera()
{
    MockCameraControl *m_cameraControl = new MockCameraControl(this);
    bool result;

    // In still mode, can't change much
    result = m_cameraControl->canChangeProperty(MockCameraControl::ImageEncodingSettings, QCamera::ActiveStatus);
    QVERIFY(!result);
    result = m_cameraControl->canChangeProperty(MockCameraControl::VideoEncodingSettings, QCamera::ActiveStatus);
    QVERIFY(result);
    result = m_cameraControl->canChangeProperty(MockCameraControl::Viewfinder, QCamera::ActiveStatus);
    QVERIFY(!result);

    // In video mode can change image encoding settings
    result = m_cameraControl->canChangeProperty(MockCameraControl::VideoEncodingSettings, QCamera::ActiveStatus);
    QVERIFY(result);
    result = m_cameraControl->canChangeProperty(MockCameraControl::Viewfinder, QCamera::ActiveStatus);
    QVERIFY(!result);

    // Flip the allow everything bit
    result = m_cameraControl->canChangeProperty(MockCameraControl::ImageEncodingSettings, QCamera::ActiveStatus);
    QVERIFY(result);
    result = m_cameraControl->canChangeProperty(MockCameraControl::VideoEncodingSettings, QCamera::ActiveStatus);
    QVERIFY(result);
    result = m_cameraControl->canChangeProperty(MockCameraControl::Viewfinder, QCamera::ActiveStatus);
    QVERIFY(result);
}

// Test case for QCameraImageProcessing class
void tst_QCamera::testContrast()
{
    QCamera camera;
    QCameraImageProcessing *cameraImageProcessing = camera.imageProcessing();
    QVERIFY(cameraImageProcessing->contrast() ==0);

    cameraImageProcessing->setContrast(0.123);
    QCOMPARE(cameraImageProcessing->contrast(), 0.123);

    cameraImageProcessing->setContrast(4.56);
    QCOMPARE(cameraImageProcessing->contrast(), 4.56);
}

void tst_QCamera::testIsAvailable()
{
    QCamera camera;
    QCameraImageProcessing *cameraImageProcessing = camera.imageProcessing();
    QVERIFY(cameraImageProcessing->isAvailable() == true);
}

void tst_QCamera::testSaturation()
{
    QCamera camera;
    QCameraImageProcessing *cameraImageProcessing = camera.imageProcessing();
    QCOMPARE(cameraImageProcessing->saturation()+1.0, 1.0);

    cameraImageProcessing->setSaturation(0.5);
    QCOMPARE(cameraImageProcessing->saturation(), 0.5);

    cameraImageProcessing->setSaturation(-0.5);
    QCOMPARE(cameraImageProcessing->saturation(), -0.5);
}

//Added test cases for QCameraFocus
void tst_QCamera::testCameraFocusIsAvailable()
{
    QCamera camera;

    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != nullptr);
    QVERIFY(cameraFocus->isAvailable());
}

//Added this code to cover QCameraFocus::HyperfocalFocus and QCameraFocus::MacroFocus
//As the HyperfocalFocus and MacroFocus are not supported we can not set the focus mode to these Focus Modes
void tst_QCamera::testFocusModes()
{
    QCamera camera;

    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != nullptr);
    QVERIFY(!cameraFocus->isFocusModeSupported(QCameraFocus::HyperfocalFocus));
    QVERIFY(!cameraFocus->isFocusModeSupported(QCameraFocus::MacroFocus));
    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::AutoFocus);
    cameraFocus->setFocusMode(QCameraFocus::HyperfocalFocus);
    QVERIFY(cameraFocus->focusMode()!= QCameraFocus::HyperfocalFocus);
    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::AutoFocus);
    cameraFocus->setFocusMode(QCameraFocus::MacroFocus);
    QVERIFY(cameraFocus->focusMode()!= QCameraFocus::MacroFocus);
    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::AutoFocus);
}

void tst_QCamera::testOpticalAndDigitalZoomChanged()
{
    QCamera camera;

    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != nullptr);
    QSignalSpy spy1(cameraFocus,SIGNAL(digitalZoomChanged(qreal)));
    QSignalSpy spy2(cameraFocus,SIGNAL(opticalZoomChanged(qreal)));
    QVERIFY(spy1.count() == 0);
    QVERIFY(spy2.count() == 0);
    cameraFocus->zoomTo(2.0,3.0);
    QVERIFY(spy1.count() == 1);
    QVERIFY(spy2.count() == 1);
}

void tst_QCamera::testMaxDigitalZoomChangedSignal()
{
    QCamera camera;

    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != nullptr);
    QSignalSpy spy(cameraFocus,SIGNAL(maximumDigitalZoomChanged(qreal)));
    QVERIFY(spy.count() == 0);
    cameraFocus->zoomTo(5.0,6.0);
    QVERIFY(spy.count() == 1);
}

void tst_QCamera::testMaxOpticalZoomChangedSignal()
{
    QCamera camera;

    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != nullptr);
    QSignalSpy spy(cameraFocus,SIGNAL(maximumOpticalZoomChanged(qreal)));
    QVERIFY(spy.count() == 0);
    cameraFocus->zoomTo(5.0,6.0);
    QVERIFY(spy.count() == 1);
}

// test constructor for abstract class of ImageProcessingControl
void tst_QCamera :: testImageProcessingControl()
{
    QObject parent;
    MockImageProcessingControl processCtrl(&parent);
}

void tst_QCamera::testSignalApertureChanged()
{
    QCamera camera;

    QCameraExposure *cameraExposure = camera.exposure(); //create camera expose instance
    QVERIFY(cameraExposure != nullptr);

    QSignalSpy spyApertureChanged(cameraExposure , SIGNAL(apertureChanged(qreal)));
    QSignalSpy spyApertureRangeChanged(cameraExposure , SIGNAL(apertureRangeChanged()));


    QVERIFY(spyApertureChanged.count() ==0);
    cameraExposure->setManualAperture(10.0);//set the ManualAperture to 10.0

    QTest::qWait(100);
    QVERIFY(spyApertureChanged.count() ==1);
    QVERIFY(spyApertureRangeChanged.count() ==1);
}

void tst_QCamera::testSignalExposureCompensationChanged()
{
    QCamera camera;

    QCameraExposure *cameraExposure = camera.exposure(); //create camera expose instance
    QVERIFY(cameraExposure != nullptr);

    QSignalSpy spyExposureCompensationChanged(cameraExposure , SIGNAL(exposureCompensationChanged(qreal)));

    QVERIFY(spyExposureCompensationChanged.count() ==0);

    QVERIFY(cameraExposure->exposureCompensation() != 800);
    cameraExposure->setExposureCompensation(2.0);

    QTest::qWait(100);

    QVERIFY(cameraExposure->exposureCompensation() == 2.0);

    QCOMPARE(spyExposureCompensationChanged.count(),1);

    // Setting the same should not result in a signal
    cameraExposure->setExposureCompensation(2.0);
    QTest::qWait(100);

    QVERIFY(cameraExposure->exposureCompensation() == 2.0);
    QCOMPARE(spyExposureCompensationChanged.count(),1);
}

void tst_QCamera::testSignalIsoSensitivityChanged()
{
    QCamera camera;

    QCameraExposure *cameraExposure = camera.exposure(); //create camera expose instance
    QVERIFY(cameraExposure != nullptr);

    QSignalSpy spyisoSensitivityChanged(cameraExposure , SIGNAL(isoSensitivityChanged(int)));

    QVERIFY(spyisoSensitivityChanged.count() ==0);

    cameraExposure->setManualIsoSensitivity(800); //set the manualiso sentivity to 800
    QTest::qWait(100);
    QVERIFY(spyisoSensitivityChanged.count() ==1);

}
void tst_QCamera::testSignalShutterSpeedChanged()
{
    QCamera camera;

    QCameraExposure *cameraExposure = camera.exposure(); //create camera expose instance
    QVERIFY(cameraExposure != nullptr);

    QSignalSpy spySignalShutterSpeedChanged(cameraExposure , SIGNAL(shutterSpeedChanged(qreal)));
    QSignalSpy spySignalShutterSpeedRangeChanged(cameraExposure , SIGNAL(shutterSpeedRangeChanged()));

    QVERIFY(spySignalShutterSpeedChanged.count() ==0);

    cameraExposure->setManualShutterSpeed(2.0);//set the ManualShutterSpeed to 2.0
    QTest::qWait(100);

    QVERIFY(spySignalShutterSpeedChanged.count() ==1);
    QVERIFY(spySignalShutterSpeedRangeChanged.count() ==1);
}

void tst_QCamera::testSignalFlashReady()
{
    QCamera camera;

    QCameraExposure *cameraExposure = camera.exposure(); //create camera expose instance
    QVERIFY(cameraExposure != nullptr);


    QSignalSpy spyflashReady(cameraExposure,SIGNAL(flashReady(bool)));

    QVERIFY(spyflashReady.count() ==0);

    QVERIFY(cameraExposure->flashMode() ==QCameraExposure::FlashAuto);

    cameraExposure->setFlashMode(QCameraExposure::FlashOff);//set theFlashMode to QCameraExposure::FlashOff

    QVERIFY(cameraExposure->flashMode() ==QCameraExposure::FlashOff);

    QVERIFY(spyflashReady.count() ==1);
}

// test constructor
void tst_QCamera::testExposureControlConstructor()
{
    // To check changes in abstract classes's pure virtual functions
    MockCameraExposureControl obj;
}

QTEST_MAIN(tst_QCamera)

#include "tst_qcamera.moc"
