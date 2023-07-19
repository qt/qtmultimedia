// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Row {
    id: root
    required property MediaRecorder recorder

    property bool settingsVisible: false
    property bool capturesVisible: false

    property alias audioInput: audioInputSelect.selected
    property alias camera: videoSourceSelect.selectedCamera
    property alias screenCapture: videoSourceSelect.selectedScreenCapture
    property alias windowCapture: videoSourceSelect.selectedWindowCapture

    spacing: Style.interSpacing * Style.ratio

    Column {
        id: inputControls
        spacing: Style.intraSpacing

        VideoSourceSelect { id: videoSourceSelect }
        AudioInputSelect { id: audioInputSelect }
    }

    Column {
        width: recordButton.width
        RecordButton {
            id: recordButton
            recording: recorder.recorderState === MediaRecorder.RecordingState
            onClicked: recording ? recorder.stop() : recorder.record()
        }
        Text {
            id: recordingTime
            anchors.horizontalCenter: parent.horizontalCenter
            font.pointSize: Style.fontSize
        }
    }

    Column {
        id: optionButtons
        spacing: Style.intraSpacing
        Button {
            height: Style.height
            width: Style.widthMedium
            background: StyleRectangle { anchors.fill: parent }
            onClicked: root.capturesVisible = !root.capturesVisible
            text: "Captures"
            font.pointSize: Style.fontSize
        }
        Button {
            height: Style.height
            width: Style.widthMedium
            background: StyleRectangle { anchors.fill: parent }
            onClicked: settingsVisible = !settingsVisible
            text: "Settings"
            font.pointSize: Style.fontSize
        }
    }

    Timer {
        running: true; interval: 100; repeat: true
        onTriggered: {
            var m = Math.floor(recorder.duration / 60000)
            var ms = (recorder.duration / 1000 - m * 60).toFixed(1)
            recordingTime.text = `${m}:${ms.padStart(4, 0)}`
        }
    }
}
