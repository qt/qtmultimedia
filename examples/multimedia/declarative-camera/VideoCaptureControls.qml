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
            anchors.margins: captureControls.buttonsmargin
            flow: captureControls.state === "MobilePortrait"
                  ? GridLayout.LeftToRight : GridLayout.TopToBottom
            Item {
                implicitWidth: captureControls.buttonsWidth
                implicitHeight: 70
                CameraButton {
                    text: "Record"
                    anchors.fill: parent
                    visible: captureControls.captureSession.recorder.recorderState !== MediaRecorder.RecordingState
                    onClicked: captureControls.captureSession.recorder.record()
                }
            }

            Item {
                implicitWidth: captureControls.buttonsWidth
                implicitHeight: 70
                CameraButton {
                    id: stopButton
                    text: "Stop"
                    anchors.fill: parent
                    visible: captureControls.captureSession.recorder.recorderState === MediaRecorder.RecordingState
                    onClicked: captureControls.captureSession.recorder.stop()
                }
            }

            Item {
                implicitWidth: captureControls.buttonsWidth
                implicitHeight: 70
                CameraButton {
                    text: "View"
                    anchors.fill: parent
                    onClicked: captureControls.previewSelected()
                    //don't show View button during recording
                    visible: captureControls.captureSession.recorder.actualLocation && !stopButton.visible
                }
            }
        }

        GridLayout {
            id: bottomColumn
            anchors.margins: captureControls.buttonsmargin
            flow: captureControls.state === "MobilePortrait"
                  ? GridLayout.LeftToRight : GridLayout.TopToBottom

            CameraListButton {
                implicitWidth: captureControls.buttonsWidth
                onValueChanged: captureControls.captureSession.camera.cameraDevice = value
                state: captureControls.state
            }

            CameraButton {
                text: "Switch to Photo"
                implicitWidth: captureControls.buttonsWidth
                onClicked: captureControls.photoModeSelected()
            }

            CameraButton {
                id: quitButton
                text: "Quit"
                implicitWidth: captureControls.buttonsWidth
                onClicked: Qt.quit()
            }
        }
    }

    ZoomControl {
        x : 0
        y : captureControls.state === "MobilePortrait" ? -buttonPaneShadow.height : 0
        width : 100
        height: parent.height

        currentZoom: captureControls.captureSession.camera.zoomFactor
        maximumZoom: captureControls.captureSession.camera.maximumZoomFactor
        onZoomTo: (target) => captureControls.captureSession.camera.zoomFactor = target
    }

    FlashControl {
        x : 10
        y : captureControls.state === "MobilePortrait" ?
                parent.height - (buttonPaneShadow.height + height) : parent.height - height

        cameraDevice: captureControls.captureSession.camera
    }

    states: [
        State {
            name: "MobilePortrait"
            PropertyChanges {
                buttonPaneShadow.width: parent.width
                buttonPaneShadow.height: buttonsPanelPortraitHeight
                buttonsColumn.height: captureControls.buttonsPanelPortraitHeight / 2 - buttonsmargin
                bottomColumn.height: captureControls.buttonsPanelPortraitHeight / 2 - buttonsmargin
            }
            AnchorChanges {
                target: buttonPaneShadow
                // qmllint disable incompatible-type
                anchors.bottom: captureControls.bottom
                anchors.left: captureControls.left
                anchors.right: captureControls.right
                // qmllint enable incompatible-type
            }
            AnchorChanges {
                target: buttonsColumn
                // qmllint disable incompatible-type
                anchors.left: buttonPaneShadow.left
                anchors.right: buttonPaneShadow.right
                anchors.top: buttonPaneShadow.top
                // qmllint enable incompatible-type
            }
            AnchorChanges {
                target: bottomColumn
                // qmllint disable incompatible-type
                anchors.bottom: buttonPaneShadow.bottom
                anchors.left: buttonPaneShadow.left
                anchors.right: buttonPaneShadow.right
                // qmllint enable incompatible-type
            }
        },
        State {
            name: "MobileLandscape"
            PropertyChanges {
                buttonPaneShadow.width: buttonsPanelWidth
                buttonPaneShadow.height: parent.height
                buttonsColumn.height: parent.height
                buttonsColumn.width: buttonPaneShadow.width / 2
                bottomColumn.height: parent.height
                bottomColumn.width: buttonPaneShadow.width / 2
            }
            AnchorChanges {
                target: buttonPaneShadow
                // qmllint disable incompatible-type
                anchors.top: captureControls.top
                anchors.right: captureControls.right
                // qmllint enable incompatible-type
            }
            AnchorChanges {
                target: buttonsColumn
                // qmllint disable incompatible-type
                anchors.top: buttonPaneShadow.top
                anchors.bottom: buttonPaneShadow.bottom
                anchors.left: buttonPaneShadow.left
                // qmllint enable incompatible-type
            }
            AnchorChanges {
                target: bottomColumn
                // qmllint disable incompatible-type
                anchors.top: buttonPaneShadow.top
                anchors.bottom: buttonPaneShadow.bottom
                anchors.right: buttonPaneShadow.right
                // qmllint enable incompatible-type
            }
        },
        State {
            name: "Other"
            PropertyChanges {
                buttonPaneShadow.width: bottomColumn.width + 16
                buttonPaneShadow.height: parent.height
            }
            AnchorChanges {
                target: buttonPaneShadow
                // qmllint disable incompatible-type
                anchors.top: captureControls.top
                anchors.right: captureControls.right
                // qmllint enable incompatible-type
            }
            AnchorChanges {
                target: buttonsColumn
                // qmllint disable incompatible-type
                anchors.top: buttonPaneShadow.top
                anchors.right: buttonPaneShadow.right
                // qmllint enable incompatible-type
            }
            AnchorChanges {
                target: bottomColumn
                // qmllint disable incompatible-type
                anchors.bottom: buttonPaneShadow.bottom
                anchors.right: buttonPaneShadow.right
                // qmllint enable incompatible-type
            }
        }
    ]
}
