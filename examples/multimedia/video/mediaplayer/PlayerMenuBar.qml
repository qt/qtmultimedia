// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtMultimedia

Item {
    id: root

    required property MediaPlayer mediaPlayer
    required property VideoOutput videoOutput
    required property MetadataInfo metadataInfo
    required property TracksInfo audioTracksInfo
    required property TracksInfo videoTracksInfo
    required property TracksInfo subtitleTracksInfo

    height: menuBar.height

    signal closePlayer

    function loadUrl(url) {
        mediaPlayer.stop()
        mediaPlayer.source = url
        mediaPlayer.play()
    }

    function closeOverlays(){
        metadataInfo.visible = false;
        audioTracksInfo.visible = false;
        videoTracksInfo.visible = false;
        subtitleTracksInfo.visible = false;
    }

    function showOverlay(overlay){
        closeOverlays();
        overlay.visible = true;
    }

    Popup {
        id: urlPopup
        anchors.centerIn: Overlay.overlay

        RowLayout {
            id: rowOpenUrl
            Label {
                text: qsTr("URL:");
            }

            TextInput {
                id: urlText
                focus: true
                Layout.minimumWidth: 400
                wrapMode: TextInput.WrapAnywhere
                Keys.onReturnPressed: { loadUrl(text); urlText.text = ""; urlPopup.close() }
            }

            Button {
                text: "Load"
                onClicked: { loadUrl(urlText.text); urlText.text = ""; urlPopup.close() }
            }
        }
        onOpened: { urlPopup.forceActiveFocus() }
    }

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        onAccepted: {
            mediaPlayer.stop()
            mediaPlayer.source = fileDialog.currentFile
            mediaPlayer.play()
        }
    }

    MenuBar {
        id: menuBar
        anchors.left: parent.left
        anchors.right: parent.right

        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Open")
                onTriggered: fileDialog.open()
            }
            Action {
                text: qsTr("&URL");
                onTriggered: urlPopup.open()
            }

            Action {
                text: qsTr("&Exit");
                onTriggered: closePlayer()
            }
        }

        Menu {
            title: qsTr("&View")
            Action {
                text: qsTr("Metadata")
                onTriggered: showOverlay(metadataInfo)
            }
        }

        Menu {
            title: qsTr("&Tracks")
            Action {
                text: qsTr("Audio")
                onTriggered: showOverlay(audioTracksInfo)
            }
            Action {
                text: qsTr("Video")
                onTriggered: showOverlay(videoTracksInfo)
            }
            Action {
                text: qsTr("Subtitles")
                onTriggered: showOverlay(subtitleTracksInfo)
            }
        }
    }
}
