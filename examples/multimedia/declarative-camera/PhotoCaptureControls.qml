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
    signal videoModeSelected

    Rectangle {
        id: buttonPaneShadow
        color: Qt.rgba(0.08, 0.08, 0.08, 1)

        GridLayout {
            id: buttonsColumn
            anchors.margins: buttonsmargin
            flow: captureControls.state === "MobilePortrait"
                  ? GridLayout.LeftToRight : GridLayout.TopToBottom
            CameraButton {
                text: "Capture"
                implicitWidth: buttonsWidth
                visible: captureSession.imageCapture.readyForCapture
                onClicked: captureSession.imageCapture.captureToFile("")
            }

            CameraPropertyButton {
                id : wbModesButton
                implicitWidth: buttonsWidth
                state: captureControls.state
                value: Camera.WhiteBalanceAuto
                model: ListModel {
                    ListElement {
                        icon: "images/camera_auto_mode.png"
                        value: Camera.WhiteBalanceAuto
                        text: "Auto"
                    }
                    ListElement {
                        icon: "images/camera_white_balance_sunny.png"
                        value: Camera.WhiteBalanceSunlight
                        text: "Sunlight"
                    }
                    ListElement {
                        icon: "images/camera_white_balance_cloudy.png"
                        value: Camera.WhiteBalanceCloudy
                        text: "Cloudy"
                    }
                    ListElement {
                        icon: "images/camera_white_balance_incandescent.png"
                        value: Camera.WhiteBalanceTungsten
                        text: "Tungsten"
                    }
                    ListElement {
                        icon: "images/camera_white_balance_flourescent.png"
                        value: Camera.WhiteBalanceFluorescent
                        text: "Fluorescent"
                    }
                }
                onValueChanged: captureControls.captureSession.camera.whiteBalanceMode = wbModesButton.value
            }

            Item {
                implicitWidth: buttonsWidth
                height: 70
                CameraButton {
                    text: "View"
                    anchors.fill: parent
                    onClicked:state = captureControls.previewSelected()
                    visible: captureControls.previewAvailable
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
                state: captureControls.state
                onValueChanged: captureSession.camera.cameraDevice = value
            }

            CameraButton {
                text: "Switch to Video"
                implicitWidth: buttonsWidth
                onClicked: captureControls.videoModeSelected()
            }

            CameraButton {
                id: quitButton
                implicitWidth: buttonsWidth
                text: "Quit"
                onClicked: Qt.quit()
            }
        }
    }

    ZoomControl {
        x : 0
        y : captureControls.state === "MobilePortrait" ? -buttonPaneShadow.height : 0
        width : 100
        height: parent.height

        currentZoom: camera.zoomFactor
        maximumZoom: camera.maximumZoomFactor
        onZoomTo: camera.zoomFactor = value
    }

    states: [
        State {
            name: "MobilePortrait"
            PropertyChanges {
                target: buttonPaneShadow
                width: parent.width
                height: captureControls.buttonsPanelPortraitHeight
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
                target: bottomColumn
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
                target: buttonsColumn
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
            }
            AnchorChanges {
                target: bottomColumn
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
            }
        },
        State {
            name: "Other"
            PropertyChanges {
                target: buttonPaneShadow
                width: bottomColumn.width + 16
                height: parent.height
            }
            AnchorChanges {
                target: buttonPaneShadow
                anchors.top: parent.top
                anchors.right: parent.right
            }
            AnchorChanges {
                target: buttonsColumn
                anchors.top: parent.top
                anchors.right: parent.right
            }
            AnchorChanges {
                target: bottomColumn
                anchors.bottom: parent.bottom
                anchors.right: parent.right
            }
        }
    ]
}
