// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimediaTestLib/private/framegrabber_p.h>
#include <QtMultimediaTestLib/private/mediabackendutils_p.h>
#include <QtMultimediaTestLib/private/mediainfo_p.h>
#include <QtMultimediaTestLib/private/qintegrationtestbase_p.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>
#include <QtMultimedia/qaudiooutput.h>
#include <QtMultimedia/qcamera.h>
#include <QtMultimedia/qcameradevice.h>
#include <QtMultimedia/qimagecapture.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qvideosink.h>
#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtMultimedia/private/qplatformimagecapture_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <QtGui/qimagereader.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qlocale.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qobject.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qtemporarydir.h>
#include <QtCore/qurl.h>
#if QT_CONFIG(process)
#  include <QtCore/qprocess.h>
#endif
#ifdef Q_OS_DARWIN
#  include <QtCore/private/qcore_mac_p.h>
#endif

#include <algorithm>
#include <chrono>
#include <memory>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

/*
 This is the backend conformance test.

 Since it relies on platform media framework and sound hardware
 it may be less stable.
*/

struct VCamParameters
{
    QString name;
    QString format = QStringLiteral("nv12");
    QSize resolution = QSize(1920, 1080);
    float fps = 60.0f;
};

// In CI, the VCamManager.exe can hang for a long time even if it's
// available. It hangs so long that it triggers the test watchdog
// of 300s that a test is timed out and considered a crash.
// We add some strict timeouts to work around.
// TODO: Need to solve hangs and flakiness in VCamManager.exe.
static constexpr std::chrono::milliseconds VCamProcessTimeout = 15s;
static constexpr std::chrono::milliseconds VCamDetectTimeout = 5s;

using namespace QtMultimediaTest;

struct CameraStreamSetup
{
    explicit CameraStreamSetup(const QCameraDevice &device)
        : cameraStorage(std::make_unique<QCamera>(device)),
          camera(*cameraStorage)
    {
        QObject::connect(
            &camera,
            &QCamera::activeChanged,
            &grabber,
            &FrameGrabber::onCaptureActiveChanged);
        QObject::connect(&camera, &QCamera::errorOccurred, &grabber, &FrameGrabber::stop);
        QObject::connect(
            &camera,
            &QCamera::errorOccurred,
            &camera,
            [this](QCamera::Error, const QString &errorString) {
                errors.append(errorString);
            });

        session.setCamera(&camera);
        session.setVideoSink(&grabber);
    }

    FrameGrabber grabber;
    QMediaCaptureSession session;
    std::unique_ptr<QCamera> cameraStorage;
    QCamera &camera;
    QStringList errors;
};

// Waits for the stream to end with a null-frame, and verifies that it follows the
// last valid frame.
static void verifyStreamEndsWithNullFrame(const FrameGrabber &grabber)
{
    // The backend may emit the null-frame from the thread it delivers frames on, in
    // which case it is still in flight after the camera has been stopped.
    bool streamTerminated = QTest::qWaitFor(
        [&] {
            const std::vector<VideoFrameInfo> &frames = grabber.getFrameInfos();
            return !frames.empty() && !frames.back().isValid;
        },
        surfaceCaptureTestTimeout());
    QVERIFY2(streamTerminated, "Stream did not end with a null-frame");

    const std::vector<VideoFrameInfo> &frames = grabber.getFrameInfos();
    QCOMPARE_GE(frames.size(), 2u);
    const VideoFrameInfo &nullFrame = frames.back();
    const VideoFrameInfo &lastValidFrame = frames[frames.size() - 2];
    QVERIFY(lastValidFrame.isValid);
    QVERIFY2(nullFrame.startTime > lastValidFrame.startTime,
        qPrintable(u"Null-frame start time (%1) is not after the last valid frame's (%2)"_s
            .arg(frameTimeToString(nullFrame.startTime))
            .arg(frameTimeToString(lastValidFrame.startTime))));
}

// Extends CameraStreamSetup with a QMediaRecorder that records to a temporary directory.
struct CameraRecorderSetup : CameraStreamSetup
{
    explicit CameraRecorderSetup(const QCameraDevice &device)
        : CameraStreamSetup(device)
    {
        QObject::connect(
            &recorder,
            &QMediaRecorder::errorOccurred,
            &recorder,
            [this](QMediaRecorder::Error, const QString &errorString) {
                recorderErrors.append(errorString);
            });

        recorder.setOutputLocation(QUrl::fromLocalFile(tempDir.filePath(u"recording"_s)));
        session.setRecorder(&recorder);
    }

    // The recorder finalizes the file asynchronously, so it must be stopped before
    // the temporary directory is removed.
    ~CameraRecorderSetup()
    {
        std::ignore = stopRecording();
    }

    // Starts the camera and the recorder, and waits until frames are being recorded.
    void startRecording()
    {
        camera.setActive(true);
        QVERIFY(camera.isActive());
        QVERIFY2(grabber.tryWaitForFirstFrameInfo(), "Camera delivered no frames");

        recorder.record();
        QTRY_VERIFY_WITH_TIMEOUT(recorder.duration() > 0, surfaceCaptureTestTimeout());
        QCOMPARE(recorder.recorderState(), QMediaRecorder::RecorderState::RecordingState);
    }

    // Stops the recorder, and waits until the media file is finalized.
    [[nodiscard]] bool stopRecording()
    {
        recorder.stop();
        return waitForRecorderStopped();
    }

    [[nodiscard]] bool waitForRecorderStopped()
    {
        return QTest::qWaitFor(
            [this] {
                return recorder.recorderState() == QMediaRecorder::RecorderState::StoppedState;
            },
            surfaceCaptureTestTimeout());
    }

    QTemporaryDir tempDir;
    QMediaRecorder recorder;
    QStringList recorderErrors;
};

