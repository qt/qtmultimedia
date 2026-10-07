// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimediaTestLib/private/mediabackendutils_p.h>
#include <QtMultimediaTestLib/private/qintegrationtestbase_p.h>
#include <QtMultimediaTestLib/private/qsyntheticvideoscene_p.h>
#include <QtMultimediaTestLib/private/surfacecapturetestutils_p.h>
#include <QtMultimediaTestLib/private/testvideosink_p.h>
#ifdef QT_MM_HAVE_FAKE_XDG_PORTAL
#  include <QtMultimediaTestLib/private/qfakexdgportalfixture_p.h>
#endif
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qscreencapture.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideosink.h>
#include <QtGui/qpainter.h>
#include <QtGui/qwindow.h>
#if defined(Q_OS_MACOS)
#include <QtMultimedia/private/qavfhelpers_p.h>
#endif

#include <chrono>
#include <utility>
#include <vector>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
Q_DECLARE_JNI_CLASS(Window, "android/view/Window")
Q_DECLARE_JNI_CLASS(View, "android/view/View")
Q_DECLARE_JNI_CLASS(WindowInsets, "android/view/WindowInsets")
Q_DECLARE_JNI_CLASS(WindowInsetsType, "android/view/WindowInsets$Type")
Q_DECLARE_JNI_CLASS(Insets, "android/graphics/Insets")
#endif

using namespace Qt::StringLiterals;

using namespace std::chrono_literals;

/*
 This is the backend conformance test.

 Since it relies on platform media framework it may be less stable.
 Note, some of screen capture backend is not implemented or has bugs.
 That's why some of the tests could get failed.
 TODO: fix and platform implementations and make it stable.
*/

class QTestWidget : public QWidget
{
public:
    QTestWidget(QColor firstColor, QColor secondColor)
        : m_firstColor(firstColor), m_secondColor(secondColor)
    {
    }

    static std::unique_ptr<QTestWidget> createAndShow(Qt::WindowFlags flags, const QRect &geometry,
                                                      QScreen *screen = nullptr,
                                                      QColor firstColor = QColor(0xFF, 0, 0),
                                                      QColor secondColor = QColor(0, 0, 0xFF),
                                                      bool show = true)
    {
        auto widget = std::make_unique<QTestWidget>(firstColor, secondColor);

        widget->setWindowTitle("Test QScreenCapture");
        widget->setScreen(screen ? screen : QApplication::primaryScreen());
        widget->setWindowFlags(flags);
        widget->setGeometry(geometry);
        // Remembered for sceneOnScreen(): on Wayland a client cannot know its own
        // position on the screen, so geometry() cannot be trusted afterwards. The
        // assertions in capture() assume the requested position anyway.
        widget->m_requestedGeometry = geometry;
#ifdef Q_OS_ANDROID
    // Android is not a Window System. When calling setGeometry() on the main widget, it will
    // be displayed at the beginning of the screen. The x,y coordinates are ignored and lost.
    // To make the test consistent on Android, let's remember the geometry and use
    // it later in the paintEvent
        widget->m_paintPosition = geometry;
#endif
        if (show)
            widget->show();

        return widget;
    }

#ifdef Q_OS_ANDROID
    QRect m_paintPosition;
    bool m_isBlinkingRectWhite = false;
#endif

    void setColors(QColor firstColor, QColor secondColor)
    {
        m_firstColor = firstColor;
        m_secondColor = secondColor;
        this->repaint();
    }

    void setAnimated(bool animated)
    {
        m_animated = animated;

        this->repaint();
    }

protected:
    void paintEvent(QPaintEvent * /*event*/) override
    {
        QPainter p(this);
        p.setPen(Qt::NoPen);
        auto rect = this->rect();

#ifdef Q_OS_ANDROID
        // Add a blinking rectangle in the corner to force the Screen Grabber to work
        m_isBlinkingRectWhite = !m_isBlinkingRectWhite;
        p.setBrush(m_isBlinkingRectWhite ? Qt::white : Qt::black);
        p.drawRect(0, 0, 10, 10);
        // use remembered position
        rect = m_paintPosition;
#endif

        // Painted through the shared routine, so that the synthetic PipeWire
        // source used by QFakeXdgPortalFixture provably renders the very same
        // thing this widget does.
        paintSyntheticVideoScene(p, sceneForRect(rect), m_animationTick);

        if (m_animated) {
            ++m_animationTick;
            if (QWindow *window = windowHandle())
                window->requestUpdate();
        }
    }

public:
    /*!
        The scene describing this widget's own content, with \a patternRect
        expressed in whatever coordinate space the caller cares about: the
        widget's local rect when painting, or its position on a virtual screen
        when handing the scene to QFakeXdgPortalFixture.
    */
    SyntheticVideoScene sceneForRect(const QRect &patternRect, QSize frameSize = {}) const
    {
        SyntheticVideoScene scene;
        scene.frameSize = frameSize.isEmpty() ? patternRect.size() : frameSize;
        scene.patternRect = patternRect;
        scene.backgroundColor = Qt::black;
        scene.firstColor = m_firstColor;
        scene.secondColor = m_secondColor;
        scene.animated = m_animated;
        return scene;
    }

