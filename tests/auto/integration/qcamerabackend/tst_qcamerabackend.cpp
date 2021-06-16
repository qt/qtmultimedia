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
#include <QtGui/QImageReader>
#include <QDebug>

#include <private/qplatformcamera_p.h>
#include <private/qplatformcameraimagecapture_p.h>
#include <qcamera.h>
#include <qcameradevice.h>
#include <qcameraimagecapture.h>
#include <qmediacapturesession.h>
#include <qobject.h>
#include <qmediadevices.h>
#include <qmediarecorder.h>

QT_USE_NAMESPACE

/*
 This is the backend conformance test.

 Since it relies on platform media framework and sound hardware
 it may be less stable.
*/

class tst_QCameraBackend: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void testCameraDevice();
    void testCtorWithCameraDevice();
    void testCtorWithPosition();

    void testCameraStates();
    void testCameraStartParallel();
    void testCameraCapture();
    void testCaptureToBuffer();
    void testCameraCaptureMetadata();
    void testExposureCompensation();
    void testExposureMode();

    void testVideoRecording_data();
    void testVideoRecording();
private:
    bool noCamera = false;
};

void tst_QCameraBackend::initTestCase()
{
    QCamera camera;
    noCamera = !camera.isAvailable();
}

void tst_QCameraBackend::cleanupTestCase()
{
}

void tst_QCameraBackend::testCameraDevice()
{
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty()) {
        QVERIFY(noCamera);
        QVERIFY(QMediaDevices::defaultVideoInput().isNull());
        QSKIP("Camera selection is not supported");
    }
    QVERIFY(!noCamera);

    for (const QCameraDevice &info : cameras) {
        QVERIFY(!info.id().isEmpty());
        QVERIFY(!info.description().isEmpty());
    }
}

void tst_QCameraBackend::testCtorWithCameraDevice()
{
    {
        // loading an invalid CameraDevice should fail
        QCamera camera(QCameraDevice{});
        QCOMPARE(camera.error(), QCamera::CameraError);
        QVERIFY(camera.cameraDevice().isNull());
    }

    if (noCamera)
        QSKIP("No camera available");

    {
        QCameraDevice info = QMediaDevices::defaultVideoInput();
        QCamera camera(info);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraDevice(), info);
    }
    {
        QCameraDevice info = QMediaDevices::videoInputs().first();
        QCamera camera(info);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraDevice(), info);
    }
}

void tst_QCameraBackend::testCtorWithPosition()
{
    if (noCamera)
        QSKIP("No camera available");

    {
        QCamera camera(QCameraDevice::UnspecifiedPosition);
        QCOMPARE(camera.error(), QCamera::NoError);
    }
    {
        QCamera camera(QCameraDevice::FrontFace);
        // even if no camera is available at this position, it should not fail
        // and load the default camera
        QCOMPARE(camera.error(), QCamera::NoError);
    }
    {
        QCamera camera(QCameraDevice::BackFace);
        // even if no camera is available at this position, it should not fail
        // and load the default camera
        QCOMPARE(camera.error(), QCamera::NoError);
    }
}

void tst_QCameraBackend::testCameraStates()
{
    QMediaCaptureSession session;
    QCamera camera;
    camera.setCameraDevice(QCameraDevice());
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy errorSignal(&camera, SIGNAL(errorOccurred(QCamera::Error, const QString &)));
    QSignalSpy activeChangedSignal(&camera, SIGNAL(activeChanged(bool)));
    QSignalSpy statusChangedSignal(&camera, SIGNAL(statusChanged(QCamera::Status)));

    QCOMPARE(camera.isActive(), false);
    QCOMPARE(camera.status(), QCamera::InactiveStatus);

    // Camera should not startup with a null QCameraDevice as device
    camera.start();
    QCOMPARE(camera.isActive(), false);

    if (noCamera)
        QSKIP("No camera available");
    camera.setCameraDevice(QMediaDevices::defaultVideoInput());
    QCOMPARE(camera.error(), QCamera::NoError);

    camera.start();
    QCOMPARE(camera.isActive(), true);
    QTRY_COMPARE(activeChangedSignal.size(), 1);
    QCOMPARE(activeChangedSignal.last().first().value<bool>(), true);
    QTRY_COMPARE(camera.status(), QCamera::ActiveStatus);
    QCOMPARE(statusChangedSignal.last().first().value<QCamera::Status>(), QCamera::ActiveStatus);

    camera.stop();
    QCOMPARE(camera.isActive(), false);
    QCOMPARE(activeChangedSignal.last().first().value<bool>(), false);
    QTRY_COMPARE(camera.status(), QCamera::InactiveStatus);
    QCOMPARE(statusChangedSignal.last().first().value<QCamera::Status>(), QCamera::InactiveStatus);

    QCOMPARE(camera.errorString(), QString());
}