class tst_QCameraBackend : public QIntegrationTestBase
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void testCameraDevice();
    void testCtorWithCameraDevice();
    void testCtorWithPosition();

    void testVirtualCameraAddition();
    void testVirtualCameraRemoval();
    void testVirtualCameraFrameChanging();

    void testCameraStartParallel();
    void testMappedFrameSurvivesCameraStop();
    void testCameraCapture();
    void testCaptureToBuffer();
    void testCaptureResolutionChangeWhileActive();
    void testCaptureResolutionChangeRapid();
    void testCaptureResolutionSetBeforeStart();
    void captureToFile_createsFileWithExpectedExtension_data();
    void captureToFile_createsFileWithExpectedExtension();
    void testCameraCaptureMetadata();
    void testExposureCompensation();
    void testExposureMode();

    void testNativeMetadata();

    void multipleCameraSet();

    void videoFormats_isNotEmpty_withEachCameraDevice_data();
    void videoFormats_isNotEmpty_withEachCameraDevice();
    void videoFormats_containsNoDuplicates_withEachCameraDevice_data();
    void videoFormats_containsNoDuplicates_withEachCameraDevice();
    void setCameraFormat_updatesPropertyAndEmitsSignal_whenNotActive_withEachCameraDevice_data();
    void setCameraFormat_updatesPropertyAndEmitsSignal_whenNotActive_withEachCameraDevice();
    void setCameraDevice_keepsCameraFormat_whenDeviceIsUnchanged_withEachCameraDevice_data();
    void setCameraDevice_keepsCameraFormat_whenDeviceIsUnchanged_withEachCameraDevice();

    void setActive_startsCameraAndDeliversFrames_withEachCameraDevice_data();
    void setActive_startsCameraAndDeliversFrames_withEachCameraDevice();
    void setActive_restartsStream_whenStartedAgainAfterStop_withEachCameraDevice_data();
    void setActive_restartsStream_whenStartedAgainAfterStop_withEachCameraDevice();
    void setActive_fails_whenCameraDeviceDoesNotExist_data();
    void setActive_fails_whenCameraDeviceDoesNotExist();
    void cameraFormat_isNotNull_whenActive_data();
    void cameraFormat_isNotNull_whenActive();

    void capturedFrames_matchCameraFormat_withEachCameraFormat_data();
    void capturedFrames_matchCameraFormat_withEachCameraFormat();
    void capturedFrames_havePresentationTimesInIncreasingOrder_data();
    void capturedFrames_havePresentationTimesInIncreasingOrder();
    void firstFrame_hasZeroStartTime_whenCameraIsStartedOrRestarted_withEachCameraDevice_data();
    void firstFrame_hasZeroStartTime_whenCameraIsStartedOrRestarted_withEachCameraDevice();
    void storingFrames_doesNotStallStream_data();
    void storingFrames_doesNotStallStream();
    void stream_endsWithNullFrame_whenStopped_withEachCameraDevice_data();
    void stream_endsWithNullFrame_whenStopped_withEachCameraDevice();
    void stream_endsWithNullFrame_whenCameraIsDestroyed_withEachCameraDevice_data();
    void stream_endsWithNullFrame_whenCameraIsDestroyed_withEachCameraDevice();

    void setCameraDevice_switchesStream_withoutDeactivatingOrErrors();
    void setCameraDevice_keepsFrameFormat_whenSwitchingToDeviceWithSameFormat_data();
    void setCameraDevice_keepsFrameFormat_whenSwitchingToDeviceWithSameFormat();
    void setCameraFormat_switchesStreamFormat_whileActive_data();
    void setCameraFormat_switchesStreamFormat_whileActive();

    void recorder_encodesFrames_toValidMediaFile_withEachCameraDevice_data();
    void recorder_encodesFrames_toValidMediaFile_withEachCameraDevice();
    void recorder_encodesFrames_withRequestedResolution_withEachCameraDevice_data();
    void recorder_encodesFrames_withRequestedResolution_withEachCameraDevice();
    void recorder_recordingHasExpectedDuration_withEachCameraDevice_data();
    void recorder_recordingHasExpectedDuration_withEachCameraDevice();
    void recorder_encodesAllCameraFrames_withEachCameraDevice_data();
    void recorder_encodesAllCameraFrames_withEachCameraDevice();
    void recorder_autoStops_whenCameraIsDeactivated_withEachCameraDevice_data();
    void recorder_autoStops_whenCameraIsDeactivated_withEachCameraDevice();
    void recorder_autoStops_whenCameraIsRemovedFromCaptureSession_withEachCameraDevice_data();
    void recorder_autoStops_whenCameraIsRemovedFromCaptureSession_withEachCameraDevice();
    void recorder_doesNotAutoStop_whenCameraDeviceIsSwitched();

private:
    static void addRowPerCameraDevice();
    // Waits for the recorder to stop on its own, and verifies the recording
    static void verifyRecorderAutoStopped(CameraRecorderSetup &setup);
    // Returns up to \a count distinct photo resolutions supported by the default camera
    static QList<QSize> photoResolutionsToTest(int count);
    // Saved images can be rotated according to device orientation
    static bool isSameSizeAllowingRotation(const QSize &actual, const QSize &expected);
    // Captures to file, returns the saved image size or an empty size on failure
    static QSize captureToFileAndGetSize(QImageCapture &imageCapture);

    bool callVcam(const QString &command, const VCamParameters &parameters) const;
    bool addVcam(const VCamParameters &parameters) const;
    void removeVcam(const VCamParameters &parameters) const;
    static QCameraDevice findCamera(const QString &name);
    QColor dominantRgbColor(const QColor &color, int tolerance) const;

    bool noCamera = false;
    bool m_vcamAvailable = false;
    VCamParameters m_defaultCamera {"VCam"};
    QByteArray m_vcamPath;
};

void tst_QCameraBackend::initTestCase()
{
    if (!initIntegrationTestCase())
        return;

#if QT_CONFIG(process)
    m_vcamPath = qgetenv("VCAM_PATH");
#endif

    // VCAM_PATH being set only means the helper is deployed, not that it can
    // actually register a camera on this machine. Test it once and skip
    // other tests based on this.
    if (!m_vcamPath.isEmpty())
        m_vcamAvailable = addVcam(m_defaultCamera);

    QCamera camera;
    noCamera = !camera.isAvailable();
}

void tst_QCameraBackend::cleanupTestCase()
{
    if (m_vcamAvailable)
        removeVcam(m_defaultCamera);
}

bool tst_QCameraBackend::callVcam(const QString &command, const VCamParameters &parameters) const
{
#if QT_CONFIG(process)
    QProcess vcamManagerProcess;

    QString program = m_vcamPath + "\\VCamManager.exe";

    vcamManagerProcess.setWorkingDirectory(m_vcamPath);

    QStringList arguments;
    arguments << command << parameters.name;
    if (command == "--add") {
        arguments << "--format" << parameters.format;
        arguments << "--resolution" << QString::number(parameters.resolution.width()) + "x"
                                     + QString::number(parameters.resolution.height());
        arguments << "--fps" << QString::number(parameters.fps);
    }

    vcamManagerProcess.start(program, arguments);

    if (!vcamManagerProcess.waitForStarted(static_cast<int>(VCamProcessTimeout.count()))) {
        qWarning() << "Failed to start" << program << arguments << ":"
                   << vcamManagerProcess.errorString();
        return false;
    }

    if (!vcamManagerProcess.waitForFinished(static_cast<int>(VCamProcessTimeout.count()))) {
        qWarning() << program << arguments << "did not finish within"
                   << VCamProcessTimeout.count() << "ms; terminating it";
        vcamManagerProcess.kill();
        vcamManagerProcess.waitForFinished(static_cast<int>(VCamProcessTimeout.count()));
        return false;
    }

    if (vcamManagerProcess.exitStatus() != QProcess::NormalExit
        || vcamManagerProcess.exitCode() != 0) {
        qWarning() << program << arguments << "failed with exit code"
                   << vcamManagerProcess.exitCode();
        return false;
    }

    return true;
#else
    Q_UNUSED(command);
    Q_UNUSED(parameters);
    return false;
#endif
}

bool tst_QCameraBackend::addVcam(const VCamParameters &parameters) const
{
    if (!callVcam("--add", parameters))
        return false;

    // The device-added notification is delivered asynchronously, so wait a
    // bounded time for the camera to actually show up.
    return QTest::qWaitFor(
        [&] {
            return !findCamera(parameters.name).isNull();
        },
        VCamDetectTimeout);
}

void tst_QCameraBackend::removeVcam(const VCamParameters &parameters) const
{
    QMediaDevices mediaDevices = QMediaDevices();
    QSignalSpy removeSignal(&mediaDevices, &QMediaDevices::videoInputsChanged);

    callVcam("--remove", parameters);

    QTRY_COMPARE(removeSignal.size(), 1);
}