    //! The scene as it appears on \a screen, in device pixels.
    SyntheticVideoScene sceneOnScreen(const QScreen &screen) const
    {
        const qreal ratio = devicePixelRatio();
        const QRect geometry =
                m_requestedGeometry.isValid() ? m_requestedGeometry : this->geometry();
        const QRect patternRect(geometry.topLeft() * ratio, geometry.size() * ratio);
        SyntheticVideoScene scene = sceneForRect(patternRect, screen.size() * ratio);
        // The scene is rendered directly into device pixels here, unlike in
        // paintEvent() where QPainter scales for us.
        scene.scale = ratio;
        return scene;
    }

private:
    QRect m_requestedGeometry;
    QColor m_firstColor;
    QColor m_secondColor;
    bool m_animated = false;
    unsigned m_animationTick = 0;
};

class tst_QScreenCaptureBackend : public QIntegrationTestBase
{
    Q_OBJECT

private:
    void removeWhileCapture(std::function<void(QScreenCapture &)> scModifier,
                            std::function<void()> deleter);

    void capture(QTestWidget &widget, const QPoint &drawingOffset, const QSize &expectedSize,
                 std::function<void(QScreenCapture &)> scModifier);

    // Publishes what \a widget shows to the synthetic PipeWire source, so that a
    // capture of the fake "screen" contains exactly the widget's own content.
    void publishScene(const QTestWidget &widget);
    [[nodiscard]] bool usingFakePortal() const;

    // With the fake portal active, captured content comes entirely from the
    // published SyntheticVideoScene rather than from what is actually on screen,
    // so the widget never needs to become a real, visible window.
    [[nodiscard]] std::unique_ptr<QTestWidget>
    createWidget(Qt::WindowFlags flags, const QRect &geometry, QScreen *screen = nullptr,
                 QColor firstColor = QColor(0xFF, 0, 0), QColor secondColor = QColor(0, 0, 0xFF))
    {
        return QTestWidget::createAndShow(flags, geometry, screen, firstColor, secondColor,
                                          !usingFakePortal());
    }

#ifdef QT_MM_HAVE_FAKE_XDG_PORTAL
    QFakeXdgPortalFixture m_fakePortal;
#endif

private slots:
    void initTestCase();
    void cleanupTestCase();

    void isActive_returnsFalse_whenNotStarted();
    void screen_isNull_whenNotSet();
    void error_isNoError_whenNotStarted();
    void screenCapture_returnsSession_whenAddedToSession();

    void setActive_startsStreamWithValidFrame_data();
    void setActive_startsStreamWithValidFrame();
    void capturedFrame_hasExpectedSize_data();
    void capturedFrame_hasExpectedSize();
    void setActive_startsAndStopsCapture();
    void setActive_isNoOp_whenStoppingCaptureThatNeverStarted();
    void setActive_isNoOp_whenAlreadyActive();
    void setActive_restartsScreenCapture_whenStartedAgainAfterStop_data();
    void setActive_restartsScreenCapture_whenStartedAgainAfterStop();
    void setFrameRate_updatesPropertyAndEmitsSignal();
    void setFrameRate_emitsFramesAtCorrectRate();
    void portalSessionClosed_doesNotWarnAboutGrabbingThreadTimer();

    void setScreen_selectsScreen_whenCalledWithWidgetsScreen();
    void constructor_selectsPrimaryScreenAsDefault();
    void setScreen_selectsSecondaryScreen_whenCalledWithSecondaryScreen();

    void capture_capturesToFile_whenConnectedToMediaRecorder();
    void removeScreenWhileCapture(); // Keep the test last defined. TODO: find a way to restore
                                     // application screens.
};

void tst_QScreenCaptureBackend::cleanupTestCase()
{
#ifdef QT_MM_HAVE_FAKE_XDG_PORTAL
    m_fakePortal.stop();
#endif
}

bool tst_QScreenCaptureBackend::usingFakePortal() const
{
#ifdef QT_MM_HAVE_FAKE_XDG_PORTAL
    return m_fakePortal.isActive();
#else
    return false;
#endif
}

void tst_QScreenCaptureBackend::publishScene(const QTestWidget &widget)
{
#ifdef QT_MM_HAVE_FAKE_XDG_PORTAL
    if (!m_fakePortal.isActive())
        return;
    const QScreen *screen = widget.screen() ? widget.screen() : QApplication::primaryScreen();
    if (auto result = m_fakePortal.setScene(widget.sceneOnScreen(*screen)); !result)
        qWarning() << "Could not publish the scene to the fake portal:" << result.error();
#else
    Q_UNUSED(widget);
#endif
}

