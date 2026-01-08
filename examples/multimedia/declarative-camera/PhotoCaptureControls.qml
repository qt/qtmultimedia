// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtMultimedia
import QtQuick
import QtQuick.Layouts

FocusScope {
    id : captureControls
    required property CaptureSession captureSession
    readonly property Camera camera: captureSession.camera
    required property bool previewAvailable

    property int buttonsmargin: 8
    required property int buttonsPanelWidth
    required property int buttonsPanelPortraitHeight
    required property int buttonsWidth

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
            //! [0]
            CameraButton {
                text: "Capture"
                implicitWidth: captureControls.buttonsWidth
                visible: captureControls.captureSession.imageCapture.readyForCapture
                onClicked: captureControls.captureSession.imageCapture.captureToFile("")
            }
            //! [0]

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

            Text {
                text: "White balance"
                font.pixelSize: 14
                font.bold: true
                color: "white"
                visible: whiteBalanceComboBox.model.length > 1
            }

            CameraWhiteBalanceButton {
                id: whiteBalanceComboBox

                Layout.preferredWidth: captureControls.buttonsWidth
                camera: captureControls.camera

                visible: model.length > 1
            }
        }

        GridLayout {
            id: bottomColumn
            anchors.margins: captureControls.buttonsmargin
            flow: captureControls.state === "MobilePortrait"
                  ? GridLayout.LeftToRight : GridLayout.TopToBottom

            Text {
                text: "Device"
                font.pixelSize: 14
                font.bold: true
                color: "white"
                visible: cameraDeviceComboBox.model.length > 1
            }
            CameraListButton {
                id: cameraDeviceComboBox

                Layout.preferredWidth: captureControls.buttonsWidth
                camera: captureControls.camera

                visible: model.length > 1
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

        currentZoom: captureControls.camera.zoomFactor
        maximumZoom: captureControls.camera.maximumZoomFactor
        minimumZoom: captureControls.camera.minimumZoomFactor
        onZoomTo: (target) => captureControls.camera.zoomFactor = target
    }

    FlashControl {
        id: flashControl
        x : 10
        y : captureControls.state === "MobilePortrait" ?
                parent.height - (buttonPaneShadow.height + height) : parent.height - height

        camera: captureControls.camera
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
