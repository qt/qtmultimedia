// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtMultimedia

ApplicationWindow {
    id: window
    width: 800
    height: 600
    visible: true
    title: qsTr("QmlMinimalCamera")

    MediaDevices {
        id: mediaDevices
    }

    CaptureSession {
        id: captureSession
        videoOutput: output
        camera: Camera {
            id: camera
            cameraDevice: mediaDevices.defaultVideoInput
        }

        Component.onCompleted: camera.start()
    }

    VideoOutput {
        id: output
        visible: true
    }
}
