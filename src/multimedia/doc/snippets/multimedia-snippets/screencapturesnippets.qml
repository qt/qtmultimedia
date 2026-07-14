// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtMultimedia

//! [Basic setup]
CaptureSession {
    id: captureSession
    screenCapture: ScreenCapture {
        id: capture
        active: true
    }
    videoOutput: VideoOutput {
        id: videoOutput
    }

    Component.onCompleted: {
        // Select the screen to capture. If no screen is set, the primary
        // screen is captured by default.
        const screens = Application.screens
        if (screens.length > 0)
            capture.screen = screens[0]
    }
}
//! [Basic setup]
