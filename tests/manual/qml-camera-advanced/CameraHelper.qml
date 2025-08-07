// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtMultimedia

// CameraHelper inherits from Camera but adds several helper properties.
// - Adds logging functionality to most signals.
// - Properties for list of various values currently supported by the selected camera-device:
//   - FocusMode
//   - FlashMode
//   - TorchMode
//   - Feature flags as a list
// - Turning PixelFormat enum into string
// - Turning cameraFormat into string
Camera {
    id: myCamera
    // Determines if we should clear all properties whenever we change the device.
    property bool clearPropertiesUponDeviceChange: false

    onActiveChanged: () => {
        console.log("Camera.onActiveChanged signal fired: " + active)
    }

    onCameraDeviceChanged: () => {
        console.log("Camera.onCameraDeviceChanged signal fired: " + cameraDevice)
        supportedFocusModes = allFocusModes.filter(item => isFocusModeSupported(item))
        currentFocusModeIsSupported = isFocusModeSupported(focusMode)
        supportedFlashModes = allFlashModes.filter(item => isFlashModeSupported(item))
        currentFlashModeIsSupported = isFlashModeSupported(flashMode)
        if (clearPropertiesUponDeviceChange) {
            zoomFactor = 1
            flashMode = Camera.FlashOff
            torchMode = Camera.TorchOff
            focusDistance = 1
        }
    }

    onErrorOccurred: (error, msg) => {
        console.log("Camera.errorOccurred signal fired: " + msg)
    }

    onCameraFormatChanged: () => {
        console.log("Camera.onCameraFormatChanged signal fired: " + cameraFormat)
    }

    onCustomFocusPointChanged: () => {
        console.log(
            "Camera.onCustomFocusPointChanged signal fired: " + customFocusPoint)
    }

    onFocusModeChanged: () => {
        console.log(
            "Camera.onFocusModeChanged signal fired: " + focusModeToString(focusMode))
    }

    onFocusDistanceChanged: () => {
        console.log(
            "Camera.onFocusDistanceChanged signal fired: " + focusDistance.toFixed(2))
    }

    onExposureModeChanged: () => {
        console.log(
            "Camera.onExposureModeChanged signal fired: " + exposureMode)
    }

    onExposureTimeChanged: () => {
        console.log(
            "Camera.onExposureTimeChanged signal fired: " + exposureTime)
    }

    onExposureCompensationChanged: () => {
        console.log(
            "Camera.onExposureCompensationChanged signal fired: " + exposureCompensation)
    }

    readonly property var allFocusModes: [Camera.FocusModeAuto, Camera.FocusModeManual]

    function focusModeToString(mode) {
        if (mode === Camera.FocusModeAuto)
            return "Auto"
        if (mode === Camera.FocusModeManual)
            return "Manual"
        return "unrecognized focus mode"
    }
    property var supportedFocusModes: allFocusModes.filter(item => isFocusModeSupported(item))

    property bool currentFocusModeIsSupported: isFocusModeSupported(focusMode)

    readonly property var allFlashModes: [
        Camera.FlashOff,
        Camera.FlashOn,
        Camera.FlashAuto]

    function flashModeToString(mode) {
        switch (mode) {
        case Camera.FlashOff: return "Off";
        case Camera.FlashOn: return "On";
        case Camera.FlashAuto: return "Auto";
        default: return "Unrecognized flash mode";
        }
    }

    property var supportedFlashModes: allFlashModes.filter(item => isFlashModeSupported(item))

    property bool currentFlashModeIsSupported: isFlashModeSupported(flashMode)

    readonly property var allTorchModes: [Camera.TorchOff, Camera.TorchOn, Camera.TorchAuto]

    property var supportedTorchModes: allTorchModes.filter(item => isTorchModeSupported(item))

    function torchModeToString(mode) {
        switch (mode) {
        case Camera.TorchOff: return "Off"
        case Camera.TorchOn: return "On"
        case Camera.TorchAuto: return "Auto"
        default: return "Unrecognized torch mode"
        }
    }

    readonly property var allFeatureFlags: [
        Camera.ColorTemperature,
        Camera.CustomFocusPoint,
        Camera.ExposureCompensation,
        Camera.FocusDistance,
    ]

    property var supportedFeaturesList: allFeatureFlags
        .filter(item => supportedFeatures & item)

    function featureFlagToString(flag) {
        switch (flag) {
        case Camera.ColorTemperature: return "ColorTemperature"
        case Camera.CustomFocusPoint: return "CustomFocusPoint"
        case Camera.ExposureCompensation: return "ExposureCompensation"
        case Camera.FocusDistance: return "FocusDistance"
        default: return "Unrecognized feature flag"
        }
    }

    function pixelFormatToString(input) {
        // TODO: PixelFormat enum is not exposed to QML
        switch (input) {
        case 13: return "YUV420P"
        case 14: return "YUV422P"
        case 15: return "YV12"
        case 16: return "UYVY"
        case 17: return "YUYV"
        case 18: return "NV12"
        case 19: return "NV21"
        case 26: return "P010"
        case 27: return "P016"
        case 29: return "Jpeg"
        case 31: return "YUV420P10"
        }
        return "Unrecognized pixel format (" + input + ")"
    }

    function isCameraFormatNull(format): boolean {
        // TODO: There is no QML equivalent of checking if a format is null or default-constructed..
        // This is a dirty workaround.
        return format.resolution.width <= 0
    }

    function cameraFormatToString(format) {
        if (isCameraFormatNull(format))
            return "null format"
        return ""
            + format.resolution.width
            + "x"
            + format.resolution.height
            + " "
            + pixelFormatToString(format.pixelFormat)
            + " "
            + format.minFrameRate.toFixed(0)
            + "-"
            + format.maxFrameRate.toFixed(0)
    }

    // Describes how we want to sort two given formats.
    function customCameraFormatSortDelegate(a, b) {
        if (a.resolution.width !== b.resolution.width) {
            return a.resolution.width - b.resolution.width; // Sort by width increasingly
        }
        if (a.resolution.height !== b.resolution.height) {
            return a.resolution.height - b.resolution.height; // Sort by height increasingly
        }
        return 0;
    }

    function errorEnumToString(errorEnum) {
        switch (errorEnum) {
        case Camera.NoError: return "NoError"
        case Camera.CameraError: return "CameraError"
        }
        return "Unrecognized error enum"
    }
}
