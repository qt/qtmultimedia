/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    property alias camera: cameraSelect.selected

    spacing: Style.interSpacing * Style.ratio

    Column {
        id: inputControls
        spacing: Style.intraSpacing

        CameraSelect { id: cameraSelect }
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