void tst_QScreenCaptureBackend::capture(QTestWidget &widget, const QPoint &drawingOffset,
                                        const QSize &expectedSize,
                                        std::function<void(QScreenCapture &)> scModifier)
{
    // With the fake portal the captured "screen" is synthesised, so the widget's
    // content has to be handed over explicitly.
    publishScene(widget);

    TestVideoSink sink;
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &sc = *screenCapture;

    QSignalSpy errorsSpy(&sc, &QScreenCapture::errorOccurred);

    if (scModifier)
        scModifier(sc);

    QMediaCaptureSession session;

    session.setScreenCapture(&sc);
    session.setVideoSink(&sink);

    const auto pixelRatio = widget.devicePixelRatio();

    sc.setActive(true);

    QVERIFY(sc.isActive());

#ifdef Q_OS_LINUX
    // In some cases, on Linux the window seems to be of a wrong color after appearance,
    // the delay helps.
    // TODO: remove the delay
    // The synthetic source has no compositor to wait for: its very first frame is
    // already correct, so the delay is pure cost there.
    if (!usingFakePortal())
        QTest::qWait(2000);
#endif
    // Let's wait for the first frame to address a potential initialization delay.
    // In practice, the delay varies between the platform and may randomly get increased.
    {
        const auto firstFrame = sink.waitForFrame();
        QVERIFY(firstFrame.isValid());
    }

    sink.setStoreImagesEnabled();

    const int delay = 200;

    QTest::qWait(delay);
    const auto expectedFramesCount =
            delay / static_cast<int>(1000 / std::min(widget.screen()->refreshRate(), 60.));
    const int framesCount = static_cast<int>(sink.images().size());
    QCOMPARE_LE(framesCount, expectedFramesCount + 2);
    QCOMPARE_GE(framesCount, 1);

    for (const auto &image : sink.images()) {
        auto pixelColor = [&drawingOffset, pixelRatio, &image](int x, int y) {
            return image.pixelColor((QPoint(x, y) + drawingOffset) * pixelRatio).toRgb();
        };
        const int capturedWidth = qRound(image.size().width() / pixelRatio);
        const int capturedHeight = qRound(image.size().height() / pixelRatio);
        QCOMPARE(QSize(capturedWidth, capturedHeight), expectedSize);
        QCOMPARE(pixelColor(0, 0), QColor(0xFF, 0, 0));

        QCOMPARE(pixelColor(39, 50), QColor(0xFF, 0, 0));
        QCOMPARE(pixelColor(40, 49), QColor(0xFF, 0, 0));

        QCOMPARE(pixelColor(40, 50), QColor(0, 0, 0xFF));
    }

    QCOMPARE(errorsSpy.size(), 0);
}

void tst_QScreenCaptureBackend::removeWhileCapture(
    std::function<void(QScreenCapture &)> scModifier, std::function<void()> deleter)
{
    QVideoSink sink;
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &sc = *screenCapture;

    QSignalSpy errorsSpy(&sc, &QScreenCapture::errorOccurred);

    QMediaCaptureSession session;

    if (scModifier)
        scModifier(sc);

    session.setScreenCapture(&sc);
    session.setVideoSink(&sink);

    sc.setActive(true);

    QTest::qWait(300);

    QCOMPARE(errorsSpy.size(), 0);

    if (deleter)
        deleter();

    QTest::qWait(100);

    QSignalSpy framesSpy(&sink, &QVideoSink::videoFrameChanged);

    QTest::qWait(100);

    QCOMPARE(errorsSpy.size(), 1);
    QCOMPARE(errorsSpy.front().front().value<QScreenCapture::Error>(),
             QScreenCapture::CaptureFailed);
    QVERIFY2(!errorsSpy.front().back().value<QString>().isEmpty(),
             "Expected not empty error description");

    QVERIFY2(framesSpy.empty(), "No frames expected after screen removal");
}

int getStatusBarHeight([[maybe_unused]] const qreal pixelRatio = 1)
{
#ifdef Q_OS_ANDROID

    using namespace QtJniTypes;

    static int statusBarHeight = -1;

    if (statusBarHeight > -1)
        return statusBarHeight;

    auto activity = QNativeInterface::QAndroidApplication::context();
    auto window = activity.callMethod<Window>("getWindow");
    if (window.isValid()) {
        auto decorView = window.callMethod<View>("getDecorView");
        if (decorView.isValid()) {
            auto rootInsets = decorView.callMethod<WindowInsets>("getRootWindowInsets");
            if (rootInsets.isValid()) {
                if (QNativeInterface::QAndroidApplication::sdkVersion() >= 30) {
                    int windowInsetsType = WindowInsetsType::callStaticMethod<jint>("statusBars");
                    auto insets = rootInsets.callMethod<Insets>(
                            "getInsetsIgnoringVisibility", windowInsetsType);
                    if (rootInsets.isValid())
                        statusBarHeight = insets.getField<jint>("top");
                } else {
                    statusBarHeight = rootInsets.callMethod<jint>("getStableInsetTop");
                }
            }
        }
    }

    if (statusBarHeight == -1) {
        qWarning() << "Failed to get status bar height, falling back to zero.";
        return 0;
    }

    if (pixelRatio != 0)
        statusBarHeight /= pixelRatio;

    return statusBarHeight;
#else
    return 0;
#endif
}

