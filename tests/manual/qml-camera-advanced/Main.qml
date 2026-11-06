// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml
import QtMultimedia

ApplicationWindow {
    id: window
    visible: true
    width: 800
    height: 600

    readonly property bool isLandscape: width >= height
    property int imageCaptureTracker: 0
    property int videoCaptureTracker: 0
    property bool showingSmallPanel: true
    property var lastRecordedVideoPath: null
    property var lastFrameInfo: undefined

    readonly property real backgroundOpacity: 0.7

    CameraPermission {
        id: cameraPermission

        function permissionStatusToString(input) {
            switch (input) {
            case Qt.PermissionStatus.Undetermined: return "Undetermined"
            case Qt.PermissionStatus.Granted: return "Granted"
            case Qt.PermissionStatus.Denied: return "Denied"
            default: return "Unrecognized PermissionStatus"
            }
        }

        onStatusChanged: () => {
            console.log("Status changed: " + permissionStatusToString(status))
            if (status === Qt.PermissionStatus.Granted)
                myCamera.start()
        }
    }

    CameraHelper {
        id: myCamera
        active: true

        clearPropertiesUponDeviceChange: settingsPanelState.clearPropertiesOnDeviceChange
    }

    MediaDevicesHelper {
        id: mediaDevicesHelper
    }

    CaptureSession {
        id: mainCaptureSession

        camera: myCamera

        imageCapture: ImageCaptureHelper {
            id: mainImageCapture
            onImageSaved: (id, fileName) => {
                console.log("Image file saved to " + fileName)
                window.imageCaptureTracker++
            }
        }

        recorder: MediaRecorder {
            id: mainRecorder
            outputLocation:
                RecordingDirectory.ensureMoviesLocation()
                + "/qml-camera-playground_"
                + window.videoCaptureTracker
            onRecorderStateChanged: (state) => {
                if (state === MediaRecorder.StoppedState) {
                    window.lastRecordedVideoPath = actualLocation
                    window.videoCaptureTracker++;
                }
            }

            onActualLocationChanged: () => {
                console.log("New actual location: " + actualLocation)
            }

            onErrorOccurred: (error, msg) => {
                console.log("Media recorder error: " + msg)
            }
        }
        videoOutput: settingsButton.previewActive ? settingsButton.previewOutput : cameraOutput
    }

    CameraFeed {
        id: cameraOutput
        anchors.fill: parent
        visible: cameraPermission.status === Qt.PermissionStatus.Granted
        camera: myCamera
    }

    // Lives here since the settings are only created while visible.
    Connections {
        target: mainCaptureSession.videoOutput ? mainCaptureSession.videoOutput.videoSink : null
        function onVideoFrameChanged() {
            window.lastFrameInfo = VideoFrameInspector.inspectLatest(
                mainCaptureSession.videoOutput.videoSink)
        }
    }

    // If permission not granted, display text
    RowLayout {
        anchors.centerIn: parent
        visible: cameraPermission.status !== Qt.PermissionStatus.Granted
        Label {
            text: "Camera permission not granted"
        }
        Button {
            text: "Request"
            onClicked: cameraPermission.request()
        }
    }

    QuickControls {
        id: controlPanel

        opacity: window.backgroundOpacity

        // Position and size
        visible: window.showingSmallPanel
        anchors.bottom: parent.bottom
        width: Math.min(implicitWidth, parent.width)
        height: window.isLandscape
            ? Math.min(implicitHeight, parent.height)
            : Math.min(implicitHeight, parent.height, 400)

        camera: myCamera
        captureSession: mainCaptureSession
        recorder: mainRecorder
        imagesTakenCount: window.imageCaptureTracker
        backgroundOpacity: window.backgroundOpacity
    }

    QtObject {
        id: settingsPanelState
        property bool clearPropertiesOnDeviceChange: false
    }

    Pane {
        background: null

        ColumnLayout {
            Button {
                Layout.fillWidth: true
                text: "Toggle"
                onClicked: () => {
                    window.showingSmallPanel = !window.showingSmallPanel
                }
            }

            SettingsPanelButton {
                id: settingsButton
                Layout.fillWidth: true
                title: "Settings"
                windowWidth: 480
                windowHeight: 640
                settings: Component {
                    SettingsPanel {
                        camera: myCamera
                        mediaDevices: mediaDevicesHelper
                        captureSession: mainCaptureSession
                        imageCapture: mainImageCapture
                        recorder: mainRecorder
                        videoOutput: cameraOutput
                        lastFrameInfo: window.lastFrameInfo
                        lastRecordingLocation: window.lastRecordedVideoPath === null
                            ? ""
                            : window.lastRecordedVideoPath
                        clearPropertiesOnDeviceChange:
                            settingsPanelState.clearPropertiesOnDeviceChange
                        onClearPropertiesOnDeviceChangeChanged: () => {
                            settingsPanelState.clearPropertiesOnDeviceChange =
                                clearPropertiesOnDeviceChange
                        }
                    }
                }
            }
        }
    }

    Pane {
        id: photoPreview
        visible: false
        anchors.fill: parent
        padding: 0
        Image {
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            smooth: true
            source: mainImageCapture.preview
        }

        MouseArea {
            anchors.fill: parent
            onClicked: photoPreview.visible = false
        }
    }

    // Plays the last recording. The player is only created while shown, so playback
    // starts each time it is opened.
    Pane {
        id: capturedVideoOutputPane
        visible: false
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent

            Loader {
                Layout.fillWidth: true
                Layout.fillHeight: true
                active: capturedVideoOutputPane.visible
                sourceComponent: RecordedVideoPlayer {
                    source: window.lastRecordedVideoPath === null
                        ? ""
                        : window.lastRecordedVideoPath
                }
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: "Close"
                onClicked: () => {
                    capturedVideoOutputPane.visible = false
                }
            }
        }
    }
}
