// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: top
    required property CaptureSession captureSession
    readonly property ImageCaptureHelper imageCapture: captureSession.imageCapture
    required property CameraHelper camera
    required property MediaDevicesHelper mediaDevices

    CheckBox {
        text: "Camera attached to capture session"
        checked: top.captureSession.camera
        onClicked: () => {
            if (top.captureSession.camera)
                top.captureSession.camera = null;
            else
                top.captureSession.camera = top.camera;
        }
    }

    CameraDeviceSelector {
        camera: top.camera
        mediaDevices: top.mediaDevices
    }

    RowLayout {
        Label {
            text: "Format"
        }
        Label {
            text: "unavailable"
            color: "red"
            function checkFormatIsAvailable(videoInputs, cameraDevice, format): bool {
                // First check if the device is available in QMediaDevices
                let foundDevice = videoInputs.find((item) => String(item.id) === String(cameraDevice.id))
                if (foundDevice === undefined)
                    return false
                let foundFormat = foundDevice.videoFormats.find((item) =>
                    top.camera.cameraFormatToString(item) === top.camera.cameraFormatToString(format))
                return foundFormat !== undefined
            }
            function calcOpacity(videoInputs, cameraDevice, format) {
                let formatIsAvailable = checkFormatIsAvailable(videoInputs, cameraDevice, format)
                if (formatIsAvailable)
                    return 0.1
                else
                    return 1
            }

            opacity: calcOpacity(mediaDevices.videoInputs, top.camera.cameraDevice, top.camera.cameraFormat)
        }

        ComboBox {
            implicitContentWidthPolicy: ComboBox.WidestText
            model:
                top.camera.cameraDevice.videoFormats
                .slice()
                .sort((a, b) => top.camera.customCameraFormatSortDelegate(a, b))
                .map((item) => { return { value: item, text: top.camera.cameraFormatToString(item) } })
            textRole: "text"
            displayText: top.camera.cameraFormatToString(top.camera.cameraFormat)
            currentIndex: model.findIndex(
                (item) => item.text === top.camera.cameraFormatToString(top.camera.cameraFormat))
            onActivated: index => {
                top.camera.cameraFormat = model[index].value
            }
        }

        Button {
            text: "Default"
            onClicked: top.camera.cameraFormat = undefined
        }
    }

    CheckBox {
        text: "Active"
        checked: top.camera.active
        onToggled: () => {
            top.camera.active = checked
            checked = Qt.binding(() => top.camera.active)
        }
    }
    ColumnLayout {
        enabled: top.camera.minimumZoomFactor < top.camera.maximumZoomFactor
        RowLayout {
            Label {
                text: "Zoom: " + top.camera.zoomFactor.toFixed(1)
                font.bold: true
            }
            Label { text: "min: " + top.camera.minimumZoomFactor.toFixed(1) + " max: " + top.camera.maximumZoomFactor.toFixed(1) }
        }
        Slider {
            Layout.fillWidth: true
            from: top.camera.minimumZoomFactor
            to: Math.min(top.camera.maximumZoomFactor, 10)
            value: top.camera.zoomFactor
            onMoved: top.camera.zoomFactor = value
        }
        Button {
            text: "Ramp zoom slowly to max"
            onClicked: top.camera.zoomTo(Math.min(top.camera.maximumZoomFactor,
                                              4), 0.1)
        }
    }

    RowLayout {
        enabled: top.camera.supportedFocusModes.length > 1
        Label {
            text: "Focus mode"
            font.bold: true
        }
        ComboBox {
            model: top.camera.supportedFocusModes
                .map(item => {
                    return {
                        "value": item,
                        "text": top.camera.focusModeToString(item)
                    }
                })
            valueRole: "value"
            textRole: "text"
            displayText: top.camera.focusModeToString(top.camera.focusMode)
            currentIndex: model.findIndex(
                              item => item.value === top.camera.focusMode)
            onActivated: index => {
                if (top.camera.isFocusModeSupported(currentValue))
                    top.camera.focusMode = currentValue
                else
                    console.log("Selected unsupported focus mode")
            }
        }
    } // Focus mode

    RowLayout {
        // Flash mode
        enabled: top.camera.supportedFlashModes.length > 1
        Label {
            text: "Flash mode"
            font.bold: true
        }
        ComboBox {
            Layout.alignment: Qt.AlignRight
            model: top.camera.supportedFlashModes
                .map(item => {
                    return {
                        "value": item,
                        "text": top.camera.flashModeToString(item) } })
            textRole: "text"
            valueRole: "value"
            currentIndex: model.findIndex(item => item.value === top.camera.flashMode)
            onActivated: index => {
                if (top.camera.isFlashModeSupported(currentValue))
                    top.camera.flashMode = currentValue
                else
                    console.log("Selected unsupported flash mode")
            }
        }
    } // Flash mode

    RowLayout {
        enabled: top.camera.supportedTorchModes.length > 1
        Label {
            text: "Torch mode"
            font.bold: true
        }
        ComboBox {
            Layout.alignment: Qt.AlignRight
            model:
                top.camera.supportedTorchModes
                .map(item => { return { "value": item, "text": top.camera.torchModeToString(item) } })
            textRole: "text"
            valueRole: "value"
            onActivated: index => {
                if (top.camera.isTorchModeSupported(currentValue))
                    top.camera.torchMode = currentValue
                else
                    console.log("Selected unsupported torch mode")
            }
        }
    }

    ColumnLayout {
        enabled: top.camera.supportedFocusModes.includes(Camera.FocusModeManual)
                 && top.camera.supportedFeatures & Camera.FocusDistance
        Label {
            text: "focusDistance " + top.camera.focusDistance.toFixed(1)
        }
        Slider {
            Layout.fillWidth: true
            from: 0
            to: 1
            value: top.camera.focusDistance
            onMoved: top.camera.focusDistance = value
        }
    }

    RowLayout {
        Label {
            text: "Image file format"
        }
        ComboBox {
            implicitContentWidthPolicy: ComboBox.WidestText
            model: imageCapture.supportedFormats
                .map((item) => { return { displayText: imageCapture.fileFormatToString(item), value: item } })
            valueRole: "value"
            textRole: "displayText"
            currentIndex: model.findIndex(
                item => item.value === imageCapture.fileFormat)
            displayText: imageCapture.fileFormatToString(imageCapture.fileFormat)
            onActivated: index => {
                imageCapture.fileFormat = currentValue
                currentIndex = model.findIndex(
                    item => item.value === imageCapture.fileFormat)
            }
        }
        Button {
            text: "Default"
            onClicked: imageCapture.fileFormat = undefined
        }
    }

    RowLayout {
        Label {
            text: "Error: "
                + top.camera.errorEnumToString(top.camera.error)
                + " ("
                + top.camera.error
                + ")"
        }
    }
    RowLayout {
        Label {
            text: "Error string: '" + top.camera.errorString + "'"
        }
    }
}
