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
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia

Column {
    id: root
    spacing: Style.intraSpacing
    Component.onCompleted: root.populateModels()

    required property MediaRecorder recorder

    function populateModels() {
        audioCodecModel.populate()
        videoCodecModel.populate()
        fileFormatModel.populate()
    }

    Connections {
        target: recorder
        function onMediaFormatChanged() { root.populateModels() }
    }

    Text { text: "Encoder settings" }

    StyleParameter {
        label: "Quality"
        model: ListModel {
            ListElement { text: "very low"; value: MediaRecorder.VeryLowQuality }
            ListElement { text: "low"; value: MediaRecorder.LowQuality }
            ListElement { text: "normal"; value: MediaRecorder.NormalQuality }
            ListElement { text: "high"; value: MediaRecorder.HighQuality }
            ListElement { text: "very high"; value: MediaRecorder.VeryHighQuality }
        }
        onActivated: (v) => { recorder.quality = v }
    }

    StyleParameter {
        id: audioCodecSelect
        label: "Audio codec"
        model: audioCodecModel
        onActivated: (v) => { recorder.mediaFormat.audioCodec = v }

        ListModel {
            id: audioCodecModel
            function populate() {
                audioCodecModel.clear()
                audioCodecModel.append({"text": "Unspecifed", "value": MediaFormat.AudioCodec.Unspecified})
                var cs = recorder.mediaFormat.supportedAudioCodecs(MediaFormat.Encode)
                for (var c of cs)
                    audioCodecModel.append({"text": recorder.mediaFormat.audioCodecName(c), "value": c})
                audioCodecSelect.currentIndex = cs.indexOf(recorder.mediaFormat.audioCodec) + 1
            }
        }
    }

    function buildModel() {}

    StyleParameter {
        id: videoCodecSelect
        label: "Video codec"
        model: videoCodecModel
        onActivated: (v) => { recorder.mediaFormat.videoCodec = v }

        ListModel {
            id: videoCodecModel
            function populate() {
                videoCodecModel.clear()
                videoCodecModel.append({"text": "Unspecifed", "value": MediaFormat.VideoCodec.Unspecified})
                var cs = recorder.mediaFormat.supportedVideoCodecs(MediaFormat.Encode)
                for (var c of cs)
                    videoCodecModel.append({"text": recorder.mediaFormat.videoCodecName(c), "value": c})
                videoCodecSelect.currentIndex = cs.indexOf(recorder.mediaFormat.videoCodec) + 1
            }
        }
    }

    StyleParameter {
        id: fileFormatSelect
        label: "File format"
        model: fileFormatModel
        onActivated: (v) => { recorder.mediaFormat.fileFormat = v }

        ListModel {
            id: fileFormatModel
            function populate() {
                fileFormatModel.clear()
                fileFormatModel.append({"text": "Unspecifed", "value": MediaFormat.AudioCodec.Unspecified})
                var cs = recorder.mediaFormat.supportedFileFormats(MediaFormat.Encode)
                for (var c of cs)
                    fileFormatModel.append({"text": recorder.mediaFormat.fileFormatName(c), "value": c})
                fileFormatSelect.currentIndex = cs.indexOf(recorder.mediaFormat.fileFormat) + 1
            }
        }
    }
}
