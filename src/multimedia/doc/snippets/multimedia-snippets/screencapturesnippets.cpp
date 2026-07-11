// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QMediaCaptureSession>
#include <QScreenCapture>
#include <QVideoWidget>

void wrapper0()
{

//! [Basic setup]
QMediaCaptureSession session;
QScreenCapture screenCapture;
session.setScreenCapture(&screenCapture);

QVideoWidget videoWidget;
session.setVideoOutput(&videoWidget);
videoWidget.show();

// With no screen set, the primary screen is captured once capturing starts.
screenCapture.start();
//! [Basic setup]

} // wrapper0()