QCameraDevice tst_QCameraBackend::findCamera(const QString &name)
{
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &camera : cameras) {
        if (camera.description().startsWith(name)) {
            return camera;
        }
    }
    return QCameraDevice();
}


QColor tst_QCameraBackend::dominantRgbColor(const QColor &color, int tolerance) const
{
    if (color.red() > 255 - tolerance && color.green() < tolerance && color.blue() < tolerance)
        return Qt::red;
    if (color.green() > 255 - tolerance && color.red() < tolerance && color.blue() < tolerance)
        return Qt::green;
    if (color.blue()  > 255 - tolerance && color.red() < tolerance && color.green() < tolerance)
        return Qt::blue;

    return QColor();
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

void tst_QCameraBackend::testVirtualCameraAddition()
{
    if (!m_vcamAvailable)
        QSKIP("Virtual camera is not available. Skipping camera addition test.");

    int lengthBeforeAdd = QMediaDevices::videoInputs().length();
    VCamParameters cameraParams {"TestVCam", QStringLiteral("nv12"), QSize(1280, 720), 33.0f};
    QScopeGuard cleanup([&] { removeVcam(cameraParams); });
    QMediaDevices mediaDevices = QMediaDevices();
    QSignalSpy changeSignal(&mediaDevices, &QMediaDevices::videoInputsChanged);

    QVERIFY(callVcam("--add", cameraParams));

    QTRY_COMPARE(changeSignal.size(), 1);

    int lengthAfterAdd = QMediaDevices::videoInputs().length();
    QCOMPARE(lengthAfterAdd, lengthBeforeAdd + 1);

    QCameraDevice addedCamera = findCamera(cameraParams.name);

    QVERIFY(!addedCamera.isNull());
    QVERIFY(!addedCamera.videoFormats().isEmpty());

    const QCameraFormat format = addedCamera.videoFormats().first();
    QCOMPARE(format.pixelFormat(), QVideoFrameFormat::Format_NV12);
    QCOMPARE(format.resolution(), cameraParams.resolution);
    QCOMPARE(format.maxFrameRate(), cameraParams.fps);
    QCOMPARE(format.minFrameRate(), 1.0f);
}

void tst_QCameraBackend::testVirtualCameraRemoval()
{
    if (!m_vcamAvailable)
        QSKIP("Virtual camera is not available. Skipping camera removal test.");

    QMediaDevices mediaDevices = QMediaDevices();
    QSignalSpy changeSignal(&mediaDevices, &QMediaDevices::videoInputsChanged);
    int lengthBeforeAdd = QMediaDevices::videoInputs().length();
    VCamParameters cameraParams {"TestVCamToRemove"};

    QVERIFY(callVcam("--add", cameraParams));

    QTRY_COMPARE(changeSignal.size(), 1);

    removeVcam(cameraParams);

    int lengthAfterRemove = QMediaDevices::videoInputs().length();
    QCOMPARE(lengthAfterRemove, lengthBeforeAdd);
    QVERIFY(findCamera(cameraParams.name).isNull());
}

void tst_QCameraBackend::testVirtualCameraFrameChanging()
{
    if (!m_vcamAvailable)
        QSKIP("Virtual camera is not available. Skipping camera frame test.");

    QMediaCaptureSession session;
    QCamera camera;
    QVideoSink sink;
    session.setCamera(&camera);
    camera.setCameraDevice(findCamera(m_defaultCamera.name));
    session.setVideoOutput(&sink);

    std::vector<QColor> colors;
    connect(&sink, &QVideoSink::videoFrameChanged, this, [&colors](const QVideoFrame &frame){
        QVERIFY(frame.isValid());
        QImage image = frame.toImage();
        QColor pcolor = image.pixelColor(1,1);
        colors.push_back(pcolor);
    });

    camera.start();
    QTRY_VERIFY(colors.size() >= 3);
    camera.stop();

    int tolerance = 5;
    for (int i = 0; i < 3; ++i) {
        colors[i] = dominantRgbColor(colors[i], tolerance);
        if (!colors[i].isValid())
            QFAIL("Captured frame color is not pure red, green or blue");
    }

    QList<QColor> rgbColors = {Qt::red, Qt::green, Qt::blue};
    QCOMPARE((rgbColors.indexOf(colors[0]) + 1) % 3, rgbColors.indexOf(colors[1]));
    QCOMPARE((rgbColors.indexOf(colors[1]) + 1) % 3, rgbColors.indexOf(colors[2]));
    QCOMPARE((rgbColors.indexOf(colors[2]) + 1) % 3, rgbColors.indexOf(colors[0]));
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

void tst_QCameraBackend::testMappedFrameSurvivesCameraStop()
{
    if (noCamera)
        QSKIP("No camera available");

    QMediaCaptureSession session;
    QCamera camera;
    QVideoSink sink;
    session.setCamera(&camera);
    session.setVideoOutput(&sink);

    QSignalSpy frameSpy(&sink, &QVideoSink::videoFrameChanged);
    QVERIFY(frameSpy.isValid());
    camera.start();
    QTRY_VERIFY_WITH_TIMEOUT(!frameSpy.isEmpty(), 10s);

    QVideoFrame frame = frameSpy.first().first().value<QVideoFrame>();
    QVERIFY(frame.map(QVideoFrame::ReadOnly));
    const QByteArray before(reinterpret_cast<const char *>(frame.bits(0)), frame.mappedBytes(0));

    camera.stop();

    // The mapping was taken before the stop, so it must still read the same bytes afterwards.
    const QByteArray after(reinterpret_cast<const char *>(frame.bits(0)), frame.mappedBytes(0));
    frame.unmap();
    QCOMPARE(after, before);
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

QList<QSize> tst_QCameraBackend::photoResolutionsToTest(int count)
{
    QList<QSize> sizes = QMediaDevices::defaultVideoInput().photoResolutions();
    // Smallest first, so every step in the sequence is a change
    std::sort(sizes.begin(), sizes.end(), [](const QSize &a, const QSize &b) {
        return a.width() * a.height() < b.width() * b.height();
    });
    return sizes.mid(0, count);
}

bool tst_QCameraBackend::isSameSizeAllowingRotation(const QSize &actual, const QSize &expected)
{
    return actual == expected || actual == expected.transposed();
}

QSize tst_QCameraBackend::captureToFileAndGetSize(QImageCapture &imageCapture)
{
    QSignalSpy savedSignal(&imageCapture, &QImageCapture::imageSaved);
    QSignalSpy errorSignal(&imageCapture, &QImageCapture::errorOccurred);

    if (!QTest::qWaitFor([&] { return imageCapture.isReadyForCapture(); }, 8s))
        return {};

    imageCapture.captureToFile();
    if (!QTest::qWaitFor([&] { return !savedSignal.isEmpty() || !errorSignal.isEmpty(); }, 8s)
        || savedSignal.isEmpty())
        return {};

    const QString location = savedSignal.last().last().toString();
    const QSize size = QImageReader(location).size();
    QFile::remove(location);
    return size;
}

void tst_QCameraBackend::testCaptureResolutionChangeWhileActive()
{
    if (noCamera)
        QSKIP("No camera available");

    const QList<QSize> sizes = photoResolutionsToTest(3);
    if (sizes.size() < 2)
        QSKIP("Camera reports fewer than two photo resolutions");

    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);
    camera.setFlashMode(QCamera::FlashOff);

    QVideoSink sink;
    session.setVideoOutput(&sink);
    camera.start();
    QTRY_VERIFY(camera.isActive());
    QTRY_VERIFY(imageCapture.isReadyForCapture());

    QSignalSpy activeChanged(&camera, &QCamera::activeChanged);
    QSignalSpy errorSignal(&camera, &QCamera::errorOccurred);

    // Go through the sizes twice so both growing and shrinking are covered
    for (const QSize &size : sizes + sizes) {
        imageCapture.setResolution(size);
        QCOMPARE(imageCapture.resolution(), size);

        QTRY_VERIFY_WITH_TIMEOUT(camera.isActive(), 8s);
        QTRY_VERIFY_WITH_TIMEOUT(imageCapture.isReadyForCapture(), 8s);

        const QSize captured = captureToFileAndGetSize(imageCapture);
        QVERIFY2(!captured.isEmpty(), "Capture failed after resolution change");
#ifdef Q_OS_ANDROID
        QVERIFY(isSameSizeAllowingRotation(captured, size));
#endif
    }

#ifdef Q_OS_ANDROID
    // The session is rebuilt in place, the camera is not restarted
    QCOMPARE(activeChanged.size(), 0);
#endif
    QCOMPARE(errorSignal.size(), 0);
    QVERIFY(camera.isActive());
}

void tst_QCameraBackend::testCaptureResolutionChangeRapid()
{
    if (noCamera)
        QSKIP("No camera available");

    const QList<QSize> sizes = photoResolutionsToTest(3);
    if (sizes.size() < 2)
        QSKIP("Camera reports fewer than two photo resolutions");

    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);
    camera.setFlashMode(QCamera::FlashOff);

    QVideoSink sink;
    session.setVideoOutput(&sink);
    camera.start();
    QTRY_VERIFY(camera.isActive());
    QTRY_VERIFY(imageCapture.isReadyForCapture());

    QSignalSpy errorSignal(&camera, &QCamera::errorOccurred);

    // Changes arrive before earlier ones have been applied
    for (int i = 0; i < 12; ++i)
        imageCapture.setResolution(sizes.at(i % sizes.size()));
    [[maybe_unused]] const QSize last = imageCapture.resolution();

    QTRY_VERIFY_WITH_TIMEOUT(camera.isActive(), 8s);
    const QSize captured = captureToFileAndGetSize(imageCapture);
    QVERIFY2(!captured.isEmpty(), "Capture failed after rapid resolution changes");
#ifdef Q_OS_ANDROID
    QVERIFY(isSameSizeAllowingRotation(captured, last));
#endif
    QCOMPARE(errorSignal.size(), 0);
}

void tst_QCameraBackend::testCaptureResolutionSetBeforeStart()
{
    if (noCamera)
        QSKIP("No camera available");

    const QList<QSize> sizes = photoResolutionsToTest(2);
    if (sizes.isEmpty())
        QSKIP("Camera reports no photo resolutions");

    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);
    camera.setFlashMode(QCamera::FlashOff);

    imageCapture.setResolution(sizes.first());

    QVideoSink sink;
    session.setVideoOutput(&sink);
    camera.start();
    QTRY_VERIFY(camera.isActive());

    const QSize captured = captureToFileAndGetSize(imageCapture);
    QVERIFY2(!captured.isEmpty(), "Capture failed");
#ifdef Q_OS_ANDROID
    QVERIFY(isSameSizeAllowingRotation(captured, sizes.first()));
#endif

    // Stop, change while inactive, then restart
    camera.stop();
    QTRY_VERIFY(!camera.isActive());
    imageCapture.setResolution(sizes.last());
    camera.start();
    QTRY_VERIFY(camera.isActive());

    const QSize capturedAfterRestart = captureToFileAndGetSize(imageCapture);
    QVERIFY2(!capturedAfterRestart.isEmpty(), "Capture failed after restart");
#ifdef Q_OS_ANDROID
    QVERIFY(isSameSizeAllowingRotation(capturedAfterRestart, sizes.last()));
#endif
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

    if (isAndroidPlatform())
        QSKIP("Crashes in QImageCapture::setFileFormat() with the android backend");

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

void tst_QCameraBackend::testNativeMetadata()
{
    QSKIP_OHOS("OH_AVRecorder/OH_AVPlayer do not yet round-trip MP4 user metadata (Title, Language, Description)");
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

void tst_QCameraBackend::addRowPerCameraDevice()
{
    QTest::addColumn<QCameraDevice>("device");

    const QList<QCameraDevice> devices = QMediaDevices::videoInputs();
    for (qsizetype i = 0; i < devices.size(); ++i)
        QTest::addRow("%lld: %s", qlonglong(i), qPrintable(devices[i].description())) << devices[i];
}

void tst_QCameraBackend::videoFormats_isNotEmpty_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

// Every camera must offer at least one video format. Other tests rely on this.
void tst_QCameraBackend::videoFormats_isNotEmpty_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    QVERIFY(!device.videoFormats().isEmpty());
}

void tst_QCameraBackend::videoFormats_containsNoDuplicates_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::videoFormats_containsNoDuplicates_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    const QList<QCameraFormat> formats = device.videoFormats();
    QStringList duplicates;
    for (qsizetype i = 0; i < formats.size(); ++i) {
        for (qsizetype j = i + 1; j < formats.size(); ++j) {
            if (formats[i] != formats[j])
                continue;
            const QCameraFormat &format = formats[i];
            duplicates.append(u"#%1 and #%2: %3x%4, %5, %6-%7 fps"_s
                .arg(i)
                .arg(j)
                .arg(format.resolution().width())
                .arg(format.resolution().height())
                .arg(QVideoFrameFormat::pixelFormatToString(format.pixelFormat()))
                .arg(format.minFrameRate())
                .arg(format.maxFrameRate()));
        }
    }

    QVERIFY2(duplicates.isEmpty(),
        qPrintable(u"Duplicate video formats:\n"_s + duplicates.join(u'\n')));
}

