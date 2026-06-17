// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// TESTED_COMPONENT=src/multimedia

#include "fixture.h"
#include "widget.h"

#include <QtCore/qcommandlineparser.h>

#include <QtGui/qpainter.h>

#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qwindowcapture.h>
#if defined(Q_OS_MACOS)
#include <QtMultimedia/private/qavfhelpers_p.h>
#endif
#include <QtMultimediaTestLib/private/mediabackendutils_p.h>

#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

#include <chrono>
#include <vector>

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;

class tst_QWindowCaptureBackend : public QObject
{
    Q_OBJECT

    bool m_skipOddSizedWindows = false;
    [[nodiscard]] bool skipOddSizedWindows() const { return m_skipOddSizedWindows; }

private slots:
    void initTestCase()
    {
#ifdef Q_OS_ANDROID
     QSKIP("Feature does not work on Android");
#endif
#if defined(Q_OS_LINUX)
     if (isCI() && qEnvironmentVariable("XDG_SESSION_TYPE").toLower() != "x11")
         QSKIP("Skip on wayland; to be fixed");
#elif defined(Q_OS_MACOS)
        // Window capturing requires screen capture permissions on macOS. Without them,
        // none of the tests can succeed, so fail here to abort the entire test run.
        QVERIFY2(
            QAVFHelpers::checkMacOsScreenCapturePermissions(),
            "Missing screen capture permissions. Tests are not expected to succeed.");

    // macOS CI machines have some issues with giving hardware frames of
    // incorrect size on odd-sized windows, so skip those sizes there.
    m_skipOddSizedWindows = isCI();
#endif

        const QWindowCapture capture;
        if (capture.error() == QWindowCapture::CapturingNotSupported)
            QSKIP("Screen capturing not supported");

#if defined(__SANITIZE_ADDRESS__) || __has_feature(address_sanitizer)
        QSKIP("QTBUG-135614, disabling tst_QWindowCaptureBackend for Address Sanitizer builds, due "
              "to flakiness");
#endif
    }

    void isActive_returnsFalse_whenNotStarted()
    {
        const WindowCaptureFixture fixture;
        QVERIFY(!fixture.m_capture.isActive());
    }

    void window_isInvalid_whenNotSet()
    {
        WindowCaptureFixture fixture;
        QVERIFY(!fixture.m_capture.window().isValid());
        QCOMPARE(fixture.m_capture.window(), QCapturableWindow{});
    }

    void error_isNoError_whenNotStarted()
    {
        WindowCaptureFixture fixture;
        QCOMPARE(fixture.m_capture.error(), QWindowCapture::Error::NoError);
    }

    void windowCapture_returnsSession_whenAddedToSession()
    {
        WindowCaptureFixture fixture;
        // The fixture adds the capture to its session on construction.
        QCOMPARE(fixture.m_capture.captureSession(), &fixture.m_session);
    }

    void capturableWindows_returnsOnlyValidWindows()
    {
        const QList<QCapturableWindow> windows = QWindowCapture::capturableWindows();

        for (const QCapturableWindow &window : windows)
            QVERIFY(window.isValid());
    }

    void setActive_failsAndEmitEerrorOccurred_whenNoWindowSelected()
    {
        WindowCaptureFixture fixture;

        fixture.m_capture.setActive(true);

        QVERIFY(!fixture.m_capture.isActive());
        QVERIFY(!fixture.m_errors.empty());
    }

    void setActive_isNoOp_whenStoppingCaptureThatNeverStarted()
    {
        WindowCaptureFixture fixture;

        QWindowCapture &windowCapture = fixture.m_capture;
        QVERIFY(!windowCapture.isActive());

        // Stopping a capture that was never started should not change state,
        // emit activeChanged, or raise an error.
        windowCapture.setActive(false);

        QVERIFY(!windowCapture.isActive());
        QVERIFY(fixture.m_activations.empty());
        QVERIFY(fixture.m_errors.empty());
    }

