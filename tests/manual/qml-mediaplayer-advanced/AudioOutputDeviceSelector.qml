// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

RowLayout {
    id: root
    required property AudioOutput audioOutput
    required property MediaDevicesHelper mediaDevices
    enabled: Object.values(mediaDevices.allAudioOutputsSoFarDict).length > 0

    Label { text: "Device" }
    ComboBox {
        implicitContentWidthPolicy: ComboBox.WidestText
        model: Object.values(root.mediaDevices.allAudioOutputsSoFarDict)
            .map((item) => { return {
                displayText: item.device.description + (item.connected ? "" : " (Disconnected)"),
                device: item.device } })
        valueRole: "device"
        textRole: "displayText"
        currentValue: root.audioOutput.device
        onActivated: () => {
            root.audioOutput.device = currentValue
        }
    }
    Button {
        text: "Default"
        onClicked: root.audioOutput.device = root.mediaDevices.defaultAudioOutput
    }
}
