/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QDebug>

#include <qabstractvideosurface.h>
#include <qcameracontrol.h>
#include <qcameralockscontrol.h>
#include <qcameraexposurecontrol.h>
#include <qcameraflashcontrol.h>
#include <qcamerafocuscontrol.h>
#include <qcameraimagecapturecontrol.h>
#include <qimageencodercontrol.h>
#include <qcameraimageprocessingcontrol.h>
#include <qcameracapturebufferformatcontrol.h>
#include <qcameracapturedestinationcontrol.h>
#include <qmediaservice.h>
#include <qcamera.h>
#include <qcameraimagecapture.h>
#include <qvideorenderercontrol.h>

#include "mockcameraservice.h"

#include "mockmediaserviceprovider.h"
#include "mockvideosurface.h"
#include "mockvideorenderercontrol.h"
#include "mockvideowindowcontrol.h"

QT_USE_NAMESPACE


class tst_QCamera: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void testSimpleCamera();
    void testSimpleCameraWhiteBalance();
    void testSimpleCameraExposure();
    void testSimpleCameraFocus();
    void testSimpleCameraCapture();
    void testSimpleCameraLock();
    void testSimpleCaptureDestination();
    void testSimpleCaptureFormat();

    void testCameraWhiteBalance();
    void testCameraExposure();
    void testCameraFocus();
    void testCameraCapture();
    void testCameraCaptureMetadata();
    void testImageSettings();
    void testCameraLock();
    void testCameraLockCancel();
    void testCameraEncodingProperyChange();
    void testCaptureDestination();
    void testCaptureFormat();

    void testConstructorWithDefaultProvider();
    void testCaptureMode();
    void testIsCaptureModeSupported();
    void testRequestedLocks();
    void testSupportedLocks();
    void testQCameraIsAvailable();
    void testAvailabilityError();
    void testSearchAndLockWithLockTypes();
    void testSetCaptureMode();
    void testUnlockWithType();
    void testCaptureModeChangedSignal();
    void testLockStatusChangedWithTypesSignal();
    void testErrorSignal();
    void testError();
    void testErrorString();
    void testStatus();
    void testLockType();
    void testLockChangeReason();


    // Test cases to for QCameraFocus
    void testCameraFocusIsAvailable();
    void testFocusModes();
    void testOpticalAndDigitalZoomChanged();
    void testMaxOpticalZoomChangedSignal();
    void testMaxDigitalZoomChangedSignal();
    void testfocusZonesChangedSignal();

    // Test cases for QCameraControl class.
    void testCameraControl();
    void testCaptureModeChanged_signal();
    void testEnumsOfQCameraControl();

    // Test case for QCameraImageProcessing class
    void testContrast();
    void testDenoisingLevel();
    void testIsAvailable();
    void testSaturation();
    void testSharpeningLevel();
    void testEnumOfQCameraImageProcessing();

    void testSetVideoOutput();
    void testSetVideoOutputNoService();
    void testSetVideoOutputNoControl();
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
    MockSimpleCameraService  *mockSimpleCameraService;
    MockMediaServiceProvider *provider;
};

void tst_QCamera::initTestCase()
{
    provider = new MockMediaServiceProvider;
    mockSimpleCameraService = new MockSimpleCameraService;
    provider->service = mockSimpleCameraService;
}

void tst_QCamera::cleanupTestCase()
{
    delete mockSimpleCameraService;
    delete provider;
}

void tst_QCamera::testSimpleCamera()
{
    QCamera camera(0, provider);
    QCOMPARE(camera.service(), (QMediaService*)mockSimpleCameraService);

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
    QCamera camera(0, provider);

    //only WhiteBalanceAuto is supported
    QVERIFY(!camera.imageProcessing()->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceAuto));
    QVERIFY(!camera.imageProcessing()->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceCloudy));
    QCOMPARE(camera.imageProcessing()->whiteBalanceMode(), QCameraImageProcessing::WhiteBalanceAuto);
    camera.imageProcessing()->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceCloudy);
    QCOMPARE(camera.imageProcessing()->whiteBalanceMode(), QCameraImageProcessing::WhiteBalanceAuto);
    QCOMPARE(camera.imageProcessing()->manualWhiteBalance(), 0);
    camera.imageProcessing()->setManualWhiteBalance(5000);
    QCOMPARE(camera.imageProcessing()->manualWhiteBalance(), 0);
}

void tst_QCamera::testSimpleCameraExposure()
{
    QCamera camera(0, provider);
    QCameraExposure *cameraExposure = camera.exposure();
    QVERIFY(cameraExposure != 0);

    QVERIFY(!cameraExposure->isExposureModeSupported(QCameraExposure::ExposureAuto));
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureAuto);
    cameraExposure->setExposureMode(QCameraExposure::ExposureManual);//should be ignored
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureAuto);

    QVERIFY(!cameraExposure->isFlashModeSupported(QCameraExposure::FlashOff));
    QCOMPARE(cameraExposure->flashMode(), QCameraExposure::FlashOff);
    QCOMPARE(cameraExposure->isFlashReady(), false);
    cameraExposure->setFlashMode(QCameraExposure::FlashOn);
    QCOMPARE(cameraExposure->flashMode(), QCameraExposure::FlashOff);

    QVERIFY(!cameraExposure->isMeteringModeSupported(QCameraExposure::MeteringAverage));
    QVERIFY(!cameraExposure->isMeteringModeSupported(QCameraExposure::MeteringSpot));
    QVERIFY(!cameraExposure->isMeteringModeSupported(QCameraExposure::MeteringMatrix));
    QCOMPARE(cameraExposure->meteringMode(), QCameraExposure::MeteringMatrix);
    cameraExposure->setMeteringMode(QCameraExposure::MeteringSpot);
    QCOMPARE(cameraExposure->meteringMode(), QCameraExposure::MeteringMatrix);

    QCOMPARE(cameraExposure->spotMeteringPoint(), QPointF());
    cameraExposure->setSpotMeteringPoint(QPointF(0.5f, 0.5f));
    QCOMPARE(cameraExposure->spotMeteringPoint(), QPointF());

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
    QCamera camera(0, provider);

    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != 0);

    QVERIFY(!cameraFocus->isFocusModeSupported(QCameraFocus::AutoFocus));
    QVERIFY(!cameraFocus->isFocusModeSupported(QCameraFocus::ContinuousFocus));
    QVERIFY(!cameraFocus->isFocusModeSupported(QCameraFocus::InfinityFocus));

    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::AutoFocus);
    QTest::ignoreMessage(QtWarningMsg, "Focus points mode selection is not supported");
    cameraFocus->setFocusMode(QCameraFocus::ContinuousFocus);
    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::AutoFocus);    

    QCOMPARE(cameraFocus->maximumOpticalZoom(), 1.0);
    QCOMPARE(cameraFocus->maximumDigitalZoom(), 1.0);
    QCOMPARE(cameraFocus->opticalZoom(), 1.0);
    QCOMPARE(cameraFocus->digitalZoom(), 1.0);

    QTest::ignoreMessage(QtWarningMsg, "The camera doesn't support zooming.");
    cameraFocus->zoomTo(100.0, 100.0);
    QCOMPARE(cameraFocus->opticalZoom(), 1.0);
    QCOMPARE(cameraFocus->digitalZoom(), 1.0);


    QVERIFY(!cameraFocus->isFocusPointModeSupported(QCameraFocus::FocusPointAuto));
    QCOMPARE(cameraFocus->focusPointMode(), QCameraFocus::FocusPointAuto);


    cameraFocus->setFocusPointMode( QCameraFocus::FocusPointCenter );
    QCOMPARE(cameraFocus->focusPointMode(), QCameraFocus::FocusPointAuto);

    QCOMPARE(cameraFocus->customFocusPoint(), QPointF(0.5, 0.5));
    QTest::ignoreMessage(QtWarningMsg, "Focus points selection is not supported");
    cameraFocus->setCustomFocusPoint(QPointF(1.0, 1.0));
    QCOMPARE(cameraFocus->customFocusPoint(), QPointF(0.5, 0.5));
}

