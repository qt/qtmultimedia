// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

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

    property int framesReceived: 0

    columns: useLandscapeLayout === true ? 2 : 1

    Connections {
        target: screenCaptureVideoOutput.videoSink
        function onVideoFrameChanged(frame) {
            root.framesReceived += 1
        }
    }

    ScreenCapture {
        id: capture

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

        onMaximumFrameRateChanged: console.log("QScreenCapture maximumFrameRate changed " + maximumFrameRate)
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
        screenCapture: capture
        videoOutput: screenCaptureVideoOutput
        recorder: mediaRecorder
    }

    ColumnLayout {
        Layout.alignment: Qt.AlignTop

        RowLayout {
            Label {
                text: "Framerate:"
            }
            TextField {
                id: floatField
                text: capture.maximumFrameRate
                inputMethodHints: Qt.ImhFormattedNumbersOnly

                validator: DoubleValidator {
                    bottom: 1.0
                    top: 1000.0
                    decimals: 2
                    notation: DoubleValidator.StandardNotation
                    locale: "C"
                }

                onEditingFinished: {
                    capture.maximumFrameRate = parseFloat(text)
                }
            }
            Button {
                text: "Reset"
                onClicked: capture.maximumFrameRate = undefined
            }
        }

        CheckBox {
            id: screenCaptureActiveCheckBox
            text: "Active"
            onClicked: () => {
                if (checkState === Qt.Checked)
                    capture.start()
                else
                    capture.stop()

                checkState = capture.active === true ? Qt.Checked : Qt.Unchecked
            }
        }

        CheckBox {
            text: "Capture cursor"
            // The flag has no change signal, so the state is read back after setting it.
            Component.onCompleted: () => {
                checked = QmlScreencaptureAdvanced.screenCaptureCursorEnabled(capture)
            }
            onToggled: () => {
                QmlScreencaptureAdvanced.setScreenCaptureCursorEnabled(capture, checked)
                checked = QmlScreencaptureAdvanced.screenCaptureCursorEnabled(capture)
            }
        }

        Label {
            text: "Frames received: " + root.framesReceived
        }

        Label {
            readonly property size frameSize: screenCaptureVideoOutput.videoSink.videoSize
            text: "Last frame size: " + frameSize.width + "x" + frameSize.height
        }

        Label {
            Layout.preferredWidth: root.preferredLeftWidth
            wrapMode: Text.WordWrap
            text: "Error: " + capture.errorLabel
        }

        SettingsButton {
            title: "MediaRecorder settings"
            previewItem: screenCaptureVideoOutput
            settings: Component {
                MediaRecorderControls {
                    recorder: mediaRecorder
                    onAboutToStartRecording: () => {
                        root.recorderCaptureTracker += 1
                    }
                }
            }
        }

        // The recording controls are in the settings, so show the state here too.
        RecorderStatusIndicator {
            recorder: mediaRecorder
        }

        // Plays the file of the last recording. Disabled while recording, since the
        // file is only complete once the recording stops.
        SettingsButton {
            title: "Play last recording"
            enabled: mediaRecorder.recorderState === MediaRecorder.StoppedState
                && mediaRecorder.actualLocation.toString() !== ""
            scrollable: false
            windowWidth: 800
            windowHeight: 600
            settings: Component {
                RecordingPlayer {
                    source: mediaRecorder.actualLocation
                }
            }
        }

        SettingsButton {
            title: "VideoOutput settings"
            previewItem: screenCaptureVideoOutput
            settings: Component {
                VideoOutputControls {
                    videoOutput: screenCaptureVideoOutput
                }
            }
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
                model: Application.screens
                currentIndex: model
                    .findIndex((item) => item === capture.screen)
                delegate: ItemDelegate {
                    required property var modelData
                    required property int index
                    highlighted: ListView.isCurrentItem
                    text: modelData.name
                    onClicked: () => {
                        capture.screen = modelData
                    }
                }
            }
        }
    }

    Frame {
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        Layout.preferredHeight: 400
        // Kept even while hidden, since the settings refer to it.
        VideoOutput {
            anchors.fill: parent
            id: screenCaptureVideoOutput
            visible: QmlScreencaptureAdvanced.screenCapturePermissionGranted
        }

        ScreenCapturePermissionPrompt {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            visible: !QmlScreencaptureAdvanced.screenCapturePermissionGranted
        }
    }
}
