// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

// Lists every audio output reported during this session, including the ones that have
// been disconnected since.
ColumnLayout {
    id: root

    required property MediaDevicesHelper mediaDevices
    required property AudioOutput audioOutput

    readonly property var audioOutputs: Object.values(mediaDevices.allAudioOutputsSoFarDict)

    Label {
        text: "Audio outputs (" + root.audioOutputs.length + ")"
        font.bold: true
    }

    Label {
        visible: root.audioOutputs.length === 0
        text: "No audio outputs"
    }

    Repeater {
        model: root.audioOutputs

        delegate: Frame {
            id: deviceFrame

            required property var modelData

            readonly property bool isActive:
                String(modelData.device.id) === String(root.audioOutput.device.id)

            // Lists the states that apply, such as "Default · Active".
            function buildStatusString() {
                const states = []
                if (modelData.device.isDefault)
                    states.push("Default")
                if (isActive)
                    states.push("Active")
                if (!modelData.connected)
                    states.push("Disconnected")
                return states.join(" · ")
            }

            Layout.fillWidth: true
            opacity: modelData.connected ? 1.0 : 0.5

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                Label {
                    Layout.fillWidth: true
                    text: deviceFrame.modelData.device.description !== ""
                        ? deviceFrame.modelData.device.description
                        : "No description"
                    font.bold: deviceFrame.isActive
                    wrapMode: Text.WordWrap
                }

                Label {
                    visible: text !== ""
                    text: deviceFrame.buildStatusString()
                    font.italic: true
                }

                Label {
                    Layout.fillWidth: true
                    text: "ID: " + deviceFrame.modelData.device.id
                    font.pointSize: Application.font.pointSize * 0.85
                    wrapMode: Text.WrapAnywhere
                }
            }
        }
    }
}