void tst_QCamera::testSimpleCameraCapture()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);

    QVERIFY(!imageCapture.isReadyForCapture());
    QVERIFY(!imageCapture.isAvailable());

    QCOMPARE(imageCapture.error(), QCameraImageCapture::NoError);
    QVERIFY(imageCapture.errorString().isEmpty());

    QSignalSpy errorSignal(&imageCapture, SIGNAL(error(int, QCameraImageCapture::Error,QString)));
    imageCapture.capture(QString::fromLatin1("/dev/null"));
    QCOMPARE(errorSignal.size(), 1);
    QCOMPARE(imageCapture.error(), QCameraImageCapture::NotSupportedFeatureError);
    QVERIFY(!imageCapture.errorString().isEmpty());
}

void tst_QCamera::testSimpleCameraLock()
{
    QCamera camera(0, provider);
    QCOMPARE(camera.lockStatus(), QCamera::Unlocked);
    QCOMPARE(camera.lockStatus(QCamera::LockExposure), QCamera::Unlocked);
    QCOMPARE(camera.lockStatus(QCamera::LockFocus), QCamera::Unlocked);
    QCOMPARE(camera.lockStatus(QCamera::LockWhiteBalance), QCamera::Unlocked);

    QSignalSpy lockedSignal(&camera, SIGNAL(locked()));
    QSignalSpy lockFailedSignal(&camera, SIGNAL(lockFailed()));
    QSignalSpy lockStatusChangedSignal(&camera, SIGNAL(lockStatusChanged(QCamera::LockStatus, QCamera::LockChangeReason)));

    camera.searchAndLock();
    QCOMPARE(camera.lockStatus(), QCamera::Locked);
    QCOMPARE(camera.lockStatus(QCamera::LockExposure), QCamera::Locked);
    QCOMPARE(camera.lockStatus(QCamera::LockFocus), QCamera::Locked);
    QCOMPARE(camera.lockStatus(QCamera::LockWhiteBalance), QCamera::Locked);
    QCOMPARE(lockedSignal.count(), 1);
    QCOMPARE(lockFailedSignal.count(), 0);
    QCOMPARE(lockStatusChangedSignal.count(), 1);

    lockedSignal.clear();
    lockFailedSignal.clear();
    lockStatusChangedSignal.clear();

    camera.unlock();
    QCOMPARE(camera.lockStatus(), QCamera::Unlocked);
    QCOMPARE(camera.lockStatus(QCamera::LockExposure), QCamera::Unlocked);
    QCOMPARE(camera.lockStatus(QCamera::LockFocus), QCamera::Unlocked);
    QCOMPARE(camera.lockStatus(QCamera::LockWhiteBalance), QCamera::Unlocked);

    QCOMPARE(lockedSignal.count(), 0);
    QCOMPARE(lockFailedSignal.count(), 0);
    QCOMPARE(lockStatusChangedSignal.count(), 1);
}

void tst_QCamera::testSimpleCaptureDestination()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);

    QVERIFY(imageCapture.isCaptureDestinationSupported(QCameraImageCapture::CaptureToFile));
    QVERIFY(!imageCapture.isCaptureDestinationSupported(QCameraImageCapture::CaptureToBuffer));
    QVERIFY(!imageCapture.isCaptureDestinationSupported(
                QCameraImageCapture::CaptureToBuffer | QCameraImageCapture::CaptureToFile));

    QCOMPARE(imageCapture.captureDestination(), QCameraImageCapture::CaptureToFile);
    imageCapture.setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    QCOMPARE(imageCapture.captureDestination(), QCameraImageCapture::CaptureToFile);
}

void tst_QCamera::testSimpleCaptureFormat()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);

    QCOMPARE(imageCapture.bufferFormat(), QVideoFrame::Format_Invalid);
    QVERIFY(imageCapture.supportedBufferFormats().isEmpty());

    imageCapture.setBufferFormat(QVideoFrame::Format_AdobeDng);
    QCOMPARE(imageCapture.bufferFormat(), QVideoFrame::Format_Invalid);
}

void tst_QCamera::testCaptureDestination()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);

    QVERIFY(imageCapture.isCaptureDestinationSupported(QCameraImageCapture::CaptureToFile));
    QVERIFY(imageCapture.isCaptureDestinationSupported(QCameraImageCapture::CaptureToBuffer));
    QVERIFY(!imageCapture.isCaptureDestinationSupported(
                QCameraImageCapture::CaptureToBuffer | QCameraImageCapture::CaptureToFile));

    QSignalSpy destinationChangedSignal(&imageCapture, SIGNAL(captureDestinationChanged(QCameraImageCapture::CaptureDestinations)));

    QCOMPARE(imageCapture.captureDestination(), QCameraImageCapture::CaptureToFile);
    imageCapture.setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    QCOMPARE(imageCapture.captureDestination(), QCameraImageCapture::CaptureToBuffer);
    QCOMPARE(destinationChangedSignal.size(), 1);
    QCOMPARE(destinationChangedSignal.first().first().value<QCameraImageCapture::CaptureDestinations>(),
             QCameraImageCapture::CaptureToBuffer);

    //not supported combination
    imageCapture.setCaptureDestination(QCameraImageCapture::CaptureToBuffer | QCameraImageCapture::CaptureToFile);
    QCOMPARE(imageCapture.captureDestination(), QCameraImageCapture::CaptureToBuffer);
    QCOMPARE(destinationChangedSignal.size(), 1);
}

void tst_QCamera::testCaptureFormat()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);

    QSignalSpy formatChangedSignal(&imageCapture, SIGNAL(bufferFormatChanged(QVideoFrame::PixelFormat)));

    QCOMPARE(imageCapture.bufferFormat(), QVideoFrame::Format_Jpeg);
    QCOMPARE(imageCapture.supportedBufferFormats().size(), 3);

    imageCapture.setBufferFormat(QVideoFrame::Format_AdobeDng);
    QCOMPARE(imageCapture.bufferFormat(), QVideoFrame::Format_AdobeDng);

    QCOMPARE(formatChangedSignal.size(), 1);
    QCOMPARE(formatChangedSignal.first().first().value<QVideoFrame::PixelFormat>(),
             QVideoFrame::Format_AdobeDng);

    imageCapture.setBufferFormat(QVideoFrame::Format_Y16);
    QCOMPARE(imageCapture.bufferFormat(), QVideoFrame::Format_AdobeDng);

    QCOMPARE(formatChangedSignal.size(), 1);
}


void tst_QCamera::testCameraCapture()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);

    QVERIFY(!imageCapture.isReadyForCapture());

    QSignalSpy capturedSignal(&imageCapture, SIGNAL(imageCaptured(int,QImage)));    
    QSignalSpy errorSignal(&imageCapture, SIGNAL(error(int, QCameraImageCapture::Error,QString)));

    imageCapture.capture(QString::fromLatin1("/dev/null"));
    QCOMPARE(capturedSignal.size(), 0);
    QCOMPARE(errorSignal.size(), 1);
    QCOMPARE(imageCapture.error(), QCameraImageCapture::NotReadyError);

    errorSignal.clear();

    camera.start();
    QVERIFY(imageCapture.isReadyForCapture());
    QCOMPARE(errorSignal.size(), 0);

    imageCapture.capture(QString::fromLatin1("/dev/null"));

    for (int i=0; i<100 && capturedSignal.isEmpty(); i++)
        QTest::qWait(10);

    QCOMPARE(capturedSignal.size(), 1);
    QCOMPARE(errorSignal.size(), 0);
    QCOMPARE(imageCapture.error(), QCameraImageCapture::NoError);
}

