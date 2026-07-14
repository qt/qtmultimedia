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

    required property var topWindow
    required property bool useLandscapeLayout
    required property int recorderCaptureTracker
    required property int preferredLeftWidth

    property int framesReceived: 0

    Connections {
        target: windowCaptureVideoOutput.videoSink
        function onVideoFrameChanged(frame) {
            root.framesReceived += 1
        }
    }

    // There is no signal for tracking when the list of CapturableWindows changed. We store them
    // here as a property so we can track them.
    property var capturableWindowsList: []
    property bool periodicallyRefreshWindows: false
    function refreshCapturableWindowsList() {
        capturableWindowsList = capture.capturableWindows()
            .sort((a, b) => a.description.localeCompare(b.description))
    }

    WindowCapture {
        id: capture

        onActiveChanged: () => {
            console.log("QWindowCapture active changed: " + active)
        }

        function errorToString(err) {
            switch (err) {
            case WindowCapture.NoError: return "NoError"
            case WindowCapture.InternalError: return "InternalError"
            case WindowCapture.CapturingNotSupported: return "CapturingNotSupported"
            case WindowCapture.CaptureFailed: return "CaptureFailed"
            case WindowCapture.NotFound: return "NotFund"
            default: return "Unknown error code"
            }
        }

        function errorToLabel(err, msg) {
            if (err === WindowCapture.NoError)
                return ""
            return "(" + errorToString(err) + ") " + msg
        }

        readonly property string errorLabel: errorToLabel(error, errorString)

        onErrorOccurred: (err, msg) => {
            console.log("QWindowCapture error occurred (" + errorToString(err) + ") " + msg)
        }

        onFrameRateChanged: console.log("QWindowCapture frameRate changed " + frameRate)
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
        windowCapture: capture
        videoOutput: windowCaptureVideoOutput
        recorder: mediaRecorder
    }

    // There is no signal for refreshing capturable windows. This
    // timer lets us refresh it frequently.
    Timer {
        interval: 1000
        running: root.periodicallyRefreshWindows
        repeat: true
        triggeredOnStart: true
        onTriggered: root.refreshCapturableWindowsList()
    }

    columns: useLandscapeLayout === true ? 2 : 1

    ColumnLayout {
        Layout.alignment: Qt.AlignTop

        Button {
            text: "Select app window"
            onClicked: () => {
                capture.window = root.topWindow
            }
        }

        Button {
            text: "Refresh list of windows"
            onClicked: root.refreshCapturableWindowsList()
        }

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
                text: capture.frameRate
                inputMethodHints: Qt.ImhFormattedNumbersOnly

                validator: DoubleValidator {
                    bottom: 1.0
                    top: 1000.0
                    decimals: 2
                    notation: DoubleValidator.StandardNotation
                    locale: "C"
                }

                onEditingFinished: {
                    capture.frameRate = parseFloat(text)
                }
            }
            Button {
                text: "Reset"
                onClicked: capture.frameRate = undefined
            }
        }

        CheckBox {
            id: windowCaptureActiveCheckBox
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
            text: "Periodically refresh windows"
            checkState: root.periodicallyRefreshWindows === true ? Qt.Checked : Qt.Unchecked
            onClicked: () => {
                root.periodicallyRefreshWindows = !root.periodicallyRefreshWindows
                checkState = root.periodicallyRefreshWindows === true ? Qt.Checked : Qt.Unchecked
            }
        }

        CheckBox {
            id: hideInvalidWindowsCheckbox
            text: "Hide invalid windows"
            checkState: Qt.Unchecked
        }

        Label {
            text: "Frames received: " + root.framesReceived
        }

        Label {
            Layout.preferredWidth: root.preferredLeftWidth
            wrapMode: Text.WordWrap
            text: "Error: " + capture.errorLabel
        }

        Label {
            text: "Capturable windows:"
        }
        Frame {
            Layout.preferredWidth: root.preferredLeftWidth
            Layout.preferredHeight: windowsListView.model.length > 0 ? 200 : 50
            ListView {
                id: windowsListView
                anchors.fill: parent
                clip: true
                model: root.capturableWindowsList
                    .filter((item) => {
                        if (hideInvalidWindowsCheckbox.checkState === Qt.Unchecked)
                            return true
                        if (item.isValid === false)
                            return false
                        return true
                    })
                currentIndex: model
                    .findIndex((item) => item === capture.window)
                delegate: ItemDelegate {
                    required property var modelData
                    required property int index
                    highlighted: ListView.isCurrentItem
                    function buildText(item) {
                        let outString = ""
                        if (modelData.isValid === false)
                            outString += "(Invalid) "
                        outString += item.description
                        return outString
                    }
                    text: buildText(modelData)
                    onClicked: () => {
                        capture.window = modelData
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
            id: windowCaptureVideoOutput
        }
    }
}
