// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QDebug>

#include <qvideosink.h>
#include <private/qplatformcamera_p.h>
#include <private/qplatformimagecapture_p.h>
#include <qcamera.h>
#include <qcameradevice.h>
#include <qimagecapture.h>
#include <qmediacapturesession.h>
#include <qobject.h>
#include <qmediadevices.h>

#include "qmockintegration.h"
#include "qmockmediacapturesession.h"
#include "qmockcamera.h"

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
    void testCameraEncodingProperyChange();

    void testConstructor();
    void testQCameraIsAvailable();
    void testErrorSignal();
    void testError();
    void testErrorString();

    void testSetCameraFormat();

    // Test cases to for focus handling
    void testFocusMode();
    void testZoomChanged();
    void testMaxZoomChangedSignal();

    // Test cases for QPlatformCamera class.
    void testCameraControl();

    void testSetVideoOutput();
    void testSetVideoOutputDestruction();

    void testEnumDebug();

    // Signals test cases for exposure related properties
    void testSignalExposureCompensationChanged();
    void testSignalIsoSensitivityChanged();
    void testSignalShutterSpeedChanged();
    void testSignalFlashReady();

private:
    QMockIntegrationFactory mockIntegrationFactory;
};

void tst_QCamera::initTestCase()
{
#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("Flakiness on macOS CI, to be investigated, QTBUG-111812");
#endif
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
    QVERIFY(camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceAuto));
    QVERIFY(!camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceManual));
    QVERIFY(!camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceCloudy));
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceAuto);
    camera.setWhiteBalanceMode(QCamera::WhiteBalanceCloudy);
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceAuto);
    QCOMPARE(camera.colorTemperature(), 0);
    camera.setColorTemperature(5000);
    QCOMPARE(camera.colorTemperature(), 0);
}

void tst_QCamera::testSimpleCameraExposure()
{
    QMockCamera::Simple simple;

    QCamera camera;
    QVERIFY(camera.isExposureModeSupported(QCamera::ExposureAuto));
    QVERIFY(!camera.isExposureModeSupported(QCamera::ExposureManual));
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
    QCOMPARE(camera.minimumIsoSensitivity(), -1);
    QCOMPARE(camera.maximumIsoSensitivity(), -1);
    camera.setManualIsoSensitivity(100);
    QCOMPARE(camera.isoSensitivity(), -1);
    camera.setAutoIsoSensitivity();
    QCOMPARE(camera.isoSensitivity(), -1);

    QVERIFY(camera.exposureTime() < 0);
    QCOMPARE(camera.minimumExposureTime(), -1.);
    QCOMPARE(camera.maximumExposureTime(), -1.);
    camera.setAutoExposureTime();
    QVERIFY(camera.exposureTime() < 0);
    camera.setManualExposureTime(1/128.0);
    QVERIFY(camera.exposureTime() < 0);
}

void tst_QCamera::testSimpleCameraFocus()
{
    QMockCamera::Simple simple;

    QCamera camera;

    QVERIFY(camera.isFocusModeSupported(QCamera::FocusModeAuto));
    QVERIFY(!camera.isFocusModeSupported(QCamera::FocusModeInfinity));

    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);
    camera.setFocusMode(QCamera::FocusModeInfinity);
    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);

    QCOMPARE(camera.maximumZoomFactor(), 1.0);
    QCOMPARE(camera.minimumZoomFactor(), 1.0);
    QCOMPARE(camera.zoomFactor(), 1.0);

    camera.zoomTo(100.0, 100.0);
    QCOMPARE(camera.zoomFactor(), 1.0);

    QCOMPARE(camera.customFocusPoint(), QPointF(-1., -1.));
    camera.setCustomFocusPoint(QPointF(1.0, 1.0));
    QCOMPARE(camera.customFocusPoint(), QPointF(-1., -1.));
}