    void setActive_isNoOp_whenAlreadyActive()
    {
        WindowCaptureWithWidgetFixture fixture;
        QVERIFY(fixture.start());
        QVERIFY(fixture.waitForFrame().isValid());

        QWindowCapture &windowCapture = fixture.m_capture;
        QVERIFY(windowCapture.isActive());
        QCOMPARE(fixture.m_activations.size(), 1);

        // Activating an already-active capture should not emit activeChanged again.
        windowCapture.setActive(true);

        QVERIFY(windowCapture.isActive());
        QCOMPARE(fixture.m_activations.size(), 1);
        QVERIFY(fixture.m_errors.empty());
    }

    void setActive_startsWindowCapture_whenCalledWithTrue()
    {
        WindowCaptureWithWidgetFixture fixture;
        QVERIFY(fixture.start());

        // Ensure that we have received a frame
        QVERIFY(fixture.waitForFrame().isValid());

        QCOMPARE(fixture.m_activations.size(), 1);
        QVERIFY(fixture.m_errors.empty());
    }

    void setActive_stopsWindowCapture_whenCalledWithFalse()
    {
        WindowCaptureWithWidgetFixture fixture;
        QVERIFY(fixture.start());

        // Ensure capture is actually running before stopping it
        QVERIFY(fixture.waitForFrame().isValid());
        QCOMPARE(fixture.m_activations.size(), 1);
        QVERIFY(fixture.m_capture.isActive());

        fixture.m_capture.setActive(false);

        // activeChanged has now fired twice: once for start, once for stop
        QCOMPARE(fixture.m_activations.size(), 2);
        QVERIFY(!fixture.m_capture.isActive());
        QVERIFY(fixture.m_errors.empty());
    }

    void setActive_restartsWindowCapture_whenStartedAgainAfterStop()
    {
        constexpr int restartCount = 3;

        WindowCaptureWithWidgetFixture fixture;
        QVERIFY(fixture.start());

        // Ensure capture is actually running before stopping it
        QVERIFY(fixture.waitForFrame().isValid());

        for (int i = 0; i < restartCount; ++i) {
            fixture.m_capture.setActive(false);
            QVERIFY(!fixture.m_capture.isActive());

            fixture.m_capture.setActive(true);

            QVERIFY(fixture.waitForFrame().isValid());
            QVERIFY(fixture.m_capture.isActive());
        }

        // activeChanged fired for the initial start plus a stop/start pair per restart
        QCOMPARE(fixture.m_activations.size(), 1 + 2 * restartCount);
        QVERIFY(fixture.m_errors.empty());
    }

    void setWindow_switchesSource_whileActive()
    {
        WindowCaptureWithWidgetFixture fixture;
        QVERIFY(fixture.start({ 60, 40 }));
        QVERIFY(fixture.waitForFrame().isValid());

        QSignalSpy windowChanges{ &fixture.m_capture, &QWindowCapture::windowChanged };

        // Create a second, differently-sized window to switch to
        TestWidget secondWidget;
        secondWidget.setSize({ 120, 80 });
        secondWidget.show();
        QVERIFY(QTest::qWaitForWindowExposed(
            &secondWidget,
            s_testTimeout));

        std::optional<QCapturableWindow> secondWindow =
            WindowCaptureWithWidgetFixture::findCaptureWindow(
                secondWidget.windowTitle(),
                secondWidget.windowHandle());
        QVERIFY(secondWindow && secondWindow->isValid());

        // Switch the captured window while capture is active
        fixture.m_capture.setWindow(*secondWindow);

        QTRY_COMPARE_WITH_TIMEOUT(windowChanges.size(), 1, s_testTimeout);
        QCOMPARE(fixture.m_capture.window(), *secondWindow);

        // Switching source keeps the capture active
        QVERIFY(fixture.m_capture.isActive());
        // activeChanged does not fire in between switching source
        QCOMPARE(fixture.m_activations.size(), 1);

        // Make sure we get frames from the new larger window
        QTRY_VERIFY_WITH_TIMEOUT(
            !fixture.m_grabber.getFrames().empty()
            && fixture.m_grabber.getFrames().back().size() == secondWidget.size(),
            s_testTimeout);

        QVERIFY(fixture.m_errors.empty());
    }

