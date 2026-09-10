// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// TESTED_COMPONENT=src/multimedia

#include "fixture.h"
#include "widget.h"

#include <QtCore/qcommandlineparser.h>
#include <QtCore/qoperatingsystemversion.h>

#include <QtGui/qwindow.h>

#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qwindowcapture.h>
#if defined(Q_OS_MACOS)
#include <QtMultimedia/private/qavfhelpers_p.h>
#endif
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/private/qwindowcapture_p.h>
#include <QtMultimediaTestLib/private/mediabackendutils_p.h>
#include <QtMultimediaTestLib/private/osdetection_p.h>
#include <QtMultimediaTestLib/private/qintegrationtestbase_p.h>

#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

#include <chrono>
#include <cstdlib>
#include <vector>

namespace ranges = QtMultimediaPrivate::ranges;

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;

using namespace std::chrono_literals;

class tst_QWindowCaptureBackend : public QIntegrationTestBase
{
    Q_OBJECT

private:
    bool m_skipOddSizedWindows = false;
    [[nodiscard]] bool skipOddSizedWindows() const { return m_skipOddSizedWindows; }

    // Keeps capturing frames until we receive the size we expect.
    // If it fails, it emits a QFAIL with a helpful message.
    void waitForVideoFrameExpectedSize(
        WindowCaptureFixture &fixture,
        QSize expectedSize,
        const QString &context)
    {
        bool matched = QTest::qWaitFor(
            [&] {
                const std::vector<QVideoFrame> &frames = fixture.m_grabber.getFrames();
                return !frames.empty() && frames.back().size() == expectedSize;
            },
            globalTestTimeout());

        const std::vector<QVideoFrame> &frames = fixture.m_grabber.getFrames();
        QSize lastFrameSize = frames.empty() ? QSize{} : frames.back().size();

        QVERIFY2(
            matched,
            qPrintable(u"%1: expected captured frame %2x%3, but after %4 frame(s) "
                       "the last one was %5x%6"_s
                .arg(context)
                .arg(expectedSize.width())
                .arg(expectedSize.height())
                .arg(int(frames.size()))
                .arg(lastFrameSize.width())
                .arg(lastFrameSize.height())));
    }

private slots:
    void initTestCase()
    {
        if (!initIntegrationTestCase())
            return;

#ifdef Q_OS_ANDROID
     QSKIP("Feature does not work on Android");
#endif
#if defined(Q_OS_LINUX)
     if (isCI() && qEnvironmentVariable("XDG_SESSION_TYPE").toLower() != "x11")
         QSKIP("Skip on wayland; to be fixed");
#elif defined(Q_OS_MACOS)
        if (isCI()) {
            // Window capturing requires screen capture permissions on macOS. Without them,
            // none of the tests can succeed, so fail here to abort the entire test run.
            QVERIFY2(
                QAVFHelpers::checkMacOsScreenCapturePermissions(),
                "Missing screen capture permissions. Tests are not expected to succeed.");
        } else if (!QAVFHelpers::checkMacOsScreenCapturePermissions()) {
            QAVFHelpers::requestMacOsScreenCapturePermissions();
            QFAIL("Missing screen capture permissions. Grant permissions and restart test.");
        }

    // macOS CI machines have some issues with giving hardware frames of
    // incorrect size on odd-sized windows, so skip those sizes there.
    m_skipOddSizedWindows = isCI();
#endif

    // On some platforms, captured frame size lags slightly behind window geometry
    // changes, so allow a short settle time after modifying geometry before
    // starting capture or checking frame sizes.
    //
    // If frame size tests are flaky, it might help to increase this value.
    if (isMacOS || isLinux)
        WindowCaptureFixture::windowGeometrySettleDelay = 200ms;

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

    void capturableWindows_containsVisibleWindow()
    {
#if defined(Q_OS_MACOS)
        // Note: Remove this skip if this test is promoted to bundle.
        QSKIP("On macOS, windows without a bundle identifier are filtered out of "
              "capturableWindows(), so our test window never appears in the list");
#endif

        TestWidget widget;
        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget, globalTestTimeout()));

        // Our visible window should be discoverable in the list of capturable windows.
        QList<QCapturableWindow> windows = QWindowCapture::capturableWindows();
        auto found = ranges::find_if(
            windows,
            [&widget](const QCapturableWindow &window) {
                return window.description() == widget.windowTitle();
            });