void tst_QCamera::testSimpleCameraCapture()
{
    QMockCamera::Simple simple;

    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(!imageCapture.isReadyForCapture());
    QVERIFY(imageCapture.isAvailable());

    QCOMPARE(imageCapture.error(), QImageCapture::NoError);
    QVERIFY(imageCapture.errorString().isEmpty());

    QSignalSpy errorSignal(&imageCapture, SIGNAL(errorOccurred(int,QImageCapture::Error,QString)));
    imageCapture.captureToFile(QString::fromLatin1("/dev/null"));
    QCOMPARE(errorSignal.size(), 1);
    QCOMPARE(imageCapture.error(), QImageCapture::NotReadyError);
    QVERIFY(!imageCapture.errorString().isEmpty());

    camera.start();
    imageCapture.captureToFile(QString::fromLatin1("/dev/null"));
    QCOMPARE(errorSignal.size(), 1);
    QCOMPARE(imageCapture.error(), QImageCapture::NoError);
    QVERIFY(imageCapture.errorString().isEmpty());
}

void tst_QCamera::testCameraCapture()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(!imageCapture.isReadyForCapture());

    QSignalSpy capturedSignal(&imageCapture, SIGNAL(imageCaptured(int,QImage)));
    QSignalSpy errorSignal(&imageCapture, SIGNAL(errorOccurred(int,QImageCapture::Error,QString)));

    imageCapture.captureToFile(QString::fromLatin1("/dev/null"));
    QCOMPARE(capturedSignal.size(), 0);
    QCOMPARE(errorSignal.size(), 1);
    QCOMPARE(imageCapture.error(), QImageCapture::NotReadyError);

    errorSignal.clear();

    camera.start();
    QVERIFY(imageCapture.isReadyForCapture());
    QCOMPARE(errorSignal.size(), 0);

    imageCapture.captureToFile(QString::fromLatin1("/dev/null"));

    QTRY_COMPARE(capturedSignal.size(), 1);
    QCOMPARE(errorSignal.size(), 0);
    QCOMPARE(imageCapture.error(), QImageCapture::NoError);
}

void tst_QCamera::testCameraCaptureMetadata()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
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
    QCOMPARE(data.keys().size(), 2);
    QCOMPARE(data[QMediaMetaData::Author].toString(), "Author");
    QCOMPARE(data[QMediaMetaData::Date].toDateTime().date().year(), 2021);
}

void tst_QCamera::testCameraWhiteBalance()
{
    QCamera camera;

    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceAuto);
    camera.setWhiteBalanceMode(QCamera::WhiteBalanceFlash);
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceAuto);
    QVERIFY(camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceAuto));
    QVERIFY(camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceManual));
    QVERIFY(camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceSunlight));
    QVERIFY(!camera.isWhiteBalanceModeSupported(QCamera::WhiteBalanceCloudy));

    camera.setWhiteBalanceMode(QCamera::WhiteBalanceSunlight);
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceSunlight);

    camera.setWhiteBalanceMode(QCamera::WhiteBalanceManual);
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceManual);
    camera.setColorTemperature(4000);
    QCOMPARE(camera.colorTemperature(), 4000);

    camera.setColorTemperature(8000);
    QCOMPARE(camera.colorTemperature(), 8000);

    camera.setWhiteBalanceMode(QCamera::WhiteBalanceAuto);
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceAuto);
    camera.setColorTemperature(4000);
    QCOMPARE(camera.colorTemperature(), 4000);
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceManual);

    camera.setColorTemperature(0);
    QCOMPARE(camera.colorTemperature(), 0);
    QCOMPARE(camera.whiteBalanceMode(), QCamera::WhiteBalanceAuto);
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

    int minIso = camera.minimumIsoSensitivity();
    int maxIso = camera.maximumIsoSensitivity();
    QCOMPARE(camera.isoSensitivity(), 100);
    QVERIFY(minIso > 0);
    QVERIFY(maxIso > 0);
    camera.setManualIsoSensitivity(minIso);
    QCOMPARE(camera.isoSensitivity(), minIso);
    camera.setManualIsoSensitivity(maxIso*10);
    QCOMPARE(camera.isoSensitivity(), maxIso);

    camera.setManualIsoSensitivity(-10);
    QCOMPARE(camera.isoSensitivity(), minIso);
    camera.setAutoIsoSensitivity();
    QCOMPARE(camera.isoSensitivity(), 100);

    qreal minExposureTime = camera.minimumExposureTime();
    qreal maxExposureTime = camera.maximumExposureTime();
    QVERIFY(minExposureTime > 0);
    QVERIFY(maxExposureTime > 0);
    QVERIFY(camera.exposureTime() >= minExposureTime);
    QVERIFY(camera.exposureTime() <= maxExposureTime);

    camera.setAutoExposureTime();
    QVERIFY(camera.exposureTime() >= minExposureTime);
    QVERIFY(camera.exposureTime() <= maxExposureTime);

    camera.setManualExposureTime(0);
    QCOMPARE(camera.exposureTime(), minExposureTime);

    camera.setManualExposureTime(10000);
    QCOMPARE(camera.exposureTime(), maxExposureTime);

    camera.setAutoExposureTime();
}

