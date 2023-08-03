// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef WINDOW_CAPTURE_FIXTURE_H
#define WINDOW_CAPTURE_FIXTURE_H

#include "grabber.h"
#include "widget.h"

#include <chrono>
#include <qmediacapturesession.h>
#include <qmediarecorder.h>
#include <qobject.h>
#include <qsignalspy.h>
#include <qtest.h>
#include <qvideoframe.h>
#include <qwindowcapture.h>
#include <qprocess.h>

QT_USE_NAMESPACE

constexpr inline std::chrono::milliseconds s_testTimeout = std::chrono::seconds(60);

/*!
    Utility used to hide application cursor for image comparison tests.
    On Windows, the mouse cursor is captured as part of the window capture.
    This and can cause differences when comparing captured images with images
    from QWindow::grab() which is used as a reference.
*/
struct DisableCursor final
{
    DisableCursor();
    ~DisableCursor();

    DisableCursor(const DisableCursor &) = delete;
    DisableCursor &operator=(const DisableCursor &) = delete;
};

/*!
    Fixture class that orchestrates setup/teardown of window capturing
*/
class WindowCaptureFixture : public QObject
{
    Q_OBJECT

public:
    WindowCaptureFixture();

    /*!
        Compare two images, ignoring format.
        If images differ, diagnostic output is logged and images are saved to file.
    */
    static bool compareImages(QImage actual, const QImage &expected,
                              const QString &fileSuffix = "");

    QMediaCaptureSession m_session;
    QWindowCapture m_capture;
    FrameGrabber m_grabber;

    QSignalSpy m_errors{ &m_capture, &QWindowCapture::errorOccurred };
    QSignalSpy m_activations{ &m_capture, &QWindowCapture::activeChanged };

private:
    /*!
        Calculate a result path based upon a single filename.
        On CI, the file will be located in COIN_CTEST_RESULTSDIR, and on developer
        computers, the file will be located in TEMP.

        The file name is on the form "testCase_testFunction_[dataTag_]fileName"
    */
    static QString getResultsPath(const QString &fileName);
};

/*!
    Fixture class that extends window capture fixture with a capturable widget
*/
class WindowCaptureWithWidgetFixture : public WindowCaptureFixture
{
    Q_OBJECT

public:
    /*!
        Starts capturing and returns true if successful.

        Two phase initialization is used to be able to detect
        failure to find widget window as a capturable window.
    */
    bool start(QSize size = { 60, 40 });

    /*!
        Waits until the a captured frame is received and returns it
    */
    QVideoFrame waitForFrame(qint64 noOlderThanTime = 0);

    DisableCursor m_cursorDisabled; // Avoid mouse cursor causing image differences
    TestWidget m_widget;
    QCapturableWindow m_captureWindow;

protected:
    static QCapturableWindow findCaptureWindow(const QString &windowTitle);
};

class WindowCaptureWithWidgetInOtherProcessFixture : public WindowCaptureWithWidgetFixture
{
    Q_OBJECT

public:
    ~WindowCaptureWithWidgetInOtherProcessFixture() { m_windowProcess.close(); }

    /*!
        Create widget in separate process and start capturing its content
    */
    bool start();

    QProcess m_windowProcess;
};

class WindowCaptureWithWidgetAndRecorderFixture : public WindowCaptureWithWidgetFixture
{
    Q_OBJECT

public:
    void start(QSize size = { 60, 40 }, bool togglePattern = true);

    /*!
        Stop recording.

        Since recorder finalizes the file asynchronously, even after destructors are called,
        we need to explicitly wait for the stopped state before ending the test. If we don't
        do this, the media file can not be deleted by the QTemporaryDir at destruction.
    */
    bool stop();

    bool testVideoFilePlayback(const QString& fileName);

public slots:
    void recorderStateChanged(QMediaRecorder::RecorderState state);

public:
    QTemporaryDir m_tempDir;
    const QString m_mediaFile = m_tempDir.filePath("test.mp4");
    QMediaRecorder m_recorder;
    QMediaRecorder::RecorderState m_recorderState = QMediaRecorder::StoppedState;
    QSignalSpy m_recorderErrors{ &m_recorder, &QMediaRecorder::errorOccurred };
};

#endif
