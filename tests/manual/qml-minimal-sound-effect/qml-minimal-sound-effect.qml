// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtMultimedia

ApplicationWindow {
    id: window
    width: 400
    height: 300
    visible: true
    title: qsTr("QmlMinimalSoundEffect")

    MediaDevices {
        id: mediaDevices
    }

    SoundEffect {
        id: effect
        audioDevice: mediaDevices.defaultAudioOutput
        source: "qrc:/triple-click.wav"
    }

    menuBar: MenuBar{
        Menu {
            title: qsTr ("&File")
            Action {
                text: qsTr("&Open...")
                onTriggered: fileDialog.open()
            }
            Action {
                text: qsTr("&Triple click sound")
                onTriggered: effect.source = "qrc:/triple-click.wav"
            }
            Action {
                text: qsTr("&Double drop sound")
                onTriggered: effect.source = "qrc:/double-drop.wav"
            }
        }
        Menu {
            id: audioOutputMenu
            title: qsTr("&Output")

            Instantiator {
                id: audioDevices
                model: mediaDevices.audioOutputs
                delegate: MenuItem {
                    checkable: true
                    checked: effect.audioDevice == device
                    autoExclusive: true
                    property var device: model.modelData
                    text: device.description
                    onTriggered: effect.audioDevice = device
                }
                onObjectAdded: (index, object) => audioOutputMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => audioOutputMenu.removeItem(object)
            }
        }
    }

    Button {
        text: qsTr("Play")
        width: 120
        height: 80
        anchors.centerIn: parent
        onClicked: effect.play()
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label {
                text: effect.source
            }
            Label {
                text: qsTr("Failed loading file")
                visible: effect.status == SoundEffect.Error
            }
        }
    }

    FileDialog {
        id: fileDialog
        nameFilters: ["waw files (*.wav)"]
        currentFolder: StandardPaths.standardLocations(StandardPaths.MusicLocation)[0]
        onAccepted: effect.source = selectedFile
    }
}