void tst_QCamera::testCameraFocus()
{
    QCamera camera;

    QVERIFY(camera.isFocusModeSupported(QCamera::FocusModeAuto));
    QVERIFY(camera.isFocusModeSupported(QCamera::FocusModeAuto));
    QVERIFY(!camera.isFocusModeSupported(QCamera::FocusModeInfinity));

    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);
    camera.setFocusMode(QCamera::FocusModeManual);
    QCOMPARE(camera.focusMode(), QCamera::FocusModeManual);
    camera.setFocusMode(QCamera::FocusModeInfinity);
    QCOMPARE(camera.focusMode(), QCamera::FocusModeManual);
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

void tst_QCamera::testCameraEncodingProperyChange()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy activeChangedSignal(&camera, SIGNAL(activeChanged(bool)));

    camera.start();
    QCOMPARE(camera.isActive(), true);
    QCOMPARE(activeChangedSignal.size(), 1);
}

void tst_QCamera::testSetVideoOutput()
{
    QMediaCaptureSession session;
    QVideoSink surface;
    QCamera camera;
    session.setCamera(&camera);

    QVERIFY(!session.videoOutput());

    session.setVideoOutput(&surface);
    QVERIFY(session.videoOutput() == &surface);
    QVERIFY(session.videoSink() == &surface);

    session.setVideoOutput(static_cast<QVideoSink *>(nullptr));
    QVERIFY(session.videoOutput() == nullptr);
    QVERIFY(session.videoSink() == nullptr);

    session.setVideoOutput(&surface);
    QVERIFY(session.videoOutput() == &surface);
    QVERIFY(session.videoSink() == &surface);

    session.setVideoSink(&surface);
    QVERIFY(session.videoOutput() == nullptr);
    QVERIFY(session.videoSink() == &surface);

    session.setVideoOutput(&surface);
    QVERIFY(session.videoOutput() == &surface);
    QVERIFY(session.videoSink() == &surface);

    session.setVideoSink(nullptr);
    QVERIFY(session.videoOutput() == nullptr);
    QVERIFY(session.videoSink() == nullptr);
}



void tst_QCamera::testSetVideoOutputDestruction()
{
    {
        QVideoSink surface;
        {
            QMediaCaptureSession session;
            QCamera camera;
            session.setCamera(&camera);

            QVERIFY(session.videoOutput() == nullptr);
            session.setVideoOutput(&surface);
        }
    }

    {
        QMediaCaptureSession session;
        {
            QVideoSink surface;
            QCamera camera;
            session.setCamera(&camera);

            QVERIFY(session.videoOutput() == nullptr);
            session.setVideoOutput(&surface);
        }
    }

    {
        QCamera camera;
        {
            QMediaCaptureSession session;
            QVideoSink surface;
            session.setCamera(&camera);

            QVERIFY(session.videoOutput() == nullptr);
            session.setVideoOutput(&surface);
        }
    }
    {
        QMediaCaptureSession session;
        {
            QVideoSink surface;
            QCamera camera;
            session.setCamera(&camera);

            QVERIFY(session.videoOutput() == nullptr);
            session.setVideoOutput(&surface);
        }
    }
}