void tst_QCameraBackend::setCameraFormat_updatesPropertyAndEmitsSignal_whenNotActive_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::setCameraFormat_updatesPropertyAndEmitsSignal_whenNotActive_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    CameraStreamSetup setup(device);
    QList<QCameraFormat> videoFormats = device.videoFormats();
    QVERIFY(!videoFormats.isEmpty());

    QSignalSpy formatChanges(&setup.camera, &QCamera::cameraFormatChanged);

    QVERIFY(setup.camera.cameraFormat().isNull());

    for (const QCameraFormat &format : std::as_const(videoFormats)) {
        setup.camera.setCameraFormat(format);
        QCOMPARE(setup.camera.cameraFormat(), format);
    }
    QCOMPARE(formatChanges.size(), videoFormats.size());

    formatChanges.clear();
    setup.camera.setCameraFormat({});
    QVERIFY(setup.camera.cameraFormat().isNull());
    QCOMPARE(formatChanges.size(), 1);

    QVERIFY(!setup.camera.isActive());
    QVERIFY(setup.grabber.getFrameInfos().empty());
    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
}

void tst_QCameraBackend::setCameraDevice_keepsCameraFormat_whenDeviceIsUnchanged_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::setCameraDevice_keepsCameraFormat_whenDeviceIsUnchanged_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    QVERIFY(!device.videoFormats().isEmpty());

    CameraStreamSetup setup(device);
    QCameraFormat format = device.videoFormats().constFirst();
    setup.camera.setCameraFormat(format);

    QSignalSpy formatChanges(&setup.camera, &QCamera::cameraFormatChanged);
    QSignalSpy deviceChanges(&setup.camera, &QCamera::cameraDeviceChanged);

    setup.camera.setCameraDevice(device);

    QCOMPARE(deviceChanges.size(), 0);
    QCOMPARE(formatChanges.size(), 0);
    QCOMPARE(setup.camera.cameraFormat(), format);
}

