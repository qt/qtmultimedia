// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQuick
import QtMultimedia

Popup {
    id: cameraListPopup

    property alias model : view.model
    property cameraDevice currentValue
    property cameraDevice currentItem : model[view.currentIndex]

    property int itemWidth : 200
    property int itemHeight : 50

    width: itemWidth + view.anchors.margins*2
    height: view.count * itemHeight + view.anchors.margins*2

    signal selected

    ListView {
        id: view
        anchors.fill: parent
        anchors.margins: 5
        snapMode: ListView.SnapOneItem
        highlightFollowsCurrentItem: true
        highlight: Rectangle { color: "gray"; radius: 5 }
        currentIndex: 0
        clip: true

        delegate: Item {
            id: cameraListItem

            required property int index
            required property cameraDevice modelData

            width: cameraListPopup.itemWidth
            height: cameraListPopup.itemHeight

            Text {
                text: cameraListItem.modelData.description

                anchors.fill: parent
                anchors.margins: 5
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                color: "white"
                font.bold: true
                style: Text.Raised
                styleColor: "black"
                font.pixelSize: 14
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    view.currentIndex = cameraListItem.index
                    cameraListPopup.currentValue = cameraListItem.modelData
                    cameraListPopup.selected()
                }
            }
        }
    }
}
