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

    onWidthChanged: {
        setState()
    }
    function setState() {
        if (Qt.platform.os === "android" || Qt.platform.os === "ios") {
            if (Screen.desktopAvailableWidth < Screen.desktopAvailableHeight) {
                stillControls.state = "MobilePortrait";
            } else {
                stillControls.state  = "MobileLandscape";
            }
        } else {
            stillControls.state = "Other";
        }
        console.log("State: " + stillControls.state);
        stillControls.buttonsWidth = (stillControls.state === "MobilePortrait")
                ? Screen.desktopAvailableWidth/3.4 : 144
    }

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

        x: 0
        y: 0
        width: ((stillControls.state === "MobilePortrait") ? parent.width : (parent.width-buttonsPanelLandscapeWidth))
        height: ((stillControls.state === "MobilePortrait") ? parent.height - buttonsPanelPortraitHeight : parent.height)
        //        autoOrientation: true
    }

    PhotoCaptureControls {
        id: stillControls
        state: setState()
        anchors.fill: parent
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
        state: stillControls.state
        anchors.fill: parent
        buttonsWidth: stillControls.buttonsWidth
        buttonsPanelPortraitHeight: cameraUI.buttonsPanelPortraitHeight
        buttonsPanelWidth: cameraUI.buttonsPanelLandscapeWidth
        captureSession: captureSession
        visible: (cameraUI.state === "VideoCapture")
        onPreviewSelected: cameraUI.state = "VideoPreview"
        onPhotoModeSelected: cameraUI.state = "PhotoCapture"
    }
}
