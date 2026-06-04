// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml
import QtMultimedia

GridLayout {
    id: root

    required property bool useLandscapeLayout
    required property int recorderCaptureTracker
    required property int preferredLeftWidth

    columns: useLandscapeLayout === true ? 2 : 1

    ScreenCapture {
        id: screenCapture

        onActiveChanged: () => {
            console.log("QScreenCapture active changed: " + active)
        }

        function errorToString(err) {
            switch (err) {
            case ScreenCapture.NoError: return "NoError"
            case ScreenCapture.InternalError: return "InternalError"
            case ScreenCapture.CapturingNotSupported: return "CapturingNotSupported"
            case ScreenCapture.CaptureFailed: return "CaptureFailed"
            case ScreenCapture.NotFound: return "NotFund"
            default: return "Unknown error code"
            }
        }

        function errorToLabel(err, msg) {
            if (err === ScreenCapture.NoError)
                return ""
            return "(" + errorToString(err) + ") " + msg
        }

        readonly property string errorLabel: errorToLabel(error, errorString)

        onErrorOccurred: (err, msg) => {
            console.log("ScreenCapture error occurred: (" + errorToString(err) + ") " + msg)
        }

        onFrameRateChanged: console.log("QScreenCapture frameRate changed " + frameRate)
    }

    MediaRecorder {
        id: mediaRecorder

        quality: MediaRecorder.VeryHighQuality

        outputLocation:
            StandardPaths.writableLocation(StandardPaths.MoviesLocation)
            + "/qml-screencapture-advanced-"
            + root.recorderCaptureTracker

        onActualLocationChanged: () => {
            console.log("New actual location: " + actualLocation)
        }

        onErrorOccurred: (error, msg) => {
            console.log("Media recorder error: " + msg)
        }
    }

    CaptureSession {
        screenCapture: screenCapture
        videoOutput: screenCaptureVideoOutput
        recorder: mediaRecorder
    }

    ColumnLayout {
        Layout.alignment: Qt.AlignTop

        Button {
            text: mediaRecorder.recorderState === MediaRecorder.StoppedState
                ? "Start recording"
                : "Stop recording"
            onClicked: {
                if (mediaRecorder.recorderState === MediaRecorder.StoppedState) {
                    root.recorderCaptureTracker += 1
                    mediaRecorder.record()
                } else {
                    mediaRecorder.stop()
                }
            }
        }

        RowLayout {
            Label {
                text: "Framerate:"
            }
            TextField {
                id: floatField
                text: screenCapture.frameRate
                inputMethodHints: Qt.ImhFormattedNumbersOnly

                validator: DoubleValidator {
                    bottom: 1.0
                    top: 1000.0
                    decimals: 2
                    notation: DoubleValidator.StandardNotation
                    locale: "C"
                }

                onEditingFinished: {
                    screenCapture.frameRate = parseFloat(text)
                }
            }
            Button {
                text: "Reset"
                onClicked: screenCapture.frameRate = undefined
            }
        }

        CheckBox {
            id: screenCaptureActiveCheckBox
            text: "Active"
            onClicked: () => {
                if (checkState === Qt.Checked)
                    screenCapture.start()
                else
                    screenCapture.stop()

                checkState = screenCapture.active === true ? Qt.Checked : Qt.Unchecked
            }
        }

        Label {
            Layout.preferredWidth: root.preferredLeftWidth
            wrapMode: Text.WordWrap
            text: "Error: " + screenCapture.errorLabel
        }

        Label {
            text: "Screens:"
        }
        Frame {
            Layout.preferredWidth: root.preferredLeftWidth
            Layout.preferredHeight: screensListView.model.length > 0 ? 200 : 50
            ListView {
                id: screensListView
                anchors.fill: parent
                clip: true
                model: Qt.application.screens
                currentIndex: model
                    .findIndex((item) => item === screenCapture.screen)
                delegate: ItemDelegate {
                    required property var modelData
                    required property int index
                    highlighted: ListView.isCurrentItem
                    text: modelData.name
                    onClicked: () => {
                        screenCapture.screen = modelData
                    }
                }
            }
        }
    }

    Frame {
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        Layout.preferredHeight: 400
        VideoOutput {
            anchors.fill: parent
            id: screenCaptureVideoOutput
        }
    }
}
