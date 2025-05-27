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

    required property bool useLandscapeLayout

    Layout.fillWidth: true
    columns: useLandscapeLayout === true ? 2 : 1

    ScreenCapture {
        id: screenCapture
        onActiveChanged: () => {
            console.log("QScreenCapture active changed: " + active)

            // CheckBox has a weird behavior where it will remove our bindings when activated.
            // Update the value manually here
            if (active === true)
                screenCaptureActiveCheckBox.checkState = Qt.Checked
            else
                screenCaptureActiveCheckBox.checkState = Qt.Unchecked
        }
        onErrorChanged: () => {
            console.log("QScreenCapture error changed: " + error)
        }
        onErrorStringChanged: (msg) => {
            console.log("QScreenCapture error string changed: " + errorString)
        }
    }

    CaptureSession {
        screenCapture: screenCapture
        videoOutput: screenCaptureVideoOutput
    }

    ColumnLayout {
        Layout.alignment: Qt.AlignTop

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
            text: "Screens:"
        }
        Frame {
            Layout.preferredWidth: 200
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
