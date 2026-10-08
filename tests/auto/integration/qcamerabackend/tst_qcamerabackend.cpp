// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtGui/QImageReader>
#include <QtCore/qurl.h>
#include <QtCore/qlocale.h>
#include <QDebug>
#include <QVideoSink>
#include <QMediaPlayer>

#include <private/qplatformcamera_p.h>
#include <private/qplatformimagecapture_p.h>
#include <private/qplatformmediaintegration_p.h>
#include <qcamera.h>
#include <qcameradevice.h>
#include <qimagecapture.h>
#include <qmediacapturesession.h>
#include <qobject.h>
#include <qmediadevices.h>
#include <qmediarecorder.h>
#include <qmediaplayer.h>
#include <qaudiooutput.h>

#ifdef Q_OS_DARWIN
#include <QtCore/private/qcore_mac_p.h>
#endif

#include <private/mediabackendutils_p.h>

#include <memory>

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

    void testCameraActive();
    void testCameraStartParallel();
    void testCameraFormat();
    void testCameraCapture();
    void testCaptureToBuffer();
    void captureToFile_createsFileWithExpectedExtension_data();
    void captureToFile_createsFileWithExpectedExtension();
    void testCameraCaptureMetadata();
    void testExposureCompensation();
    void testExposureMode();

    void testVideoRecording_data();
    void testVideoRecording();

    void testNativeMetadata();

    void multipleCameraSet();

private:
    bool noCamera = false;
};

class TestVideoFormat : public QVideoSink
{
    Q_OBJECT
public:
    explicit TestVideoFormat(const QCameraFormat &format)
        : formatMismatch(0),
          cameraFormat(format)
    {
        connect(this, &QVideoSink::videoFrameChanged, this, &TestVideoFormat::checkVideoFrameFormat);
    }

    void setCameraFormatToTest(const QCameraFormat &format)
    {
        formatMismatch = -1;
        cameraFormat = format;
    }

    int formatMismatch = -1;

private:
    QCameraFormat cameraFormat;

public Q_SLOTS:
    void checkVideoFrameFormat(const QVideoFrame &frame)
    {
        QVideoFrameFormat surfaceFormat = frame.surfaceFormat();
        if (surfaceFormat.pixelFormat() == cameraFormat.pixelFormat()
            && surfaceFormat.frameSize() == cameraFormat.resolution()) {
            formatMismatch = 0;
#ifdef Q_OS_ANDROID
        } else if ((surfaceFormat.pixelFormat() == QVideoFrameFormat::Format_YUV420P
                   || surfaceFormat.pixelFormat() == QVideoFrameFormat::Format_NV12
                   || surfaceFormat.pixelFormat() == QVideoFrameFormat::Format_NV21)
            && cameraFormat.pixelFormat() == QVideoFrameFormat::Format_YUV420P
            && surfaceFormat.frameSize() == cameraFormat.resolution()) {
            formatMismatch = 0;
#endif
        } else {
            formatMismatch = 1;
        }
    }
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
    if (noCamera) {
        // only verify that we get an error trying to create a camera
        QCamera camera;
        QCOMPARE(camera.error(), QCamera::CameraError);
        QVERIFY(camera.cameraDevice().isNull());

        QSKIP("No camera available");
    }

    QCameraDevice defaultCamera = QMediaDevices::defaultVideoInput();

    {
        // should use default camera
        QCamera camera;
        QCOMPARE(camera.error(), QCamera::NoError);
        QVERIFY(!camera.cameraDevice().isNull());
        QCOMPARE(camera.cameraDevice(), defaultCamera);
    }

    {
        // should use default camera
        QCamera camera(QCameraDevice{});
        QCOMPARE(camera.error(), QCamera::NoError);
        QVERIFY(!camera.cameraDevice().isNull());
        QCOMPARE(camera.cameraDevice(), defaultCamera);
    }

