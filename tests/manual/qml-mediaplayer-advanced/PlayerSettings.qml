// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtMultimedia

// Controls for inspecting and modifying the MediaPlayer.
ColumnLayout {
    id: root

    required property MediaPlayer mediaPlayer
    required property AudioOutput audioOutput
    required property VideoOutput videoOutput
    required property MediaDevicesHelper mediaDevices
    required property RecentSources recentSources

    function playbackStateToString(state) {
        switch (state) {
        case MediaPlayer.PlaybackState.StoppedState: return "Stopped"
        case MediaPlayer.PlaybackState.PlayingState: return "Playing"
        case MediaPlayer.PlaybackState.PausedState: return "Paused"
        default: return "Unknown"
        }
    }

    function mediaStatusToString(status) {
        switch (status) {
        case MediaPlayer.MediaStatus.NoMedia: return "NoMedia"
        case MediaPlayer.MediaStatus.LoadingMedia: return "LoadingMedia"
        case MediaPlayer.MediaStatus.LoadedMedia: return "LoadedMedia"
        case MediaPlayer.MediaStatus.BufferingMedia: return "BufferingMedia"
        case MediaPlayer.MediaStatus.StalledMedia: return "StalledMedia"
        case MediaPlayer.MediaStatus.BufferedMedia: return "BufferedMedia"
        case MediaPlayer.MediaStatus.EndOfMedia: return "EndOfMedia"
        case MediaPlayer.MediaStatus.InvalidMedia: return "InvalidMedia"
        default: return "Unknown"
        }
    }

    function errorToString(error) {
        switch (error) {
        case MediaPlayer.Error.NoError: return "NoError"
        case MediaPlayer.Error.ResourceError: return "ResourceError"
        case MediaPlayer.Error.FormatError: return "FormatError"
        case MediaPlayer.Error.NetworkError: return "NetworkError"
        case MediaPlayer.Error.AccessDeniedError: return "AccessDeniedError"
        default: return "Unknown"
        }
    }

    function pitchCompensationAvailabilityToString(availability) {
        switch (availability) {
        case MediaPlayer.PitchCompensationAvailability.AlwaysOn: return "always on"
        case MediaPlayer.PitchCompensationAvailability.Available: return "available"
        case MediaPlayer.PitchCompensationAvailability.Unavailable: return "unavailable"
        default: return "unknown"
        }
    }

    // Formats milliseconds as m:ss.zzz.
    function formatTime(ms) {
        const totalSeconds = Math.floor(ms / 1000)
        const seconds = totalSeconds % 60
        const millis = Math.floor(ms % 1000)
        return Math.floor(totalSeconds / 60) + ":"
            + String(seconds).padStart(2, "0") + "."
            + String(millis).padStart(3, "0")
    }

    // Builds a ComboBox model for a list of tracks, with an entry first for disabling
    // the track type.
    function buildTrackModel(tracks) {
        return [{ text: "Off", value: -1 }].concat(tracks.map((metaData, index) => {
            const details = [
                metaData.stringValue(MediaMetaData.Key.Title),
                metaData.stringValue(MediaMetaData.Key.Language)
            ].filter((item) => item !== "")
            return {
                text: "Track " + index + (details.length > 0 ? " (" + details.join(", ") + ")" : ""),
                value: index
            }
        }))
    }

    // Playback options only take effect when the source changes, so this sets the
    // source again.
    function reloadSource() {
        const source = root.mediaPlayer.source
        root.mediaPlayer.source = ""
        root.mediaPlayer.source = source
    }

    // Seeks relative to the current position, staying within the media.
    function seekBy(ms) {
        root.mediaPlayer.position = Math.max(
            0,
            Math.min(root.mediaPlayer.duration, root.mediaPlayer.position + ms))
    }

    Label {
        text: "Source"
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: "Current: " + (root.mediaPlayer.source.toString() !== ""
            ? root.mediaPlayer.source
            : "(none)")
        wrapMode: Text.WrapAnywhere
    }

    RowLayout {
        Layout.fillWidth: true
        TextField {
            id: sourceField
            Layout.fillWidth: true
            placeholderText: "URL or file path"
            text: root.mediaPlayer.source
            selectByMouse: true
            onAccepted: () => {
                root.mediaPlayer.source = text
            }
        }
        Button {
            text: "Apply"
            onClicked: () => {
                root.mediaPlayer.source = sourceField.text
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: "Recent:"
        }
        ComboBox {
            Layout.fillWidth: true
            enabled: root.recentSources.sources.length > 0
            textRole: "text"
            valueRole: "value"
            model: root.recentSources.sources.map((source) => {
                return { text: decodeURIComponent(source), value: source }
            })
            // source is a url, which does not compare equal to the strings in the model.
            currentValue: root.mediaPlayer.source.toString()
            displayText: count === 0 ? "None" : (currentIndex === -1 ? "Select…" : currentText)
            onActivated: () => {
                root.mediaPlayer.source = currentValue
            }
        }
        Button {
            text: "Forget all"
            enabled: root.recentSources.sources.length > 0
            onClicked: () => {
                root.recentSources.clear()
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: "Online:"
        }
        ComboBox {
            id: onlineSampleBox
            Layout.fillWidth: true
            textRole: "name"
            valueRole: "url"
            model: QmlMediaPlayerAdvanced.onlineSamples
            // source is a url, which does not compare equal to the strings in the model.
            currentValue: root.mediaPlayer.source.toString()
            displayText: currentIndex === -1 ? "Select…" : currentText
            onActivated: () => {
                root.mediaPlayer.source = currentValue
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: onlineSampleBox.currentIndex !== -1
        text: {
            const sample = QmlMediaPlayerAdvanced.onlineSamples[onlineSampleBox.currentIndex]
            return sample ? sample.description + "\nLicense: " + sample.license : ""
        }
        wrapMode: Text.Wrap
        font.italic: true
    }

    RowLayout {
        Button {
            text: "Open file…"
            onClicked: () => {
                fileDialog.open()
            }
        }
        Button {
            text: "Reload"
            enabled: root.mediaPlayer.source.toString() !== ""
            onClicked: () => {
                root.reloadSource()
            }
        }
        Button {
            text: "Clear"
            enabled: root.mediaPlayer.source.toString() !== ""
            onClicked: () => {
                root.mediaPlayer.source = ""
            }
        }
    }

    FileDialog {
        id: fileDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.MoviesLocation)[0]
        onAccepted: () => {
            root.mediaPlayer.source = selectedFile
        }
    }

    Label {
        Layout.topMargin: 12
        text: "State"
        font.bold: true
    }

    GridLayout {
        columns: 2
        columnSpacing: 12

        Label {
            text: "Playback state:"
        }
        Label {
            text: root.playbackStateToString(root.mediaPlayer.playbackState)
        }

        Label {
            text: "Media status:"
        }
        Label {
            text: root.mediaStatusToString(root.mediaPlayer.mediaStatus)
        }

        Label {
            text: "Position:"
        }
        Label {
            text: root.formatTime(root.mediaPlayer.position)
                + " / " + root.formatTime(root.mediaPlayer.duration)
        }

        Label {
            text: "Buffer progress:"
        }
        Label {
            text: Math.round(root.mediaPlayer.bufferProgress * 100) + "%"
        }

        Label {
            text: "Has audio / video:"
        }
        Label {
            text: (root.mediaPlayer.hasAudio ? "yes" : "no")
                + " / " + (root.mediaPlayer.hasVideo ? "yes" : "no")
        }

        Label {
            text: "Seekable:"
        }
        Label {
            text: root.mediaPlayer.seekable ? "yes" : "no"
        }

        Label {
            text: "Error:"
        }
        Label {
            Layout.fillWidth: true
            text: root.errorToString(root.mediaPlayer.error)
                + (root.mediaPlayer.errorString !== "" ? ": " + root.mediaPlayer.errorString : "")
            color: root.mediaPlayer.error === MediaPlayer.Error.NoError ? palette.text : "red"
            wrapMode: Text.WordWrap
        }
    }

    Label {
        Layout.topMargin: 12
        text: "Playback"
        font.bold: true
    }

    RowLayout {
        Button {
            text: "Play"
            onClicked: () => {
                root.mediaPlayer.play()
            }
        }
        Button {
            text: "Pause"
            onClicked: () => {
                root.mediaPlayer.pause()
            }
        }
        Button {
            text: "Stop"
            onClicked: () => {
                root.mediaPlayer.stop()
            }
        }
    }

    CheckBox {
        text: "Auto play"
        checked: root.mediaPlayer.autoPlay
        onToggled: () => {
            root.mediaPlayer.autoPlay = checked
        }
    }

    RowLayout {
        Label {
            text: "Loops:"
        }
        CheckBox {
            id: infiniteLoopsCheckBox
            text: "Infinite"
            checked: root.mediaPlayer.loops === MediaPlayer.Loops.Infinite
            onToggled: () => {
                root.mediaPlayer.loops = checked ? MediaPlayer.Loops.Infinite : loopsSpinBox.value
            }
        }
        SpinBox {
            id: loopsSpinBox
            enabled: !infiniteLoopsCheckBox.checked
            from: 1
            to: 1000
            editable: true
            value: root.mediaPlayer.loops > 0 ? root.mediaPlayer.loops : 1
            onValueModified: () => {
                root.mediaPlayer.loops = value
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: "Rate:"
        }
        Slider {
            Layout.fillWidth: true
            from: -1.0
            to: 4.0
            value: root.mediaPlayer.playbackRate
            onMoved: () => {
                root.mediaPlayer.playbackRate = value
            }
        }
        Label {
            text: root.mediaPlayer.playbackRate.toFixed(2) + "×"
        }
    }

    RowLayout {
        Repeater {
            model: [0.0, 0.5, 1.0, 1.5, 2.0]
            delegate: Button {
                required property real modelData
                text: modelData + "×"
                onClicked: () => {
                    root.mediaPlayer.playbackRate = modelData
                }
            }
        }
    }

    CheckBox {
        text: "Pitch compensation ("
            + root.pitchCompensationAvailabilityToString(
                root.mediaPlayer.pitchCompensationAvailability)
            + ")"
        enabled: root.mediaPlayer.pitchCompensationAvailability
            === MediaPlayer.PitchCompensationAvailability.Available
        checked: root.mediaPlayer.pitchCompensation
        onToggled: () => {
            root.mediaPlayer.pitchCompensation = checked
        }
    }

    Label {
        Layout.topMargin: 12
        text: "Seeking"
        font.bold: true
    }

    RowLayout {
        enabled: root.mediaPlayer.seekable
        Repeater {
            model: [
                { text: "−10 s", ms: -10000 },
                { text: "−1 s", ms: -1000 },
                { text: "+1 s", ms: 1000 },
                { text: "+10 s", ms: 10000 }
            ]
            delegate: Button {
                required property var modelData
                text: modelData.text
                onClicked: () => {
                    root.seekBy(modelData.ms)
                }
            }
        }
    }

    RowLayout {
        enabled: root.mediaPlayer.seekable
        Label {
            text: "Position (ms):"
        }
        TextField {
            id: seekField
            Layout.fillWidth: true
            placeholderText: String(root.mediaPlayer.position)
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator {
                bottom: 0
            }
            onAccepted: () => {
                root.mediaPlayer.position = parseInt(text)
            }
        }
        Button {
            text: "Seek"
            enabled: seekField.acceptableInput
            onClicked: () => {
                root.mediaPlayer.position = parseInt(seekField.text)
            }
        }
    }

    Label {
        Layout.topMargin: 12
        text: "Tracks"
        font.bold: true
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2

        Label {
            text: "Audio:"
        }
        ComboBox {
            Layout.fillWidth: true
            enabled: root.mediaPlayer.audioTracks.length > 0
            textRole: "text"
            valueRole: "value"
            model: root.buildTrackModel(root.mediaPlayer.audioTracks)
            currentValue: root.mediaPlayer.activeAudioTrack
            onActivated: () => {
                root.mediaPlayer.activeAudioTrack = currentValue
            }
        }

        Label {
            text: "Video:"
        }
        ComboBox {
            Layout.fillWidth: true
            enabled: root.mediaPlayer.videoTracks.length > 0
            textRole: "text"
            valueRole: "value"
            model: root.buildTrackModel(root.mediaPlayer.videoTracks)
            currentValue: root.mediaPlayer.activeVideoTrack
            onActivated: () => {
                root.mediaPlayer.activeVideoTrack = currentValue
            }
        }

        Label {
            text: "Subtitles:"
        }
        ComboBox {
            Layout.fillWidth: true
            enabled: root.mediaPlayer.subtitleTracks.length > 0
            textRole: "text"
            valueRole: "value"
            model: root.buildTrackModel(root.mediaPlayer.subtitleTracks)
            currentValue: root.mediaPlayer.activeSubtitleTrack
            onActivated: () => {
                root.mediaPlayer.activeSubtitleTrack = currentValue
            }
        }
    }

    Label {
        Layout.topMargin: 12
        text: "Outputs"
        font.bold: true
    }

    CheckBox {
        text: "AudioOutput attached"
        checked: root.mediaPlayer.audioOutput !== null
        onToggled: () => {
            root.mediaPlayer.audioOutput = checked ? root.audioOutput : null
        }
    }

    CheckBox {
        text: "VideoOutput attached"
        checked: root.mediaPlayer.videoOutput !== null
        onToggled: () => {
            root.mediaPlayer.videoOutput = checked ? root.videoOutput : null
        }
    }

    AudioOutputDeviceSelector {
        audioOutput: root.audioOutput
        mediaDevices: root.mediaDevices
    }

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: "Volume:"
        }
        Slider {
            Layout.fillWidth: true
            from: 0.0
            to: 1.0
            value: root.audioOutput.volume
            onMoved: () => {
                root.audioOutput.volume = value
            }
        }
        Label {
            text: Math.round(root.audioOutput.volume * 100) + "%"
        }
        CheckBox {
            text: "Muted"
            checked: root.audioOutput.muted
            onToggled: () => {
                root.audioOutput.muted = checked
            }
        }
    }

    Label {
        Layout.topMargin: 12
        text: "Playback options"
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: "Applied the next time the source changes."
        font.italic: true
        wrapMode: Text.WordWrap
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2

        Label {
            text: "Intent:"
        }
        ComboBox {
            Layout.fillWidth: true
            textRole: "text"
            valueRole: "value"
            model: [
                { text: "Playback", value: PlaybackOptions.PlaybackIntent.Playback },
                {
                    text: "Low latency streaming",
                    value: PlaybackOptions.PlaybackIntent.LowLatencyStreaming
                }
            ]
            currentValue: root.mediaPlayer.playbackOptions.playbackIntent
            onActivated: () => {
                root.mediaPlayer.playbackOptions.playbackIntent = currentValue
            }
        }

        Label {
            text: "Network timeout (ms):"
        }
        SpinBox {
            Layout.fillWidth: true
            from: 1
            to: 600000
            stepSize: 1000
            editable: true
            value: root.mediaPlayer.playbackOptions.networkTimeoutMs
            onValueModified: () => {
                root.mediaPlayer.playbackOptions.networkTimeoutMs = value
            }
        }

        Label {
            text: "Probe size (bytes):"
        }
        TextField {
            Layout.fillWidth: true
            placeholderText: "Default"
            text: root.mediaPlayer.playbackOptions.probeSize > 0
                ? root.mediaPlayer.playbackOptions.probeSize
                : ""
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator {
                bottom: 1
            }
            onEditingFinished: () => {
                root.mediaPlayer.playbackOptions.probeSize = text !== "" ? parseInt(text) : -1
            }
        }
    }

    RowLayout {
        Button {
            text: "Reset options"
            onClicked: () => {
                // Assigning undefined resets an option to its default.
                root.mediaPlayer.playbackOptions.playbackIntent = undefined
                root.mediaPlayer.playbackOptions.networkTimeoutMs = undefined
                root.mediaPlayer.playbackOptions.probeSize = undefined
            }
        }
        Button {
            text: "Reload source"
            enabled: root.mediaPlayer.source.toString() !== ""
            onClicked: () => {
                root.reloadSource()
            }
        }
    }

    Label {
        Layout.topMargin: 12
        text: "Metadata"
        font.bold: true
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: 12

        Repeater {
            // Alternating name and value, filling the two grid columns.
            model: root.mediaPlayer.metaData.keys()
                .filter((key) => root.mediaPlayer.metaData.stringValue(key) !== "")
                .reduce((list, key) => list.concat([
                    root.mediaPlayer.metaData.metaDataKeyToString(key) + ":",
                    root.mediaPlayer.metaData.stringValue(key)
                ]), [])
            delegate: Label {
                required property string modelData
                required property int index
                Layout.fillWidth: index % 2 === 1
                text: modelData
                wrapMode: index % 2 === 1 ? Text.WrapAnywhere : Text.NoWrap
            }
        }
    }

    Label {
        visible: root.mediaPlayer.metaData.isEmpty()
        text: "No metadata"
    }
}