    void setWindow_stopsCapture_whenSwitchedToInvalidWindow()
    {
        WindowCaptureWithWidgetFixture fixture;
        QVERIFY(fixture.start());

        // Ensure capture is actually running before switching window
        QVERIFY(fixture.waitForFrame().isValid());
        QCOMPARE(fixture.m_activations.size(), 1);
        QVERIFY(fixture.m_capture.isActive());

        // Switching to a default-constructed (invalid) window cannot be captured,
        // so the capture stops and becomes inactive.
        fixture.m_capture.setWindow(QCapturableWindow{});

        QVERIFY(!fixture.m_capture.isActive());

        // activeChanged fired again for the deactivation
        QCOMPARE(fixture.m_activations.size(), 2);
        QCOMPARE(fixture.m_capture.window(), QCapturableWindow{});
        // We should emit an error when invalid window is assigned to
        // active QWindowCapture.
        QCOMPARE(fixture.m_errors.size(), 1);
    }

    void setFrameRate_updatesPropertyAndEmitsSignal()
    {
        WindowCaptureFixture fixture;

        auto frameRateEquals = [](std::optional<qreal> frameRate, float value) {
            return frameRate && qFuzzyCompare(*frameRate, static_cast<qreal>(value));
        };

        // No preferred frame rate initially
        QVERIFY(!fixture.m_capture.frameRate());

        // Setting a frame rate updates the property and emits frameRateChanged
        const float newFrameRate = 1.f;
        fixture.m_capture.setFrameRate(newFrameRate);

        QCOMPARE(fixture.m_frameRates.size(), 1);
        QVERIFY(frameRateEquals(fixture.m_capture.frameRate(), newFrameRate));

        // Resetting clears the property and emits frameRateChanged again
        fixture.m_capture.resetFrameRate();

        QCOMPARE(fixture.m_frameRates.size(), 2);
        QVERIFY(!fixture.m_capture.frameRate());

        QVERIFY(fixture.m_errors.empty());
    }

    void setFrameRate_emitsFramesAtCorrectRate()
    {
#ifdef Q_OS_ANDROID // QTBUG-141824
        QSKIP("Framerate setting not implemented on Android");
#endif
#ifdef Q_OS_LINUX
        if (QGuiApplication::platformName() == u"wayland"_s)
            QSKIP("Framerate setting not implemented on Wayland");
#endif

        WindowCaptureWithWidgetFixture fixture;

        // Use animated content to make sure backend does not
        // consider the content idle.
        fixture.m_widget.setDisplayPattern(TestWidget::Pattern::Animated);

        const float newFrameRate = 1.f;
        fixture.m_capture.setFrameRate(newFrameRate);

        QVERIFY(fixture.start());

        // Range [0, 1]. Lower is better, but may increase flakiness.
        float slopFactor = 0.1;
        if (isCI()) {
            slopFactor = 0.2;
        }

        // Check framerate is roughly 1fps
        using namespace std::chrono;
        auto durationBetweenFrames = fixture.m_grabber.durationBetweenFrames(3);
        QVERIFY2(
            durationBetweenFrames > 0ms,
            "Did not receive enough QVideoFrames to measure framerate");
        const qreal actualFps = 1000.0 / durationBetweenFrames.count();
        QCOMPARE_GT(actualFps, newFrameRate * (1 - slopFactor));
        QCOMPARE_LT(actualFps, newFrameRate * (1 + slopFactor));
    }

