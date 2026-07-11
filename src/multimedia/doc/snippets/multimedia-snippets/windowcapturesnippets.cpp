// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCapturableWindow>
#include <QMediaCaptureSession>
#include <QVideoWidget>
#include <QWindowCapture>

void wrapper0() {

//! [Basic setup]
QMediaCaptureSession session;
QWindowCapture windowCapture;
session.setWindowCapture(&windowCapture);

QVideoWidget videoWidget;
session.setVideoOutput(&videoWidget);
videoWidget.show();

// A window must be selected before capturing can start.
const QList<QCapturableWindow> windows = QWindowCapture::capturableWindows();
if (!windows.isEmpty()) {
    windowCapture.setWindow(windows.first());
    windowCapture.start();
}
//! [Basic setup]

} // wrapper0()