void tst_QCamera::testCameraCaptureMetadata()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);

    QSignalSpy metadataSignal(&imageCapture, SIGNAL(imageMetadataAvailable(int,QString,QVariant)));
    QSignalSpy savedSignal(&imageCapture, SIGNAL(imageSaved(int,QString)));

    camera.start();
    int id = imageCapture.capture(QString::fromLatin1("/dev/null"));

    for (int i=0; i<100 && savedSignal.isEmpty(); i++)
        QTest::qWait(10);

    QCOMPARE(savedSignal.size(), 1);

    QCOMPARE(metadataSignal.size(), 3);

    QVariantList metadata = metadataSignal[0];
    QCOMPARE(metadata[0].toInt(), id);
    QCOMPARE(metadata[1].toString(), QtMultimedia::MetaData::FocalLengthIn35mmFilm);
    QCOMPARE(metadata[2].value<QVariant>().toInt(), 50);

    metadata = metadataSignal[1];
    QCOMPARE(metadata[0].toInt(), id);
    QCOMPARE(metadata[1].toString(), QtMultimedia::MetaData::DateTimeOriginal);
    QDateTime captureTime = metadata[2].value<QVariant>().value<QDateTime>();
    QVERIFY(qAbs(captureTime.secsTo(QDateTime::currentDateTime()) < 5)); //it should not takes more than 5 seconds for signal to arrive here

    metadata = metadataSignal[2];
    QCOMPARE(metadata[0].toInt(), id);
    QCOMPARE(metadata[1].toString(), QLatin1String("Answer to the Ultimate Question of Life, the Universe, and Everything"));
    QCOMPARE(metadata[2].value<QVariant>().toInt(), 42);
}


void tst_QCamera::testCameraWhiteBalance()
{
    QSet<QCameraImageProcessing::WhiteBalanceMode> whiteBalanceModes;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceAuto;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceFlash;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceTungsten;

    MockCameraService service;
    service.mockImageProcessingControl->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceFlash);
    service.mockImageProcessingControl->setSupportedWhiteBalanceModes(whiteBalanceModes);
    service.mockImageProcessingControl->setProcessingParameter(
                QCameraImageProcessingControl::ColorTemperature,
                QVariant(34));

    MockMediaServiceProvider provider;
    provider.service = &service;

    QCamera camera(0, &provider);
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
    QCOMPARE(cameraImageProcessing->manualWhiteBalance(), 34);

    cameraImageProcessing->setManualWhiteBalance(432);
    QCOMPARE(cameraImageProcessing->manualWhiteBalance(), 432);
}

void tst_QCamera::testCameraExposure()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    QCameraExposure *cameraExposure = camera.exposure();
    QVERIFY(cameraExposure != 0);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureAuto));
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureAuto);

    // Test Cases For QCameraExposure
    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureManual));
    cameraExposure->setExposureMode(QCameraExposure::ExposureManual);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureManual);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureNight));
    cameraExposure->setExposureMode(QCameraExposure::ExposureNight);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureNight);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureBacklight));
    cameraExposure->setExposureMode(QCameraExposure::ExposureBacklight);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureBacklight);


    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureSpotlight ));
    cameraExposure->setExposureMode(QCameraExposure::ExposureSpotlight);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureSpotlight);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureSports ));
    cameraExposure->setExposureMode(QCameraExposure::ExposureSports);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureSports);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureSnow ));
    cameraExposure->setExposureMode(QCameraExposure::ExposureSnow);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureSnow);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureBeach ));
    cameraExposure->setExposureMode(QCameraExposure::ExposureBeach);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureBeach);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureLargeAperture ));
    cameraExposure->setExposureMode(QCameraExposure::ExposureLargeAperture);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureLargeAperture);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureSmallAperture ));
    cameraExposure->setExposureMode(QCameraExposure::ExposureSmallAperture);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureSmallAperture);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposurePortrait ));
    cameraExposure->setExposureMode(QCameraExposure::ExposurePortrait);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposurePortrait);

    QVERIFY(cameraExposure->isExposureModeSupported(QCameraExposure::ExposureModeVendor ));
    cameraExposure->setExposureMode(QCameraExposure::ExposureModeVendor);
    QCOMPARE(cameraExposure->exposureMode(), QCameraExposure::ExposureModeVendor);


    cameraExposure->setFlashMode(QCameraExposure::FlashAuto);
    QCOMPARE(cameraExposure->flashMode(), QCameraExposure::FlashAuto);
    QCOMPARE(cameraExposure->isFlashReady(), true);
    cameraExposure->setFlashMode(QCameraExposure::FlashRedEyeReduction);
    QCOMPARE(cameraExposure->flashMode(), QCameraExposure::FlashRedEyeReduction);
    cameraExposure->setFlashMode(QCameraExposure::FlashOn);
    QCOMPARE(cameraExposure->flashMode(), QCameraExposure::FlashOn);
    cameraExposure->setFlashMode(QCameraExposure::FlashFill);
    QCOMPARE(cameraExposure->flashMode(), QCameraExposure::FlashFill);
    cameraExposure->setFlashMode(QCameraExposure::FlashTorch);
    QCOMPARE(cameraExposure->flashMode(), QCameraExposure::FlashTorch);
    cameraExposure->setFlashMode(QCameraExposure::FlashSlowSyncFrontCurtain);
    QCOMPARE(cameraExposure->flashMode(), QCameraExposure::FlashSlowSyncFrontCurtain);


    cameraExposure->setMeteringMode(QCameraExposure::MeteringMatrix);
    QCOMPARE(cameraExposure->meteringMode(), QCameraExposure::MeteringMatrix);
    cameraExposure->setMeteringMode(QCameraExposure::MeteringAverage);
    QCOMPARE(cameraExposure->meteringMode(), QCameraExposure::MeteringAverage);
    cameraExposure->setMeteringMode(QCameraExposure::MeteringSpot);
    QCOMPARE(cameraExposure->meteringMode(), QCameraExposure::MeteringSpot);

    cameraExposure->setSpotMeteringPoint(QPointF(0.5f, 0.25f));
    QCOMPARE(cameraExposure->spotMeteringPoint(), QPointF(0.5f, 0.25f));
    cameraExposure->setSpotMeteringPoint(QPointF(0.25f, 56.3f));
    QCOMPARE(cameraExposure->spotMeteringPoint(), QPointF(0.5f, 0.25f));
    cameraExposure->setSpotMeteringPoint(QPointF(0, 0));
    QCOMPARE(cameraExposure->spotMeteringPoint(), QPointF(0, 0));
    cameraExposure->setSpotMeteringPoint(QPointF(1, 1));
    QCOMPARE(cameraExposure->spotMeteringPoint(), QPointF(1, 1));

    QCOMPARE(cameraExposure->exposureCompensation(), 0.0);
    cameraExposure->setExposureCompensation(2.0);
    QCOMPARE(cameraExposure->exposureCompensation(), 2.0);

    int minIso = cameraExposure->supportedIsoSensitivities().first();
    int maxIso = cameraExposure->supportedIsoSensitivities().last();
    QVERIFY(cameraExposure->isoSensitivity() > 0);
    QVERIFY(minIso > 0);
    QVERIFY(maxIso > 0);
    cameraExposure->setManualIsoSensitivity(minIso);
    QCOMPARE(cameraExposure->isoSensitivity(), minIso);
    cameraExposure->setManualIsoSensitivity(maxIso*10);
    QCOMPARE(cameraExposure->isoSensitivity(), maxIso);
    cameraExposure->setManualIsoSensitivity(-10);
    QCOMPARE(cameraExposure->isoSensitivity(), minIso);
    cameraExposure->setAutoIsoSensitivity();
    QCOMPARE(cameraExposure->isoSensitivity(), 100);

    qreal minAperture = cameraExposure->supportedApertures().first();
    qreal maxAperture = cameraExposure->supportedApertures().last();
    QVERIFY(minAperture > 0);
    QVERIFY(maxAperture > 0);
    QVERIFY(cameraExposure->aperture() >= minAperture);
    QVERIFY(cameraExposure->aperture() <= maxAperture);

    cameraExposure->setAutoAperture();
    QVERIFY(cameraExposure->aperture() >= minAperture);
    QVERIFY(cameraExposure->aperture() <= maxAperture);

    cameraExposure->setManualAperture(0);
    QCOMPARE(cameraExposure->aperture(), minAperture);

    cameraExposure->setManualAperture(10000);
    QCOMPARE(cameraExposure->aperture(), maxAperture);


    qreal minShutterSpeed = cameraExposure->supportedShutterSpeeds().first();
    qreal maxShutterSpeed = cameraExposure->supportedShutterSpeeds().last();
    QVERIFY(minShutterSpeed > 0);
    QVERIFY(maxShutterSpeed > 0);
    QVERIFY(cameraExposure->shutterSpeed() >= minShutterSpeed);
    QVERIFY(cameraExposure->shutterSpeed() <= maxShutterSpeed);

    cameraExposure->setAutoShutterSpeed();
    QVERIFY(cameraExposure->shutterSpeed() >= minShutterSpeed);
    QVERIFY(cameraExposure->shutterSpeed() <= maxShutterSpeed);

    cameraExposure->setManualShutterSpeed(0);
    QCOMPARE(cameraExposure->shutterSpeed(), minShutterSpeed);

    cameraExposure->setManualShutterSpeed(10000);
    QCOMPARE(cameraExposure->shutterSpeed(), maxShutterSpeed);
}

