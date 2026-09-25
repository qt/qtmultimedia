// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia

// Shows the state of a MediaRecorder at a glance, similar to the recording indicator
// of a camera: a pulsing red dot while recording, a steady orange dot while paused,
// and a grey dot while stopped, followed by the state and the recorded duration.
RowLayout {
    id: root

    required property MediaRecorder recorder

    readonly property bool recording: recorder.recorderState === MediaRecorder.RecordingState
    readonly property bool paused: recorder.recorderState === MediaRecorder.PausedState

    function formatDuration(milliseconds) {
        const totalSeconds = Math.floor(milliseconds / 1000)
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return String(minutes).padStart(2, "0") + ":" + String(seconds).padStart(2, "0")
    }

    function statusText() {
        if (root.recording)
            return "Recording " + root.formatDuration(root.recorder.duration)
        if (root.paused)
            return "Paused " + root.formatDuration(root.recorder.duration)
        return "Stopped"
    }

    // Layouts fill the width by default when nested in another layout, which would
    // let the indicator stretch the column it is placed in.
    Layout.fillWidth: false
    spacing: 8

    Rectangle {
        id: dot

        implicitWidth: 12
        implicitHeight: 12
        radius: width / 2
        color: root.recording
            ? "#d32f2f"
            : root.paused ? "#f57c00" : root.palette.mid

        SequentialAnimation on opacity {
            running: root.recording
            loops: Animation.Infinite
            // Ends fully opaque, so the dot does not stay faded once stopped.
            alwaysRunToEnd: true

            NumberAnimation {
                to: 0.3
                duration: 600
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                to: 1.0
                duration: 600
                easing.type: Easing.InOutQuad
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: root.statusText()
        font.bold: root.recording || root.paused
        elide: Text.ElideRight
    }
}
