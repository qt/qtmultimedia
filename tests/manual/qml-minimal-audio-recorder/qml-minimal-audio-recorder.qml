// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtMultimedia

pragma ComponentBehavior: Bound

ApplicationWindow {
    id: window
    width: 400
    height: 300
    visible: true
    title: qsTr("QmlMinimalAudioRecorder")

    CaptureSession {
        id: session
        audioInput: AudioInput{}
        recorder: MediaRecorder {
            id: recorder
            outputLocation: StandardPaths.standardLocations(StandardPaths.MusicLocation)[0] + "/" + "recording"
            onActualLocationChanged: (url) => {
                status.text = url
            }
            onErrorOccurred: (error, errorString) => {
                status.text = errorString
            }
        }
    }

    MediaDevices {
        id: mediaDevices
    }

    menuBar: MenuBar{
        Menu {
            title: qsTr ("&File")
            Action {
                text: qsTr("&Open...")
                onTriggered: fileDialog.open()
            }
        }
        Menu {
            id: audioInputMenu
            title: qsTr("&Input")

            Instantiator {
                id: audioDevices
                model: mediaDevices.audioInputs
                delegate: Action {
                    required property var modelData
                    readonly property var device: modelData
                    checkable: true
                    // Compare string representation of object id to overcome defect in
                    // QAudioInput's comparison operator
                    checked: String(session.audioInput.device.id) === String(device.id)
                    text: device.description
                    onTriggered: session.audioInput.device = device
                }
                onObjectAdded: (index, object) => audioInputMenu.insertAction(index, object)
                onObjectRemoved: (index, object) => audioInputMenu.removeAction(object)
            }
        }
    }

    Button {
        anchors.centerIn: parent
        text: qsTr("Record")
        width: 120
        height: 80
        onClicked: recorder.record()
        visible: recorder.recorderState !== MediaRecorder.RecordingState
    }

    Button {
        anchors.centerIn: parent
        text: qsTr("Stop")
        width: 120
        height: 80
        onClicked: recorder.stop()
        visible: recorder.recorderState === MediaRecorder.RecordingState
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label {
                id: status
                text: "Not recording"
            }
            Label {
                text: qsTr("Duration: %1").arg(recorder.duration)
            }
        }
    }

    FileDialog {
        id: fileDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.MusicLocation)[0]
        onAccepted: recorder.outputLocation = selectedFile
        fileMode: FileDialog.SaveFile
    }
}
