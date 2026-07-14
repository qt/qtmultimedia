// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtMultimedia

//! [Basic setup]
CaptureSession {
    id: captureSession
    windowCapture: WindowCapture {
        id: capture
    }
    videoOutput: VideoOutput {
        id: videoOutput
    }

    Component.onCompleted: {
        let windows = capture.capturableWindows()
        if (windows.length > 0) {
            capture.window = windows[0]
            capture.active = true
        }
    }
}
//! [Basic setup]
