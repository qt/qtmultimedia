// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>

#include <qvideosink.h>
#include <qvideoframe.h>
#include <qmediacapturesession.h>
#include <qpainter.h>
#include <qscreencapture.h>
#include <qsignalspy.h>
#include <qmediarecorder.h>
#include <qmediaplayer.h>

#include <vector>

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
        widget->show();

        return widget;
    }

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

        p.setBrush(m_firstColor);
        auto rect = this->rect();
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
public:
    TestVideoSink() = default;

    void setStoreImagesEnabled(bool storeImages = true) {
        if (storeImages)
            connect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::storeImage, Qt::UniqueConnection);
        else
            disconnect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::storeImage);
    }

    const std::vector<QImage> &images() const { return m_images; }

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
#ifdef Q_OS_ANDROID
    // Should be removed after fixing QTBUG-112855
    auto widget = QTestWidget::createAndShow(Qt::Window | Qt::FramelessWindowHint,
                                             QRect{ 200, 100, 430, 351 });
    QVERIFY(QTest::qWaitForWindowExposed(widget.get()));
    QTest::qWait(100);
#endif
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

    QTest::qWait(300);

    sink.setStoreImagesEnabled();

    const int delay = 200;

    QTest::qWait(delay);
    const auto expectedFramesCount =
            delay / static_cast<int>(1000 / std::min(widget.screen()->refreshRate(), 60.));
    const int framesCount = static_cast<int>(sink.images().size());
    QCOMPARE_LE(framesCount, expectedFramesCount + 2);
    QCOMPARE_GE(framesCount, expectedFramesCount / 2);

    const QSize expectedSizeForComparison(qFloor(expectedSize.width() * pixelRatio),
                                           qFloor(expectedSize.height() * pixelRatio));

    for (const auto &image : sink.images()) {
        auto pixelColor = [&drawingOffset, pixelRatio, &image](int x, int y) {
            return image.pixelColor((QPoint(x, y) + drawingOffset) * pixelRatio).toRgb();
        };

        QCOMPARE(image.size(), expectedSizeForComparison);
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

void tst_QScreenCaptureBackend::initTestCase()
{
#if defined(Q_OS_LINUX)
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci" &&
        qEnvironmentVariable("XDG_SESSION_TYPE").toLower() != "x11")
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
    auto widget = QTestWidget::createAndShow(Qt::Window | Qt::FramelessWindowHint
                                                     | Qt::WindowStaysOnTopHint
#ifdef Q_OS_ANDROID
                                                     | Qt::Popup
#endif
                                                     ,
                                             QRect{ 200, 100, 430, 351 });
    QVERIFY(QTest::qWaitForWindowExposed(widget.get()));

    capture(*widget, { 200, 100 }, widget->screen()->size(),
            [&widget](QScreenCapture &sc) { sc.setScreen(widget->screen()); });
}

void tst_QScreenCaptureBackend::constructor_selectsPrimaryScreenAsDefault()
{
    auto widget = QTestWidget::createAndShow(Qt::Window | Qt::FramelessWindowHint
                                                     | Qt::WindowStaysOnTopHint
#ifdef Q_OS_ANDROID
                                                     | Qt::Popup
#endif
                                                     ,
                                             QRect{ 200, 100, 430, 351 });
    QVERIFY(QTest::qWaitForWindowExposed(widget.get()));

    capture(*widget, { 200, 100 }, QApplication::primaryScreen()->size(), nullptr);
}

void tst_QScreenCaptureBackend::setScreen_selectsSecondaryScreen_whenCalledWithSecondaryScreen()
{
    const auto screens = QApplication::screens();
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
    capture(*widgetOnSecondaryScreen, { 200, 100 }, screens.back()->size(),
            [&screens](QScreenCapture &sc) { sc.setScreen(screens.back()); });
}

void tst_QScreenCaptureBackend::capture_capturesToFile_whenConnectedToMediaRecorder()
{
#ifdef Q_OS_LINUX
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("QTBUG-116671: SKIP on linux CI to avoid crashes in ffmpeg. To be fixed.");
#endif

    // Create widget with blue color
    auto widget = QTestWidget::createAndShow(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint,
                                                 QRect{ 200, 100, 430, 351 });
    widget->setColors(QColor(0, 0, 0xFF), QColor(0, 0, 0xFF));

    QScreenCapture sc;
    QSignalSpy errorsSpy(&sc, &QScreenCapture::errorOccurred);
    QMediaCaptureSession session;
    QMediaRecorder recorder;
    session.setScreenCapture(&sc);
    session.setRecorder(&recorder);
    auto screen = QApplication::primaryScreen();
    QSize screenSize = screen->geometry().size();
    QSize videoResolution = QSize(1920, 1080);
    recorder.setVideoResolution(videoResolution);
    recorder.setQuality(QMediaRecorder::VeryHighQuality);

    // Insert metadata
    QMediaMetaData metaData;
    metaData.insert(QMediaMetaData::Author, QString::fromUtf8("Author"));
    metaData.insert(QMediaMetaData::Date, QDateTime::currentDateTime());
    recorder.setMetaData(metaData);

    sc.setActive(true);

    QTest::qWait(200); // wait a bit for SC threading activating

    {
        QSignalSpy recorderStateChanged(&recorder, &QMediaRecorder::recorderStateChanged);

        recorder.record();

        QTRY_VERIFY(!recorderStateChanged.empty());
        QCOMPARE(recorder.recorderState(), QMediaRecorder::RecordingState);
    }

    QTest::qWait(300);
    widget->setColors(QColor(0, 0xFF, 0), QColor(0, 0xFF, 0)); // Change widget color
    QTest::qWait(300);

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
    QCOMPARE_EQ(player.metaData().value(QMediaMetaData::Resolution).toSize(), QSize(videoResolution));
    QCOMPARE_GT(player.duration(), 350);
    QCOMPARE_LT(player.duration(), 650);

    // Convert video frames to QImages
    player.setVideoSink(&sink);
    sink.setStoreImagesEnabled();
    player.setPlaybackRate(10);
    player.play();
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    const size_t framesCount = sink.images().size();

    // Find pixel point at center of widget
    int x = 415 * videoResolution.width() / screenSize.width();
    int y = 275 * videoResolution.height() / screenSize.height();
    auto point = QPoint(x, y);

    // Verify color of first fourth of the video frames
    for (size_t i = 0; i <= static_cast<size_t>(framesCount * 0.25); i++) {
        QImage image = sink.images().at(i);
        QVERIFY(!image.isNull());
        QRgb rgb = image.pixel(point);
//        qDebug() << QString("RGB: %1, %2, %3").arg(qRed(rgb)).arg(qGreen(rgb)).arg(qBlue(rgb));

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
//        qDebug() << QString("RGB: %1, %2, %3").arg(qRed(rgb)).arg(qGreen(rgb)).arg(qBlue(rgb));

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
