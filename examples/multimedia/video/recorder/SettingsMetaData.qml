// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia

pragma ComponentBehavior: Bound

ColumnLayout {
    id: root
    required property MediaRecorder recorder

    Text { text: "Metadata settings" }

    ListModel { id: metaDataModel }

    Connections {
        target: root.recorder
        function onMetaDataChanged() {
            metaDataModel.clear()
            for (var key of root.recorder.metaData.keys()) {
                if (recorder.metaData.stringValue(key))
                    metaDataModel.append(
                                { text: recorder.metaData.metaDataKeyToString(key)
                                , value: key })
            }
        }
    }

    Row {
        id: metaDataAdd
        spacing: Style.intraSpacing
        ComboBox {
            id: metaDataType
            width: Style.widthMedium
            height: Style.height
            font.pointSize: Style.fontSize
            model: ListModel {
                ListElement { text: "Title"; value: MediaMetaData.Title }
                ListElement { text: "Author"; value: MediaMetaData.Author }
                ListElement { text: "Comment"; value: MediaMetaData.Comment }
                ListElement { text: "Description"; value: MediaMetaData.Description }
                ListElement { text: "Genre"; value: MediaMetaData.Genre }
                ListElement { text: "Publisher"; value: MediaMetaData.Publisher }
                ListElement { text: "Copyright"; value: MediaMetaData.Copyright }
                ListElement { text: "Date"; value: MediaMetaData.Date }
                ListElement { text: "Url"; value: MediaMetaData.Url }
                ListElement { text: "MediaType"; value: MediaMetaData.MediaType }
                ListElement { text: "AlbumTitle"; value: MediaMetaData.AlbumTitle }
                ListElement { text: "AlbumArtist"; value: MediaMetaData.AlbumArtist }
                ListElement { text: "ContributingArtist"; value: MediaMetaData.ContributingArtist }
                ListElement { text: "Composer"; value: MediaMetaData.Composer }
                ListElement { text: "LeadPerformer"; value: MediaMetaData.LeadPerformer }
            }
            textRole: "text"
            valueRole: "value"
            background: StyleRectangle { anchors.fill: parent; width: metaDataType.width }
        }
        Item {
            width: Style.widthMedium
            height: Style.height
            StyleRectangle { anchors.fill: parent }
            TextInput {
                id: textInput
                anchors.fill: parent
                anchors.bottom: parent.bottom
                anchors.margins: 4
                font.pointSize: Style.fontSize
                clip: true
                onAccepted: {
                    root.recorder.metaData.insert(metaDataType.currentValue, text)
                    recorder.metaDataChanged()
                    text = ""
                    textInput.deselect()
                }
            }
        }
        Button {
            width: Style.widthShort
            height: Style.height
            text: "add"
            font.pointSize: Style.fontSize

            background: StyleRectangle { anchors.fill: parent }
            onClicked: textInput.accepted()
        }
    }

    ListView {
        id: listView
        Layout.fillHeight: true
        Layout.minimumWidth: metaDataAdd.width
        spacing: Style.intraSpacing
        clip: true
        model: metaDataModel

        delegate: Row {
            id: r
            height: Style.height
            spacing: Style.intraSpacing

            required property string text
            required property string value

            Text {
                width: Style.widthShort
                height: Style.height
                text: r.text
                font.pointSize: Style.fontSize

                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }
            Item {
                width: Style.widthMedium
                height: Style.height

                StyleRectangle { anchors.fill: parent }
                TextInput {
                    anchors.fill: parent
                    anchors.margins: 4
                    anchors.bottom: parent.bottom
                    font.pointSize: Style.fontSize
                    clip: true
                    text: root.recorder.metaData.stringValue(r.value)
                    onAccepted: root.recorder.metaData.insert(r.value, text)

                }
            }
            Button {
                width: Style.widthShort
                height: Style.height
                text: "del"
                font.pointSize: Style.fontSize
                background: StyleRectangle { anchors.fill: parent }
                onClicked: { root.recorder.metaData.remove(r.value); recorder.metaDataChanged() }
            }
        }
    }
}