void tst_QCamera::testCameraFocus()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != 0);

    QVERIFY(cameraFocus->isFocusModeSupported(QCameraFocus::AutoFocus));
    QVERIFY(cameraFocus->isFocusModeSupported(QCameraFocus::ContinuousFocus));
    QVERIFY(!cameraFocus->isFocusModeSupported(QCameraFocus::InfinityFocus));

    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::AutoFocus);
    cameraFocus->setFocusMode(QCameraFocus::ManualFocus);
    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::AutoFocus);
    cameraFocus->setFocusMode(QCameraFocus::ContinuousFocus);
    QCOMPARE(cameraFocus->focusMode(), QCameraFocus::ContinuousFocus);

    QVERIFY(cameraFocus->maximumOpticalZoom() >= 1.0);
    QVERIFY(cameraFocus->maximumDigitalZoom() >= 1.0);
    QCOMPARE(cameraFocus->opticalZoom(), 1.0);
    QCOMPARE(cameraFocus->digitalZoom(), 1.0);
    cameraFocus->zoomTo(0.5, 1.0);
    QCOMPARE(cameraFocus->opticalZoom(), 1.0);
    QCOMPARE(cameraFocus->digitalZoom(), 1.0);
    cameraFocus->zoomTo(2.0, 0.5);
    QCOMPARE(cameraFocus->opticalZoom(), 2.0);
    QCOMPARE(cameraFocus->digitalZoom(), 1.0);
    cameraFocus->zoomTo(2.0, 2.5);
    QCOMPARE(cameraFocus->opticalZoom(), 2.0);
    QCOMPARE(cameraFocus->digitalZoom(), 2.5);
    cameraFocus->zoomTo(2000000.0, 1000000.0);
    QVERIFY(qFuzzyCompare(cameraFocus->opticalZoom(), cameraFocus->maximumOpticalZoom()));
    QVERIFY(qFuzzyCompare(cameraFocus->digitalZoom(), cameraFocus->maximumDigitalZoom()));

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

    QCOMPARE(settings.codec(), QString());
    settings.setCodec(QLatin1String("codecName"));
    QCOMPARE(settings.codec(), QLatin1String("codecName"));
    QVERIFY(!settings.isNull());
    QVERIFY(settings != QImageEncoderSettings());

    settings = QImageEncoderSettings();
    QCOMPARE(settings.quality(), QtMultimedia::NormalQuality);
    settings.setQuality(QtMultimedia::HighQuality);
    QCOMPARE(settings.quality(), QtMultimedia::HighQuality);
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
    QCOMPARE(settings.codec(), QString());
    QCOMPARE(settings.quality(), QtMultimedia::NormalQuality);
    QCOMPARE(settings.resolution(), QSize());

    {
        QImageEncoderSettings settings1;
        QImageEncoderSettings settings2;
        QCOMPARE(settings2, settings1);

        settings2 = settings1;
        QCOMPARE(settings2, settings1);
        QVERIFY(settings2.isNull());

        settings1.setQuality(QtMultimedia::HighQuality);

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

        settings1.setQuality(QtMultimedia::HighQuality);

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
    settings1.setCodec("codec1");
    settings2 = QImageEncoderSettings();
    settings2.setCodec("codec1");
    QVERIFY(settings1 == settings2);
    settings2.setCodec("codec2");
    QVERIFY(settings1 != settings2);

    settings1 = QImageEncoderSettings();
    settings1.setQuality(QtMultimedia::NormalQuality);
    settings2 = QImageEncoderSettings();
    settings2.setQuality(QtMultimedia::NormalQuality);
    QVERIFY(settings1 == settings2);
    settings2.setQuality(QtMultimedia::LowQuality);
    QVERIFY(settings1 != settings2);
}

void tst_QCamera::testCameraLock()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    camera.focus()->setFocusMode(QCameraFocus::AutoFocus);

    QCOMPARE(camera.lockStatus(), QCamera::Unlocked);

    QSignalSpy lockedSignal(&camera, SIGNAL(locked()));
    QSignalSpy lockFailedSignal(&camera, SIGNAL(lockFailed()));
    QSignalSpy lockStatusChangedSignal(&camera, SIGNAL(lockStatusChanged(QCamera::LockStatus,QCamera::LockChangeReason)));

    camera.searchAndLock();
    QCOMPARE(camera.lockStatus(), QCamera::Searching);
    QCOMPARE(lockedSignal.count(), 0);
    QCOMPARE(lockFailedSignal.count(), 0);
    QCOMPARE(lockStatusChangedSignal.count(), 1);

    lockedSignal.clear();
    lockFailedSignal.clear();
    lockStatusChangedSignal.clear();

    for (int i=0; i<200 && camera.lockStatus() == QCamera::Searching; i++)
        QTest::qWait(10);

    QCOMPARE(camera.lockStatus(), QCamera::Locked);
    QCOMPARE(lockedSignal.count(), 1);
    QCOMPARE(lockFailedSignal.count(), 0);
    QCOMPARE(lockStatusChangedSignal.count(), 1);

    lockedSignal.clear();
    lockFailedSignal.clear();
    lockStatusChangedSignal.clear();

    camera.searchAndLock();
    QCOMPARE(camera.lockStatus(), QCamera::Searching);
    QCOMPARE(lockedSignal.count(), 0);
    QCOMPARE(lockFailedSignal.count(), 0);
    QCOMPARE(lockStatusChangedSignal.count(), 1);

    lockedSignal.clear();
    lockFailedSignal.clear();
    lockStatusChangedSignal.clear();

    camera.unlock();
    QCOMPARE(camera.lockStatus(), QCamera::Unlocked);
    QCOMPARE(lockedSignal.count(), 0);
    QCOMPARE(lockFailedSignal.count(), 0);
    QCOMPARE(lockStatusChangedSignal.count(), 1);
}

void tst_QCamera::testCameraLockCancel()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    camera.focus()->setFocusMode(QCameraFocus::AutoFocus);

    QCOMPARE(camera.lockStatus(), QCamera::Unlocked);

    QSignalSpy lockedSignal(&camera, SIGNAL(locked()));
    QSignalSpy lockFailedSignal(&camera, SIGNAL(lockFailed()));
    QSignalSpy lockStatusChangedSignal(&camera, SIGNAL(lockStatusChanged(QCamera::LockStatus, QCamera::LockChangeReason)));
    camera.searchAndLock();
    QCOMPARE(camera.lockStatus(), QCamera::Searching);
    QCOMPARE(lockedSignal.count(), 0);
    QCOMPARE(lockFailedSignal.count(), 0);
    QCOMPARE(lockStatusChangedSignal.count(), 1);

    lockedSignal.clear();
    lockFailedSignal.clear();
    lockStatusChangedSignal.clear();

    camera.unlock();
    QCOMPARE(camera.lockStatus(), QCamera::Unlocked);
    QCOMPARE(lockedSignal.count(), 0);
    QCOMPARE(lockFailedSignal.count(), 0);
    QCOMPARE(lockStatusChangedSignal.count(), 1);
}

