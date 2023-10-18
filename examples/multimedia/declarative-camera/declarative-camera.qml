// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtMultimedia

Rectangle {
    id : cameraUI

    width: 800
    height: 480
    color: "black"
    state: "PhotoCapture"

    property string platformScreen: ""
    property int buttonsPanelLandscapeWidth: 328
    property int buttonsPanelPortraitHeight: 180

    states: [
        State {
            name: "PhotoCapture"
            StateChangeScript {
                script: {
                    camera.start()
                }
            }
        },
        State {
            name: "PhotoPreview"
        },
        State {
            name: "VideoCapture"
            StateChangeScript {
                script: {
                    camera.start()
                }
            }
        },
        State {
            name: "VideoPreview"
            StateChangeScript {
                script: {
                    camera.stop()
                }
            }
        }
    ]

    CaptureSession {
        id: captureSession
        camera: Camera {
            id: camera
        }
        imageCapture: ImageCapture {
            id: imageCapture
        }

        recorder: MediaRecorder {
            id: recorder
//             resolution: "640x480"
//             frameRate: 30
        }
        videoOutput: viewfinder
    }

    PhotoPreview {
        id : photoPreview
        anchors.fill : parent
        onClosed: cameraUI.state = "PhotoCapture"
        visible: (cameraUI.state === "PhotoPreview")
        focus: visible
        source: imageCapture.preview
    }

    VideoPreview {
        id : videoPreview
        anchors.fill : parent
        onClosed: cameraUI.state = "VideoCapture"
        visible: (cameraUI.state === "VideoPreview")
        focus: visible

        //don't load recorded video if preview is invisible
        source: visible ? recorder.actualLocation : ""
    }

    VideoOutput {
        id: viewfinder
        visible: ((cameraUI.state === "PhotoCapture") || (cameraUI.state === "VideoCapture"))
        anchors.fill: parent
        //        autoOrientation: true
    }

    Item {
        id: controlLayout

        readonly property bool isMobile: Qt.platform.os === "android" || Qt.platform.os === "ios"
        readonly property bool isLandscape: Screen.desktopAvailableWidth >= Screen.desktopAvailableHeight
        property int buttonsWidth: state === "MobilePortrait" ? Screen.desktopAvailableWidth / 3.4 : 114

        states: [
            State {
                name: "MobileLandscape"
                when: controlLayout.isMobile && controlLayout.isLandscape
            },
            State {
                name: "MobilePortrait"
                when: controlLayout.isMobile && !controlLayout.isLandscape
            },
            State {
                name: "Other"
                when: !controlLayout.isMobile
            }
        ]

        onStateChanged: {
            console.log("State: " + controlLayout.state)
        }
    }

    PhotoCaptureControls {
        id: stillControls
        state: controlLayout.state
        anchors.fill: parent
        buttonsWidth: controlLayout.buttonsWidth
        buttonsPanelPortraitHeight: cameraUI.buttonsPanelPortraitHeight
        buttonsPanelWidth: cameraUI.buttonsPanelLandscapeWidth
        captureSession: captureSession
        visible: (cameraUI.state === "PhotoCapture")
        onPreviewSelected: cameraUI.state = "PhotoPreview"
        onVideoModeSelected: cameraUI.state = "VideoCapture"
        previewAvailable: imageCapture.preview.length !== 0
    }

    VideoCaptureControls {
        id: videoControls
        state: controlLayout.state
        anchors.fill: parent
        buttonsWidth: controlLayout.buttonsWidth
        buttonsPanelPortraitHeight: cameraUI.buttonsPanelPortraitHeight
        buttonsPanelWidth: cameraUI.buttonsPanelLandscapeWidth
        captureSession: captureSession
        visible: (cameraUI.state === "VideoCapture")
        onPreviewSelected: cameraUI.state = "VideoPreview"
        onPhotoModeSelected: cameraUI.state = "PhotoCapture"
    }
}