void tst_QScreenCaptureBackend::initTestCase()
{
    if (!initIntegrationTestCase())
        return;

#ifdef Q_OS_ANDROID
    // QTBUG-132249:
    // Security Popup can be automatically accepted with adb command:
    // "adb shell appops set org.qtproject.example.tst_qscreencapturebackend PROJECT_MEDIA allow"
    // Need to find a way to call it by androidtestrunner after installation and before running the test
    QSKIP("Skip on Android; There is a security popup that need to be accepted");
#endif

    // The offscreen platform plugin never shows windows on the native desktop, so the
    // capture backends cannot capture any of the test content.
    if (QGuiApplication::platformName() == u"offscreen"_s)
        QSKIP("Screen capturing does not work with the offscreen platform plugin");

    if (!QApplication::primaryScreen())
        QSKIP("No screens found");

#ifdef QT_MM_HAVE_FAKE_XDG_PORTAL
    // On Wayland the PipeWire backend goes through xdg-desktop-portal, whose real
    // implementation puts up an interactive consent dialog. Stand in for it, so
    // that the very same assertions can run unattended.
    if (QFakeXdgPortalFixture::isRequested()) {
        QString unavailableReason;
        if (!QFakeXdgPortalFixture::prerequisitesAvailable(&unavailableReason))
            QSKIP(qPrintable(u"Fake ScreenCast portal unavailable: "_s + unavailableReason));

        SyntheticVideoScene scene;
        const QScreen &screen = *QApplication::primaryScreen();
        scene.frameSize = screen.size() * screen.devicePixelRatio();

        auto result = m_fakePortal.start(scene);
        QVERIFY2(result,
                 qPrintable(u"Could not start the fake ScreenCast portal: "_s
                            + (result ? QString() : result.error())));
    }
#endif

#if defined(Q_OS_LINUX)
    if (!usingFakePortal() && isCI()
        && qEnvironmentVariable("XDG_SESSION_TYPE").toLower() != u"x11"_s)
        QSKIP("Skip on wayland; to be fixed");
#endif

#ifdef Q_OS_MACOS
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
#endif

    QScreenCapture sc;
    if (sc.error() == QScreenCapture::CapturingNotSupported)
        QSKIP("Screen capturing not supported");
}

void tst_QScreenCaptureBackend::isActive_returnsFalse_whenNotStarted()
{
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &capture = *screenCapture;
    QVERIFY(!capture.isActive());
}

void tst_QScreenCaptureBackend::screen_isNull_whenNotSet()
{
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &capture = *screenCapture;
    QCOMPARE(capture.screen(), nullptr);
}

void tst_QScreenCaptureBackend::error_isNoError_whenNotStarted()
{
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &capture = *screenCapture;
    QCOMPARE(capture.error(), QScreenCapture::Error::NoError);
    QCOMPARE(capture.errorString(), "");
}

void tst_QScreenCaptureBackend::screenCapture_returnsSession_whenAddedToSession()
{
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &capture = *screenCapture;
    QCOMPARE(capture.captureSession(), nullptr);

    QMediaCaptureSession session;
    session.setScreenCapture(&capture);

    QCOMPARE(capture.captureSession(), &session);
}

void tst_QScreenCaptureBackend::setActive_startsStreamWithValidFrame_data()
{
    QTest::addColumn<QScreen *>("screen");

    auto screens = QApplication::screens();
    for (qsizetype i = 0; i < screens.size(); ++i) {
        QByteArray rowName = u"QScreen #%1 - %2"_s
            .arg(i)
            .arg(screens[i]->name())
            .toUtf8();
        QTest::newRow(rowName.constData()) << screens[i];
    }
}

void tst_QScreenCaptureBackend::setActive_startsStreamWithValidFrame()
{
    QFETCH(QScreen *, screen);

    TestVideoSink sink;
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &sc = *screenCapture;

    QSignalSpy errorsSpy(&sc, &QScreenCapture::errorOccurred);

    QMediaCaptureSession session;
    session.setScreenCapture(&sc);
    session.setVideoSink(&sink);

    sc.setScreen(screen);
    sc.setActive(true);

    QVERIFY(sc.isActive());

    // The first frame may be delayed due to backend initialization, so wait for it.
    const QVideoFrame firstFrame = sink.waitForFrame();
    QVERIFY(firstFrame.isValid());

    QCOMPARE(errorsSpy.size(), 0);
}