void tst_QCameraBackend::testCameraStartParallel()
{
    if (noCamera)
        QSKIP("No camera available");

    QMediaCaptureSession session1;
    QMediaCaptureSession session2;
    QCamera camera1(QMediaDevices::defaultVideoInput());
    QCamera camera2(QMediaDevices::defaultVideoInput());
    session1.setCamera(&camera1);
    session2.setCamera(&camera2);
    QSignalSpy errorSpy1(&camera1, &QCamera::errorOccurred);
    QSignalSpy errorSpy2(&camera2, &QCamera::errorOccurred);

    camera1.start();
    camera2.start();

    QCOMPARE(camera1.isActive(), true);
    QTRY_COMPARE(camera1.status(), QCamera::ActiveStatus);
    QCOMPARE(camera1.error(), QCamera::NoError);
    QCOMPARE(camera2.isActive(), true);
    QCOMPARE(camera2.error(), QCamera::NoError);

    QCOMPARE(errorSpy1.count(), 0);
    QCOMPARE(errorSpy2.count(), 0);
}

void tst_QCameraBackend::testCameraCapture()
{
    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    //prevents camera to flash during the test
    camera.setFlashMode(QCamera::FlashOff);

    QVERIFY(!imageCapture.isReadyForCapture());

    QSignalSpy capturedSignal(&imageCapture, SIGNAL(imageCaptured(int,QImage)));
    QSignalSpy savedSignal(&imageCapture, SIGNAL(imageSaved(int,QString)));
    QSignalSpy errorSignal(&imageCapture, SIGNAL(errorOccurred(int,QCameraImageCapture::Error,const QString&)));

    imageCapture.captureToFile();
    QTRY_COMPARE(errorSignal.size(), 1);
    QCOMPARE(imageCapture.error(), QCameraImageCapture::NotReadyError);
    QCOMPARE(capturedSignal.size(), 0);
    errorSignal.clear();

    if (noCamera)
        QSKIP("No camera available");

    camera.start();

    QTRY_VERIFY(imageCapture.isReadyForCapture());
    QCOMPARE(camera.status(), QCamera::ActiveStatus);
    QCOMPARE(errorSignal.size(), 0);

    int id = imageCapture.captureToFile();

    QTRY_VERIFY(!savedSignal.isEmpty());

    QTRY_COMPARE(capturedSignal.size(), 1);
    QCOMPARE(capturedSignal.last().first().toInt(), id);
    QCOMPARE(errorSignal.size(), 0);
    QCOMPARE(imageCapture.error(), QCameraImageCapture::NoError);

    QCOMPARE(savedSignal.last().first().toInt(), id);
    QString location = savedSignal.last().last().toString();
    QVERIFY(!location.isEmpty());
    QVERIFY(QFileInfo(location).exists());
    QImageReader reader(location);
    reader.setScaledSize(QSize(320,240));
    QVERIFY(!reader.read().isNull());

    QFile(location).remove();
}