void tst_QCamera::testCameraEncodingProperyChange()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
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


    camera.setCaptureMode(QCamera::CaptureVideo);
    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::LoadedStatus);

    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    QTest::qWait(10);

    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::ActiveStatus);
    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    //backens should not be stopped since the capture mode is Video
    imageCapture.setEncodingSettings(QImageEncoderSettings());
    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 0);

    camera.setCaptureMode(QCamera::CaptureStillImage);
    QTest::qWait(10);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    //the settings change should trigger camera stop/start
    imageCapture.setEncodingSettings(QImageEncoderSettings());
    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::LoadedStatus);

    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    QTest::qWait(10);

    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::ActiveStatus);
    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    //the settings change should trigger camera stop/start only once
    camera.setCaptureMode(QCamera::CaptureVideo);
    camera.setCaptureMode(QCamera::CaptureStillImage);
    imageCapture.setEncodingSettings(QImageEncoderSettings());
    imageCapture.setEncodingSettings(QImageEncoderSettings());

    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::LoadedStatus);

    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);
    stateChangedSignal.clear();
    statusChangedSignal.clear();

    QTest::qWait(10);

    QCOMPARE(camera.state(), QCamera::ActiveState);
    QCOMPARE(camera.status(), QCamera::ActiveStatus);
    QCOMPARE(stateChangedSignal.count(), 0);
    QCOMPARE(statusChangedSignal.count(), 1);
}

void tst_QCamera::testSetVideoOutput()
{
    MockVideoSurface surface;

    MockCameraService service;
    MockMediaServiceProvider provider;
    provider.service = &service;
    QCamera camera(0, &provider);

    camera.setViewfinder(reinterpret_cast<QVideoWidget *>(0));

    camera.setViewfinder(reinterpret_cast<QGraphicsVideoItem *>(0));

    camera.setViewfinder(&surface);
    QVERIFY(service.rendererControl->surface() == &surface);

    camera.setViewfinder(reinterpret_cast<QAbstractVideoSurface *>(0));
    QVERIFY(service.rendererControl->surface() == 0);

    camera.setViewfinder(&surface);
    QVERIFY(service.rendererControl->surface() == &surface);

    camera.setViewfinder(reinterpret_cast<QVideoWidget *>(0));
    QVERIFY(service.rendererControl->surface() == 0);

    camera.setViewfinder(&surface);
    QVERIFY(service.rendererControl->surface() == &surface);
}


void tst_QCamera::testSetVideoOutputNoService()
{
    MockVideoSurface surface;

    MockMediaServiceProvider provider;
    provider.service = 0;
    QCamera camera(0, &provider);

    camera.setViewfinder(&surface);
    // Nothing we can verify here other than it doesn't assert.
}

void tst_QCamera::testSetVideoOutputNoControl()
{
    MockVideoSurface surface;

    MockCameraService service;
    service.rendererRef = 1;

    MockMediaServiceProvider provider;
    provider.service = &service;
    QCamera camera(0, &provider);

    camera.setViewfinder(&surface);
    QVERIFY(service.rendererControl->surface() == 0);
}

void tst_QCamera::testSetVideoOutputDestruction()
{
    MockVideoSurface surface;

    MockCameraService service;
    MockMediaServiceProvider provider;
    provider.service = &service;

    {
        QCamera camera(0, &provider);
        camera.setViewfinder(&surface);
        QVERIFY(service.rendererControl->surface() == &surface);
        QCOMPARE(service.rendererRef, 1);
    }
    QVERIFY(service.rendererControl->surface() == 0);
    QCOMPARE(service.rendererRef, 0);
}

void tst_QCamera::testEnumDebug()
{
    QTest::ignoreMessage(QtDebugMsg, "QCamera::ActiveState ");
    qDebug() << QCamera::ActiveState;
    QTest::ignoreMessage(QtDebugMsg, "QCamera::ActiveStatus ");
    qDebug() << QCamera::ActiveStatus;
    QTest::ignoreMessage(QtDebugMsg, "QCamera::CaptureVideo ");
    qDebug() << QCamera::CaptureVideo;
    QTest::ignoreMessage(QtDebugMsg, "QCamera::CameraError ");
    qDebug() << QCamera::CameraError;
    QTest::ignoreMessage(QtDebugMsg, "QCamera::Unlocked ");
    qDebug() << QCamera::Unlocked;
    QTest::ignoreMessage(QtDebugMsg, "QCamera::LockAcquired ");
    qDebug() << QCamera::LockAcquired;
    QTest::ignoreMessage(QtDebugMsg, "QCamera::NoLock ");
    qDebug() << QCamera::NoLock;
    QTest::ignoreMessage(QtDebugMsg, "QCamera::LockExposure ");
    qDebug() << QCamera::LockExposure;
}

void tst_QCamera::testCameraControl()
{
    MockCameraControl *m_cameraControl=new MockCameraControl(this);
    QVERIFY(m_cameraControl != NULL);
}

/* Test case for constructor with default provider */
void tst_QCamera::testConstructorWithDefaultProvider()
{
    QCamera *camera = new QCamera(0);
    QVERIFY(camera != NULL);
    QCOMPARE(camera->state(), QCamera::UnloadedState);
    delete camera;
}

/* captureModeChanged Signal test case. */
void tst_QCamera::testCaptureModeChanged_signal()
{
    MockCameraControl *m_cameraControl= new MockCameraControl(this);
    QSignalSpy spy(m_cameraControl, SIGNAL(captureModeChanged(QCamera::CaptureModes)));
    QVERIFY(spy.size() == 0);

    m_cameraControl->setCaptureMode(QCamera::CaptureVideo);
    QVERIFY(spy.size() == 1);

    m_cameraControl->setCaptureMode(QCamera::CaptureStillImage);
    QVERIFY(spy.size() == 2);
}

/* Test case for captureMode */
void tst_QCamera::testCaptureMode()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QVERIFY(camera.captureMode() == QCamera::CaptureStillImage);

    camera.setCaptureMode(QCamera::CaptureVideo);
    QVERIFY(camera.captureMode() == QCamera::CaptureVideo);
}

/* Test case for isCaptureModeSupported */
void tst_QCamera::testIsCaptureModeSupported()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QVERIFY(camera.isCaptureModeSupported(QCamera::CaptureStillImage) == true);
    QVERIFY(camera.isCaptureModeSupported(QCamera::CaptureVideo) == true);
}

/* Test case for requestedLocks. LockType is stored in OR combination so all
   types of combinations are verified here.*/
void tst_QCamera::testRequestedLocks()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    QCOMPARE(camera.requestedLocks(),QCamera::NoLock);

    camera.searchAndLock(QCamera::LockExposure);
    QCOMPARE(camera.requestedLocks(),QCamera::LockExposure);

    camera.unlock();
    camera.searchAndLock(QCamera::LockFocus);
    QCOMPARE(camera.requestedLocks(),QCamera::LockFocus );

    camera.unlock();
    camera.searchAndLock(QCamera::LockWhiteBalance);
    QCOMPARE(camera.requestedLocks(),QCamera::LockWhiteBalance);

    camera.unlock();
    camera.searchAndLock(QCamera::LockExposure |QCamera::LockFocus );
    QCOMPARE(camera.requestedLocks(),QCamera::LockExposure |QCamera::LockFocus );
    camera.searchAndLock(QCamera::LockWhiteBalance);
    QCOMPARE(camera.requestedLocks(),QCamera::LockExposure |QCamera::LockFocus|QCamera::LockWhiteBalance );
    camera.unlock(QCamera::LockExposure);
    QCOMPARE(camera.requestedLocks(),QCamera::LockFocus|QCamera::LockWhiteBalance );
    camera.unlock(QCamera::LockFocus);
    camera.searchAndLock(QCamera::LockExposure |QCamera::LockWhiteBalance );
    QCOMPARE(camera.requestedLocks(),QCamera::LockExposure|QCamera::LockWhiteBalance );
}

/* Test case for supportedLocks() */
void tst_QCamera::testSupportedLocks()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    QCOMPARE(camera.supportedLocks(),QCamera::LockExposure | QCamera::LockFocus);
}

/* Test case for isAvailable */
void tst_QCamera::testQCameraIsAvailable()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QVERIFY(camera.isAvailable());

    QCamera *camera1 = new QCamera("random");

    QVERIFY(camera1->error() == QCamera::ServiceMissingError);
    QVERIFY(!camera1->isAvailable());
    delete camera1;
}

/* Test case for availabilityError */
void tst_QCamera::testAvailabilityError()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QVERIFY(camera.availabilityError() == QtMultimedia::NoError);

    QCamera *camera1 = new QCamera("random");
    QVERIFY(camera1->availabilityError() == QtMultimedia::ServiceMissingError);
    delete camera1;
}