void tst_QCameraBackend::setActive_startsCameraAndDeliversFrames_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::setActive_startsCameraAndDeliversFrames_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    CameraStreamSetup setup(device);
    QCOMPARE(setup.camera.cameraDevice(), device);

    setup.camera.setActive(true);
    QVERIFY(setup.camera.isActive());

    std::optional<VideoFrameInfo> frame = setup.grabber.tryWaitForFirstFrameInfo();
    QVERIFY2(frame, "Camera delivered no frames");
    QVERIFY(!frame->size.isEmpty());
    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
}

void tst_QCameraBackend::setActive_restartsStream_whenStartedAgainAfterStop_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::setActive_restartsStream_whenStartedAgainAfterStop_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    constexpr int restartCount = 3;

    CameraStreamSetup setup(device);
    QVERIFY(!setup.camera.isActive());
    QSignalSpy activeChanged(&setup.camera, &QCamera::activeChanged);

    for (int i = 0; i <= restartCount; ++i) {
        if (i > 0) {
            setup.camera.setActive(false);
            QVERIFY(!setup.camera.isActive());
        }

        setup.camera.setActive(true);
        QVERIFY(setup.camera.isActive());
        QVERIFY2(setup.grabber.tryWaitForFirstFrameInfo(),
            qPrintable(u"Camera delivered no frames after start #%1"_s.arg(i)));
    }

    QCOMPARE(activeChanged.size(), restartCount * 2 + 1);
    for (qsizetype i = 0; i < activeChanged.size(); ++i)
        QCOMPARE(activeChanged[i].first().toBool(), i % 2 == 0);
    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
}

void tst_QCameraBackend::setActive_fails_whenCameraDeviceDoesNotExist_data()
{
    QTest::addColumn<bool>("setViaConstructor");

    QTest::addRow("constructor") << true;
    QTest::addRow("setCameraDevice") << false;
}

void tst_QCameraBackend::setActive_fails_whenCameraDeviceDoesNotExist()
{
    QFETCH(const bool, setViaConstructor);

    auto formatPrivate = std::make_unique<QCameraFormatPrivate>();
    formatPrivate->pixelFormat = QVideoFrameFormat::PixelFormat::Format_NV12;
    formatPrivate->resolution = QSize(640, 480);
    formatPrivate->minFrameRate = 30;
    formatPrivate->maxFrameRate = 30;

    auto devicePrivate = std::make_unique<QCameraDevicePrivate>();
    devicePrivate->id = "qt-test-nonexistent-camera";
    devicePrivate->description = u"Nonexistent camera"_s;
    devicePrivate->videoFormats = { QCameraFormatPrivate::create(std::move(formatPrivate)) };
    QCameraDevice device = QCameraDevicePrivate::create(std::move(devicePrivate));
    QVERIFY(!device.isNull());
    QVERIFY(!QMediaDevices::videoInputs().contains(device));

    std::unique_ptr<QCamera> camera = setViaConstructor
        ? std::make_unique<QCamera>(device)
        : std::make_unique<QCamera>();
    QSignalSpy errorSignal(camera.get(), &QCamera::errorOccurred);
    if (!setViaConstructor)
        camera->setCameraDevice(device);

    QtMultimediaTest::FrameGrabber grabber;
    QMediaCaptureSession session;
    session.setCamera(camera.get());
    session.setVideoSink(&grabber);

    camera->setActive(true);

    QVERIFY(!camera->isActive());
    QCOMPARE(camera->error(), QCamera::Error::CameraError);
    QVERIFY(!camera->errorString().isEmpty());
    QVERIFY(!errorSignal.isEmpty());
    QVERIFY(grabber.getFrameInfos().empty());
}

void tst_QCameraBackend::cameraFormat_isNotNull_whenActive_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::cameraFormat_isNotNull_whenActive()
{
    QFETCH(const QCameraDevice, device);

    CameraStreamSetup setup(device);
    QVERIFY(setup.camera.cameraFormat().isNull());

    setup.camera.setActive(true);
    QVERIFY(setup.camera.isActive());

    QCameraFormat format = setup.camera.cameraFormat();
    QVERIFY(!format.isNull());
    QVERIFY(device.videoFormats().contains(format));

    std::optional<VideoFrameInfo> frame = setup.grabber.tryWaitForFirstFrameInfo();
    QVERIFY2(frame, "Camera delivered no frames");
    QCOMPARE(frame->size, format.resolution());
    QCOMPARE(frame->pixelFormat, format.pixelFormat());
}

void tst_QCameraBackend::capturedFrames_matchCameraFormat_withEachCameraFormat_data()
{
    QTest::addColumn<QCameraDevice>("device");
    QTest::addColumn<QCameraFormat>("format");

    if (noCamera)
        QSKIP("No camera available");

    const QList<QCameraDevice> devices = QMediaDevices::videoInputs();
    for (qsizetype i = 0; i < devices.size(); ++i) {
        const QList<QCameraFormat> formats = devices[i].videoFormats();
        for (qsizetype j = 0; j < formats.size(); ++j) {
            const QCameraFormat &format = formats[j];
            QTest::addRow(
                "%lld: %s, format %lld: %dx%d, %s, %g-%g fps",
                qlonglong(i),
                qPrintable(devices[i].description()),
                qlonglong(j),
                format.resolution().width(),
                format.resolution().height(),
                qPrintable(QVideoFrameFormat::pixelFormatToString(format.pixelFormat())),
                format.minFrameRate(),
                format.maxFrameRate())
                << devices[i] << format;
        }
    }
}

void tst_QCameraBackend::capturedFrames_matchCameraFormat_withEachCameraFormat()
{
    QFETCH(const QCameraDevice, device);
    QFETCH(const QCameraFormat, format);

    constexpr size_t frameCount = 5;

    CameraStreamSetup setup(device);
    setup.camera.setCameraFormat(format);
    QCOMPARE(setup.camera.cameraFormat(), format);

    setup.camera.setActive(true);
    QVERIFY(setup.camera.isActive());
    QVERIFY2(setup.grabber.tryWaitForFirstFrameInfo(), "Camera delivered no frames");

    std::vector<VideoFrameInfo> frames = setup.grabber.waitAndTakeFrameInfos(frameCount);
    QCOMPARE_GE(frames.size(), frameCount);

    for (const VideoFrameInfo &frame : frames) {
        QVERIFY(frame.isValid);
        QCOMPARE(frame.size, format.resolution());
        QCOMPARE(frame.pixelFormat, format.pixelFormat());
    }
    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
}

