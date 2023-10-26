// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// TESTED_COMPONENT=src/multimedia

#include "fixture.h"
#include "widget.h"

#include <qmediarecorder.h>
#include <qpainter.h>
#include <qsignalspy.h>
#include <qtest.h>
#include <qwindowcapture.h>
#include <qcommandlineparser.h>

#include <chrono>
#include <vector>

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;

QT_USE_NAMESPACE

class tst_QWindowCaptureBackend : public QObject
{
    Q_OBJECT

private slots:
    static void initTestCase()
    {
#if defined(Q_OS_LINUX)
        if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci"
            && qEnvironmentVariable("XDG_SESSION_TYPE").toLower() != "x11")
            QSKIP("Skip on wayland; to be fixed");
#elif defined(Q_OS_MACOS)
        if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
            QSKIP("QTBUG-116285: Skip on macOS CI because of permissions issues");
#endif

        const QWindowCapture capture;
        if (capture.error() == QWindowCapture::CapturingNotSupported)
            QSKIP("Screen capturing not supported");
    }

    void isActive_returnsFalse_whenNotStarted()
    {
        const WindowCaptureFixture fixture;
        QVERIFY(!fixture.m_capture.isActive());
    }

    void setActive_failsAndEmitEerrorOccurred_whenNoWindowSelected()
    {
        WindowCaptureFixture fixture;

        fixture.m_capture.setActive(true);

        QVERIFY(!fixture.m_capture.isActive());
        QVERIFY(!fixture.m_errors.empty());
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

    void capturedImage_equals_imageFromGrab_data()
    {
        QTest::addColumn<QSize>("windowSize");
        QTest::newRow("single-pixel-window") << QSize{1, 1};
        QTest::newRow("small-window") << QSize{60, 40};
        QTest::newRow("odd-width-window") << QSize{ 61, 40 };
        QTest::newRow("odd-height-window") << QSize{ 60, 41 };
        QTest::newRow("big-window") << QApplication::primaryScreen()->size();
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

        const auto startTime = high_resolution_clock::now();

        const QVideoFrame colorFrame = fixture.waitForFrame();
        QVERIFY(colorFrame.isValid());

        fixture.m_widget.setDisplayPattern(TestWidget::Grid);

        // Ignore all frames that were grabbed since the colored frame,
        // to ensure that we get a frame after we changed display pattern
        const high_resolution_clock::duration delay = high_resolution_clock::now() - startTime;
        const QVideoFrame gridFrame = fixture.waitForFrame(
                colorFrame.endTime() + duration_cast<microseconds>(delay).count());

        QVERIFY(gridFrame.isValid());

        // Make sure that the gridFrame has a different content than the colorFrame
        QCOMPARE(gridFrame.size(), colorFrame.size());
        QCOMPARE_NE(gridFrame.toImage(), colorFrame.toImage());

        const QImage actualGridImage = fixture.m_widget.grabImage();
        QVERIFY(fixture.compareImages(gridFrame.toImage(), actualGridImage));
    }

    void sequenceOfCapturedImages_compareEqual_whenWindowContentIsUnchanged()
    {
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
        QTest::newRow("odd-width-window") << QSize{ 61, 40 };
        QTest::newRow("odd-height-window") << QSize{ 60, 41 };
        QTest::newRow("big-window") << QSize{ 800, 600 };
    }

    void recorder_encodesFrames_toValidMediaFile()
    {
#ifdef Q_OS_LINUX
        if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
            QSKIP("QTBUG-116671: SKIP on linux CI to avoid crashes in ffmpeg. To be fixed.");
#endif
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
#ifdef Q_OS_LINUX
        if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
            QSKIP("QTBUG-116671: SKIP on linux CI to avoid crashes in ffmpeg. To be fixed.");
#endif
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
        WindowCaptureWithWidgetInOtherProcessFixture fixture;
        QVERIFY(fixture.start());

        // Get capturing started
        fixture.m_grabber.waitAndTakeFrames(3);

        // Closing the process waits for it to exit
        fixture.m_windowProcess.close();

        const bool captureFailed = QTest::qWaitFor([&] { return !fixture.m_errors.empty(); },
                                                   static_cast<int>(s_testTimeout.count()));

        QVERIFY(captureFailed);
    }
};

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

    // If no special arguments are set, enter the regular QTest main routine
    TESTLIB_SELFCOVERAGE_START("tst_QWindowCaptureatioBackend")
    QT_PREPEND_NAMESPACE(QTest::Internal::callInitMain)<tst_QWindowCaptureBackend>();
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    tst_QWindowCaptureBackend tc;
    QTEST_SET_MAIN_SOURCE_PATH return QTest::qExec(&tc, argc, argv);

}

#include "tst_qwindowcapturebackend.moc"
