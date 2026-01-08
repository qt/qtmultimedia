// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Controls.Material

import QtMultimedia
import QtQuick
import QtQuick.Controls

ComboBox {
    required property Camera camera

    Material.theme: Material.Dark

    MediaDevices {
        id: mediaDevices
    }

    model: mediaDevices.videoInputs
        .map((device) => { return {
            value: device,
            displayText: device.description } })
    valueRole: "value"
    textRole: "displayText"
    visible: model.length > 1
    font.pixelSize: 14
    font.bold: true
    onActivated: {
        camera.cameraDevice = currentValue
    }
    currentIndex: model.findIndex(item => item.value === camera.cameraDevice)
}
