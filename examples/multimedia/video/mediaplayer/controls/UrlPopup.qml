// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia


Popup {
    id: popupController
    width: Math.min(500, root.width - 40)

    required property MediaPlayer mediaPlayer

    function loadUrl(url) {
        popupController.mediaPlayer.stop()
        popupController.mediaPlayer.source = url
        popupController.mediaPlayer.play()
    }

    RowLayout {
        id: rowOpenUrl
        anchors.fill: parent
        Label {
            text: qsTr("URL:");
        }

        TextField {
            id: urlText
            Layout.fillWidth: true
            focus: true

            placeholderText:  qsTr("Enter text here...")
            wrapMode: TextInput.WrapAnywhere

            Keys.onReturnPressed: {
                popupController.loadUrl(text)
                urlText.text = ""
                popupController.close()
            }
        }

        Button {
            text: qsTr("Load")
            enabled: urlText.text !== ""
            onClicked: {
                popupController.loadUrl(urlText.text)
                urlText.text = ""
                popupController.close()
            }
        }
    }
    onOpened: { popupController.forceActiveFocus() }
}
