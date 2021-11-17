/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
        onZoomTo: camera.setDigitalZoom(value)
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