/* Test case for searchAndLock ( QCamera::LockTypes locks ) */
void tst_QCamera::testSearchAndLockWithLockTypes()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    QCOMPARE(camera.lockStatus(), QCamera::Unlocked);

    /* Spy the signals */
    QSignalSpy lockedSignal(&camera, SIGNAL(locked()));
    QSignalSpy lockFailedSignal(&camera, SIGNAL(lockFailed()));
    QSignalSpy lockStatusChangedSignal(&camera, SIGNAL(lockStatusChanged(QCamera::LockStatus, QCamera::LockChangeReason)));
    QSignalSpy lockStatusChangedSignalWithType(&camera, SIGNAL(lockStatusChanged(QCamera::LockType,QCamera::LockStatus, QCamera::LockChangeReason)));

    /* search and lock the camera with QCamera::LockExposure and verify if the signal is emitted correctly */
    camera.searchAndLock(QCamera::LockExposure);
    QCOMPARE(camera.lockStatus(), QCamera::Locked);
    QCOMPARE(lockedSignal.count(), 1);
    QCOMPARE(lockFailedSignal.count(), 0);
    QCOMPARE(lockStatusChangedSignal.count(), 1);
    QCOMPARE(lockStatusChangedSignalWithType.count(), 1);
}

/* Test case for setCaptureMode() */
void tst_QCamera::testSetCaptureMode()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    /* Set the capture mode and verify if it set correctly */
    camera.setCaptureMode(QCamera::CaptureVideo);
    QVERIFY(camera.captureMode() == QCamera::CaptureVideo);

    camera.setCaptureMode(QCamera::CaptureStillImage);
    QVERIFY(camera.captureMode() == QCamera::CaptureStillImage);
}

/* Test case for unlock (QCamera::LockTypes) */
void tst_QCamera::testUnlockWithType()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    QCOMPARE(camera.lockStatus(), QCamera::Unlocked);

    /* Spy the signal */
    QSignalSpy lockedSignal(&camera, SIGNAL(locked()));
    QSignalSpy lockFailedSignal(&camera, SIGNAL(lockFailed()));
    QSignalSpy lockStatusChangedSignal(&camera, SIGNAL(lockStatusChanged(QCamera::LockStatus, QCamera::LockChangeReason)));
    QSignalSpy lockStatusChangedSignalWithType(&camera, SIGNAL(lockStatusChanged(QCamera::LockType,QCamera::LockStatus, QCamera::LockChangeReason)));

    /* lock the camera with QCamera::LockExposure and Verify if the signal is emitted correctly */
    camera.searchAndLock(QCamera::LockExposure);
    QCOMPARE(camera.lockStatus(), QCamera::Locked);
    QCOMPARE(lockedSignal.count(), 1);
    QCOMPARE(lockFailedSignal.count(), 0);
    QCOMPARE(lockStatusChangedSignal.count(), 1);
    QCOMPARE(lockStatusChangedSignalWithType.count(), 1);

    /* Clear the signal */
    lockedSignal.clear();
    lockFailedSignal.clear();
    lockStatusChangedSignal.clear();
    lockStatusChangedSignalWithType.clear();

    /* Unlock the camera and verify if the signal is emitted correctly */
    camera.unlock(QCamera::LockExposure);
    QCOMPARE(camera.lockStatus(), QCamera::Unlocked);
    QCOMPARE(lockedSignal.count(), 0);
    QCOMPARE(lockFailedSignal.count(), 0);
    QCOMPARE(lockStatusChangedSignal.count(), 1);
    QCOMPARE(lockStatusChangedSignalWithType.count(), 1);
    QCamera::LockType lockType = qvariant_cast<QCamera::LockType >(lockStatusChangedSignalWithType.at(0).at(0));
    QCamera::LockStatus lockStatus = qvariant_cast<QCamera::LockStatus >(lockStatusChangedSignalWithType.at(0).at(1));
    QVERIFY(lockType == QCamera::LockExposure);
    QVERIFY(lockStatus == QCamera::Unlocked);

    lockedSignal.clear();
    lockFailedSignal.clear();
    lockStatusChangedSignal.clear();
    lockStatusChangedSignalWithType.clear();

    /* Lock the camera with QCamera::LockFocus */
    camera.searchAndLock(QCamera::LockFocus);
    lockedSignal.clear();
    lockFailedSignal.clear();
    lockStatusChangedSignal.clear();
    lockStatusChangedSignalWithType.clear();

    /* Unlock the camera and Verify if the signal is emitted correctly */
    camera.unlock(QCamera::LockFocus);
    QCOMPARE(camera.lockStatus(), QCamera::Unlocked);
    QCOMPARE(lockedSignal.count(), 0);
    QCOMPARE(lockFailedSignal.count(), 0);
    QCOMPARE(lockStatusChangedSignal.count(), 1);
    QCOMPARE(lockStatusChangedSignalWithType.count(), 1);
    lockType = qvariant_cast<QCamera::LockType >(lockStatusChangedSignalWithType.at(0).at(0));
    lockStatus = qvariant_cast<QCamera::LockStatus >(lockStatusChangedSignalWithType.at(0).at(1));
    QVERIFY(lockType == QCamera::LockFocus);
    QVERIFY(lockStatus == QCamera::Unlocked);
}

/* Test case for signal captureModeChanged(QCamera::CaptureModes) */
void tst_QCamera::testCaptureModeChangedSignal()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QVERIFY(camera.captureMode() == QCamera::CaptureStillImage);

    qRegisterMetaType<QCamera::CaptureModes>("QCamera::CaptureModes");

    /* Spy the signal */
    QSignalSpy lockCaptureModeChangedSignal(&camera, SIGNAL(captureModeChanged(QCamera::CaptureModes)));

    /* set the capture mode and Verify if the signal is emitted */
    camera.setCaptureMode(QCamera::CaptureVideo);
    QVERIFY(camera.captureMode() == QCamera::CaptureVideo);
    QCOMPARE(lockCaptureModeChangedSignal.count(), 1);
    QCamera::CaptureModes lockCaptureMode = qvariant_cast<QCamera::CaptureModes >(lockCaptureModeChangedSignal.at(0).at(0));
    QVERIFY(lockCaptureMode == QCamera::CaptureVideo);
}

/* Test case for signal lockStatusChanged(QCamera::LockType,QCamera::LockStatus, QCamera::LockChangeReason) */
void tst_QCamera::testLockStatusChangedWithTypesSignal()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    QCOMPARE(camera.lockStatus(), QCamera::Unlocked);

    /* Spy the signal lockStatusChanged(QCamera::LockType,QCamera::LockStatus, QCamera::LockChangeReason) */
    QSignalSpy lockStatusChangedSignalWithType(&camera, SIGNAL(lockStatusChanged(QCamera::LockType,QCamera::LockStatus, QCamera::LockChangeReason)));

    /* Lock the camera with type QCamera::LockExposure */
    camera.searchAndLock(QCamera::LockExposure);

    /* Verify if the signal is emitted and lock status is set correclty */
    QCOMPARE(camera.lockStatus(), QCamera::Locked);
    QCOMPARE(lockStatusChangedSignalWithType.count(), 1);
    QCamera::LockType lockType = qvariant_cast<QCamera::LockType >(lockStatusChangedSignalWithType.at(0).at(0));
    QCamera::LockStatus lockStatus = qvariant_cast<QCamera::LockStatus >(lockStatusChangedSignalWithType.at(0).at(1));
    QVERIFY(lockType == QCamera::LockExposure);
    QVERIFY(lockStatus == QCamera::Locked);

    lockStatusChangedSignalWithType.clear();

    /* Unlock the camera */
    camera.unlock();

    /* Verify if the signal is emitted and lock status is set correclty */
    QCOMPARE(camera.lockStatus(), QCamera::Unlocked);
    QCOMPARE(lockStatusChangedSignalWithType.count(), 1);
    lockType = qvariant_cast<QCamera::LockType >(lockStatusChangedSignalWithType.at(0).at(0));
    lockStatus = qvariant_cast<QCamera::LockStatus >(lockStatusChangedSignalWithType.at(0).at(1));
    QVERIFY(lockType == QCamera::LockExposure);
    QVERIFY(lockStatus == QCamera::Unlocked);
}