    {
        QCamera camera(defaultCamera);
        QCOMPARE(camera.error(), QCamera::NoError);
        QCOMPARE(camera.cameraDevice(), defaultCamera);
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

void tst_QCameraBackend::testCameraActive()
{
    QMediaCaptureSession session;
    QCamera camera;
    camera.setCameraDevice(QCameraDevice());
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy errorSignal(&camera, &QCamera::errorOccurred);
    QSignalSpy activeChangedSignal(&camera, &QCamera::activeChanged);

    QCOMPARE(camera.isActive(), false);

    if (noCamera)
        QSKIP("No camera available");
    camera.setCameraDevice(QMediaDevices::defaultVideoInput());
    QCOMPARE(camera.error(), QCamera::NoError);

    camera.start();
    QTRY_COMPARE(camera.isActive(), true);
    QTRY_COMPARE(activeChangedSignal.size(), 1);
    QCOMPARE(activeChangedSignal.last().first().value<bool>(), true);

    camera.stop();
    QCOMPARE(camera.isActive(), false);
    QCOMPARE(activeChangedSignal.last().first().value<bool>(), false);

    QCOMPARE(camera.errorString(), QString());
}

void tst_QCameraBackend::testCameraStartParallel()
{
#ifdef Q_OS_ANDROID
    QSKIP("Multi-camera feature is currently not supported on Android. "
          "Cannot open same device twice.");
#endif
#ifdef Q_OS_LINUX
    QSKIP("Multi-camera feature is currently not supported on Linux. "
          "Cannot open same device twice.");
#endif
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
    QCOMPARE(camera1.error(), QCamera::NoError);
    QCOMPARE(camera2.isActive(), true);
    QCOMPARE(camera2.error(), QCamera::NoError);

    QCOMPARE(errorSpy1.size(), 0);
    QCOMPARE(errorSpy2.size(), 0);
}

void tst_QCameraBackend::testCameraFormat()
{
    QCamera camera;
    QCameraDevice device = camera.cameraDevice();
    auto videoFormats = device.videoFormats();
    if (videoFormats.isEmpty())
        QSKIP("No Camera available, skipping test.");
    QCameraFormat cameraFormat = videoFormats.first();
    QSignalSpy spy(&camera, &QCamera::cameraFormatChanged);
    QVERIFY(spy.size() == 0);

    QMediaCaptureSession session;
    session.setCamera(&camera);
    QVERIFY(videoFormats.size());
    camera.setCameraFormat(cameraFormat);
    QCOMPARE(camera.cameraFormat(), cameraFormat);
    QVERIFY(spy.size() == 1);

    TestVideoFormat videoFormatTester(cameraFormat);
    session.setVideoOutput(&videoFormatTester);
    camera.start();
    QTRY_VERIFY(videoFormatTester.formatMismatch == 0);

    spy.clear();
    camera.stop();
    // Change camera format
    if (videoFormats.size() > 1) {
        QCameraFormat secondFormat = videoFormats.at(1);
        camera.setCameraFormat(secondFormat);
        QCOMPARE(camera.cameraFormat(), secondFormat);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(camera.cameraFormat(), secondFormat);
        videoFormatTester.setCameraFormatToTest(secondFormat);
        camera.start();
        QTRY_VERIFY(videoFormatTester.formatMismatch == 0);

        // check that frame format is not same as previous camera format
        videoFormatTester.setCameraFormatToTest(cameraFormat);
        QTRY_VERIFY(videoFormatTester.formatMismatch == 1);
    }

    // Set null format
    spy.clear();
    camera.stop();
    camera.setCameraFormat({});
    QCOMPARE(spy.size(), 1);
    videoFormatTester.setCameraFormatToTest({});
    camera.start();
    // In case of a null format, the backend should have picked
    // a decent format to render frames
    QTRY_VERIFY(videoFormatTester.formatMismatch == 1);
    camera.stop();

    spy.clear();
    // Shouldn't change anything as it's the same device
    camera.setCameraDevice(device);
    QCOMPARE(spy.size(), 0);
}

void tst_QCameraBackend::testCameraCapture()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    //prevents camera to flash during the test
    camera.setFlashMode(QCamera::FlashOff);

    QVERIFY(!imageCapture.isReadyForCapture());

    QSignalSpy capturedSignal(&imageCapture, &QImageCapture::imageCaptured);
    QSignalSpy savedSignal(&imageCapture, &QImageCapture::imageSaved);
    QSignalSpy errorSignal(&imageCapture, &QImageCapture::errorOccurred);

    imageCapture.captureToFile();
    QTRY_COMPARE(errorSignal.size(), 1);
    QCOMPARE(imageCapture.error(), QImageCapture::NotReadyError);
    QCOMPARE(capturedSignal.size(), 0);
    errorSignal.clear();

    if (noCamera)
        QSKIP("No camera available");

    QVideoSink sink;
    session.setVideoOutput(&sink);
    camera.start();

    QTRY_VERIFY(imageCapture.isReadyForCapture());
    QVERIFY(camera.isActive());
    QCOMPARE(errorSignal.size(), 0);

    int id = imageCapture.captureToFile();

    QTRY_VERIFY_WITH_TIMEOUT(!savedSignal.isEmpty(), 8s);

    QTRY_COMPARE(capturedSignal.size(), 1);
    QCOMPARE(capturedSignal.last().first().toInt(), id);
    QCOMPARE(errorSignal.size(), 0);
    QCOMPARE(imageCapture.error(), QImageCapture::NoError);

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
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    camera.setFlashMode(QCamera::FlashOff);

    camera.setActive(true);

    QTRY_VERIFY(camera.isActive());

    QSignalSpy capturedSignal(&imageCapture, &QImageCapture::imageCaptured);
    QSignalSpy imageAvailableSignal(&imageCapture, &QImageCapture::imageAvailable);
    QSignalSpy savedSignal(&imageCapture, &QImageCapture::imageSaved);
    QSignalSpy errorSignal(&imageCapture, &QImageCapture::errorOccurred);

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

void tst_QCameraBackend::captureToFile_createsFileWithExpectedExtension_data()
{
    QTest::addColumn<QString>("inputFilename");
    QTest::addColumn<QImageCapture::FileFormat>("format");
    QTest::addColumn<QString>("expectedFilename");

    {
        QTest::addRow("Add file extension to JPEG file without extension")
                << "file" << QImageCapture::JPEG << "file.jpg";
        QTest::addRow("Keep extension of JPEG file with extension")
                << "file.jpeg" << QImageCapture::JPEG << "file.jpeg";
        QTest::addRow("Keep extension of JPEG file with wrong extension")
                << "file.png" << QImageCapture::JPEG << "file.png";
    }

    {
        QTest::addRow("Add file extension to PNG file without extension")
                << "file" << QImageCapture::PNG << "file.png";
        QTest::addRow("Keep extension of PNG file with extension")
                << "file.apng" << QImageCapture::PNG << "file.apng";
        QTest::addRow("Keep extension of PNG file with wrong extension")
                << "file.jpg" << QImageCapture::PNG << "file.jpg";
    }

    {
        QTest::addRow("Add file extension to WebP file without extension")
                << "file" << QImageCapture::WebP << "file.webp";
        QTest::addRow("Keep extension of WebP file with extension")
                << "file.web" << QImageCapture::WebP << "file.web";
        QTest::addRow("Keep extension of WebP file with wrong extension")
                << "file.png" << QImageCapture::WebP << "file.png";
    }

    {
        QTest::addRow("Add file extension to Tiff file without extension")
                << "file" << QImageCapture::Tiff << "file.tiff";
        QTest::addRow("Keep extension of Tiff file with extension")
                << "file.cr2" << QImageCapture::Tiff << "file.cr2";
        QTest::addRow("Keep extension of Tiff file with wrong extension")
                << "file.png" << QImageCapture::Tiff << "file.png";
    }
}

void tst_QCameraBackend::captureToFile_createsFileWithExpectedExtension()
{
    if (noCamera)
        QSKIP("No camera available");

    QFETCH(const QString, inputFilename);
    QFETCH(const QImageCapture::FileFormat, format);
    QFETCH(const QString, expectedFilename);

    QCamera camera;
    camera.setFlashMode(QCamera::FlashOff);

    QImageCapture imageCapture;
    imageCapture.setFileFormat(format);

    QMediaCaptureSession session;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy savedSignal(&imageCapture, &QImageCapture::imageSaved);

    camera.start();

    QTRY_VERIFY(imageCapture.isReadyForCapture());

    QTemporaryDir tempDir;
    tempDir.setAutoRemove(false);
    const QString tempFile = tempDir.filePath(inputFilename);
    imageCapture.captureToFile(tempFile);

    QTRY_VERIFY(!savedSignal.isEmpty());

    const QString imagePath = tempDir.filePath(expectedFilename);
    QVERIFY(QFile::exists(imagePath));

    QImage image;
    QVERIFY(image.load(imagePath));
    QVERIFY(!image.isNull());
}

void tst_QCameraBackend::testCameraCaptureMetadata()
{
    if (noCamera)
        QSKIP("No camera available");

    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    camera.setFlashMode(QCamera::FlashOff);

    QMediaMetaData referenceMetaData;
    referenceMetaData.insert(QMediaMetaData::Title, QStringLiteral("Title"));
    referenceMetaData.insert(QMediaMetaData::Language, QVariant::fromValue(QLocale::German));
    referenceMetaData.insert(QMediaMetaData::Description, QStringLiteral("Description"));
    imageCapture.setMetaData(referenceMetaData);

    QSignalSpy metadataSignal(&imageCapture, &QImageCapture::imageMetadataAvailable);
    QSignalSpy savedSignal(&imageCapture, &QImageCapture::imageSaved);

    camera.start();

    QTRY_VERIFY(imageCapture.isReadyForCapture());

    QTemporaryDir dir;
    auto tmpFile = dir.filePath("testImage");
    int id = imageCapture.captureToFile(tmpFile);
    QTRY_VERIFY(!savedSignal.isEmpty());
    QVERIFY(!metadataSignal.isEmpty());

    QCOMPARE(metadataSignal.first().first().toInt(), id);
    QMediaMetaData receivedMetaData = metadataSignal.first()[1].value<QMediaMetaData>();

    if (isGStreamerPlatform()) {
        for (auto key : {
                     QMediaMetaData::Title,
                     QMediaMetaData::Language,
                     QMediaMetaData::Description,
             })
            QCOMPARE(receivedMetaData[key], referenceMetaData[key]);
        QVERIFY(receivedMetaData[QMediaMetaData::Resolution].isValid());
    }
}

void tst_QCameraBackend::testExposureCompensation()
{
    if (noCamera)
        QSKIP("No camera available");

    QMediaCaptureSession session;
    QCamera camera;
    session.setCamera(&camera);

    QSignalSpy exposureCompensationSignal(&camera, &QCamera::exposureCompensationChanged);

    // it should be possible to set exposure parameters in Unloaded state
    QCOMPARE(camera.exposureCompensation(), 0.);
    if (!(camera.supportedFeatures() & QCamera::Feature::ExposureCompensation))
        return;

    camera.setExposureCompensation(1.0);
    QCOMPARE(camera.exposureCompensation(), 1.0);
    QTRY_COMPARE(exposureCompensationSignal.size(), 1);
    QCOMPARE(exposureCompensationSignal.last().first().toReal(), 1.0);

    //exposureCompensationChanged should not be emitted when value is not changed
    camera.setExposureCompensation(1.0);
    QTest::qWait(50);
    QCOMPARE(exposureCompensationSignal.size(), 1);

    //exposure compensation should be preserved during start
    camera.start();
    QTRY_VERIFY(camera.isActive());

    QCOMPARE(camera.exposureCompensation(), 1.0);

    exposureCompensationSignal.clear();
    camera.setExposureCompensation(-1.0);
    QCOMPARE(camera.exposureCompensation(), -1.0);
    QTRY_COMPARE(exposureCompensationSignal.size(), 1);
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
    QTRY_VERIFY(!camera.isActive());

    // Auto
    camera.setExposureMode(QCamera::ExposureAuto);
    QCOMPARE(camera.exposureMode(), QCamera::ExposureAuto);
    camera.start();
    QTRY_VERIFY(camera.isActive());
    QCOMPARE(camera.exposureMode(), QCamera::ExposureAuto);

    // Manual
    if (camera.isExposureModeSupported(QCamera::ExposureManual)) {
        camera.setExposureMode(QCamera::ExposureManual);
        QCOMPARE(camera.exposureMode(), QCamera::ExposureManual);
        camera.start();
        QVERIFY(camera.isActive());
        QCOMPARE(camera.exposureMode(), QCamera::ExposureManual);

        camera.setManualExposureTime(.02f); // ~20ms should be supported by most cameras
        QVERIFY(camera.manualExposureTime() > .01 && camera.manualExposureTime() < .04);
    }

    camera.setExposureMode(QCamera::ExposureAuto);
}

void tst_QCameraBackend::testVideoRecording_data()
{
    QTest::addColumn<QCameraDevice>("device");

    const auto devices = QMediaDevices::videoInputs();
    int i = 0;

    for (const auto &device : devices)
        QTest::addRow("%d - %s", i++, device.description().toUtf8().constData()) << device;

    if (devices.isEmpty())
        QTest::newRow("Null device") << QCameraDevice();
}

void tst_QCameraBackend::testVideoRecording()
{
    if (noCamera)
        QSKIP("No camera available");
    QFETCH(QCameraDevice, device);

    QMediaCaptureSession session;
    std::unique_ptr<QCamera> camera(new QCamera(device));
    session.setCamera(camera.get());

    QMediaRecorder recorder;
    session.setRecorder(&recorder);

    QSignalSpy errorSignal(camera.get(), &QCamera::errorOccurred);
    QSignalSpy recorderErrorSignal(&recorder, &QMediaRecorder::errorOccurred);
    QSignalSpy recorderStateChanged(&recorder, &QMediaRecorder::recorderStateChanged);
    QSignalSpy durationChanged(&recorder, &QMediaRecorder::durationChanged);

    recorder.setVideoResolution(320, 240);

    // Insert metadata
    QMediaMetaData metaData;
    metaData.insert(QMediaMetaData::Author, QStringLiteral("Author"));
    metaData.insert(QMediaMetaData::Date, QDateTime::currentDateTime());
    recorder.setMetaData(metaData);

    camera->start();
    if (noCamera || device.isNull()) {
        QVERIFY(!camera->isActive());
        return;
    }
    QTRY_VERIFY(camera->isActive());

    QTRY_VERIFY(camera->isActive());

    recorder.record();
    if (!recorderErrorSignal.empty() || recorderErrorSignal.wait(550)) {
        QEXPECT_FAIL_GSTREAMER("", "QTBUG-124148: GStreamer might return ResourceError", Continue);

        QCOMPARE(recorderErrorSignal.last().first().toInt(), QMediaRecorder::FormatError);
        return;
    }

    QTRY_VERIFY(durationChanged.size());

    QCOMPARE(recorder.metaData(), metaData);

    recorderStateChanged.clear();
    recorder.stop();
    QTRY_VERIFY(recorderStateChanged.size() > 0);
    QVERIFY(recorder.recorderState() == QMediaRecorder::StoppedState);

    QVERIFY(errorSignal.isEmpty());
    QVERIFY(recorderErrorSignal.isEmpty());

    QString fileName = recorder.actualLocation().toLocalFile();
    QVERIFY(!fileName.isEmpty());
    QVERIFY(QFileInfo(fileName).size() > 0);

    QMediaPlayer player;
    player.setSource(fileName);

    QTRY_COMPARE_WITH_TIMEOUT(player.mediaStatus(), QMediaPlayer::LoadedMedia, 8s);
    QCOMPARE_EQ(player.metaData().value(QMediaMetaData::Resolution).toSize(), QSize(320, 240));
    QCOMPARE_GT(player.duration(), 350);
    QCOMPARE_LT(player.duration(), 650);

    // TODO: integrate with a virtual camera and check mediaplayer output

    QFile(fileName).remove();
}

void tst_QCameraBackend::testNativeMetadata()
{
    if (noCamera)
        QSKIP("No camera available");

    QMediaCaptureSession session;
    QCameraDevice device = QMediaDevices::defaultVideoInput();
    QCamera camera(device);
    session.setCamera(&camera);

    QMediaRecorder recorder;
    session.setRecorder(&recorder);

    QSignalSpy errorSignal(&camera, &QCamera::errorOccurred);
    QSignalSpy recorderErrorSignal(&recorder, &QMediaRecorder::errorOccurred);
    QSignalSpy recorderStateChanged(&recorder, &QMediaRecorder::recorderStateChanged);
    QSignalSpy durationChanged(&recorder, &QMediaRecorder::durationChanged);

    camera.start();
    if (device.isNull()) {
        QVERIFY(!camera.isActive());
        return;
    }

    QTRY_VERIFY(camera.isActive());

    // Insert common metadata supported on all platforms
    // Don't use Date, as some backends set that on their own
    QMediaMetaData metaData;
    metaData.insert(QMediaMetaData::Title, QStringLiteral("Title"));
    metaData.insert(QMediaMetaData::Language, QVariant::fromValue(QLocale::German));
    metaData.insert(QMediaMetaData::Description, QStringLiteral("Description"));

    recorder.setMetaData(metaData);

    recorder.record();
    QTRY_VERIFY(durationChanged.size());
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::RecorderState::RecordingState);

    QCOMPARE(recorder.metaData(), metaData);

    recorderStateChanged.clear();
    recorder.stop();

    QTRY_VERIFY(recorderStateChanged.size() > 0);
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::RecorderState::StoppedState);