void tst_QScreenCaptureBackend::capturedFrame_hasExpectedSize_data()
{
    QTest::addColumn<QScreen *>("screen");

    const auto screens = QApplication::screens();
    for (qsizetype i = 0; i < screens.size(); ++i) {
        QByteArray rowName = u"QScreen #%1 - %2"_s
            .arg(i)
            .arg(screens[i]->name())
            .toUtf8();
        QTest::newRow(rowName.constData()) << screens[i];
    }
}

void tst_QScreenCaptureBackend::capturedFrame_hasExpectedSize()
{
    QFETCH(QScreen *, screen);

    TestVideoSink sink;
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &sc = *screenCapture;

    QSignalSpy errorsSpy(&sc, &QScreenCapture::errorOccurred);

    QMediaCaptureSession session;
    session.setScreenCapture(&sc);
    session.setVideoSink(&sink);

    sc.setScreen(screen);
    sc.setActive(true);
    QVERIFY(sc.isActive());

    // The captured frame is delivered in physical pixels, so scale the screen's
    // logical size by its device pixel ratio to get the expected frame size.
    const QSize expectedSize = (QSizeF(screen->size()) * screen->devicePixelRatio()).toSize();

    const QVideoFrame firstFrame = sink.waitForFrame();
    QVERIFY2(firstFrame.isValid(), "Did not receive a frame from the screen capture");

    const QSize actualSize = firstFrame.size();
    QVERIFY2(
        actualSize == expectedSize,
        qPrintable(
            u"First captured frame for screen '%1' was %2x%3, but expected %4x%5"_s
            .arg(screen->name())
            .arg(actualSize.width())
            .arg(actualSize.height())
            .arg(expectedSize.width())
            .arg(expectedSize.height())));

    QCOMPARE(errorsSpy.size(), 0);
}

void tst_QScreenCaptureBackend::setActive_startsAndStopsCapture()
{
    TestVideoSink sink;
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &sc = *screenCapture;

    QSignalSpy errorsSpy(&sc, &QScreenCapture::errorOccurred);
    QSignalSpy activeStateSpy(&sc, &QScreenCapture::activeChanged);

    QMediaCaptureSession session;

    session.setScreenCapture(&sc);
    session.setVideoSink(&sink);

    QCOMPARE(activeStateSpy.size(), 0);
    QVERIFY(!sc.isActive());

    // set active true
    {
        sc.setActive(true);

        QVERIFY(sc.isActive());
        QCOMPARE(activeStateSpy.size(), 1);
        QCOMPARE(activeStateSpy.front().front().toBool(), true);
        QCOMPARE(errorsSpy.size(), 0);
    }

    // wait a bit
    {
        activeStateSpy.clear();
        QTest::qWait(50);

        QCOMPARE(activeStateSpy.size(), 0);
    }

    // set active false
    {
        sc.setActive(false);

        sink.setStoreImagesEnabled(true);

        QVERIFY(!sc.isActive());
        QCOMPARE(sink.images().size(), 0u);
        QCOMPARE(activeStateSpy.size(), 1);
        QCOMPARE(activeStateSpy.front().front().toBool(), false);
        QCOMPARE(errorsSpy.size(), 0);
    }

    // set active false again
    {
        activeStateSpy.clear();

        sc.setActive(false);

        QVERIFY(!sc.isActive());
        QCOMPARE(activeStateSpy.size(), 0);
        QCOMPARE(errorsSpy.size(), 0);
    }
}

void tst_QScreenCaptureBackend::setActive_isNoOp_whenStoppingCaptureThatNeverStarted()
{
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &capture = *screenCapture;

    QSignalSpy activeStateSpy(&capture, &QScreenCapture::activeChanged);

    QVERIFY(!capture.isActive());

    capture.setActive(false);

    QVERIFY(!capture.isActive());
    QCOMPARE(activeStateSpy.count(), 0);
}

void tst_QScreenCaptureBackend::setActive_isNoOp_whenAlreadyActive()
{
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &capture = *screenCapture;

    QSignalSpy errorsSpy(&capture, &QScreenCapture::errorOccurred);
    QSignalSpy activeStateSpy(&capture, &QScreenCapture::activeChanged);

    QMediaCaptureSession session;
    session.setScreenCapture(&capture);

    capture.setActive(true);
    QVERIFY(capture.isActive());
    QCOMPARE(activeStateSpy.size(), 1);
    QVERIFY(errorsSpy.empty());

    // Activating an already-active capture should not emit activeChanged again.
    capture.setActive(true);

    QVERIFY(capture.isActive());
    QCOMPARE(activeStateSpy.size(), 1);
    QVERIFY(errorsSpy.empty());
}

