/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia

ColumnLayout {
    required property MediaRecorder recorder

    Text { text: "Metadata settings" }

    ListModel { id: metaDataModel }

    Connections {
        target: recorder
        function onMetaDataChanged() {
            metaDataModel.clear()
            for (var key of recorder.metaData.keys()) {
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
                    recorder.metaData.insert(metaDataType.currentValue, text)
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
                    text: recorder.metaData.stringValue(r.value)
                    onAccepted: recorder.metaData.insert(r.value, text)

                }
            }
            Button {
                width: Style.widthShort
                height: Style.height
                text: "del"
                font.pointSize: Style.fontSize
                background: StyleRectangle { anchors.fill: parent }
                onClicked: { recorder.metaData.remove(r.value); recorder.metaDataChanged() }
            }
        }
    }
}
