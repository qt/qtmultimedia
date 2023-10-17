// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQuick

Popup {
    id: propertyPopup

    property alias model : view.model
    property var currentValue
    property var currentItem : model.get(view.currentIndex)

    property int itemWidth : 100
    property int itemHeight : 70
    property int columns : 2

    width: columns*itemWidth + view.anchors.margins*2
    height: Math.ceil(model.count/columns)*itemHeight + view.anchors.margins*2 + 25

    signal selected

    function indexForValue(value) {
        for (var i = 0; i < view.count; i++) {
            if (model.get(i).value == value) {
                return i;
            }
        }

        return 0;
    }

    GridView {
        id: view
        anchors.fill: parent
        anchors.margins: 5
        cellWidth: propertyPopup.itemWidth
        cellHeight: propertyPopup.itemHeight
        snapMode: ListView.SnapOneItem
        highlightFollowsCurrentItem: true
        highlight: Rectangle { color: "gray"; radius: 5 }
        currentIndex: propertyPopup.indexForValue(propertyPopup.currentValue)

        delegate: Item {
            id: propertyItem

            required property url icon
            required property var value

            width: propertyPopup.itemWidth
            height: 70

            Image {
                anchors.centerIn: parent
                source: propertyItem.icon
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    propertyPopup.currentValue = propertyItem.value
                    propertyPopup.selected()
                }
            }
        }
    }

    Text {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        anchors.left: parent.left
        anchors.leftMargin: 16

        color: "#ffffff"
        font.bold: true
        style: Text.Raised;
        styleColor: "black"
        font.pixelSize: 14

        text: view.model.get(view.currentIndex).text
    }
}