void tst_QScreenCaptureBackend::setActive_restartsScreenCapture_whenStartedAgainAfterStop_data()
{
    QTest::addColumn<QScreen *>("screen");

    auto screens = QApplication::screens();
    for (qsizetype i = 0; i < screens.size(); ++i) {
        QByteArray rowName = u"QScreen #%1 - %2"_s
            .arg(i)
            .arg(screens[i]->name())
            .toUtf8();
        QTest::newRow(rowName.constData()) << screens[i];
    }
}

void tst_QScreenCaptureBackend::setActive_restartsScreenCapture_whenStartedAgainAfterStop()
{
    QFETCH(QScreen *, screen);

    constexpr int restartCount = 3;

    TestVideoSink sink;
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &sc = *screenCapture;

    QSignalSpy errorsSpy(&sc, &QScreenCapture::errorOccurred);
    QSignalSpy activeStateSpy(&sc, &QScreenCapture::activeChanged);

    QMediaCaptureSession session;
    session.setScreenCapture(&sc);
    session.setVideoSink(&sink);

    sc.setScreen(screen);
    sc.setActive(true);

    // Ensure capture is actually running before stopping it
    QVERIFY(sink.waitForFrame().isValid());
    QVERIFY(sc.isActive());

    for (int i = 0; i < restartCount; ++i) {
        sc.setActive(false);
        QVERIFY(!sc.isActive());

        sc.setActive(true);

        QVERIFY(sink.waitForFrame().isValid());
        QVERIFY(sc.isActive());
    }

    // activeChanged fired for the initial start plus a stop/start pair per restart
    QCOMPARE(activeStateSpy.size(), 1 + 2 * restartCount);
    QCOMPARE(errorsSpy.size(), 0);
}

void tst_QScreenCaptureBackend::setFrameRate_updatesPropertyAndEmitsSignal()
{
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &capture = *screenCapture;

    QSignalSpy errorsSpy(&capture, &QScreenCapture::errorOccurred);
    QSignalSpy frameRateSpy(&capture, &QScreenCapture::maximumFrameRateChanged);

    auto frameRateEquals = [](std::optional<qreal> frameRate, float value) {
        return frameRate && qFuzzyCompare(*frameRate, static_cast<qreal>(value));
    };

    // No preferred frame rate initially
    QVERIFY(!capture.maximumFrameRate());

    // Setting a frame rate updates the property and emits frameRateChanged
    const float newFrameRate = 1.f;
    capture.setMaximumFrameRate(newFrameRate);

    QCOMPARE(frameRateSpy.size(), 1);
    QVERIFY(frameRateEquals(capture.maximumFrameRate(), newFrameRate));

    capture.setMaximumFrameRate(std::nullopt);

    QCOMPARE(frameRateSpy.size(), 2);
    QVERIFY(!capture.maximumFrameRate());

    QVERIFY(errorsSpy.empty());
}

void tst_QScreenCaptureBackend::setFrameRate_emitsFramesAtCorrectRate()
{
#ifdef Q_OS_ANDROID
    QSKIP("Framerate setting not implemented on Android");
#endif

    // Some backends will stop transmitting frames if the content is unchanged.
    // Keep it changing: an animated window for a real screen capture, or (under
    // the fake portal) an animated published scene.
    auto widget = createWidget(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint,
                               QRect{ 200, 100, 430, 351 });
    widget->setAnimated(true);
    publishScene(*widget);
    if (!usingFakePortal())
        QVERIFY(QTest::qWaitForWindowExposed(widget.get()));

    TestVideoSink sink;
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &capture = *screenCapture;
    capture.setScreen(widget->screen());
    QMediaCaptureSession session;
    session.setScreenCapture(&capture);
    session.setVideoSink(&sink);

    float newFrameRate = 1.f;

    capture.setMaximumFrameRate(newFrameRate);
    capture.setActive(true);
    QVERIFY(capture.isActive());

    // Range [0, 1]. Lower is better, but may increase flakiness.
    float slopFactor = 0.1;
    if (isCI())
        slopFactor = 0.2;

    // Check framerate is roughly 1fps
    auto durationBetweenFrames = sink.durationBetweenFrames(3);
    QVERIFY2(
        durationBetweenFrames > 0ms,
        "Did not receive enough QVideoFrames to measure framerate");
    const qreal actualFps = 1000.0 / durationBetweenFrames.count();
    QCOMPARE_LT(actualFps, newFrameRate * (1 + slopFactor));
}

