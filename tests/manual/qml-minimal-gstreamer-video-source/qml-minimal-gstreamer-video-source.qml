// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtMultimedia

ApplicationWindow {
    id: window
    width: 800
    height: 600
    visible: true
    title: qsTr("QmlMinimalGstreamerVideoSource")

    CaptureSession {
        id: captureSession
        videoOutput: output
        nativeVideoSource: GstreamerVideoSource {
            id: videoSource
            gstBinDescription: "videotestsrc"
            active: true
        }
    }

    VideoOutput {
        id: output
        visible: true
        anchors.fill: parent
    }
}
