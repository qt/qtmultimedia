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
    property bool showingAdvancedPanel: false
    property var lastRecordedVideoPath: null

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

        clearPropertiesUponDeviceChange: advancedPanel.clearCameraPropertiesOnDeviceChange
    }

    MediaDevicesHelper {
        id: mediaDevices
    }

    CaptureSession {
        id: captureSession

        camera: myCamera

        imageCapture: ImageCaptureHelper {
            id: imageCapture
            onImageSaved: (id, fileName) => {
                console.log("Image file saved to " + fileName)
                window.imageCaptureTracker++
            }
        }

        recorder: MediaRecorder {
            id: recorder
            outputLocation:
                StandardPaths.writableLocation(StandardPaths.MoviesLocation)
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
        videoOutput: cameraOutput
    }

    MediaPlayer {
        id: mediaPlayer
        videoOutput: capturedVideoOutput
        source: window.lastRecordedVideoPath === null ? "" : window.lastRecordedVideoPath

        property var allMediaStatuses: [
            MediaPlayer.NoMedia,
            MediaPlayer.LoadingMedia,
            MediaPlayer.LoadedMedia,
            MediaPlayer.BufferingMedia,
            MediaPlayer.StalledMedia,
            MediaPlayer.BufferedMedia,
            MediaPlayer.EndOfMedia,
            MediaPlayer.InvalidMedia, ]
        function mediaStatusToString(input) {
            switch (input) {
            case MediaPlayer.NoMedia: return "NoMedia";
            case MediaPlayer.LoadingMedia: return "LoadingMedia";
            case MediaPlayer.LoadedMedia: return "LoadedMedia";
            case MediaPlayer.BufferingMedia: return "BufferingMedia";
            case MediaPlayer.StalledMedia: return "StalledMedia";
            case MediaPlayer.BufferedMedia: return "BufferedMedia";
            case MediaPlayer.EndOfMedia: return "EndOfMedia";
            case MediaPlayer.InvalidMedia: return "InvalidMedia";
            default: return "Unrecognized media status";
            }
        }

        onMediaStatusChanged: () => {
            console.log("Media status changed: " + mediaStatusToString(mediaStatus))
        }
    }

    CameraFeed {
        id: cameraOutput
        anchors.fill: parent
        visible: cameraPermission.status === Qt.PermissionStatus.Granted
        camera: myCamera
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
        visible: window.showingSmallPanel && !window.showingAdvancedPanel
        anchors.bottom: parent.bottom
        width: Math.min(implicitWidth, parent.width)
        height: window.isLandscape
            ? Math.min(implicitHeight, parent.height)
            : Math.min(implicitHeight, parent.height, 400)

        camera: myCamera
        captureSession: captureSession
        recorder: recorder
        imagesTakenCount: window.imageCaptureTracker
        backgroundOpacity: window.backgroundOpacity
    }

    AdvancedPanel {
        id: advancedPanel
        width: Math.min(window.width, implicitWidth)
        height: Math.min(window.height, implicitHeight)
        visible: window.showingAdvancedPanel
        opacity: window.backgroundOpacity

        camera: myCamera
        mediaDevices: mediaDevices
        captureSession: captureSession
        backgroundOpacity: window.backgroundOpacity
    }

    Pane {
        visible: !window.showingAdvancedPanel
        ColumnLayout {
            Button {
                Layout.fillWidth: true
                text: "Toggle"
                onClicked: () => {
                    window.showingSmallPanel = !window.showingSmallPanel
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Advanced"
                onClicked: () => {
                    window.showingAdvancedPanel = !window.showingAdvancedPanel
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
            source: imageCapture.preview
        }

        MouseArea {
            anchors.fill: parent
            onClicked: photoPreview.visible = false
        }
    }

    Pane {
        id: capturedVideoOutputPane
        visible: false
        anchors.fill: parent
        padding: 0
        VideoOutput {
            anchors.fill: parent
            id: capturedVideoOutput
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                capturedVideoOutputPane.visible = false
                mediaPlayer.stop()
            }
        }
    }
}