void tst_QScreenCaptureBackend::portalSessionClosed_doesNotWarnAboutGrabbingThreadTimer()
{
#ifdef QT_MM_HAVE_FAKE_XDG_PORTAL
    if (!usingFakePortal())
        QSKIP("Requires the fake ScreenCast portal");

    // The fixture's re-exec pins QT_SCREEN_CAPTURE_BACKEND to pipewire, so a
    // portal session always exists here. Other backends report their errors
    // from the grabbing thread itself and cannot produce these warnings.

    // Only the grabbing thread may touch its polling timer. Fail if reporting
    // the portal error from the main thread reaches across threads; every
    // other warning (session teardown, device enumeration) passes through.
    QTest::failOnWarning(QRegularExpression(u".*another thread.*"_s));

    TestVideoSink sink;
    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &sc = *screenCapture;

    QSignalSpy errorsSpy(&sc, &QScreenCapture::errorOccurred);

    QMediaCaptureSession session;
    session.setScreenCapture(&sc);
    session.setVideoSink(&sink);

    sc.setActive(true);

    // Frames flowing proves the grabbing thread and its timer are up.
    QVERIFY(sink.waitForFrame().isValid());

    auto closed = m_fakePortal.closeSessions();
    QVERIFY2(closed,
             qPrintable(u"Could not close the fake portal session: "_s
                        + (closed ? QString() : closed.error())));

    QVERIFY(errorsSpy.wait(10000));
    QCOMPARE(errorsSpy.size(), 1);

    sc.setActive(false);
    QVERIFY(!sc.isActive());
#else
    QSKIP("Requires the fake ScreenCast portal");
#endif
}

void tst_QScreenCaptureBackend::setScreen_selectsScreen_whenCalledWithWidgetsScreen()
{
    auto widget = createWidget(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint,
                               QRect{ 200, 100, 430, 351 });
    if (!usingFakePortal())
        QVERIFY(QTest::qWaitForWindowExposed(widget.get()));

    const QPoint drawingOffset(200, 100 + getStatusBarHeight(widget->devicePixelRatio()));
    capture(*widget, drawingOffset, widget->screen()->size(),
            [&widget](QScreenCapture &sc) { sc.setScreen(widget->screen()); });
}

void tst_QScreenCaptureBackend::constructor_selectsPrimaryScreenAsDefault()
{
    auto widget = createWidget(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint,
                               QRect{ 200, 100, 430, 351 });
    if (!usingFakePortal())
        QVERIFY(QTest::qWaitForWindowExposed(widget.get()));

    const QPoint drawingOffset(200, 100 + getStatusBarHeight(widget->devicePixelRatio()));
    capture(*widget, drawingOffset, QApplication::primaryScreen()->size(), nullptr);
}

void tst_QScreenCaptureBackend::setScreen_selectsSecondaryScreen_whenCalledWithSecondaryScreen()
{
    auto screens = QApplication::screens();
    if (screens.size() < 2)
        QSKIP("2 or more screens required");

    auto topLeft = screens.back()->geometry().topLeft().x();

    auto widgetOnSecondaryScreen =
            createWidget(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint,
                         QRect{ topLeft + 200, 100, 430, 351 }, screens.back());
    if (!usingFakePortal())
        QVERIFY(QTest::qWaitForWindowExposed(widgetOnSecondaryScreen.get()));

    auto widgetOnPrimaryScreen = createWidget(
            Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint,
            QRect{ 200, 100, 430, 351 }, screens.front(), QColor(0, 0, 0), QColor(0, 0, 0));
    if (!usingFakePortal())
        QVERIFY(QTest::qWaitForWindowExposed(widgetOnPrimaryScreen.get()));
    const QPoint drawingOffset(200, 100 + getStatusBarHeight(widgetOnSecondaryScreen->devicePixelRatio()));
    capture(*widgetOnSecondaryScreen, drawingOffset, screens.back()->size(),
            [&screens](QScreenCapture &sc) { sc.setScreen(screens.back()); });
}