void tst_QCameraBackend::capturedFrames_havePresentationTimesInIncreasingOrder_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::capturedFrames_havePresentationTimesInIncreasingOrder()
{
    QFETCH(const QCameraDevice, device);

    CameraStreamSetup setup(device);
    setup.camera.setActive(true);
    QVERIFY(setup.camera.isActive());

    std::vector<VideoFrameInfo> frames = setup.grabber.waitAndTakeFrameInfos(10);
    QVERIFY2(frames.size() >= 2,
        "Did not receive enough QVideoFrames to compare presentation times");

    for (size_t i = 0; i < frames.size(); ++i) {
        const VideoFrameInfo &frame = frames[i];

        QVERIFY2(frame.startTime, qPrintable(u"Frame #%1 has no start time"_s.arg(i)));

        bool endsAfterStart = frame.endTime && *frame.endTime > *frame.startTime;
        QVERIFY2(!frame.endTime || endsAfterStart,
            qPrintable(u"Frame #%1 end time (%2) is neither unset nor after its start time (%3)"_s
                .arg(i)
                .arg(frameTimeToString(frame.endTime))
                .arg(frameTimeToString(frame.startTime))));

        if (frame.streamFrameRate > 0) {
            QVERIFY2(endsAfterStart,
                qPrintable(u"Frame #%1 end time (%2) is not after its start time (%3), "
                    u"although its frame rate (%4) is reported"_s
                    .arg(i)
                    .arg(frameTimeToString(frame.endTime))
                    .arg(frameTimeToString(frame.startTime))
                    .arg(frame.streamFrameRate)));
        }

        if (i == 0)
            continue;

        std::chrono::microseconds prevStart = *frames[i - 1].startTime;
        QVERIFY2(*frame.startTime > prevStart,
            qPrintable(u"Frame #%1 start time (%2) is not after frame #%3 start time (%4)"_s
                .arg(i)
                .arg(frameTimeToString(frame.startTime))
                .arg(i - 1)
                .arg(frameTimeToString(prevStart))));
    }
}

void tst_QCameraBackend::firstFrame_hasZeroStartTime_whenCameraIsStartedOrRestarted_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::firstFrame_hasZeroStartTime_whenCameraIsStartedOrRestarted_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    constexpr int restartCount = 2;

    CameraStreamSetup setup(device);

    for (int i = 0; i <= restartCount; ++i) {
        if (i > 0) {
            setup.camera.setActive(false);
            // Frames the backend already produced may still be queued towards the sink.
            // Deliver them before restarting, so they are not mistaken for the first
            // frame of the new stream.
            QCoreApplication::processEvents();
        }

        setup.camera.setActive(true);
        QVERIFY(setup.camera.isActive());

        std::optional<VideoFrameInfo> frame = setup.grabber.tryWaitForFirstFrameInfo();
        QVERIFY2(frame, qPrintable(u"Camera delivered no frames after start #%1"_s.arg(i)));
        QVERIFY(frame->startTime);
        QCOMPARE(*frame->startTime, 0us);
    }
}

void tst_QCameraBackend::storingFrames_doesNotStallStream_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

// For some platforms, the hardware frames we receive are tied to internal frame pools.
// If our QVideoFrames point into these pools, holding on to them can stall the stream.
void tst_QCameraBackend::storingFrames_doesNotStallStream()
{
    QFETCH(const QCameraDevice, device);

    // We assume this is larger than the internal frame pool of any backend
    constexpr size_t capturedFrameCount = 30;

    CameraStreamSetup setup(device);
    setup.camera.setActive(true);
    QVERIFY(setup.camera.isActive());
    QVERIFY2(setup.grabber.tryWaitForFirstFrameInfo(), "Camera delivered no frames");

    std::vector<QVideoFrame> frames = setup.grabber.waitAndTakeVideoFrames(capturedFrameCount);

    QCOMPARE_GE(frames.size(), capturedFrameCount);
    for (const QVideoFrame &frame : frames)
        QVERIFY(frame.isValid());
}

void tst_QCameraBackend::stream_endsWithNullFrame_whenStopped_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::stream_endsWithNullFrame_whenStopped_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    CameraStreamSetup setup(device);
    setup.camera.setActive(true);
    QVERIFY(setup.camera.isActive());
    QVERIFY2(setup.grabber.tryWaitForFirstFrameInfo(), "Camera delivered no frames");

    setup.camera.setActive(false);
    QVERIFY(!setup.camera.isActive());

    verifyStreamEndsWithNullFrame(setup.grabber);
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
}

void tst_QCameraBackend::stream_endsWithNullFrame_whenCameraIsDestroyed_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::stream_endsWithNullFrame_whenCameraIsDestroyed_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    CameraStreamSetup setup(device);
    setup.camera.setActive(true);
    QVERIFY(setup.camera.isActive());
    QVERIFY2(setup.grabber.tryWaitForFirstFrameInfo(), "Camera delivered no frames");
    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));

    // Destroying the QCamera must terminate the stream with a null-frame. The grabber
    // (video sink) outlives the camera, so it still receives the terminating null-frame.
    setup.cameraStorage.reset();

    verifyStreamEndsWithNullFrame(setup.grabber);
}

void tst_QCameraBackend::setCameraDevice_switchesStream_withoutDeactivatingOrErrors()
{
    const QList<QCameraDevice> devices = QMediaDevices::videoInputs();
    if (devices.size() < 2)
        QSKIP("At least two cameras are required to switch between them");

    constexpr size_t frameCount = 5;

    CameraStreamSetup setup(devices.first());
    setup.camera.setActive(true);
    QVERIFY(setup.camera.isActive());
    QVERIFY2(setup.grabber.tryWaitForFirstFrameInfo(), "Camera delivered no frames");

    QSignalSpy activeChanged(&setup.camera, &QCamera::activeChanged);

    QList<QCameraDevice> switchOrder = devices.mid(1);
    switchOrder.append(devices.first());
    for (const QCameraDevice &device : std::as_const(switchOrder)) {
        setup.camera.setCameraDevice(device);
        QCOMPARE(setup.camera.cameraDevice(), device);
        QVERIFY(setup.camera.isActive());

        size_t framesBefore = setup.grabber.getFrameInfos().size();
        bool receivedFrames = QTest::qWaitFor(
            [&] {
                return setup.grabber.getFrameInfos().size() >= framesBefore + frameCount;
            },
            surfaceCaptureTestTimeout());
        QVERIFY2(receivedFrames,
            qPrintable(u"No frames after switching to %1"_s.arg(device.description())));
        QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
    }

    QVERIFY(setup.camera.isActive());
    QCOMPARE(activeChanged.size(), 0);
}

