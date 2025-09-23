// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
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
            recording: root.recorder.recorderState === MediaRecorder.RecordingState
            onClicked: recording ? root.recorder.stop() : root.recorder.record()
        }
        Text {
            id: recordingTime
            anchors.horizontalCenter: parent.horizontalCenter
            font.pointSize: Style.fontSize
            color: palette.text
        }
    }

    Column {
        id: optionButtons
        spacing: Style.intraSpacing
        Button {
            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0
            height: Style.height
            width: Style.widthMedium
            background: StyleRectangle { anchors.fill: parent }
            onClicked: root.capturesVisible = !root.capturesVisible
            text: "Captures"
            font.pointSize: Style.fontSize
        }
        Button {
            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0
            height: Style.height
            width: Style.widthMedium
            background: StyleRectangle { anchors.fill: parent }
            onClicked: root.settingsVisible = !root.settingsVisible
            text: "Settings"
            font.pointSize: Style.fontSize
        }
    }

    Timer {
        running: true; interval: 100; repeat: true
        onTriggered: {
            var m = Math.floor(root.recorder.duration / 60000)
            var ms = (root.recorder.duration / 1000 - m * 60).toFixed(1)
            recordingTime.text = `${m}:${ms.padStart(4, 0)}`
        }
    }
}