void tst_QScreenCaptureBackend::capture_capturesToFile_whenConnectedToMediaRecorder()
{
#ifdef Q_OS_LINUX
    if (isCI())
        QSKIP("QTBUG-116671: SKIP on linux CI to avoid crashes in ffmpeg. To be fixed.");
#endif

    // Create widget with blue color
    auto widget = createWidget(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint,
                               QRect{ 200, 100, 430, 351 });
    widget->setColors(QColor(0, 0, 0xFF), QColor(0, 0, 0xFF));
    publishScene(*widget);

    const std::unique_ptr<QScreenCapture> screenCapture = QtMultimediaTest::makeScreenCapture();
    QScreenCapture &sc = *screenCapture;
    QSignalSpy errorsSpy(&sc, &QScreenCapture::errorOccurred);
    QMediaCaptureSession session;
    QMediaRecorder recorder;
#ifdef Q_OS_ANDROID
    // ADD dummy sink just for trigger repainting (for blinking rectangle)
    TestVideoSink dummySink;
    session.setVideoSink(&dummySink);
#endif
    session.setScreenCapture(&sc);
    session.setRecorder(&recorder);
    auto screen = QApplication::primaryScreen();
    QSize screenSize = screen->geometry().size();
    QSize videoResolution = QSize(1920, 1080);
    recorder.setVideoResolution(videoResolution);
    recorder.setQuality(QMediaRecorder::VeryHighQuality);

    // Insert metadata
    QMediaMetaData metaData;
    metaData.insert(QMediaMetaData::Author, QStringLiteral("Author"));
    metaData.insert(QMediaMetaData::Date, QDateTime::currentDateTime());
    recorder.setMetaData(metaData);

    sc.setActive(true);

    QTest::qWait(1000); // wait a bit for SC threading activating

    {
        QSignalSpy recorderStateChanged(&recorder, &QMediaRecorder::recorderStateChanged);

        recorder.record();

        QTRY_VERIFY(!recorderStateChanged.empty());
        QCOMPARE(recorder.recorderState(), QMediaRecorder::RecordingState);
    }

    QTest::qWait(1000);
    widget->setColors(QColor(0, 0xFF, 0), QColor(0, 0xFF, 0)); // Change widget color
    publishScene(*widget);
    QTest::qWait(1000);

    {
        QSignalSpy recorderStateChanged(&recorder, &QMediaRecorder::recorderStateChanged);

        recorder.stop();

        QTRY_VERIFY(!recorderStateChanged.empty());
        QCOMPARE(recorder.recorderState(), QMediaRecorder::StoppedState);
    }

    QString fileName = recorder.actualLocation().toLocalFile();
    QVERIFY(!fileName.isEmpty());
    QVERIFY(QFileInfo(fileName).size() > 0);

    TestVideoSink sink;
    QMediaPlayer player;
    // Should this be recorder.actualLocation() instead?
    player.setSource(QUrl::fromLocalFile(fileName));
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE_EQ(player.metaData().value(QMediaMetaData::Resolution).toSize(),
                QSize(videoResolution));
    QCOMPARE_GT(player.duration(), 350);
    QCOMPARE_LT(player.duration(), 3000);

    // Convert video frames to QImages
    player.setVideoSink(&sink);
    sink.setStoreImagesEnabled();
    player.setPlaybackRate(10);
    player.play();
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    const size_t framesCount = sink.images().size();

    // Find pixel point at center of widget. Do not allow to get out of the frame size
    int x = std::min(415 * videoResolution.width() / screenSize.width(), videoResolution.width() - 1);
    int y = std::min(275 * videoResolution.height() / screenSize.height(), videoResolution.height() - 1);

    auto point = QPoint(x, y);

    // Verify color of first fourth of the video frames
    for (size_t i = 0; i <= static_cast<size_t>(framesCount * 0.25); i++) {
        QImage image = sink.images().at(i);
        QVERIFY(!image.isNull());
        QRgb rgb = image.pixel(point);
//        qDebug() << QStringLiteral("RGB: %1, %2, %3").arg(qRed(rgb)).arg(qGreen(rgb)).arg(qBlue(rgb));

        // RGB values should be 0, 0, 255. Compensating for inaccurate video encoding.
        QVERIFY(qRed(rgb) <= 60);
        QVERIFY(qGreen(rgb) <= 60);
        QVERIFY(qBlue(rgb) >= 200);
    }

    // Verify color of last fourth of the video frames
    for (size_t i = static_cast<size_t>(framesCount * 0.75); i < framesCount - 1; i++) {
        QImage image = sink.images().at(i);
        QVERIFY(!image.isNull());
        QRgb rgb = image.pixel(point);
//        qDebug() << QStringLiteral("RGB: %1, %2, %3").arg(qRed(rgb)).arg(qGreen(rgb)).arg(qBlue(rgb));

        // RGB values should be 0, 255, 0. Compensating for inaccurate video encoding.
        QVERIFY(qRed(rgb) <= 60);
        QVERIFY(qGreen(rgb) >= 200);
        QVERIFY(qBlue(rgb) <= 60);
    }

    QFile(fileName).remove();
}

void tst_QScreenCaptureBackend::removeScreenWhileCapture()
{
    QSKIP("TODO: find a reliable way to emulate it");

    removeWhileCapture([](QScreenCapture &sc) { sc.setScreen(QApplication::primaryScreen()); },
                       []() {
                           // It's something that doesn't look safe but it performs required flow
                           // and allows to test the corener case.
                           delete QApplication::primaryScreen();
                       });
}

#ifdef QT_MM_HAVE_FAKE_XDG_PORTAL

// QTEST_MAIN's main() is renamed so that the fake portal can get in first. Both
// steps have to happen before QApplication exists: the helper process must not
// build one at all, and the re-exec has to precede the first use of the session
// bus, which QDBusConnection caches for the lifetime of the process.
#  define main testlib_main
QTEST_MAIN(tst_QScreenCaptureBackend)
#  undef main

int main(int argc, char *argv[])
{
    if (QFakeXdgPortalFixture::isHelperProcess(argc, argv))
        return QFakeXdgPortalFixture::runHelperProcess(argc, argv);

    QFakeXdgPortalFixture::reexecUnderPrivateBusIfRequested(argc, argv);
    return testlib_main(argc, argv);
}

#else
QTEST_MAIN(tst_QScreenCaptureBackend)
#endif

#include "tst_qscreencapturebackend.moc"
