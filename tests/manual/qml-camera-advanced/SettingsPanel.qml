// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: top

    required property CameraHelper camera
    required property MediaDevicesHelper mediaDevices
    required property CaptureSession captureSession
    required property ImageCaptureHelper imageCapture
    required property MediaRecorder recorder
    required property VideoOutput videoOutput
    property var lastFrameInfo
    property url lastRecordingLocation

    // Stored by the owner, as the panel is only instantiated while visible.
    property bool clearPropertiesOnDeviceChange: false

    spacing: 8

    TabBar {
        id: tabBar
        Layout.fillWidth: true
        Layout.minimumWidth: 0

        TabButton {
            text: "Camera"
            width: implicitWidth + 20
        }
        TabButton {
            text: "Image capture"
            width: implicitWidth + 20
        }
        TabButton {
            text: "Recorder"
            width: implicitWidth + 20
        }
        TabButton {
            text: "Video output"
            width: implicitWidth + 20
        }
        TabButton {
            text: "Extras"
            width: implicitWidth + 20
        }
        TabButton {
            text: "Info"
            width: implicitWidth + 20
        }
    }

    StackLayout {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        currentIndex: tabBar.currentIndex

        AllCameraControls {
            captureSession: top.captureSession
            camera: top.camera
            mediaDevices: top.mediaDevices
        }

        ImageCaptureSettings {
            imageCapture: top.imageCapture
            camera: top.camera
        }

        RecorderSettings {
            recorder: top.recorder
            lastRecordingLocation: top.lastRecordingLocation
        }

        VideoOutputSettings {
            videoOutput: top.videoOutput
            lastFrameInfo: top.lastFrameInfo
        }

        ColumnLayout {
            CheckBox {
                text: "Clear properties on device change"
                checked: top.clearPropertiesOnDeviceChange
                onToggled: () => {
                    top.clearPropertiesOnDeviceChange = checked
                }
            }
        }

        InfoPage {
            camera: top.camera
            mediaDevices: top.mediaDevices
        }
    }
}
