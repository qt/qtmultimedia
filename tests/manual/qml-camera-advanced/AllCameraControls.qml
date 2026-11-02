// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: top
    required property CaptureSession captureSession
    required property CameraHelper camera
    required property MediaDevicesHelper mediaDevices

    readonly property bool formatAvailable: {
        const device = mediaDevices.videoInputs.find(
            item => String(item.id) === String(camera.cameraDevice.id))
        if (device === undefined)
            return false
        return device.videoFormats.some(
            item => camera.cameraFormatToString(item)
                === camera.cameraFormatToString(camera.cameraFormat))
    }

    spacing: 12

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "Device"

        ColumnLayout {
            anchors.fill: parent

            CameraDeviceSelector {
                camera: top.camera
                mediaDevices: top.mediaDevices
            }

            RowLayout {
                Label {
                    text: "Format"
                }
                ComboBox {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    implicitContentWidthPolicy: ComboBox.WidestText
                    model: top.camera.cameraDevice.videoFormats
                        .slice()
                        .sort((a, b) => top.camera.customCameraFormatSortDelegate(a, b))
                        .map((item) => {
                            return { value: item, text: top.camera.cameraFormatToString(item) }
                        })
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

            Label {
                text: "The selected format is not reported by the current device"
                color: "red"
                visible: !top.formatAvailable
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "State"

        ColumnLayout {
            anchors.fill: parent

            CheckBox {
                text: "Active"
                checked: top.camera.active
                onToggled: () => {
                    top.camera.active = checked
                    checked = Qt.binding(() => top.camera.active)
                }
            }

            CheckBox {
                text: "Attached to capture session"
                checked: top.captureSession.camera
                onClicked: () => {
                    if (top.captureSession.camera)
                        top.captureSession.camera = null;
                    else
                        top.captureSession.camera = top.camera;
                }
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "Zoom"
        enabled: top.camera.minimumZoomFactor < top.camera.maximumZoomFactor

        ColumnLayout {
            anchors.fill: parent

            RowLayout {
                Label {
                    text: top.camera.zoomFactor.toFixed(1) + "x"
                    font.bold: true
                }
                Item {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                }
                Label {
                    text: "min " + top.camera.minimumZoomFactor.toFixed(1)
                        + ", max " + top.camera.maximumZoomFactor.toFixed(1)
                    opacity: 0.7
                }
            }
            Slider {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                from: top.camera.minimumZoomFactor
                to: Math.min(top.camera.maximumZoomFactor, 10)
                value: top.camera.zoomFactor
                onMoved: top.camera.zoomFactor = value
            }
            Button {
                text: "Ramp zoom slowly to max"
                onClicked: top.camera.zoomTo(Math.min(top.camera.maximumZoomFactor, 4), 0.1)
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "Focus"

        ColumnLayout {
            anchors.fill: parent

            RowLayout {
                enabled: top.camera.supportedFocusModes.length > 1
                Label {
                    text: "Mode"
                }
                ComboBox {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
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
                    currentIndex: model.findIndex(item => item.value === top.camera.focusMode)
                    onActivated: index => {
                        if (top.camera.isFocusModeSupported(currentValue))
                            top.camera.focusMode = currentValue
                        else
                            console.log("Selected unsupported focus mode")
                    }
                }
            }

            ColumnLayout {
                enabled: top.camera.supportedFocusModes.includes(Camera.FocusModeManual)
                    && top.camera.supportedFeatures & Camera.FocusDistance
                Label {
                    text: "Distance: " + top.camera.focusDistance.toFixed(1)
                }
                Slider {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    from: 0
                    to: 1
                    value: top.camera.focusDistance
                    onMoved: top.camera.focusDistance = value
                }
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "Flash and torch"

        GridLayout {
            anchors.fill: parent
            columns: 2
            columnSpacing: 12

            Label {
                text: "Flash mode"
                enabled: top.camera.supportedFlashModes.length > 1
            }
            ComboBox {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                enabled: top.camera.supportedFlashModes.length > 1
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

            Label {
                text: "Torch mode"
                enabled: top.camera.supportedTorchModes.length > 1
            }
            ComboBox {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                enabled: top.camera.supportedTorchModes.length > 1
                model: top.camera.supportedTorchModes
                    .map(item => {
                        return { "value": item, "text": top.camera.torchModeToString(item) } })
                textRole: "text"
                valueRole: "value"
                currentIndex: model.findIndex(item => item.value === top.camera.torchMode)
                onActivated: index => {
                    if (top.camera.isTorchModeSupported(currentValue))
                        top.camera.torchMode = currentValue
                    else
                        console.log("Selected unsupported torch mode")
                }
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "Error"

        GridLayout {
            anchors.fill: parent
            columns: 2
            columnSpacing: 12

            Label {
                text: "Code"
                opacity: 0.7
            }
            Label {
                text: top.camera.errorEnumToString(top.camera.error) + " (" + top.camera.error + ")"
                color: top.camera.error === Camera.NoError ? palette.windowText : "red"
            }

            Label {
                text: "Message"
                opacity: 0.7
            }
            Label {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                wrapMode: Text.WordWrap
                text: top.camera.errorString === "" ? "-" : top.camera.errorString
            }
        }
    }
}
