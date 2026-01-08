// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Controls.Material

import QtMultimedia
import QtQuick
import QtQuick.Controls

ComboBox {
    id: root

    Material.theme: Material.Dark

    required property Camera camera

    // This list is not exhaustive. Contains only the modes
    // supported in this example.
    readonly property var allWhiteBalanceModes: [
        Camera.WhiteBalanceAuto,
        Camera.WhiteBalanceCloudy,
        Camera.WhiteBalanceFluorescent,
        Camera.WhiteBalanceSunlight,
        Camera.WhiteBalanceTungsten, ]
    property var supportedWhiteBalanceModes:
        allWhiteBalanceModes.filter(item => camera.isWhiteBalanceModeSupported(item))

    // Because .isWhiteBalanceModeSupported is not a reactive binding,
    // we need to explicitly update the list of supported white-balance modes
    // whenever the camera-device changes.
    Connections {
        target: root.camera
        function onCameraDeviceChanged() {
            root.supportedWhiteBalanceModes = root.allWhiteBalanceModes
                .filter(item => root.camera.isWhiteBalanceModeSupported(item))
        }
    }

    // Helper function to turn a Camera.WhiteBalanceMode to a string.
    function cameraWhiteBalanceModeToString(input) {
        switch (input) {
            case Camera.WhiteBalanceAuto: return "Auto"
            case Camera.WhiteBalanceCloudy: return "Cloudy"
            case Camera.WhiteBalanceFluorescent: return "Fluorescent"
            case Camera.WhiteBalanceSunlight: return "Sunlight"
            case Camera.WhiteBalanceTungsten: return "Tungsten"
            default: return "Unknown"
        }
    }

    model: supportedWhiteBalanceModes
        .map((mode) => { return {
            value: mode,
            displayText: cameraWhiteBalanceModeToString(mode) } })
    valueRole: "value"
    textRole: "displayText"
    font.pixelSize: 14
    font.bold: true

    onActivated: index => {
        camera.whiteBalanceMode = currentValue
    }
    currentIndex: model.findIndex(item => item.value === camera.whiteBalanceMode)
}
