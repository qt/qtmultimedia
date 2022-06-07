// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtMultimedia

Item {
    id: propertyButton
    property alias value : popup.currentValue
    property alias model : popup.model

    width : 144
    height: 70

    BorderImage {
        id: buttonImage
        source: "images/toolbutton.sci"
        width: propertyButton.width; height: propertyButton.height
    }

    CameraButton {
        anchors.fill: parent
        Image {
            anchors.centerIn: parent
            source: popup.currentItem.icon
        }

        onClicked: popup.toggle()
    }

    CameraPropertyPopup {
        id: popup
        anchors.rightMargin: 16
        visible: opacity > 0

        currentValue: propertyButton.value

        onSelected: popup.toggle()
    }

    states: [
        State {
            name: "MobilePortrait"
            AnchorChanges {
                target: popup
                anchors.bottom: parent.top;
            }
        },
        State {
            name: "MobileLandscape"
            AnchorChanges {
                target: popup
                anchors.verticalCenter: parent.top;
                anchors.right: parent.left;
            }
        },
        State {
            name: "Other"
            AnchorChanges {
                target: popup
                anchors.top: parent.top;
                anchors.right: parent.left;
            }
        }
    ]
}