        QVERIFY2(found != windows.end(), "Visible window not found among capturable windows");
        QVERIFY(found->isValid());
    }

    void capturableWindow_constructedFromWindow_canBeCaptured()
    {
        QVERIFY(!QCapturableWindow{}.isValid());
        QVERIFY(!QCapturableWindow{ static_cast<QWindow *>(nullptr) }.isValid());

        TestWidget widget;
        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget, globalTestTimeout()));

        QWindow *const window = widget.windowHandle();
        QVERIFY(window);

        // Constructing a QCapturableWindow directly from a live QWindow yields a
        // valid handle describing that window.
        QCapturableWindow capturable{ window };
        QVERIFY(capturable.isValid());
        QCOMPARE(capturable.description(), widget.windowTitle());

        // The handle constructed from the QWindow can actually be captured.
        WindowCaptureFixture fixture;
        fixture.m_capture.setWindow(capturable);
        fixture.m_capture.setActive(true);

        QVERIFY(fixture.waitForFrame().isValid());
        QVERIFY(fixture.m_capture.isActive());
        QVERIFY(fixture.m_errors.empty());
    }

    void setActive_failsAndEmitsErrors_whenNoWindowSelected()
    {
        WindowCaptureFixture fixture;

        QWindowCapture &windowCapture = fixture.m_capture;

        QSignalSpy errorChangedSpy { &windowCapture, &QWindowCapture::errorChanged };

        QCOMPARE(windowCapture.error(), QWindowCapture::Error::NoError);

        windowCapture.setActive(true);

        QVERIFY(!windowCapture.isActive());

        // Make sure we received exactly one signal, and the error code is NotFound.
        QCOMPARE(fixture.m_errors.count(), 1);
        QCOMPARE(errorChangedSpy.count(), 1);

        // TODO: Verify last error emitted is a specific error code.
        /*
        auto lastErrorEmitted = QWindowCapture::Error(fixture.m_errors[0][0].toInt());
        QCOMPARE(lastErrorEmitted, QWindowCapture::Error::NotFound);

        QCOMPARE(windowCapture.error(), QWindowCapture::Error::NotFound);
        */
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
        QVERIFY(fixture.m_activations.at(0).at(0).toBool());
        QVERIFY(fixture.m_errors.empty());
    }

    void setActive_stopsWindowCapture_whenCalledWithFalse()
    {
        WindowCaptureWithWidgetFixture fixture;
        QVERIFY(fixture.start());

        QWindowCapture &windowCapture = fixture.m_capture;

        QVERIFY(windowCapture.isActive());
        QCOMPARE(fixture.m_activations.size(), 1);

        // Ensure capture is actually running before stopping it
        QVERIFY(fixture.waitForFrame().isValid());

        windowCapture.setActive(false);

        // activeChanged has now fired twice: once for start, once for stop
        QCOMPARE(fixture.m_activations.size(), 2);
        QVERIFY(!fixture.m_activations.at(1).at(0).toBool());
        QVERIFY(!windowCapture.isActive());
        QVERIFY(fixture.m_errors.empty());
    }

    void setActive_restartsWindowCapture_whenStartedAgainAfterStop()
    {
        constexpr int restartCount = 3;

        WindowCaptureWithWidgetFixture fixture;
        QVERIFY(fixture.start());

        QWindowCapture &windowCapture = fixture.m_capture;

        // Ensure capture is actually running before stopping it
        QVERIFY(fixture.waitForFrame().isValid());

        for (int i = 0; i < restartCount; ++i) {
            windowCapture.setActive(false);
            QVERIFY(!fixture.m_capture.isActive());

            windowCapture.setActive(true);

            QVERIFY(fixture.waitForFrame().isValid());
            QVERIFY(windowCapture.isActive());
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

        QWindowCapture &windowCapture = fixture.m_capture;

        QSignalSpy windowChanges{ &windowCapture, &QWindowCapture::windowChanged };

        // Create a second, differently-sized window to switch to
        TestWidget secondWidget;
        secondWidget.setSize({ 120, 80 });
        secondWidget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&secondWidget, globalTestTimeout()));

        std::optional<QCapturableWindow> secondWindow =
            WindowCaptureWithWidgetFixture::findCaptureWindow(
                secondWidget.windowTitle(),
                secondWidget.windowHandle());
        QVERIFY(secondWindow && secondWindow->isValid());

        // Switch the captured window while capture is active
        windowCapture.setWindow(*secondWindow);

        QCOMPARE(windowChanges.size(), 1);
        QCOMPARE(windowCapture.window(), *secondWindow);

        // Switching source keeps the capture active
        QVERIFY(windowCapture.isActive());
        // activeChanged does not fire in between switching source
        QCOMPARE(fixture.m_activations.size(), 1);

        // Make sure we get frames from the new larger window. The captured frame
        // is delivered in physical pixels, so scale by the device pixel ratio.
        const QSize expectedSize =
            (QSizeF(secondWidget.size()) * secondWidget.devicePixelRatio()).toSize();
        QTRY_VERIFY_WITH_TIMEOUT(
            !fixture.m_grabber.getFrames().empty()
            && fixture.m_grabber.getFrames().back().size() == expectedSize,
            globalTestTimeout());

        QVERIFY(fixture.m_errors.empty());
    }

    void setWindow_stopsCapture_whenSwitchedToInvalidWindow()
    {
        WindowCaptureWithWidgetFixture fixture;
        QVERIFY(fixture.start());

        QWindowCapture &windowCapture = fixture.m_capture;

        // Ensure capture is actually running before switching window
        QVERIFY(fixture.waitForFrame().isValid());
        QCOMPARE(fixture.m_activations.size(), 1);
        QVERIFY(windowCapture.isActive());

        // Switching to a default-constructed (invalid) window cannot be captured,
        // so the capture stops and becomes inactive.
        windowCapture.setWindow(QCapturableWindow{});

        QVERIFY(!windowCapture.isActive());

        // activeChanged fired again for the deactivation
        QCOMPARE(fixture.m_activations.size(), 2);
        QCOMPARE(windowCapture.window(), QCapturableWindow{});
        // We should emit an error when invalid window is assigned to
        // active QWindowCapture.
        QCOMPARE(fixture.m_errors.size(), 1);

        // TODO: Check that last error state is a specific error code.
        // QCOMPARE(windowCapture.error(), QWindowCapture::Error::NotFound);
    }

    void setWindow_updatesPropertyAndEmitsSignal_whenNotActive()
    {
        WindowCaptureWithWidgetFixture fixture;

        fixture.m_widget.setSize({ 60, 40 });
        fixture.m_widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(
            &fixture.m_widget,
            globalTestTimeout()));

        const std::optional<QCapturableWindow> window =
            WindowCaptureWithWidgetFixture::findCaptureWindow(
                fixture.m_widget.windowTitle(),
                fixture.m_widget.windowHandle());
        QVERIFY(window && window->isValid());

        QWindowCapture &windowCapture = fixture.m_capture;
        QSignalSpy windowChanges{ &windowCapture, &QWindowCapture::windowChanged };

        windowCapture.setWindow(*window);

        QCOMPARE(windowChanges.size(), 1);
        QCOMPARE(windowCapture.window(), *window);

        // Selecting a window without activating must not start capturing or raise errors.
        QVERIFY(!windowCapture.isActive());
        QVERIFY(fixture.m_activations.empty());
        QVERIFY(fixture.m_errors.empty());

        // Setting the same window again is a no-op and must not emit windowChanged.
        windowCapture.setWindow(*window);
        QCOMPARE(windowChanges.size(), 1);
    }

    void setFrameRate_updatesPropertyAndEmitsSignal()
    {
        WindowCaptureFixture fixture;

        auto frameRateEquals = [](std::optional<qreal> frameRate, float value) {
            return frameRate && qFuzzyCompare(*frameRate, static_cast<qreal>(value));
        };

        // No preferred frame rate initially
        QVERIFY(!fixture.m_capture.maximumFrameRate());

        // Setting a frame rate updates the property and emits maximumFrameRateChanged
        const float newFrameRate = 1.f;
        fixture.m_capture.setMaximumFrameRate(newFrameRate);

        QCOMPARE(fixture.m_frameRates.size(), 1);
        QVERIFY(frameRateEquals(fixture.m_capture.maximumFrameRate(), newFrameRate));

        fixture.m_capture.setMaximumFrameRate(std::nullopt);

        QCOMPARE(fixture.m_frameRates.size(), 2);
        QVERIFY(!fixture.m_capture.maximumFrameRate());

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
        fixture.m_capture.setMaximumFrameRate(newFrameRate);

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

    void capturedFrame_hasExpectedSize_data()
    {
        QTest::addColumn<QSize>("windowSize");
        QTest::newRow("small-window") << QSize{ 60, 40 };
        QTest::newRow("big-window") << QSize{ 800, 600 };
        if (!skipOddSizedWindows()) {
            QTest::newRow("odd-width-window") << QSize{ 61, 40 };
            QTest::newRow("odd-height-window") << QSize{ 60, 41 };
        }
    }

    void capturedFrame_hasExpectedSize()
    {
        QFETCH(QSize, windowSize);

        WindowCaptureWithWidgetFixture fixture;

        QVERIFY(fixture.start(windowSize, FirstFrameSizePolicy::MustMatchWindowSize));
        QVERIFY(fixture.m_capture.isActive());

        const QSize expectedSize =
            (QSizeF(fixture.m_widget.size()) * fixture.m_widget.devicePixelRatio()).toSize();

        QVERIFY(QTest::qWaitFor(
            [&] { return !fixture.m_grabber.getFrames().empty(); }, globalTestTimeout()));

        const std::vector<QVideoFrame> frames = fixture.m_grabber.getFrames();
        for (size_t i = 0; i < frames.size(); ++i) {
            QVERIFY2(
                frames[i].size() == expectedSize,
                qPrintable(u"Captured frame #%1 was %2x%3, but expected %4x%5"_s
                    .arg(i)
                    .arg(frames[i].size().width())
                    .arg(frames[i].size().height())
                    .arg(expectedSize.width())
                    .arg(expectedSize.height())));
        }

        // The video sink should report the same size as the frames it receives.
        QVERIFY2(
            fixture.m_grabber.videoSize() == expectedSize,
            qPrintable(u"Video sink size %1x%2 differs from expected %3x%4"_s
                .arg(fixture.m_grabber.videoSize().width())
                .arg(fixture.m_grabber.videoSize().height())
                .arg(expectedSize.width())
                .arg(expectedSize.height())));

        QVERIFY(fixture.m_errors.empty());
    }

    // For some backends it's possible to unintentionally store size-snapshots
    // in QCapturableWindow (i.e ScreenCaptureKit.SCWindow). This test
    // checks that our stream starts at the current window size, not
    // when the QCapturableWindow handle was constructed.
    void capture_startsAtCurrentSize_whenWindowResizedBeforeStart()
    {
        WindowCaptureWithWidgetFixture fixture;

        const QSize initialSize{ 200, 150 };
        const QSize resizedSize{ 400, 300 };

        fixture.m_widget.setSize(initialSize);
        fixture.m_widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&fixture.m_widget, globalTestTimeout()));

        // Create the capturable-window handle while the window is still at the
        // initial size.
        const QCapturableWindow handle{ fixture.m_widget.windowHandle() };
        QVERIFY(handle.isValid());

        // Resize, and wait until the new size has actually taken effect before we
        // start capturing, so the new size is the window's live size.
        fixture.m_widget.setSize(resizedSize);
        QVERIFY(QTest::qWaitFor(
            [&] { return fixture.m_widget.size() == resizedSize; },
            globalTestTimeout()));
        // This test inspects the very first captured frame's size, so let the
        // capture backend observe the new geometry before we start the stream
        fixture.waitForWindowGeometryToSettle(FirstFrameSizePolicy::MustMatchWindowSize);

        // Start capturing using the handle that was created at the old size.
        fixture.m_capture.setWindow(handle);
        fixture.m_capture.setActive(true);
        QVERIFY(fixture.m_capture.isActive());

        // Every captured frame, including the very first, must reflect the
        // new size of the window.
        const QSize expectedSize =
            (QSizeF(resizedSize) * fixture.m_widget.devicePixelRatio()).toSize();

        QVERIFY(QTest::qWaitFor(
            [&] { return !fixture.m_grabber.getFrames().empty(); }, globalTestTimeout()));

        const std::vector<QVideoFrame> frames = fixture.m_grabber.getFrames();
        for (size_t i = 0; i < frames.size(); ++i) {
            QVERIFY2(
                frames[i].size() == expectedSize,
                qPrintable(u"Captured frame #%1 was %2x%3, but expected %4x%5"_s
                    .arg(i)
                    .arg(frames[i].size().width())
                    .arg(frames[i].size().height())
                    .arg(expectedSize.width())
                    .arg(expectedSize.height())));
        }

        QVERIFY(fixture.m_errors.empty());
    }

    void capturedFrame_hasNewSize_whenWindowResizedOnce()
    {
        // The captured frame is delivered in physical pixels, so scale the
        // window's logical size by the device pixel ratio to get the expected
        // frame size.
        const auto expectedFrameSize = [](const TestWidget &widget) {
            return (QSizeF(widget.size()) * widget.devicePixelRatio()).toSize();
        };

        WindowCaptureWithWidgetFixture fixture;
        // Use animated content to make sure backend does not
        // consider the content idle.
        fixture.m_widget.setDisplayPattern(TestWidget::Pattern::Animated);
        QVERIFY(fixture.start({ 200, 150 }));
        QVERIFY(fixture.m_capture.isActive());

        // Make sure we are receiving frames at the initial window size before
        // we resize.
        waitForVideoFrameExpectedSize(
            fixture,
            expectedFrameSize(fixture.m_widget), u"Initial capture at 200x150"_s);
        if (QTest::currentTestFailed())
            return;

        fixture.resizeWidget({ 400, 300 });

        // At some point we have to receive frames with the new size.
        waitForVideoFrameExpectedSize(
            fixture,
            expectedFrameSize(fixture.m_widget), u"Resized window to 400x300"_s);

        QVERIFY(fixture.m_errors.empty());
    }

    void capturedFrame_hasNewSize_whenWindowResized_data()
    {
        using QOSVersion = QOperatingSystemVersion;
        if (isCI()
            && QOSVersion::currentType() == QOSVersion::OSType::MacOS
            && QOSVersion::current() < QOSVersion(QOSVersion::OSType::MacOS, 26))
        {
            QSKIP("macOS 15 CI machines and older have issues with reporting content size during window resizing");
        }

        QTest::addColumn<int>("increment");
        QTest::newRow("shrink by 1") << -1;
        QTest::newRow("shrink by 2") << -2;
        QTest::newRow("grow by 1") << 1;
        QTest::newRow("grow by 2") << 2;
    }

    void capturedFrame_hasNewSize_whenWindowResized()
    {
        QFETCH(int, increment);

        constexpr int incrementalSteps = 10;

        // The captured frame is delivered in physical pixels, so scale the
        // window's logical size by the device pixel ratio to get the expected
        // frame size.
        const auto expectedFrameSizeFn = [](const TestWidget &widget) {
            return (QSizeF(widget.size()) * widget.devicePixelRatio()).toSize();
        };

        WindowCaptureWithWidgetFixture fixture;

        QSize windowSize = { 200, 150 };
        QVERIFY(fixture.start(windowSize));
        QVERIFY(fixture.m_capture.isActive());

        // Make sure we are receiving frames at the initial window size before
        // we start resizing.
        waitForVideoFrameExpectedSize(
            fixture,
            expectedFrameSizeFn(fixture.m_widget),
            u"Initial capture at 200x150"_s);
        if (QTest::currentTestFailed())
            return;

        for (int step = 0; step < incrementalSteps; ++step) {
            windowSize += QSize{ increment, increment };
            fixture.resizeWidget(windowSize);

            // At some point, we must receive a frame that has the
            // expected size.
            waitForVideoFrameExpectedSize(
                fixture,
                expectedFrameSizeFn(fixture.m_widget),
                u"Resized window to %1x%2 (step %3)"_s
                    .arg(windowSize.width())
                    .arg(windowSize.height())
                    .arg(step));
            if (QTest::currentTestFailed())
                return;
        }

        QVERIFY(fixture.m_errors.empty());
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
            globalTestTimeout());
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
        QVERIFY(fixture.waitForFrame().isValid());

        // Closing the process waits for it to exit
        fixture.m_windowProcess.close();

        QTRY_VERIFY_WITH_TIMEOUT(
            !fixture.m_errors.empty(),
            globalTestTimeout());

        // TODO: Verify that the QWindowCapture goes inactive whenever we encounter an error
        // like this.
        // QVERIFY(!windowCapture.isActive());

        // TODO: Enforce specific error code when a window is lost and capture stops.
        // Need to investigate if this is a case we can specifically detect on all platforms.
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