void tst_QCameraBackend::setCameraDevice_keepsFrameFormat_whenSwitchingToDeviceWithSameFormat_data()
{
    QTest::addColumn<QCameraDevice>("fromDevice");
    QTest::addColumn<QCameraFormat>("fromFormat");
    QTest::addColumn<QCameraDevice>("toDevice");
    QTest::addColumn<QCameraFormat>("toFormat");

    if (noCamera)
        QSKIP("No camera available");

    // The frame rate does not affect the format of the frames, so formats are matched
    // by resolution and pixel format only.
    const auto haveSameFrameFormat = [](const QCameraFormat &a, const QCameraFormat &b) {
        return a.resolution() == b.resolution() && a.pixelFormat() == b.pixelFormat();
    };

    const QList<QCameraDevice> devices = QMediaDevices::videoInputs();
    qsizetype rowCount = 0;
    for (qsizetype i = 0; i < devices.size(); ++i) {
        for (qsizetype j = 0; j < devices.size(); ++j) {
            if (i == j)
                continue;

            const QList<QCameraFormat> fromFormats = devices[i].videoFormats();
            const QList<QCameraFormat> toFormats = devices[j].videoFormats();
            for (const QCameraFormat &fromFormat : fromFormats) {
                auto toFormat = std::find_if(
                    toFormats.cbegin(),
                    toFormats.cend(),
                    [&](const QCameraFormat &format) {
                        return haveSameFrameFormat(fromFormat, format);
                    });
                if (toFormat == toFormats.cend())
                    continue;

                QTest::addRow(
                    "%lld: %s to %lld: %s, %dx%d, %s",
                    qlonglong(i),
                    qPrintable(devices[i].description()),
                    qlonglong(j),
                    qPrintable(devices[j].description()),
                    fromFormat.resolution().width(),
                    fromFormat.resolution().height(),
                    qPrintable(QVideoFrameFormat::pixelFormatToString(fromFormat.pixelFormat())))
                    << devices[i] << fromFormat << devices[j] << *toFormat;
                ++rowCount;
                break; // One shared frame format per pair of devices is enough
            }
        }
    }

    if (rowCount == 0)
        QSKIP("No two camera devices support the same resolution and pixel format");
}

void tst_QCameraBackend::setCameraDevice_keepsFrameFormat_whenSwitchingToDeviceWithSameFormat()
{
    QFETCH(const QCameraDevice, fromDevice);
    QFETCH(const QCameraFormat, fromFormat);
    QFETCH(const QCameraDevice, toDevice);
    QFETCH(const QCameraFormat, toFormat);

    constexpr size_t frameCount = 5;

    CameraStreamSetup setup(fromDevice);
    setup.camera.setCameraFormat(fromFormat);
    setup.camera.setActive(true);
    QVERIFY(setup.camera.isActive());
    QVERIFY2(setup.grabber.tryWaitForFirstFrameInfo(), "Camera delivered no frames");

    // Changing the camera device resets the camera format, so it is set again right away.
    setup.camera.setCameraDevice(toDevice);
    setup.camera.setCameraFormat(toFormat);
    QCOMPARE(setup.camera.cameraDevice(), toDevice);
    QCOMPARE(setup.camera.cameraFormat(), toFormat);
    QVERIFY(setup.camera.isActive());

    size_t framesBefore = setup.grabber.getFrameInfos().size();
    bool receivedFrames = QTest::qWaitFor(
        [&] {
            return setup.grabber.getFrameInfos().size() >= framesBefore + frameCount;
        },
        surfaceCaptureTestTimeout());
    QVERIFY2(receivedFrames, "No frames after switching camera device");

    // Both camera devices deliver frames with the same resolution and pixel format, so
    // the format of the stream must not change at any point, not even while switching.
    const std::vector<VideoFrameInfo> &frames = setup.grabber.getFrameInfos();
    const VideoFrameInfo &firstFrame = frames.front();
    for (size_t i = 0; i < frames.size(); ++i) {
        const VideoFrameInfo &frame = frames[i];
        QVERIFY2(
            frame.isValid && frame.size == firstFrame.size
                && frame.pixelFormat == firstFrame.pixelFormat,
            qPrintable(u"Frame #%1 is %2 %3x%4, %5, but the first frame is %6x%7, %8"_s
                .arg(i)
                .arg(frame.isValid ? u"valid"_s : u"null"_s)
                .arg(frame.size.width())
                .arg(frame.size.height())
                .arg(QVideoFrameFormat::pixelFormatToString(frame.pixelFormat))
                .arg(firstFrame.size.width())
                .arg(firstFrame.size.height())
                .arg(QVideoFrameFormat::pixelFormatToString(firstFrame.pixelFormat))));
    }
    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
}

void tst_QCameraBackend::setCameraFormat_switchesStreamFormat_whileActive_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::setCameraFormat_switchesStreamFormat_whileActive()
{
    QFETCH(const QCameraDevice, device);

    CameraStreamSetup setup(device);
    setup.camera.setActive(true);
    QVERIFY(setup.camera.isActive());
    QVERIFY2(setup.grabber.tryWaitForFirstFrameInfo(), "Camera delivered no frames");

    const QList<QCameraFormat> videoFormats = device.videoFormats();
    QVERIFY(!videoFormats.isEmpty());
    for (const QCameraFormat &format : videoFormats) {
        QString formatDescription = u"%1x%2, %3, %4-%5 fps"_s
            .arg(format.resolution().width())
            .arg(format.resolution().height())
            .arg(QVideoFrameFormat::pixelFormatToString(format.pixelFormat()))
            .arg(format.minFrameRate())
            .arg(format.maxFrameRate());

        size_t framesBefore = setup.grabber.getFrameInfos().size();
        setup.camera.setCameraFormat(format);
        QCOMPARE(setup.camera.cameraFormat(), format);
        QVERIFY(setup.camera.isActive());

        // Frames already in flight may still have the previous format, so wait
        // until the stream eventually delivers frames in the new one.
        bool receivedNewFormat = QTest::qWaitFor(
            [&] {
                const std::vector<VideoFrameInfo> &frames = setup.grabber.getFrameInfos();
                for (size_t i = framesBefore; i < frames.size(); ++i) {
                    if (frames[i].isValid && frames[i].size == format.resolution()
                        && frames[i].pixelFormat == format.pixelFormat()) {
                        return true;
                    }
                }
                return false;
            },
            surfaceCaptureTestTimeout());

        if (!receivedNewFormat) {
            const std::vector<VideoFrameInfo> &frames = setup.grabber.getFrameInfos();
            QString lastFrame = frames.size() > framesBefore
                ? u"%1x%2, %3"_s
                    .arg(frames.back().size.width())
                    .arg(frames.back().size.height())
                    .arg(QVideoFrameFormat::pixelFormatToString(frames.back().pixelFormat))
                : u"none"_s;
            QFAIL(qPrintable(u"No frames in format %1 after switching to it. Last frame: %2"_s
                .arg(formatDescription, lastFrame)));
        }
        QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
    }

    QVERIFY(setup.camera.isActive());
}

void tst_QCameraBackend::verifyRecorderAutoStopped(CameraRecorderSetup &setup)
{
    if (!setup.waitForRecorderStopped()) {
        // Stop explicitly, so the recording is finalized before the test ends.
        QVERIFY(setup.stopRecording());
        QFAIL("The recorder did not auto-stop");
    }

    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
    QVERIFY2(setup.recorderErrors.isEmpty(), qPrintable(setup.recorderErrors.join(u'\n')));

    const std::optional<MediaInfo> info = MediaInfo::create(setup.recorder.actualLocation());
    QVERIFY(info);
    QVERIFY(info->m_hasVideo);
    QCOMPARE_GT(info->m_frameCount, 0);
}

