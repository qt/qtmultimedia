// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml
import QtMultimedia

GridLayout {
    id: top

    required property var topWindow
    required property bool useLandscapeLayout

    // There is no signal for tracking when the list of CapturableWindows changed. We store them
    // here as a property so we can track them.
    property var capturableWindowsList: []
    property bool periodicallyRefreshWindows: false

    WindowCapture {
        id: windowCapture
        onActiveChanged: () => {
            console.log("QWindowCapture active changed: " + active)

            // CheckBox has a weird behavior where it will remove our bindings when activated.
            // Update the value manually here
            if (active === true)
                windowCaptureActiveCheckBox.checkState = Qt.Checked
            else
                windowCaptureActiveCheckBox.checkState = Qt.Unchecked
        }
        onErrorChanged: () => {
            console.log("QWindowCapture error changed: " + error)
        }
        onErrorStringChanged: (msg) => {
            console.log("QWindowCapture error string changed: " + errorString)
        }
    }

    CaptureSession {
        windowCapture: windowCapture
        videoOutput: windowCaptureVideoOutput
    }

    // There is no signal for refreshing capturable windows. This
    // timer lets us refresh it frequently.
    Timer {
        interval: 500
        running: top.periodicallyRefreshWindows
        repeat: true
        triggeredOnStart: true
        onTriggered: () => {
            top.capturableWindowsList = windowCapture.capturableWindows()
        }
    }

    Layout.fillWidth: true
    columns: useLandscapeLayout === true ? 2 : 1

    ColumnLayout {
        Layout.alignment: Qt.AlignTop

        Button {
            text: "Select app window"
            onClicked: () => {
                windowCapture.window = topWindow
            }
        }

        Button {
            text: "Refresh list of windows"
            onClicked: () => {
                top.capturableWindowsList = windowCapture.capturableWindows()
            }
        }

        CheckBox {
            id: windowCaptureActiveCheckBox
            text: "Active"
            onClicked: () => {
                if (checkState === Qt.Checked)
                    windowCapture.start()
                else
                    windowCapture.stop()

                checkState = windowCapture.active === true ? Qt.Checked : Qt.Unchecked
            }
        }

        CheckBox {
            text: "Periodically refresh windows"
            checkState: top.periodicallyRefreshWindows === true ? Qt.Checked : Qt.Unchecked
            onClicked: () => {
                top.periodicallyRefreshWindows = !top.periodicallyRefreshWindows
                checkState = top.periodicallyRefreshWindows === true ? Qt.Checked : Qt.Unchecked
            }
        }

        CheckBox {
            id: hideInvalidWindowsCheckbox
            text: "Hide invalid windows"
            checkState: Qt.Unchecked
        }

        Label {
            text: "Capturable windows:"
        }
        Frame {
            Layout.preferredWidth: 200
            Layout.preferredHeight: windowsListView.model.length > 0 ? 200 : 50
            ListView {
                id: windowsListView
                anchors.fill: parent
                clip: true
                model: top.capturableWindowsList
                    .filter((item) => {
                        if (hideInvalidWindowsCheckbox.checkState === Qt.Unchecked)
                            return true
                        if (item.isValid === false)
                            return false
                        return true
                    })
                currentIndex: model
                    .findIndex((item) => item === windowCapture.window)
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
                        windowCapture.window = modelData
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
