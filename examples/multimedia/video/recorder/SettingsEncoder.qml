// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