void tst_QCamera::testEnumDebug()
{
    QTest::ignoreMessage(QtDebugMsg, "QCamera::CameraError");
    qDebug() << QCamera::CameraError;
//    QTest::ignoreMessage(QtDebugMsg, "QCameraDevice::FrontFace");
//    qDebug() << QCameraDevice::FrontFace;
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
    QCameraDevice defaultCamera = QMediaDevices::defaultVideoInput();
    QCameraDevice frontCamera, backCamera;
    for (const auto &c : cameras) {
        if (frontCamera.isNull() && c.position() == QCameraDevice::FrontFace)
            frontCamera = c;
        if (backCamera.isNull() && c.position() == QCameraDevice::BackFace)
            backCamera = c;
    }
    QVERIFY(!defaultCamera.isNull());
    QVERIFY(!frontCamera.isNull());
    QVERIFY(!backCamera.isNull());

    {
        QCamera camera;
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraDevice(), defaultCamera);
    }

    {
        QCamera camera(QCameraDevice::FrontFace);
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraDevice(), frontCamera);
    }

    {
        QCamera camera(QMediaDevices::defaultVideoInput());
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraDevice(), defaultCamera);
    }

    {
        QCameraDevice cameraDevice = QMediaDevices::videoInputs().at(0);
        QCamera camera(cameraDevice);
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraDevice(), cameraDevice);
    }

    {
        QCamera camera(QCameraDevice::BackFace);
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraDevice(), backCamera);
    }

    {
        // Should load the default camera when UnspecifiedPosition is requested
        QCamera camera(QCameraDevice::UnspecifiedPosition);
        QCOMPARE(camera.isAvailable(), true);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraDevice(), defaultCamera);
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
    auto *service = QMockIntegration::instance()->lastCaptureService();
    Q_ASSERT(service);
    Q_ASSERT(service->mockCameraControl);

    QSignalSpy spyError(&camera, SIGNAL(errorOccurred(QCamera::Error,const QString&)));

    /* Set the QPlatformCamera error and verify if the signal is emitted correctly in QCamera */
    service->mockCameraControl->setError(QCamera::CameraError,QString("Camera Error"));

    QVERIFY(spyError.size() == 1);
    QCamera::Error err = qvariant_cast<QCamera::Error >(spyError.at(0).at(0));
    QVERIFY(err == QCamera::CameraError);

    spyError.clear();

    /* Set the QPlatformCamera error and verify if the signal is emitted correctly in QCamera */
    service->mockCameraControl->setError(QCamera::CameraError,QString("InvalidRequestError Error"));
    QVERIFY(spyError.size() == 1);
    err = qvariant_cast<QCamera::Error >(spyError.at(0).at(0));
    QVERIFY(err == QCamera::CameraError);

    spyError.clear();

    /* Set the QPlatformCamera error and verify if the signal is emitted correctly in QCamera */
    service->mockCameraControl->setError(QCamera::CameraError,QString("NotSupportedFeatureError Error"));
    QVERIFY(spyError.size() == 1);
    err = qvariant_cast<QCamera::Error >(spyError.at(0).at(0));
    QVERIFY(err == QCamera::CameraError);

}

/* Test case for verifying the QCamera error */
void tst_QCamera::testError()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);
    auto *service = QMockIntegration::instance()->lastCaptureService();

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
    auto *service = QMockIntegration::instance()->lastCaptureService();

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

void tst_QCamera::testSetCameraFormat()
{
    QCamera camera;
    QCameraDevice device = camera.cameraDevice();
    auto videoFormats = device.videoFormats();
    QVERIFY(videoFormats.size());
    QCameraFormat cameraFormat = videoFormats.first();
    QSignalSpy spy(&camera, SIGNAL(cameraFormatChanged()));
    QVERIFY(spy.size() == 0);
    camera.setCameraFormat(cameraFormat);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(camera.cameraFormat(), cameraFormat);
    QCOMPARE(camera.cameraFormat().pixelFormat(), QVideoFrameFormat::Format_ARGB8888);
    QCOMPARE(camera.cameraFormat().resolution(), QSize(640, 480));
    QCOMPARE(camera.cameraFormat().minFrameRate(), 0);
    QCOMPARE(camera.cameraFormat().maxFrameRate(), 30);
    QCOMPARE(spy.size(), 1);

    spy.clear();
    camera.setCameraFormat({});
    QCOMPARE(spy.size(), 1);
    QCOMPARE(camera.cameraFormat(), QCameraFormat());

    spy.clear();
    camera.setCameraDevice(QMediaDevices::videoInputs().at(1));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(camera.cameraFormat(), QCameraFormat());
    camera.setCameraFormat(camera.cameraDevice().videoFormats().first());
    QCOMPARE(camera.cameraFormat().pixelFormat(), QVideoFrameFormat::Format_XRGB8888);
    QCOMPARE(camera.cameraFormat().resolution(), QSize(1280, 720));
    QCOMPARE(camera.cameraFormat().minFrameRate(), 0);
    QCOMPARE(camera.cameraFormat().maxFrameRate(), 30);
    QCOMPARE(spy.size(), 2);
}

