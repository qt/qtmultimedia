// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtMultimedia

Item {
    id: cameraListButton
    property alias value : popup.currentValue
    property alias model : popup.model

    width : 144
    height: 70
    visible: model.length > 0

    BorderImage {
        id: buttonImage
        source: "images/toolbutton.sci"
        width: cameraListButton.width; height: cameraListButton.height
    }

    CameraButton {
        anchors.fill: parent
        text: popup.currentItem !== null ? popup.currentItem.description : ""

        onClicked: popup.toggle()
    }

    CameraListPopup {
        id: popup
        anchors.rightMargin: 16
        visible: opacity > 0
        model: mediaDevices.videoInputs

        MediaDevices {
            id: mediaDevices
        }

        onSelected: popup.toggle()
    }

    states: [
        State {
            name: "MobilePortrait"
            AnchorChanges {
                target: popup
                // qmllint disable incompatible-type
                anchors.bottom: cameraListButton.top
                anchors.left: cameraListButton.left
                // qmllint enable incompatible-type
            }
        },
        State {
            name: "MobileLandscape"
            AnchorChanges {
                target: popup
                // qmllint disable incompatible-type
                anchors.top: cameraListButton.top
                anchors.right: cameraListButton.left
                // qmllint enable incompatible-type
            }
        },
        State {
            name: "Other"
            AnchorChanges {
                target: popup
                // qmllint disable incompatible-type
                anchors.top: cameraListButton.top
                anchors.right: cameraListButton.left
                // qmllint enable incompatible-type
            }
        }
    ]
}
