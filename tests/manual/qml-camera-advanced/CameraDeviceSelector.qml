// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    required property CameraHelper camera
    required property MediaDevicesHelper mediaDevices
    enabled: Object.values(mediaDevices.allCameraDevicesSoFarDict).length > 0

    Label { text: "Device" }
    ComboBox {
        implicitContentWidthPolicy: ComboBox.WidestText
        model: Object.values(mediaDevices.allCameraDevicesSoFarDict)
            .map((item) => { return {
                displayText: item.device.description + (item.connected ? "" : " (Disconnected)"),
                device: item.device } })
        valueRole: "device"
        textRole: "displayText"
        onActivated: index => {
            camera.cameraDevice = currentValue
        }
        currentIndex: model.findIndex(
            item => String(item.device.id) === String(camera.cameraDevice.id))
    }
    Button {
        text: "Default"
        onClicked: camera.cameraDevice = mediaDevices.defaultVideoInput
    }
}