    void capturedImage_equals_imageFromGrab_data()
    {
        QTest::addColumn<QSize>("windowSize");
        QTest::newRow("small-window") << QSize{60, 40};
        QTest::newRow("big-window") << QApplication::primaryScreen()->size();

        if (!skipOddSizedWindows()) {
            QTest::newRow("single-pixel-window") << QSize{1, 1};
            QTest::newRow("odd-width-window") << QSize{ 61, 40 };
            QTest::newRow("odd-height-window") << QSize{ 60, 41 };
        }
    }

    void capturedImage_equals_imageFromGrab()
    {
        QFETCH(QSize, windowSize);

        WindowCaptureWithWidgetFixture fixture;
        QVERIFY(fixture.start(windowSize));

        const QImage expected = fixture.m_widget.grabImage();
        const QImage actual = fixture.waitForFrame().toImage();

        QVERIFY(fixture.compareImages(actual, expected));
    }

    void capturedImage_changes_whenWindowContentChanges()
    {
        WindowCaptureWithWidgetFixture fixture;
        QVERIFY(fixture.start());

        QVERIFY(fixture.waitForFrame().isValid());

        fixture.m_widget.setDisplayPattern(TestWidget::Pattern::Grid);

        const QImage expectedGridImage = fixture.m_widget.grabImage();

        // Compare every new frame we have received since we changed the
        // target window content. If any of the new frames match,
        // the test succeeds.
        size_t checkedFrames = 0;
        const auto anyNewFramesMatchesNewContent = [&] {
            const std::vector<QVideoFrame> &frames = fixture.m_grabber.getFrames();
            for (; checkedFrames < frames.size(); ++checkedFrames) {
                const QImage image = frames[checkedFrames].toImage();
                if (image.convertToFormat(expectedGridImage.format()) == expectedGridImage)
                    return true;
            }
            return false;
        };

        QTRY_VERIFY_WITH_TIMEOUT(
            anyNewFramesMatchesNewContent(),
            s_testTimeout);
    }

    void sequenceOfCapturedImages_compareEqual_whenWindowContentIsUnchanged()
    {
#ifdef Q_OS_WIN
        QSKIP(
            "Windows does not emit frames if content is unchanged. "
            "Cannot test framerates reliably in CI. QTBUG-147051");
#endif
#ifdef Q_OS_MACOS
        QSKIP(
            "The macOS ScreenCaptureKit backend will often not emit "
            "new frames if content is unchanged");
#endif

        WindowCaptureWithWidgetFixture fixture;
        QVERIFY(fixture.start());

        const std::vector<QVideoFrame> frames = fixture.m_grabber.waitAndTakeFrames(10);
        QVERIFY(!frames.empty());

        QImage firstFrame = frames.front().toImage();
        QVERIFY(!firstFrame.isNull());

        qsizetype index = 0;
        for (const auto &frame : std::as_const(frames)){
            QVERIFY(fixture.compareImages(frame.toImage(), firstFrame, QString::number(index)));
            ++index;
        }
    }

    void recorder_encodesFrames_toValidMediaFile_data()
    {
        QTest::addColumn<QSize>("windowSize");
        //QTest::newRow("empty-window") << QSize{ 0, 0 };           TODO: Crash
        //QTest::newRow("single-pixel-window") << QSize{ 1, 1 };    TODO: Crash
        QTest::newRow("small-window") << QSize{ 60, 40 };
        QTest::newRow("big-window") << QSize{ 800, 600 };

        if (!skipOddSizedWindows()) {
            QTest::newRow("odd-width-window") << QSize{ 61, 40 };
            QTest::newRow("odd-height-window") << QSize{ 60, 41 };
        }
    }