/* Test case for verifying if error signal generated correctly */
void tst_QCamera::testErrorSignal()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    QSignalSpy spyError(&camera, SIGNAL(error(QCamera::Error)));

    /* Set the QCameraControl error and verify if the signal is emitted correctly in QCamera */
    service.mockControl->setError(QCamera::CameraError,QString("Camera Error"));

    QVERIFY(spyError.count() == 1);
    QCamera::Error err = qvariant_cast<QCamera::Error >(spyError.at(0).at(0));
    QVERIFY(err == QCamera::CameraError);

    spyError.clear();

    /* Set the QCameraControl error and verify if the signal is emitted correctly in QCamera */
    service.mockControl->setError(QCamera::InvalidRequestError,QString("InvalidRequestError Error"));
    QVERIFY(spyError.count() == 1);
    err = qvariant_cast<QCamera::Error >(spyError.at(0).at(0));
    QVERIFY(err == QCamera::InvalidRequestError);

    spyError.clear();

    /* Set the QCameraControl error and verify if the signal is emitted correctly in QCamera */
    service.mockControl->setError(QCamera::NotSupportedFeatureError,QString("NotSupportedFeatureError Error"));
    QVERIFY(spyError.count() == 1);
    err = qvariant_cast<QCamera::Error >(spyError.at(0).at(0));
    QVERIFY(err == QCamera::NotSupportedFeatureError);

}

/* Test case for verifying the QCamera error */
void tst_QCamera::testError()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    /* Set the QCameraControl error and verify if it is set correctly in QCamera */
    service.mockControl->setError(QCamera::CameraError,QString("Camera Error"));
    QVERIFY(camera.error() == QCamera::CameraError);

    /* Set the QCameraControl error and verify if it is set correctly in QCamera */
    service.mockControl->setError(QCamera::InvalidRequestError,QString("InvalidRequestError Error"));
    QVERIFY(camera.error() == QCamera::InvalidRequestError);

    /* Set the QCameraControl error and verify if it is set correctly in QCamera */
    service.mockControl->setError(QCamera::NotSupportedFeatureError,QString("NotSupportedFeatureError Error"));
    QVERIFY(camera.error() == QCamera::NotSupportedFeatureError);

}

/* Test the error strings for QCamera class */
void tst_QCamera::testErrorString()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    /* Set the QCameraControl error and verify if it is set correctly in QCamera */
    service.mockControl->setError(QCamera::CameraError,QString("Camera Error"));
    QVERIFY(camera.errorString() == QString("Camera Error"));

    /* Set the QCameraControl error and verify if it is set correctly in QCamera */
    service.mockControl->setError(QCamera::InvalidRequestError,QString("InvalidRequestError Error"));
    QVERIFY(camera.errorString() == QString("InvalidRequestError Error"));

    /* Set the QCameraControl error and verify if it is set correctly in QCamera */
    service.mockControl->setError(QCamera::NotSupportedFeatureError,QString("NotSupportedFeatureError Error"));
    QVERIFY(camera.errorString() == QString("NotSupportedFeatureError Error"));
}

/* Test case for verifying Status of QCamera. */
void tst_QCamera::testStatus()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    /* Set the QCameraControl status and verify if it is set correctly in QCamera */
    service.mockControl->setStatus(QCamera::StartingStatus);
    QVERIFY(camera.status() == QCamera::StartingStatus);

    /* Set the QCameraControl status and verify if it is set correctly in QCamera */
    service.mockControl->setStatus(QCamera::StandbyStatus);
    QVERIFY(camera.status() == QCamera::StandbyStatus);

    /* Set the QCameraControl status and verify if it is set correctly in QCamera */
    service.mockControl->setStatus(QCamera::LoadingStatus);
    QVERIFY(camera.status() == QCamera::LoadingStatus);

    /* Set the QCameraControl status and verify if it is set correctly in QCamera */
    service.mockControl->setStatus(QCamera::UnavailableStatus);
    QVERIFY(camera.status() == QCamera::UnavailableStatus);
}

/* Test case for verifying default locktype QCamera::NoLock */
void tst_QCamera::testLockType()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    QCOMPARE(camera.requestedLocks(),QCamera::NoLock);
}

/* Test case for QCamera::LockChangeReason with QCamera::LockAcquired */
void tst_QCamera::testLockChangeReason()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);

    QSignalSpy lockStatusChangedSignalWithType(&camera, SIGNAL(lockStatusChanged(QCamera::LockType,QCamera::LockStatus, QCamera::LockChangeReason)));

    /* Set the lockChangeReason */
    service.mockLocksControl->setLockChangeReason(QCamera::LockAcquired);

    /* Verify if lockChangeReson is eqaul toQCamera::LockAcquired */
    QCOMPARE(lockStatusChangedSignalWithType.count(), 1);
    QCamera::LockChangeReason LockChangeReason = qvariant_cast<QCamera::LockChangeReason >(lockStatusChangedSignalWithType.at(0).at(2));
    QVERIFY(LockChangeReason == QCamera::LockAcquired);

}
/* All the enums test case for QCameraControl class*/
void tst_QCamera::testEnumsOfQCameraControl()
{
    MockCameraControl *m_cameraControl = new MockCameraControl(this);
    bool result;

    // In still mode, can't change much
    QVERIFY(m_cameraControl->captureMode() == QCamera::CaptureStillImage);
    result = m_cameraControl->canChangeProperty(MockCameraControl::CaptureMode, QCamera::ActiveStatus);
    QVERIFY(!result);
    result = m_cameraControl->canChangeProperty(MockCameraControl::ImageEncodingSettings, QCamera::ActiveStatus);
    QVERIFY(!result);
    result = m_cameraControl->canChangeProperty(MockCameraControl::VideoEncodingSettings, QCamera::ActiveStatus);
    QVERIFY(result);
    result = m_cameraControl->canChangeProperty(MockCameraControl::Viewfinder, QCamera::ActiveStatus);
    QVERIFY(!result);

    // In video mode can change image encoding settings
    m_cameraControl->setCaptureMode(QCamera::CaptureVideo);
    result = m_cameraControl->canChangeProperty(MockCameraControl::ImageEncodingSettings, QCamera::ActiveStatus);
    QVERIFY(result);
    result = m_cameraControl->canChangeProperty(MockCameraControl::VideoEncodingSettings, QCamera::ActiveStatus);
    QVERIFY(result);
    result = m_cameraControl->canChangeProperty(MockCameraControl::Viewfinder, QCamera::ActiveStatus);
    QVERIFY(!result);

    // Flip the allow everything bit
    m_cameraControl->m_propertyChangesSupported = true;
    result = m_cameraControl->canChangeProperty(MockCameraControl::CaptureMode, QCamera::ActiveStatus);
    QVERIFY(result);
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
    MockCameraService service;
    MockMediaServiceProvider provider;
    provider.service = &service;

    QCamera camera(0, &provider);
    QCameraImageProcessing *cameraImageProcessing = camera.imageProcessing();
    QVERIFY(cameraImageProcessing->contrast() ==0);

    cameraImageProcessing->setContrast(123);
    QVERIFY(cameraImageProcessing->contrast() ==123);

    cameraImageProcessing->setContrast(4.56);
    QVERIFY(cameraImageProcessing->contrast() ==4);
}

void tst_QCamera::testDenoisingLevel()
{
    MockCameraService service;
    MockMediaServiceProvider provider;
    provider.service = &service;

    QCamera camera(0, &provider);
    QCameraImageProcessing *cameraImageProcessing = camera.imageProcessing();

    if (cameraImageProcessing->isDenoisingSupported())
    {
        QVERIFY(cameraImageProcessing->denoisingLevel() == -1);

        cameraImageProcessing->setDenoisingLevel(0);
        QVERIFY(cameraImageProcessing->denoisingLevel() == 0);

        cameraImageProcessing->setDenoisingLevel(12);
        QVERIFY(cameraImageProcessing->denoisingLevel() == 12);
    }
    else
        QVERIFY(cameraImageProcessing->denoisingLevel() == -1);
}

