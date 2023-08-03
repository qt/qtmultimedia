// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fixture.h"

#include <qmediaplayer.h>
#include <qvideowidget.h>
#include <qsystemsemaphore.h>
#include <quuid.h>

DisableCursor::DisableCursor()
{
    QCursor cursor(Qt::BlankCursor);
    QApplication::setOverrideCursor(cursor);
}

DisableCursor::~DisableCursor()
{
    QGuiApplication::restoreOverrideCursor();
}

WindowCaptureFixture::WindowCaptureFixture()
{
    m_session.setWindowCapture(&m_capture);
    m_session.setVideoSink(&m_grabber);
}

QString WindowCaptureFixture::getResultsPath(const QString &fileName)
{
    const QString sep = QString::fromLatin1("--");

    QString stem = QCoreApplication::applicationName();
    if (const char *currentTest = QTest::currentTestFunction())
        stem += sep + QString::fromLatin1(currentTest);

    if (const char *currentTag = QTest::currentDataTag())
        stem += sep + QString::fromLatin1(currentTag);

    stem += sep + fileName;

    const QDir resultsDir = qEnvironmentVariable("COIN_CTEST_RESULTSDIR", QDir::tempPath());

    return resultsDir.filePath(stem);
}

bool WindowCaptureFixture::compareImages(QImage actual, const QImage &expected,
                                         const QString &fileSuffix)
{
    // Convert to same format so that we can compare images
    actual = actual.convertToFormat(expected.format());

    if (actual == expected)
        return true;

    qWarning() << "Image comparison failed.";
    qWarning() << "Actual image:";
    qWarning() << actual;
    qWarning() << "Expected image:";
    qWarning() << expected;

    const QString actualName = getResultsPath(QString("actual%1.png").arg(fileSuffix));
    if (!actual.save(actualName))
        qWarning() << "Failed to save actual file to " << actualName;

    const QString expectedName = getResultsPath(QString("expected%1.png").arg(fileSuffix));
    if (!expected.save(expectedName))
        qWarning() << "Failed to save expected file to " << expectedName;

    return false;
}

bool WindowCaptureWithWidgetFixture::start(QSize size)
{
    // In case of window capture failure, signal the grabber so we can stop
    // waiting for frames that will never come.
    connect(&m_capture, &QWindowCapture::errorOccurred, &m_grabber, &FrameGrabber::stop);

    m_widget.setSize(size);

    m_widget.show();

    // Make sure window is in a state that allows it to be found by QWindowCapture.
    // Not necessary on Windows, but seems to be necessary on some platforms.
    if (!QTest::qWaitForWindowExposed(&m_widget, static_cast<int>(s_testTimeout.count()))) {
        qWarning() << "Failed to display widget within timeout";
        return false;
    }

    m_captureWindow = findCaptureWindow(m_widget.windowTitle());

    if (!m_captureWindow.isValid())
        return false;

    m_capture.setWindow(m_captureWindow);
    m_capture.setActive(true);

    return true;
}

QVideoFrame WindowCaptureWithWidgetFixture::waitForFrame(qint64 noOlderThanTime)
{
    const std::vector<QVideoFrame> frames = m_grabber.waitAndTakeFrames(1u, noOlderThanTime);
    if (frames.empty())
        return QVideoFrame{};

    return frames.back();
}

QCapturableWindow WindowCaptureWithWidgetFixture::findCaptureWindow(const QString &windowTitle)
{
    QList<QCapturableWindow> allWindows = QWindowCapture::capturableWindows();

    const auto window = std::find_if(allWindows.begin(), allWindows.end(),
                                     [windowTitle](const QCapturableWindow &win) {
                                         return win.description() == windowTitle;
                                     });

    // Extra debug output to help understanding if test widget window could not be found
    if (window == allWindows.end()) {
        qDebug() << "Could not find window" << windowTitle << ". Existing capturable windows:";
        std::for_each(allWindows.begin(), allWindows.end(), [](const QCapturableWindow &win) {
            qDebug() << "    " << win.description();
        });
        return QCapturableWindow{};
    }

    return *window;
}

void WindowCaptureWithWidgetAndRecorderFixture::start(QSize size, bool togglePattern)
{
    if (togglePattern) {
        // Drive animation
        connect(&m_grabber, &FrameGrabber::videoFrameChanged, &m_widget,
                &TestWidget::togglePattern);
    }

    connect(&m_recorder, &QMediaRecorder::recorderStateChanged, this,
            &WindowCaptureWithWidgetAndRecorderFixture::recorderStateChanged);

    m_session.setRecorder(&m_recorder);
    m_recorder.setQuality(QMediaRecorder::HighQuality);
    m_recorder.setOutputLocation(QUrl::fromLocalFile(m_mediaFile));
    m_recorder.setVideoResolution(size);

    WindowCaptureWithWidgetFixture::start(size);

    m_recorder.record();
}

bool WindowCaptureWithWidgetAndRecorderFixture::stop()
{
    m_recorder.stop();

    const auto recorderStopped = [this] { return m_recorderState == QMediaRecorder::StoppedState; };

    return QTest::qWaitFor(recorderStopped, static_cast<int>(s_testTimeout.count()));
}

bool WindowCaptureWithWidgetAndRecorderFixture::testVideoFilePlayback(const QString &fileName)
{
    QVideoWidget widget;

    QMediaPlayer player;

    bool playing = true;
    connect(&player, &QMediaPlayer::playbackStateChanged, this,
            [&](QMediaPlayer::PlaybackState state) {
                if (state == QMediaPlayer::StoppedState)
                    playing = false;
            });

    QMediaPlayer::Error error = QMediaPlayer::NoError;
    connect(&player, &QMediaPlayer::errorOccurred, this,
            [&](QMediaPlayer::Error e, const QString &errorString) {
                error = e;
                qWarning() << errorString;
            });

    player.setSource(QUrl{ fileName });
    player.setVideoOutput(&widget);
    widget.show();
    player.play();

    const bool completed =
            QTest::qWaitFor([&] { return !playing || error != QMediaPlayer::NoError; },
                            static_cast<int>(s_testTimeout.count()));

    return completed && error == QMediaPlayer::NoError;
}

void WindowCaptureWithWidgetAndRecorderFixture::recorderStateChanged(
        QMediaRecorder::RecorderState state)
{
    m_recorderState = state;
}

bool WindowCaptureWithWidgetInOtherProcessFixture::start()
{
    // In case of window capture failure, signal the grabber so we can stop
    // waiting for frames that will never come.
    connect(&m_capture, &QWindowCapture::errorOccurred, &m_grabber, &FrameGrabber::stop);

    // Create a new window title that is also used as a semaphore key with less than 30 characters
    const QString windowTitle = QString::number(qHash(QUuid::createUuid().toString()));

    QSystemSemaphore windowVisible{ QNativeIpcKey{ windowTitle } };

    // Start another instance of the test executable and ask it to show a
    // its test widget.
    m_windowProcess.setArguments({ "--show", windowTitle });
    m_windowProcess.setProgram(QApplication::applicationFilePath());
    m_windowProcess.start();

    // Make sure window is in a state that allows it to be found by QWindowCapture.
    // We do this by waiting for the process to release the semaphore once its window is visible
    windowVisible.acquire();

    m_captureWindow = findCaptureWindow(windowTitle);

    if (!m_captureWindow.isValid())
        return false;

    // Start capturing the out-of-process window
    m_capture.setWindow(m_captureWindow);
    m_capture.setActive(true);

    // Show in-process widget used to create a reference image
    m_widget.show();

    return true;
}


#include "moc_fixture.cpp"
