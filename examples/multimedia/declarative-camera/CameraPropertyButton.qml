// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

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
                // qmllint disable incompatible-type
                anchors.bottom: propertyButton.top
                anchors.left: propertyButton.left
                // qmllint enable incompatible-type
            }
        },
        State {
            name: "MobileLandscape"
            AnchorChanges {
                target: popup
                // qmllint disable incompatible-type
                anchors.verticalCenter: propertyButton.top
                anchors.right: propertyButton.left
                // qmllint enable incompatible-type
            }
        },
        State {
            name: "Other"
            AnchorChanges {
                target: popup
                // qmllint disable incompatible-type
                anchors.top: propertyButton.top
                anchors.right: propertyButton.left
                // qmllint enable incompatible-type
            }
        }
    ]
}
