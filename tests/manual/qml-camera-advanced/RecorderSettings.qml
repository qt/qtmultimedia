// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml
import QtMultimedia

// The encoder settings are only applied when a recording starts, so they are disabled while
// recording.
ColumnLayout {
    id: root

    required property MediaRecorder recorder

    readonly property bool settingsEditable: recorder.recorderState === MediaRecorder.StoppedState

    function buildFormatModel(unspecifiedValue, supportedValues, nameFn) {
        return [{ text: "Default", value: unspecifiedValue }]
            .concat(supportedValues.map((value) => {
                return { text: nameFn(value), value: value }
            }))
    }

    Label {
        text: "State: " + Qt.enumValueToString(MediaRecorder.RecorderState,
                                                root.recorder.recorderState)
    }

    Label {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
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
        Layout.minimumWidth: 0
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
        currentIndex: count > 0 && model ? indexOfValue(root.recorder.quality) : -1
        onActivated: () => {
            root.recorder.quality = currentValue
        }
    }

    Label {
        text: "Encoding mode:"
    }
    ComboBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        enabled: root.settingsEditable
        textRole: "text"
        valueRole: "value"
        model: [
            { text: "Constant quality", value: MediaRecorder.ConstantQualityEncoding },
            { text: "Constant bit rate", value: MediaRecorder.ConstantBitRateEncoding },
            { text: "Average bit rate", value: MediaRecorder.AverageBitRateEncoding },
            { text: "Two pass", value: MediaRecorder.TwoPassEncoding }
        ]
        currentIndex: count > 0 && model ? indexOfValue(root.recorder.encodingMode) : -1
        onActivated: () => {
            root.recorder.encodingMode = currentValue
        }
    }

    Label {
        text: "File format:"
    }
    ComboBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        enabled: root.settingsEditable
        textRole: "text"
        valueRole: "value"
        model: root.buildFormatModel(
            MediaFormat.UnspecifiedFormat,
            root.recorder.mediaFormat.supportedFileFormats(MediaFormat.Encode),
            (value) => root.recorder.mediaFormat.fileFormatName(value))
        currentIndex: count > 0 && model ? indexOfValue(root.recorder.mediaFormat.fileFormat) : -1
        onActivated: () => {
            root.recorder.mediaFormat.fileFormat = currentValue
        }
    }

    Label {
        text: "Video codec:"
    }
    ComboBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        enabled: root.settingsEditable
        textRole: "text"
        valueRole: "value"
        model: root.buildFormatModel(
            MediaFormat.VideoCodec.Unspecified,
            root.recorder.mediaFormat.supportedVideoCodecs(MediaFormat.Encode),
            (value) => root.recorder.mediaFormat.videoCodecName(value))
        currentIndex: count > 0 && model ? indexOfValue(root.recorder.mediaFormat.videoCodec) : -1
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
            Layout.minimumWidth: 0
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
            Layout.minimumWidth: 0
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
        Layout.minimumWidth: 0
        wrapMode: Text.WrapAnywhere
        text: "Output location: " + root.recorder.actualLocation
    }
}
