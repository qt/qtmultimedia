// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml
import QtMultimedia

// Controls for operating and configuring a MediaRecorder. The encoder settings are
// only applied when a recording starts, so they are disabled while recording.
ColumnLayout {
    id: root

    required property MediaRecorder recorder

    // Emitted right before a new recording starts, so the output location can be
    // updated first.
    signal aboutToStartRecording()

    readonly property bool settingsEditable: recorder.recorderState === MediaRecorder.StoppedState

    // Builds a ComboBox model for a MediaFormat enum, with an entry for the
    // unspecified value first so the backend can pick a default.
    function buildFormatModel(unspecifiedValue, supportedValues, nameFn) {
        return [{ text: "Default", value: unspecifiedValue }]
            .concat(supportedValues.map((value) => {
                return { text: nameFn(value), value: value }
            }))
    }

    RecorderStatusIndicator {
        Layout.fillWidth: true
        recorder: root.recorder
    }

    RowLayout {
        // Record also resumes a paused recording, like MediaRecorder.record() does.
        Button {
            Layout.fillWidth: true
            enabled: root.recorder.recorderState !== MediaRecorder.RecordingState
            text: root.recorder.recorderState === MediaRecorder.PausedState
                ? "Resume"
                : "Record"
            onClicked: () => {
                if (root.recorder.recorderState === MediaRecorder.StoppedState)
                    root.aboutToStartRecording()
                root.recorder.record()
            }
        }
        Button {
            Layout.fillWidth: true
            enabled: root.recorder.recorderState === MediaRecorder.RecordingState
            text: "Pause"
            onClicked: () => {
                root.recorder.pause()
            }
        }
        Button {
            Layout.fillWidth: true
            enabled: root.recorder.recorderState !== MediaRecorder.StoppedState
            text: "Stop"
            onClicked: () => {
                root.recorder.stop()
            }
        }
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        visible: root.recorder.error !== MediaRecorder.NoError
        text: "Error: " + root.recorder.errorString
    }

    CheckBox {
        text: "autoStop"
        checked: root.recorder.autoStop
        onToggled: () => {
            root.recorder.autoStop = checked
        }
    }

    Label {
        text: "Quality:"
    }
    ComboBox {
        Layout.fillWidth: true
        enabled: root.settingsEditable
        textRole: "text"
        valueRole: "value"
        model: [
            { text: "Very low", value: MediaRecorder.VeryLowQuality },
            { text: "Low", value: MediaRecorder.LowQuality },
            { text: "Normal", value: MediaRecorder.NormalQuality },
            { text: "High", value: MediaRecorder.HighQuality },
            { text: "Very high", value: MediaRecorder.VeryHighQuality }
        ]
        currentValue: root.recorder.quality
        onActivated: () => {
            root.recorder.quality = currentValue
        }
    }

    Label {
        text: "Encoding mode:"
    }
    ComboBox {
        Layout.fillWidth: true
        enabled: root.settingsEditable
        textRole: "text"
        valueRole: "value"
        model: [
            { text: "Constant quality", value: MediaRecorder.ConstantQualityEncoding },
            { text: "Constant bit rate", value: MediaRecorder.ConstantBitRateEncoding },
            { text: "Average bit rate", value: MediaRecorder.AverageBitRateEncoding },
            { text: "Two pass", value: MediaRecorder.TwoPassEncoding }
        ]
        currentValue: root.recorder.encodingMode
        onActivated: () => {
            root.recorder.encodingMode = currentValue
        }
    }

    Label {
        text: "File format:"
    }
    ComboBox {
        Layout.fillWidth: true
        enabled: root.settingsEditable
        textRole: "text"
        valueRole: "value"
        model: root.buildFormatModel(
            MediaFormat.UnspecifiedFormat,
            root.recorder.mediaFormat.supportedFileFormats(MediaFormat.Encode),
            (value) => root.recorder.mediaFormat.fileFormatName(value))
        currentValue: root.recorder.mediaFormat.fileFormat
        onActivated: () => {
            root.recorder.mediaFormat.fileFormat = currentValue
        }
    }

    Label {
        text: "Video codec:"
    }
    ComboBox {
        Layout.fillWidth: true
        enabled: root.settingsEditable
        textRole: "text"
        valueRole: "value"
        model: root.buildFormatModel(
            MediaFormat.VideoCodec.Unspecified,
            root.recorder.mediaFormat.supportedVideoCodecs(MediaFormat.Encode),
            (value) => root.recorder.mediaFormat.videoCodecName(value))
        currentValue: root.recorder.mediaFormat.videoCodec
        onActivated: () => {
            root.recorder.mediaFormat.videoCodec = currentValue
        }
    }

    Label {
        text: "Video resolution:"
    }
    RowLayout {
        enabled: root.settingsEditable
        TextField {
            id: resolutionWidthField
            Layout.preferredWidth: 60
            placeholderText: "Width"
            text: root.recorder.videoResolution.width > 0 ? root.recorder.videoResolution.width : ""
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator {
                bottom: 1
                top: 16384
            }
        }
        Label {
            text: "x"
        }
        TextField {
            id: resolutionHeightField
            Layout.preferredWidth: 60
            placeholderText: "Height"
            text: root.recorder.videoResolution.height > 0 ? root.recorder.videoResolution.height : ""
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator {
                bottom: 1
                top: 16384
            }
        }
        Button {
            text: "Set"
            enabled: resolutionWidthField.acceptableInput && resolutionHeightField.acceptableInput
            onClicked: () => {
                root.recorder.videoResolution = Qt.size(
                    parseInt(resolutionWidthField.text),
                    parseInt(resolutionHeightField.text))
            }
        }
        Button {
            text: "Reset"
            onClicked: () => {
                root.recorder.videoResolution = Qt.size(-1, -1)
            }
        }
    }

    Label {
        text: "Video frame rate:"
    }
    RowLayout {
        enabled: root.settingsEditable
        TextField {
            Layout.fillWidth: true
            placeholderText: "Default"
            text: root.recorder.videoFrameRate > 0 ? root.recorder.videoFrameRate : ""
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            validator: DoubleValidator {
                bottom: 1.0
                top: 1000.0
                decimals: 2
                notation: DoubleValidator.StandardNotation
                locale: "C"
            }
            onEditingFinished: {
                root.recorder.videoFrameRate = parseFloat(text)
            }
        }
        Button {
            text: "Reset"
            onClicked: () => {
                root.recorder.videoFrameRate = -1
            }
        }
    }

    Label {
        text: "Video bit rate (bit/s):"
    }
    RowLayout {
        enabled: root.settingsEditable
        TextField {
            Layout.fillWidth: true
            placeholderText: "Default"
            text: root.recorder.videoBitRate > 0 ? root.recorder.videoBitRate : ""
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator {
                bottom: 1
            }
            onEditingFinished: {
                root.recorder.videoBitRate = parseInt(text)
            }
        }
        Button {
            text: "Reset"
            onClicked: () => {
                root.recorder.videoBitRate = -1
            }
        }
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WrapAnywhere
        text: "Output location: " + root.recorder.actualLocation
    }
}
