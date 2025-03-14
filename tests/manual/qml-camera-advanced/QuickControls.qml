// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtQuick.Layouts
import QtQuick.Controls

Pane {
    id: top

    required property real backgroundOpacity

    required property var camera
    required property var captureSession
    required property var recorder
    required property var imagesTakenCount

    ScrollView {
        width: parent.width
        height: parent.height
        clip: true

        ColumnLayout {
            RowLayout {
                // Capture image
                Button {
                    text: "Snap"
                    onClicked: top.captureSession.imageCapture.captureToFile("")
                }
                Label {
                    text: "Captured: " + top.imagesTakenCount
                }

                Button {
                    enabled: window.imageCaptureTracker > 0
                    text: "View snap"
                    onClicked: () => {
                        photoPreview.visible = true
                    }
                }
            }

            RowLayout {
                Button {
                    visible: top.recorder.recorderState === MediaRecorder.StoppedState
                    text: "Record"
                    onClicked: () => { top.recorder.record() }
                }
                Button {
                    visible:
                        top.recorder.recorderState === MediaRecorder.RecordingState ||
                        top.recorder.recorderState === MediaRecorder.PausedState
                    text: "Stop"
                    onClicked: () => { top.recorder.stop() }
                }
                Button {
                    visible: top.recorder.recorderState !== MediaRecorder.PausedState
                    enabled: top.recorder.recorderState === MediaRecorder.RecordingState
                    text: "Pause"
                    onClicked: () => { top.recorder.pause() }
                }
                Button {
                    visible: top.recorder.recorderState === MediaRecorder.PausedState
                    text: "Continue"
                    onClicked: () => { top.recorder.record() }
                }

                Button {
                    enabled: window.videoCaptureTracker > 0
                    Layout.fillWidth: true
                    text: "View video"
                    onClicked: () => {
                        capturedVideoOutputPane.visible = true
                        mediaPlayer.play()
                    }
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
            }

            RowLayout {
                enabled: top.camera.supportedFocusModes.length > 1
                Label { text: "Focus mode" }
                ComboBox {
                    Layout.alignment: Qt.AlignRight
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
                Label { text: "Flash mode" }
                ComboBox {
                    Layout.alignment: Qt.AlignRight
                    model: top.camera.supportedFlashModes.map(item => {
                                                              return {
                                                                  "value": item,
                                                                  "text": top.camera.flashModeToString(item)
                                                              }
                                                          })
                    textRole: "text"
                    valueRole: "value"
                    currentIndex: model.findIndex(
                                      item => item.value === top.camera.flashMode)
                    onActivated: index => {
                        if (top.camera.isFlashModeSupported(currentValue))
                            top.camera.flashMode = currentValue
                        else
                            console.log("Selected unsupported flash mode")
                    }
                }
            } // Flash mode

            ColumnLayout {
                enabled: top.camera.supportedFocusModes.includes(Camera.FocusModeManual)
                         && top.camera.supportedFeatures & Camera.FocusDistance
                Label {
                    text: "focusDistance " + top.camera.focusDistance.toFixed(1)
                    font.bold: true
                }
                Slider {
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: top.camera.focusDistance
                    onMoved: top.camera.focusDistance = value
                }
            }
        }
    }
}