    void recorder_encodesFrames_toValidMediaFile()
    {
        QFETCH(QSize, windowSize);

        WindowCaptureWithWidgetAndRecorderFixture fixture;
        fixture.start(windowSize);

        // Wait on grabber to ensure that video recorder also get some frames
        fixture.m_grabber.waitAndTakeFrames(60);

        // Wait for recorder finalization
        fixture.stop();

        QVERIFY(fixture.m_recorderErrors.empty());
        QVERIFY(QFile{ fixture.m_mediaFile }.exists());
        QVERIFY(fixture.testVideoFilePlayback(fixture.m_mediaFile));
    }

    void recorder_encodesFrames_toValidMediaFile_whenWindowResizes_data()
    {
        QTest::addColumn<int>("increment");
        QTest::newRow("shrink") << -1;
        QTest::newRow("grow") << 1;
    }

    void recorder_encodesFrames_toValidMediaFile_whenWindowResizes()
    {
        QFETCH(int, increment);

        QSize windowSize = { 200, 150 };
        WindowCaptureWithWidgetAndRecorderFixture fixture;
        fixture.start(windowSize, /*toggle pattern*/ false);

        for (qsizetype i = 0; i < 20; ++i) {
            windowSize.setWidth(windowSize.width() + increment);
            windowSize.setHeight(windowSize.height() + increment);
            fixture.m_widget.setSize(windowSize);

            // Wait on grabber to ensure that video recorder also get some frames
            fixture.m_grabber.waitAndTakeFrames(1);
        }

        // Wait for recorder finalization
        fixture.stop();

        QVERIFY(fixture.m_recorderErrors.empty());
        QVERIFY(QFile{ fixture.m_mediaFile }.exists());
        QVERIFY(fixture.testVideoFilePlayback(fixture.m_mediaFile));
    }

    void windowCapture_capturesWindowsInOtherProcesses()
    {
#if defined(Q_OS_MACOS)
        QSKIP("Separate process tests do not work on macOS because they are launched without bundle identifiers");
#endif

        WindowCaptureWithWidgetInOtherProcessFixture fixture;
        QVERIFY(fixture.start());

        // Get reference image from our in-process widget
        const QImage expected = fixture.m_widget.grabImage();

        // Get actual image grabbed from out-of-process widget
        const QImage actual = fixture.waitForFrame().toImage();

        QVERIFY(fixture.compareImages(actual, expected));
    }

    /*
        This test is not a requirement per se, but we want all platforms
        to behave the same. A reasonable alternative could have been to
        treat closed window as a regular 'Stop' capture (not an error).
    */
    void windowCapture_stopsWithError_whenProcessCloses()
    {
#if defined(Q_OS_MACOS)
        QSKIP("Separate process tests do not work on macOS because they are launched without bundle identifiers");
#endif

        WindowCaptureWithWidgetInOtherProcessFixture fixture;
        QVERIFY(fixture.start());

        // Get capturing started
        fixture.m_grabber.waitAndTakeFrames(3);

        // Closing the process waits for it to exit
        fixture.m_windowProcess.close();

        const bool captureFailed =
                QTest::qWaitFor([&] { return !fixture.m_errors.empty(); }, s_testTimeout);

        QVERIFY(captureFailed);
    }
};

// QTEST_MAIN defines main, but we want to override it, so ensure that it emits
// `testlib_main` instead of `main`
#define main testlib_main
QTEST_MAIN(tst_QWindowCaptureBackend)
#undef main

int main(int argc, char *argv[])
{
    QCommandLineParser cmd;
    const QCommandLineOption showTestWidget{ QStringList{ "show" },
                                             "Creates a test widget with given title",
                                             "windowTitle" };
    cmd.addOption(showTestWidget);
    cmd.parse({ argv, argv + argc });

    if (cmd.isSet(showTestWidget)) {
        QApplication app{ argc, argv };
        const QString windowTitle = cmd.value(showTestWidget);
        const bool result = showCaptureWindow(windowTitle);
        return result ? 0 : 1;
    }

    return testlib_main(argc, argv);
}

#include "tst_qwindowcapturebackend.moc"