//Added this code to cover QCamera::FocusModeHyperfocal and QCamera::FocusModeAutoNear
//As the FocusModeHyperfocal and FocusModeAutoNear are not supported we can not set the focus mode to these Focus Modes
void tst_QCamera::testFocusMode()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QVERIFY(!camera.isFocusModeSupported(QCamera::FocusModeInfinity));
    QVERIFY(camera.isFocusModeSupported(QCamera::FocusModeAutoNear));
    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);
    camera.setFocusMode(QCamera::FocusModeInfinity);
    QVERIFY(camera.focusMode() != QCamera::FocusModeInfinity);
    QCOMPARE(camera.focusMode(), QCamera::FocusModeAuto);
    camera.setFocusMode(QCamera::FocusModeAutoNear);
    QVERIFY(camera.focusMode() == QCamera::FocusModeAutoNear);
}

void tst_QCamera::testZoomChanged()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy spy(&camera, SIGNAL(zoomFactorChanged(float)));
    QVERIFY(spy.size() == 0);
    camera.setZoomFactor(2.0);
    QVERIFY(spy.size() == 1);
    camera.zoomTo(3.0, 1);
    QVERIFY(spy.size() == 2);
    camera.zoomTo(1.0, 0);
    QVERIFY(spy.size() == 3);
}

void tst_QCamera::testMaxZoomChangedSignal()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);
    QMockCamera *mock = QMockIntegration::instance()->lastCamera();

    // ### change max zoom factor on backend, e.g. by changing camera
    QSignalSpy spy(&camera, SIGNAL(maximumZoomFactorChanged(float)));
    mock->maximumZoomFactorChanged(55);
    QVERIFY(spy.size() == 1);
    QCOMPARE(camera.maximumZoomFactor(), 55);
}

void tst_QCamera::testSignalExposureCompensationChanged()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy spyExposureCompensationChanged(&camera, SIGNAL(exposureCompensationChanged(float)));

    QVERIFY(spyExposureCompensationChanged.size() ==0);

    QVERIFY(camera.exposureCompensation() != 800);
    camera.setExposureCompensation(2.0);

    QTest::qWait(100);

    QVERIFY(camera.exposureCompensation() == 2.0);

    QCOMPARE(spyExposureCompensationChanged.size(),1);

    // Setting the same should not result in a signal
    camera.setExposureCompensation(2.0);
    QTest::qWait(100);

    QVERIFY(camera.exposureCompensation() == 2.0);
    QCOMPARE(spyExposureCompensationChanged.size(),1);
}

void tst_QCamera::testSignalIsoSensitivityChanged()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy spyisoSensitivityChanged(&camera, SIGNAL(isoSensitivityChanged(int)));

    QVERIFY(spyisoSensitivityChanged.size() ==0);

    camera.setManualIsoSensitivity(800); //set the manualiso sentivity to 800
    QTest::qWait(100);
    QVERIFY(spyisoSensitivityChanged.size() ==1);

}
void tst_QCamera::testSignalShutterSpeedChanged()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy spySignalExposureTimeChanged(&camera, SIGNAL(exposureTimeChanged(float)));

    QVERIFY(spySignalExposureTimeChanged.size() ==0);

    camera.setManualExposureTime(2.0);//set the ManualShutterSpeed to 2.0
    QTest::qWait(100);

    QVERIFY(spySignalExposureTimeChanged.size() ==1);
}

void tst_QCamera::testSignalFlashReady()
{
    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy spyflashReady(&camera,SIGNAL(flashReady(bool)));

    QVERIFY(spyflashReady.size() == 0);

    QVERIFY(camera.flashMode() == QCamera::FlashAuto);

    camera.setFlashMode(QCamera::FlashOff);//set theFlashMode to QCamera::FlashOff

    QVERIFY(camera.flashMode() == QCamera::FlashOff);

    QCOMPARE(spyflashReady.size(), 1);
}

QTEST_MAIN(tst_QCamera)

#include "tst_qcamera.moc"