void tst_QCamera::testIsAvailable()
{
    MockCameraService service;
    MockMediaServiceProvider provider;
    provider.service = &service;

    QCamera camera(0, &provider);
    QCameraImageProcessing *cameraImageProcessing = camera.imageProcessing();
    QVERIFY(cameraImageProcessing->isAvailable() == true);
}

void tst_QCamera::testSaturation()
{
    MockCameraService service;
    MockMediaServiceProvider provider;
    provider.service = &service;

    QCamera camera(0, &provider);
    QCameraImageProcessing *cameraImageProcessing = camera.imageProcessing();
    QVERIFY(cameraImageProcessing->saturation() == 0);

    cameraImageProcessing->setSaturation(50);
    QVERIFY(cameraImageProcessing->saturation() == 50);

    cameraImageProcessing->setSaturation(-50);
    QVERIFY(cameraImageProcessing->saturation() == -50);

    cameraImageProcessing->setSaturation(100);
    QVERIFY(cameraImageProcessing->saturation() == 100);

    cameraImageProcessing->setSaturation(-100);
    QVERIFY(cameraImageProcessing->saturation() == -100);
}

void tst_QCamera::testSharpeningLevel()
{
    MockCameraService service;
    MockMediaServiceProvider provider;
    provider.service = &service;

    QCamera camera(0, &provider);
    QCameraImageProcessing *cameraImageProcessing = camera.imageProcessing();
    QVERIFY(cameraImageProcessing->isSharpeningSupported());
    QVERIFY(cameraImageProcessing->sharpeningLevel() == -1);

    cameraImageProcessing->setSharpeningLevel(123);
    QVERIFY(cameraImageProcessing->sharpeningLevel() == 123);

    cameraImageProcessing->setSharpeningLevel(4.67);
    QVERIFY(cameraImageProcessing->sharpeningLevel() == 4);
}

void tst_QCamera::testEnumOfQCameraImageProcessing()
{
    QSet<QCameraImageProcessing::WhiteBalanceMode> whiteBalanceModes;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceManual;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceAuto;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceSunlight;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceCloudy;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceShade;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceTungsten;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceFluorescent;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceFlash;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceSunset;
    whiteBalanceModes << QCameraImageProcessing::WhiteBalanceVendor;

    MockCameraService service;
    service.mockImageProcessingControl->setSupportedWhiteBalanceModes(whiteBalanceModes);

    service.mockImageProcessingControl->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceManual);
    QVERIFY(service.mockImageProcessingControl->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceManual));
    QVERIFY(service.mockImageProcessingControl->whiteBalanceMode() == QCameraImageProcessing::WhiteBalanceManual);

    service.mockImageProcessingControl->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceAuto);
    QVERIFY(service.mockImageProcessingControl->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceAuto));
    QVERIFY(service.mockImageProcessingControl->whiteBalanceMode() == QCameraImageProcessing::WhiteBalanceAuto);

    service.mockImageProcessingControl->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceSunlight);
    QVERIFY(service.mockImageProcessingControl->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceSunlight));
    QVERIFY(service.mockImageProcessingControl->whiteBalanceMode() == QCameraImageProcessing::WhiteBalanceSunlight);

    service.mockImageProcessingControl->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceCloudy);
    QVERIFY(service.mockImageProcessingControl->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceCloudy));
    QVERIFY(service.mockImageProcessingControl->whiteBalanceMode() == QCameraImageProcessing::WhiteBalanceCloudy);

    service.mockImageProcessingControl->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceShade);
    QVERIFY(service.mockImageProcessingControl->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceShade));
    QVERIFY(service.mockImageProcessingControl->whiteBalanceMode() == QCameraImageProcessing::WhiteBalanceShade);

    service.mockImageProcessingControl->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceTungsten);
    QVERIFY(service.mockImageProcessingControl->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceTungsten));
    QVERIFY(service.mockImageProcessingControl->whiteBalanceMode() == QCameraImageProcessing::WhiteBalanceTungsten);

    service.mockImageProcessingControl->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceFluorescent);
    QVERIFY(service.mockImageProcessingControl->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceFluorescent));
    QVERIFY(service.mockImageProcessingControl->whiteBalanceMode() == QCameraImageProcessing::WhiteBalanceFluorescent);

    service.mockImageProcessingControl->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceFlash);
    QVERIFY(service.mockImageProcessingControl->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceFlash));
    QVERIFY(service.mockImageProcessingControl->whiteBalanceMode() == QCameraImageProcessing::WhiteBalanceFlash);

    service.mockImageProcessingControl->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceSunset);
    QVERIFY(service.mockImageProcessingControl->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceSunset));
    QVERIFY(service.mockImageProcessingControl->whiteBalanceMode() == QCameraImageProcessing::WhiteBalanceSunset);

    service.mockImageProcessingControl->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceVendor);
    QVERIFY(service.mockImageProcessingControl->isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceVendor));
    QVERIFY(service.mockImageProcessingControl->whiteBalanceMode() == QCameraImageProcessing::WhiteBalanceVendor);
}

//Added test cases for QCameraFocus
void tst_QCamera::testCameraFocusIsAvailable()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != 0);
    QVERIFY(cameraFocus->isAvailable());
}

//Added this code to cover QCameraFocus::HyperfocalFocus and QCameraFocus::MacroFocus
//As the HyperfocalFocus and MacroFocus are not supported we can not set the focus mode to these Focus Modes
void tst_QCamera::testFocusModes()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != 0);
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
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != 0);
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
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != 0);
    QSignalSpy spy(cameraFocus,SIGNAL(maximumDigitalZoomChanged(qreal)));
    QVERIFY(spy.count() == 0);
    cameraFocus->zoomTo(5.0,6.0);
    QVERIFY(spy.count() == 1);
}

void tst_QCamera::testMaxOpticalZoomChangedSignal()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != 0);
    QSignalSpy spy(cameraFocus,SIGNAL(maximumOpticalZoomChanged(qreal)));
    QVERIFY(spy.count() == 0);
    cameraFocus->zoomTo(5.0,6.0);
    QVERIFY(spy.count() == 1);
}

void tst_QCamera::testfocusZonesChangedSignal()
{
    MockCameraService service;
    provider->service = &service;
    QCamera camera(0, provider);
    QCameraFocus *cameraFocus = camera.focus();
    QVERIFY(cameraFocus != 0);

    QSignalSpy spy(cameraFocus,SIGNAL(focusZonesChanged()));
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
    MockMediaServiceProvider provider1;
    MockCameraService service;
    provider1.service = &service;
    QCamera camera(0, &provider1);

    QCameraExposure *cameraExposure = camera.exposure(); //create camera expose instance
    QVERIFY(cameraExposure != 0);

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
    MockMediaServiceProvider provider1;
    MockCameraService service;
    provider1.service = &service;
    QCamera camera(0, &provider1);

    QCameraExposure *cameraExposure = camera.exposure(); //create camera expose instance
    QVERIFY(cameraExposure != 0);

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

    MockMediaServiceProvider provider1;
    MockCameraService service;
    provider1.service = &service;
    QCamera camera(0, &provider1);

    QCameraExposure *cameraExposure = camera.exposure(); //create camera expose instance
    QVERIFY(cameraExposure != 0);

    QSignalSpy spyisoSensitivityChanged(cameraExposure , SIGNAL(isoSensitivityChanged(int)));

    QVERIFY(spyisoSensitivityChanged.count() ==0);

    cameraExposure->setManualIsoSensitivity(800); //set the manualiso sentivity to 800
    QTest::qWait(100);
    QVERIFY(spyisoSensitivityChanged.count() ==1);

}
void tst_QCamera::testSignalShutterSpeedChanged()
{

    MockMediaServiceProvider provider1;
    MockCameraService service;
    provider1.service = &service;
    QCamera camera(0, &provider1);

    QCameraExposure *cameraExposure = camera.exposure(); //create camera expose instance
    QVERIFY(cameraExposure != 0);

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
    MockMediaServiceProvider provider1;
    MockCameraService service;
    provider1.service = &service;
    QCamera camera(0, &provider1);

    QCameraExposure *cameraExposure = camera.exposure(); //create camera expose instance
    QVERIFY(cameraExposure != 0);


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
