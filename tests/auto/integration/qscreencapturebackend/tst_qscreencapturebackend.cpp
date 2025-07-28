// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <qvideosink.h>
#include <qvideoframe.h>
#include <qmediacapturesession.h>
#include <qpainter.h>
#include <qscreencapture.h>
#include <qsignalspy.h>
#include <qmediarecorder.h>
#include <qmediaplayer.h>

#include <private/mediabackendutils_p.h>

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

QT_USE_NAMESPACE

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
                                                      QColor secondColor = QColor(0, 0, 0xFF))
    {
        auto widget = std::make_unique<QTestWidget>(firstColor, secondColor);

        widget->setWindowTitle("Test QScreenCapture");
        widget->setScreen(screen ? screen : QApplication::primaryScreen());
        widget->setWindowFlags(flags);
        widget->setGeometry(geometry);
#ifdef Q_OS_ANDROID
    // Android is not a Window System. When calling setGeometry() on the main widget, it will
    // be displayed at the beginning of the screen. The x,y coordinates are ignored and lost.
    // To make the test consistent on Android, let's remember the geometry and use
    // it later in the paintEvent
        widget->m_paintPosition = geometry;
#endif
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

        p.setBrush(m_firstColor);
        p.drawRect(rect);

        if (m_firstColor != m_secondColor) {
            rect.adjust(40, 50, -60, -70);
            p.setBrush(m_secondColor);
            p.drawRect(rect);
        }
    }

private:
    QColor m_firstColor;
    QColor m_secondColor;
};

class TestVideoSink : public QVideoSink
{
    Q_OBJECT
public:
    TestVideoSink()
    {
        connect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::videoFrameChangedSync);

#ifdef Q_OS_ANDROID
        // Repaint to force the Screen Grabber to work
        connect(this, &QVideoSink::videoFrameChanged, this, []() {
            for (QWidget* widget : QApplication::topLevelWidgets()) { widget->update(); }});
#endif
    }

    void setStoreImagesEnabled(bool storeImages = true) {
        if (storeImages)
            connect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::storeImage, Qt::UniqueConnection);
        else
            disconnect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::storeImage);
    }

    const std::vector<QImage> &images() const { return m_images; }

    QVideoFrame waitForFrame()
    {
        QSignalSpy spy(this, &TestVideoSink::videoFrameChangedSync);
        return spy.wait() ? spy.at(0).at(0).value<QVideoFrame>() : QVideoFrame{};
    }

signals:
    void videoFrameChangedSync(QVideoFrame frame);

private:
    void storeImage(const QVideoFrame &frame) {
        auto image = frame.toImage();
        image.detach();
        m_images.push_back(std::move(image));
    }

private:
    std::vector<QImage> m_images;
};

class tst_QScreenCaptureBackend : public QObject
{
    Q_OBJECT

    void removeWhileCapture(std::function<void(QScreenCapture &)> scModifier,
                            std::function<void()> deleter);

    void capture(QTestWidget &widget, const QPoint &drawingOffset, const QSize &expectedSize,
                 std::function<void(QScreenCapture &)> scModifier);

private slots:
    void initTestCase();
    void setActive_startsAndStopsCapture();
    void setScreen_selectsScreen_whenCalledWithWidgetsScreen();
    void constructor_selectsPrimaryScreenAsDefault();
    void setScreen_selectsSecondaryScreen_whenCalledWithSecondaryScreen();

    void capture_capturesToFile_whenConnectedToMediaRecorder();
    void removeScreenWhileCapture(); // Keep the test last defined. TODO: find a way to restore
                                     // application screens.
};

void tst_QScreenCaptureBackend::setActive_startsAndStopsCapture()
{
    TestVideoSink sink;
    QScreenCapture sc;

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

void tst_QScreenCaptureBackend::capture(QTestWidget &widget, const QPoint &drawingOffset,
                                            const QSize &expectedSize,
                                            std::function<void(QScreenCapture &)> scModifier)
{
    TestVideoSink sink;
    QScreenCapture sc;

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
    QScreenCapture sc;

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
#ifdef Q_OS_ANDROID
    // QTBUG-132249:
    // Security Popup can be automatically accepted with adb command:
    // "adb shell appops set org.qtproject.example.tst_qscreencapturebackend PROJECT_MEDIA allow"
    // Need to find a way to call it by androidtestrunner after installation and before running the test
    QSKIP("Skip on Android; There is a security popup that need to be accepted");
#endif
#if defined(Q_OS_LINUX)
    if (isCI() && qEnvironmentVariable("XDG_SESSION_TYPE").toLower() != "x11")
        QSKIP("Skip on wayland; to be fixed");
#endif

    if (!QApplication::primaryScreen())
        QSKIP("No screens found");

    QScreenCapture sc;
    if (sc.error() == QScreenCapture::CapturingNotSupported)
        QSKIP("Screen capturing not supported");
}

void tst_QScreenCaptureBackend::setScreen_selectsScreen_whenCalledWithWidgetsScreen()
{
    auto widget = QTestWidget::createAndShow(
            Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint,
            QRect{ 200, 100, 430, 351 });
    QVERIFY(QTest::qWaitForWindowExposed(widget.get()));

    const QPoint drawingOffset(200, 100 + getStatusBarHeight(widget->devicePixelRatio()));
    capture(*widget, drawingOffset, widget->screen()->size(),
            [&widget](QScreenCapture &sc) { sc.setScreen(widget->screen()); });
}

void tst_QScreenCaptureBackend::constructor_selectsPrimaryScreenAsDefault()
{
    auto widget = QTestWidget::createAndShow(
            Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint,
            QRect{ 200, 100, 430, 351 });
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

    auto widgetOnSecondaryScreen = QTestWidget::createAndShow(
            Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint,
            QRect{ topLeft + 200, 100, 430, 351 }, screens.back());
    QVERIFY(QTest::qWaitForWindowExposed(widgetOnSecondaryScreen.get()));

    auto widgetOnPrimaryScreen = QTestWidget::createAndShow(
            Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint,
            QRect{ 200, 100, 430, 351 }, screens.front(), QColor(0, 0, 0), QColor(0, 0, 0));
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
    auto widget = QTestWidget::createAndShow(
            Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint,
            QRect{ 200, 100, 430, 351 });
    widget->setColors(QColor(0, 0, 0xFF), QColor(0, 0, 0xFF));

    QScreenCapture sc;
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
    player.setSource(fileName);
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

QTEST_MAIN(tst_QScreenCaptureBackend)

#include "tst_qscreencapturebackend.moc"