void tst_QCameraBackend::recorder_encodesFrames_toValidMediaFile_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::recorder_encodesFrames_toValidMediaFile_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    constexpr size_t frameCount = 30;

    CameraRecorderSetup setup(device);
    setup.startRecording();
    if (QTest::currentTestFailed())
        return;

    // Wait on the grabber to make sure the recorder also gets some frames
    std::vector<VideoFrameInfo> frames = setup.grabber.waitAndTakeFrameInfos(frameCount);
    QCOMPARE_GE(frames.size(), frameCount);

    QVERIFY(setup.stopRecording());
    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
    QVERIFY2(setup.recorderErrors.isEmpty(), qPrintable(setup.recorderErrors.join(u'\n')));

    const QUrl location = setup.recorder.actualLocation();
    QVERIFY(QFileInfo::exists(location.toLocalFile()));

    const std::optional<MediaInfo> info = MediaInfo::create(location);
    QVERIFY(info);
    QVERIFY(info->m_hasVideo);
    QCOMPARE_GT(info->m_frameCount, 0);

    // Without a requested resolution, the recording keeps the size of the camera frames
    QCOMPARE(info->m_size, frames.back().size);
}

void tst_QCameraBackend::recorder_encodesFrames_withRequestedResolution_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::recorder_encodesFrames_withRequestedResolution_withEachCameraDevice()
{
    QSKIP_OHOS("OH_AVPlayer does not extract MP4 stream metadata (resolution) yet");

    QFETCH(const QCameraDevice, device);

    constexpr QSize resolution{ 320, 240 };

    CameraRecorderSetup setup(device);
    setup.recorder.setVideoResolution(resolution);
    setup.startRecording();
    if (QTest::currentTestFailed())
        return;

    std::ignore = setup.grabber.waitAndTakeFrameInfos(10);

    QVERIFY(setup.stopRecording());
    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
    QVERIFY2(setup.recorderErrors.isEmpty(), qPrintable(setup.recorderErrors.join(u'\n')));

    const std::optional<MediaInfo> info = MediaInfo::create(setup.recorder.actualLocation());
    QVERIFY(info);
    QCOMPARE(info->m_size, resolution);
}

void tst_QCameraBackend::recorder_recordingHasExpectedDuration_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::recorder_recordingHasExpectedDuration_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    constexpr auto recordingDuration = 2s;

    CameraRecorderSetup setup(device);
    setup.startRecording();
    if (QTest::currentTestFailed())
        return;

    QElapsedTimer recordingTimer;
    recordingTimer.start();

    QTest::qWait(recordingDuration);

    const std::chrono::milliseconds expectedDuration{ recordingTimer.elapsed() };

    QVERIFY(setup.stopRecording());
    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
    QVERIFY2(setup.recorderErrors.isEmpty(), qPrintable(setup.recorderErrors.join(u'\n')));

    const std::optional<MediaInfo> info = MediaInfo::create(setup.recorder.actualLocation());
    QVERIFY(info);

    // Allow some slack for the time it takes to start and stop the stream
    constexpr std::chrono::milliseconds tolerance = 250ms;
    QVERIFY2(
        std::chrono::abs(info->m_duration - expectedDuration) <= tolerance,
        qPrintable(u"Recording lasted %1ms, but the camera was recorded for %2ms"_s
            .arg(info->m_duration.count())
            .arg(expectedDuration.count())));
}

void tst_QCameraBackend::recorder_encodesAllCameraFrames_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::recorder_encodesAllCameraFrames_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    CameraRecorderSetup setup(device);
    setup.startRecording();
    if (QTest::currentTestFailed())
        return;

    size_t framesAtStart = setup.grabber.getFrameInfos().size();
    QTest::qWait(2s);
    size_t framesAtStop = setup.grabber.getFrameInfos().size();

    QVERIFY(setup.stopRecording());
    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
    QVERIFY2(setup.recorderErrors.isEmpty(), qPrintable(setup.recorderErrors.join(u'\n')));

    const std::optional<MediaInfo> info = MediaInfo::create(setup.recorder.actualLocation());
    QVERIFY(info);

    // The recording starts slightly before and ends slightly after the frames we
    // counted, so allow for a few frames at the ends. The recording can contain more
    // frames than the camera delivered, since the encoder repeats frames to keep a
    // constant frame rate when the camera delivers fewer frames than its format reports.
    const int deliveredFrames = int(framesAtStop - framesAtStart);
    const int tolerance = qMax(3, deliveredFrames / 10);
    QVERIFY2(
        info->m_frameCount >= deliveredFrames - tolerance,
        qPrintable(u"Recording has %1 frames, but the camera delivered %2 frames while recording"_s
            .arg(info->m_frameCount)
            .arg(deliveredFrames)));
}

void tst_QCameraBackend::recorder_autoStops_whenCameraIsDeactivated_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::recorder_autoStops_whenCameraIsDeactivated_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    CameraRecorderSetup setup(device);
    setup.recorder.setAutoStop(true);
    setup.startRecording();
    if (QTest::currentTestFailed())
        return;

    setup.camera.setActive(false);

    verifyRecorderAutoStopped(setup);
}

void tst_QCameraBackend::recorder_autoStops_whenCameraIsRemovedFromCaptureSession_withEachCameraDevice_data()
{
    if (noCamera)
        QSKIP("No camera available");

    addRowPerCameraDevice();
}

void tst_QCameraBackend::recorder_autoStops_whenCameraIsRemovedFromCaptureSession_withEachCameraDevice()
{
    QFETCH(const QCameraDevice, device);

    CameraRecorderSetup setup(device);
    setup.recorder.setAutoStop(true);
    setup.startRecording();
    if (QTest::currentTestFailed())
        return;

    setup.session.setCamera(nullptr);

    verifyRecorderAutoStopped(setup);
}

// Switching the camera device ends the stream of the old device. This must not
// be treated as the end of the input, so the recorder must not auto-stop.
void tst_QCameraBackend::recorder_doesNotAutoStop_whenCameraDeviceIsSwitched()
{
    const QList<QCameraDevice> devices = QMediaDevices::videoInputs();
    if (devices.size() < 2)
        QSKIP("At least two cameras are required to switch between them");

    constexpr size_t frameCount = 10;

    CameraRecorderSetup setup(devices.first());
    setup.recorder.setAutoStop(true);
    setup.startRecording();
    if (QTest::currentTestFailed())
        return;

    QSignalSpy recorderStateChanges(&setup.recorder, &QMediaRecorder::recorderStateChanged);

    setup.camera.setCameraDevice(devices.at(1));
    QVERIFY(setup.camera.isActive());

    // Give the recorder time to process frames from the new device
    QCOMPARE_GE(setup.grabber.waitAndTakeFrameInfos(frameCount).size(), frameCount);

    const QMetaEnum stateEnum = QMetaEnum::fromType<QMediaRecorder::RecorderState>();
    QStringList stateChanges;
    for (const QList<QVariant> &stateChange : std::as_const(recorderStateChanges))
        stateChanges.append(QString::fromLatin1(stateEnum.valueToKey(stateChange.at(0).toInt())));
    QVERIFY2(
        recorderStateChanges.isEmpty(),
        qPrintable(u"Recorder state changed to %1 after switching camera device. Errors: %2"_s
            .arg(stateChanges.join(u", "_s), setup.recorderErrors.join(u'\n'))));
    QCOMPARE(setup.recorder.recorderState(), QMediaRecorder::RecorderState::RecordingState);

    // Deactivating the camera ends the input, which must trigger auto-stop.
    // This ensures that auto-stop works at all in this setup.
    setup.camera.setActive(false);
    verifyRecorderAutoStopped(setup);

    QVERIFY2(setup.errors.isEmpty(), qPrintable(setup.errors.join(u'\n')));
}

QTEST_MAIN(tst_QCameraBackend)

#include "tst_qcamerabackend.moc"