void tst_QCameraBackend::testCaptureToBuffer()
{
    if (noCamera)
        QSKIP("No camera available");

    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    camera.setFlashMode(QCamera::FlashOff);

    camera.setActive(true);

    QTRY_COMPARE(camera.status(), QCamera::ActiveStatus);

    QSignalSpy capturedSignal(&imageCapture, SIGNAL(imageCaptured(int,QImage)));
    QSignalSpy imageAvailableSignal(&imageCapture, SIGNAL(imageAvailable(int,QVideoFrame)));
    QSignalSpy savedSignal(&imageCapture, SIGNAL(imageSaved(int,QString)));
    QSignalSpy errorSignal(&imageCapture, SIGNAL(errorOccurred(int,QCameraImageCapture::Error,const QString&)));

    camera.start();
    QTRY_VERIFY(imageCapture.isReadyForCapture());

    int id = imageCapture.capture();
    QTRY_VERIFY(!imageAvailableSignal.isEmpty());

    QVERIFY(errorSignal.isEmpty());
    QTRY_VERIFY(!capturedSignal.isEmpty());
    QVERIFY(!imageAvailableSignal.isEmpty());

    QTest::qWait(2000);
    QVERIFY(savedSignal.isEmpty());

    QCOMPARE(capturedSignal.first().first().toInt(), id);
    QCOMPARE(imageAvailableSignal.first().first().toInt(), id);

    QVideoFrame frame = imageAvailableSignal.first().last().value<QVideoFrame>();
    QVERIFY(!frame.toImage().isNull());

    frame = QVideoFrame();
    capturedSignal.clear();
    imageAvailableSignal.clear();
    savedSignal.clear();

    QTRY_VERIFY(imageCapture.isReadyForCapture());
}

void tst_QCameraBackend::testCameraCaptureMetadata()
{
    if (noCamera)
        QSKIP("No camera available");

    QMediaCaptureSession session;
    QCamera camera;
    QCameraImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    camera.setFlashMode(QCamera::FlashOff);

    QSignalSpy metadataSignal(&imageCapture, SIGNAL(imageMetadataAvailable(int,const QMediaMetaData&)));
    QSignalSpy savedSignal(&imageCapture, SIGNAL(imageSaved(int,QString)));

    camera.start();

    QTRY_VERIFY(imageCapture.isReadyForCapture());

    int id = imageCapture.captureToFile(QString::fromLatin1("/dev/null"));
    QTRY_VERIFY(!savedSignal.isEmpty());
    QVERIFY(!metadataSignal.isEmpty());
    QCOMPARE(metadataSignal.first().first().toInt(), id);
}

void tst_QCameraBackend::testExposureCompensation()
{
    if (noCamera)
        QSKIP("No camera available");

    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy exposureCompensationSignal(&camera, SIGNAL(exposureCompensationChanged(qreal)));

    //it should be possible to set exposure parameters in Unloaded state
    QCOMPARE(camera.exposureCompensation(), 0.);
    if (!(camera.supportedFeatures() & QCamera::Feature::ExposureCompensation))
        return;

    camera.setExposureCompensation(1.0);
    QCOMPARE(camera.exposureCompensation(), 1.0);
    QTRY_COMPARE(exposureCompensationSignal.count(), 1);
    QCOMPARE(exposureCompensationSignal.last().first().toReal(), 1.0);

    //exposureCompensationChanged should not be emitted when value is not changed
    camera.setExposureCompensation(1.0);
    QTest::qWait(50);
    QCOMPARE(exposureCompensationSignal.count(), 1);

    //exposure compensation should be preserved during start
    camera.start();
    QTRY_COMPARE(camera.status(), QCamera::ActiveStatus);

    QCOMPARE(camera.exposureCompensation(), 1.0);

    exposureCompensationSignal.clear();
    camera.setExposureCompensation(-1.0);
    QCOMPARE(camera.exposureCompensation(), -1.0);
    QTRY_COMPARE(exposureCompensationSignal.count(), 1);
    QCOMPARE(exposureCompensationSignal.last().first().toReal(), -1.0);
}

