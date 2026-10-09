// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtMultimedia/qcamera.h>
#include <QtMultimedia/qcameradevice.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideosink.h>
#include <QtGui/qimage.h>

#include <QtCore/qversionnumber.h>

#include <private/mediabackendutils_p.h>
#include <private/qfakexdgportalfixture_p.h>
#include <private/qsyntheticvideoscene_p.h>

#include <pipewire/pipewire.h>

#include <algorithm>
#include <optional>

using namespace Qt::StringLiterals;

namespace {

std::optional<QCameraDevice> findFakePipeWireCamera()
{
    const QList<QCameraDevice> devices = QMediaDevices::videoInputs();
    auto it = std::find_if(devices.begin(), devices.end(), [](const QCameraDevice &device) {
        return device.id().startsWith("pw:");
    });
    if (it == devices.end())
        return std::nullopt;
    return *it;
}

bool hasRecentEnoughPipeWire()
{
    // NB: pw_check_library_version() only exists since 0.3.75, so compare
    // the ancient pw_get_library_version() string instead.
    return QVersionNumber::fromString(QString::fromLatin1(pw_get_library_version()))
            >= QVersionNumber({ 0, 3, 49 });
}

} // namespace

class tst_QPipeWireCamera : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void videoInputs_listsTheFakeCamera();
    void camera_receivesFramesMatchingThePublishedScene();

private:
    QFakeXdgPortalFixture m_fakePortal;
    SyntheticVideoScene m_scene;
};

void tst_QPipeWireCamera::initTestCase()
{
    if (!initIntegrationTestCase())
        return;

    if (!QFakeXdgPortalFixture::isRequested(QtFakeXdgPortal::Role::Camera))
        QSKIP("The fake Camera portal fixture is disabled");

    QString unavailableReason;
    if (!QFakeXdgPortalFixture::prerequisitesAvailable(&unavailableReason))
        QSKIP(qPrintable(u"Fake Camera portal unavailable: "_s + unavailableReason));

    if (!hasRecentEnoughPipeWire())
        QSKIP("The fake Camera portal needs PipeWire 0.3.49 or newer");

    m_scene.frameSize = QSize(640, 480);
    m_scene.patternRect = QRect(50, 40, 300, 240);

    auto result = m_fakePortal.start(m_scene, QtFakeXdgPortal::Role::Camera);
    QVERIFY2(result,
             qPrintable(u"Could not start the fake Camera portal: "_s
                        + (result ? QString() : result.error())));
}

void tst_QPipeWireCamera::cleanupTestCase()
{
    m_fakePortal.stop();
}

void tst_QPipeWireCamera::videoInputs_listsTheFakeCamera()
{
    QVERIFY2(findFakePipeWireCamera(),
             "No PipeWire camera device (id starting with \"pw:\") "
             "was found in QMediaDevices::videoInputs()");
}

void tst_QPipeWireCamera::camera_receivesFramesMatchingThePublishedScene()
{
    const std::optional<QCameraDevice> device = findFakePipeWireCamera();
    QVERIFY(device);

    QCamera camera(*device);
    QMediaCaptureSession session;
    session.setCamera(&camera);
    QVideoSink sink;
    session.setVideoSink(&sink);

    QSignalSpy frameSpy(&sink, &QVideoSink::videoFrameChanged);
    camera.start();

    QVERIFY2(frameSpy.wait(10000), "Timed out waiting for a frame from the fake PipeWire camera");

    const QVideoFrame frame = sink.videoFrame();
    QVERIFY(frame.isValid());

    const QImage actual = frame.toImage();
    const QImage expected = renderSyntheticVideoScene(m_scene);
    QCOMPARE_EQ(actual.size(), expected.size());

    // A spot check inside the pattern's inset, rather than a full-image
    // comparison: format conversion on the way through QVideoFrame is not
    // guaranteed to be lossless at every edge pixel.
    const QPoint insidePattern = m_scene.insetRect().center();
    QCOMPARE(actual.pixelColor(insidePattern), expected.pixelColor(insidePattern));
}

// QTEST_MAIN's main() is renamed so that the fake portal can get in first. Both
// steps have to happen before QApplication exists: the helper process must not
// build one at all, and the re-exec has to precede the first use of the session
// bus, which QDBusConnection caches for the lifetime of the process.
#define main testlib_main
QTEST_MAIN(tst_QPipeWireCamera)
#undef main

int main(int argc, char *argv[])
{
    if (QFakeXdgPortalFixture::isHelperProcess(argc, argv))
        return QFakeXdgPortalFixture::runHelperProcess(argc, argv);

    qputenv("QT_CAMERA_BACKEND", "pipewire");
    qputenv("QT_PIPEWIRE_CAMERA_FORCE_PORTAL", "1");

    QFakeXdgPortalFixture::reexecUnderPrivateBusIfRequested(argc, argv,
                                                            QtFakeXdgPortal::Role::Camera);
    return testlib_main(argc, argv);
}

#include "tst_qpipewirecamera.moc"