    QVERIFY(errorSignal.isEmpty());
    if (!isGStreamerPlatform()) {
        // https://bugreports.qt.io/browse/QTBUG-124183
        QVERIFY(recorderErrorSignal.isEmpty());
    }

    QString fileName = recorder.actualLocation().toLocalFile();
    QVERIFY(!fileName.isEmpty());
    QVERIFY(QFileInfo(fileName).size() > 0);

    // QMediaRecorder::metaData() can only test that QMediaMetaData is set properly on the recorder.
    // Use QMediaPlayer to test that the native metadata is properly set on the track
    QAudioOutput output;
    QMediaPlayer player;
    player.setAudioOutput(&output);

    QSignalSpy metadataChangedSpy(&player, &QMediaPlayer::metaDataChanged);

    player.setSource(QUrl::fromLocalFile(fileName));
    player.play();

    int metadataChangedRequiredCount = isGStreamerPlatform() ? 2 : 1;

    QTRY_VERIFY(metadataChangedSpy.size() >= metadataChangedRequiredCount);

    QCOMPARE(player.metaData().value(QMediaMetaData::Title).toString(),
             metaData.value(QMediaMetaData::Title).toString());
    auto lang = player.metaData().value(QMediaMetaData::Language).value<QLocale::Language>();
    if (lang != QLocale::AnyLanguage)
        QCOMPARE(lang, metaData.value(QMediaMetaData::Language).value<QLocale::Language>());
    QCOMPARE(player.metaData().value(QMediaMetaData::Description).toString(), metaData.value(QMediaMetaData::Description).toString());
    QVERIFY(player.metaData().value(QMediaMetaData::Resolution).isValid());

    if (isGStreamerPlatform())
        QVERIFY(player.metaData().value(QMediaMetaData::Date).isValid());

    player.stop();
    player.setSource({});
    QFile(fileName).remove();
}

void tst_QCameraBackend::multipleCameraSet()
{
    if (noCamera)
        QSKIP("No camera available");

    QMediaCaptureSession session;
    QCameraDevice device = QMediaDevices::defaultVideoInput();

    QMediaRecorder recorder;
    session.setRecorder(&recorder);

    for (int i = 0; i < 5; ++i) {
#ifdef Q_OS_DARWIN
        QMacAutoReleasePool releasePool;
#endif

        QCamera camera(device);
        session.setCamera(&camera);

        camera.start();

        QTest::qWait(100);
    }
}

QTEST_MAIN(tst_QCameraBackend)

#include "tst_qcamerabackend.moc"