void tst_QCameraBackend::testExposureMode()
{
    if (noCamera)
        QSKIP("No camera available");

    QCamera camera;

    QCOMPARE(camera.exposureMode(), QCamera::ExposureAuto);

    // Night
    if (camera.isExposureModeSupported(QCamera::ExposureNight)) {
        camera.setExposureMode(QCamera::ExposureNight);
        QCOMPARE(camera.exposureMode(), QCamera::ExposureNight);
        camera.start();
        QVERIFY(camera.isActive());
        QCOMPARE(camera.exposureMode(), QCamera::ExposureNight);
    }

    camera.stop();
    QTRY_COMPARE(camera.status(), QCamera::InactiveStatus);

    // Auto
    camera.setExposureMode(QCamera::ExposureAuto);
    QCOMPARE(camera.exposureMode(), QCamera::ExposureAuto);
    camera.start();
    QVERIFY(camera.isActive());
    QCOMPARE(camera.exposureMode(), QCamera::ExposureAuto);

    // Manual
    if (camera.isExposureModeSupported(QCamera::ExposureManual)) {
        camera.setExposureMode(QCamera::ExposureManual);
        QCOMPARE(camera.exposureMode(), QCamera::ExposureManual);
        camera.start();
        QVERIFY(camera.isActive());
        QCOMPARE(camera.exposureMode(), QCamera::ExposureManual);

        camera.setManualExposureTime(.02); // ~20ms should be supported by most cameras
        QVERIFY(camera.manualExposureTime() > .01 && camera.manualExposureTime() < .04);
    }

    camera.setExposureMode(QCamera::ExposureAuto);
}

void tst_QCameraBackend::testVideoRecording_data()
{
    QTest::addColumn<QCameraDevice>("device");

    const auto devices = QMediaDevices::videoInputs();

    for (const auto &device : devices)
        QTest::newRow(device.description().toUtf8()) << device;

    if (devices.isEmpty())
        QTest::newRow("Null device") << QCameraDevice();
}

void tst_QCameraBackend::testVideoRecording()
{
    if (noCamera)
        QSKIP("No camera available");
    QFETCH(QCameraDevice, device);

    QMediaCaptureSession session;
    QScopedPointer<QCamera> camera(new QCamera(device));
    session.setCamera(camera.data());

    QMediaRecorder recorder;
    session.setEncoder(&recorder);

    QSignalSpy errorSignal(camera.data(), SIGNAL(errorOccurred(QCamera::Error, const QString &)));
    QSignalSpy recorderErrorSignal(&recorder, SIGNAL(errorOccurred(Error, const QString &)));
    QSignalSpy recorderStatusSignal(&recorder, SIGNAL(statusChanged(Status)));

    recorder.setVideoResolution(320, 240);

    QCOMPARE(recorder.status(), QMediaRecorder::StoppedStatus);

    camera->start();
    if (noCamera || device.isNull()) {
        QVERIFY(!camera->isActive());
        return;
    }
    QTRY_VERIFY(camera->isActive());

    QTRY_COMPARE(camera->status(), QCamera::ActiveStatus);
    QTRY_COMPARE(recorder.status(), QMediaRecorder::StoppedStatus);

    for (int recordings = 0; recordings < 2; ++recordings) {
        //record 200ms clip
        recorder.record();
        QTRY_COMPARE(recorder.status(), QMediaRecorder::RecordingStatus);
        QCOMPARE(recorderStatusSignal.last().first().value<QMediaRecorder::Status>(), recorder.status());
        QTest::qWait(200);
        recorderStatusSignal.clear();
        recorder.stop();
        bool foundFinalizingStatus = false;
        for (auto &list : recorderStatusSignal) {
            if (qvariant_cast<QMediaRecorder::Status>(list.first()) == QMediaRecorder::FinalizingStatus) {
                foundFinalizingStatus = true;
                break;
            }
        }
        QVERIFY(foundFinalizingStatus);
        QTRY_COMPARE(recorder.status(), QMediaRecorder::StoppedStatus);
        QCOMPARE(recorderStatusSignal.last().first().value<QMediaRecorder::Status>(), recorder.status());

        QVERIFY(errorSignal.isEmpty());
        QVERIFY(recorderErrorSignal.isEmpty());

        QString fileName = recorder.actualLocation().toLocalFile();
        QVERIFY(!fileName.isEmpty());

        QVERIFY(QFileInfo(fileName).size() > 0);
        QFile(fileName).remove();

        QTRY_COMPARE(recorder.status(), QMediaRecorder::StoppedStatus);
        QCOMPARE(recorderStatusSignal.last().first().value<QMediaRecorder::Status>(), recorder.status());
    }
}

QTEST_MAIN(tst_QCameraBackend)

#include "tst_qcamerabackend.moc"
