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
            anchors.margins: captureControls.buttonsmargin
            flow: captureControls.state === "MobilePortrait"
                  ? GridLayout.LeftToRight : GridLayout.TopToBottom
            CameraButton {
                text: "Capture"
                implicitWidth: captureControls.buttonsWidth
                visible: captureControls.captureSession.imageCapture.readyForCapture
                onClicked: captureControls.captureSession.imageCapture.captureToFile("")
            }

            CameraPropertyButton {
                id : wbModesButton
                implicitWidth: captureControls.buttonsWidth
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
                implicitWidth: captureControls.buttonsWidth
                implicitHeight: 70
                CameraButton {
                    text: "View"
                    anchors.fill: parent
                    onClicked: captureControls.previewSelected()
                    visible: captureControls.previewAvailable
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
                state: captureControls.state
                onValueChanged: captureControls.captureSession.camera.cameraDevice = value
            }

            CameraButton {
                text: "Switch to Video"
                implicitWidth: captureControls.buttonsWidth
                onClicked: captureControls.videoModeSelected()
            }

            CameraButton {
                id: quitButton
                implicitWidth: captureControls.buttonsWidth
                text: "Quit"
                onClicked: Qt.quit()
            }
        }
    }

    ZoomControl {
        id: zoomControl
        x : 0
        y : 0
        width : 100
        height: parent.height - (flashControl.visible * flashControl.height) -
                (captureControls.state === "MobilePortrait" ? buttonPaneShadow.height : 0)

        currentZoom: captureControls.captureSession.camera.zoomFactor
        maximumZoom: captureControls.captureSession.camera.maximumZoomFactor
        minimumZoom: captureControls.captureSession.camera.minimumZoomFactor
        onZoomTo: (target) => captureControls.captureSession.camera.zoomFactor = target
    }

    FlashControl {
        id: flashControl
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
                buttonPaneShadow.height: captureControls.buttonsPanelPortraitHeight
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
