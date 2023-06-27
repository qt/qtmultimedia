// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtMultimedia
import QtQuick.Layouts

FocusScope {
    id : captureControls
    property CaptureSession captureSession
    property bool previewAvailable : false

    property int buttonsmargin: 8
    property int buttonsPanelWidth
    property int buttonsPanelPortraitHeight
    property int buttonsWidth

    signal previewSelected
    signal photoModeSelected

    Rectangle {
        id: buttonPaneShadow
        color: Qt.rgba(0.08, 0.08, 0.08, 1)

        GridLayout {
            id: buttonsColumn
            anchors.margins: buttonsmargin
            flow: captureControls.state === "MobilePortrait"
                  ? GridLayout.LeftToRight : GridLayout.TopToBottom
            Item {
                implicitWidth: buttonsWidth
                height: 70
                CameraButton {
                    text: "Record"
                    anchors.fill: parent
                    visible: captureSession.recorder.recorderState !== MediaRecorder.RecordingState
                    onClicked: captureSession.recorder.record()
                }
            }

            Item {
                implicitWidth: buttonsWidth
                height: 70
                CameraButton {
                    id: stopButton
                    text: "Stop"
                    anchors.fill: parent
                    visible: captureSession.recorder.recorderState === MediaRecorder.RecordingState
                    onClicked: captureSession.recorder.stop()
                }
            }

            Item {
                implicitWidth: buttonsWidth
                height: 70
                CameraButton {
                    text: "View"
                    anchors.fill: parent
                    onClicked: captureControls.previewSelected()
                    //don't show View button during recording
                    visible: captureSession.recorder.actualLocation && !stopButton.visible
                }
            }
        }

        GridLayout {
            id: bottomColumn
            anchors.margins: buttonsmargin
            flow: captureControls.state === "MobilePortrait"
                  ? GridLayout.LeftToRight : GridLayout.TopToBottom

            CameraListButton {
                implicitWidth: buttonsWidth
                onValueChanged: captureSession.camera.cameraDevice = value
                state: captureControls.state
            }

            CameraButton {
                text: "Switch to Photo"
                implicitWidth: buttonsWidth
                onClicked: captureControls.photoModeSelected()
            }

            CameraButton {
                id: quitButton
                text: "Quit"
                implicitWidth: buttonsWidth
                onClicked: Qt.quit()
            }
        }
    }

    ZoomControl {
        x : 0
        y : captureControls.state === "MobilePortrait" ? -buttonPaneShadow.height : 0
        width : 100
        height: parent.height

        currentZoom: captureSession.camera.zoomFactor
        maximumZoom: captureSession.camera.maximumZoomFactor
        onZoomTo: captureSession.camera.zoomFactor = target
    }

    states: [
        State {
            name: "MobilePortrait"
            PropertyChanges {
                target: buttonPaneShadow
                width: parent.width
                height: buttonsPanelPortraitHeight
            }
            PropertyChanges {
                target: buttonsColumn
                height: captureControls.buttonsPanelPortraitHeight / 2 - buttonsmargin
            }
            PropertyChanges {
                target: bottomColumn
                height: captureControls.buttonsPanelPortraitHeight / 2 - buttonsmargin
            }
            AnchorChanges {
                target: buttonPaneShadow
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
            }
            AnchorChanges {
                target: buttonsColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
            }
            AnchorChanges {
                target: bottomColumn;
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
            }
        },
        State {
            name: "MobileLandscape"
            PropertyChanges {
                target: buttonPaneShadow
                width: buttonsPanelWidth
                height: parent.height
            }
            PropertyChanges {
                target: buttonsColumn
                height: parent.height
                width: buttonPaneShadow.width / 2
            }
            PropertyChanges {
                target: bottomColumn
                height: parent.height
                width: buttonPaneShadow.width / 2
            }
            AnchorChanges {
                target: buttonPaneShadow
                anchors.top: parent.top
                anchors.right: parent.right
            }
            AnchorChanges {
                target: buttonsColumn;
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left;
            }
            AnchorChanges {
                target: bottomColumn;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: parent.right;
            }
        },
        State {
            name: "Other"
            PropertyChanges {
                target: buttonPaneShadow;
                width: bottomColumn.width + 16;
                height: parent.height;
            }
            AnchorChanges {
                target: buttonPaneShadow;
                anchors.top: parent.top;
                anchors.right: parent.right;
            }
            AnchorChanges {
                target: buttonsColumn;
                anchors.top: parent.top
                anchors.right: parent.right
            }
            AnchorChanges {
                target: bottomColumn;
                anchors.bottom: parent.bottom;
                anchors.right: parent.right;
            }
        }
    ]
}
