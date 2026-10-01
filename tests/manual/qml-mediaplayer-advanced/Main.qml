// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

ApplicationWindow {
    id: root
    width: 800
    height: 600
    visible: true
    title: qsTr("QmlMinimalPlayer")

    readonly property double overlayOpacity: 0.75

    RecentSources {
        id: mainRecentSources
    }

    // The source that was last set, restored on the next launch. Empty if the source was
    // cleared.
    Settings {
        id: lastSourceSettings

        category: "Player"

        property string lastSource: ""
    }

    MediaDevicesHelper {
        id: mainMediaDevices

        onAudioOutputsChanged: console.log("Signaled MediaDevices.onAudioOutputsChanged")
    }

    MediaPlayer {
        id: mainMediaPlayer
        autoPlay: false

        // Every source that is set, whether picked from a file dialog or typed in, is
        // remembered for later sessions. Online samples are skipped, since they are always
        // available from their own list.
        onSourceChanged: () => {
            const sourceString = source.toString()
            lastSourceSettings.lastSource = sourceString
            if (sourceString === "")
                return
            const isOnlineSample = QmlMediaPlayerAdvanced.onlineSamples.some((sample) => {
                return sample.url === sourceString
            })
            if (!isOnlineSample)
                mainRecentSources.add(source)
        }
        // Settings has loaded its values by the time any Component.onCompleted runs.
        Component.onCompleted: () => {
            source = lastSourceSettings.lastSource
        }
        videoOutput: mainVideoOutput
        audioOutput: mainAudioOutput


        function mediaStatusToString(input) {
            switch (input) {
            case MediaPlayer.NoMedia: return "NoMedia"
            case MediaPlayer.LoadingMedia: return "LoadingMedia"
            case MediaPlayer.LoadedMedia: return "LoadedMedia"
            case MediaPlayer.BufferingMedia: return "BufferingMedia"
            case MediaPlayer.StalledMedia: return "StalledMedia"
            case MediaPlayer.BufferedMedia: return "BufferedMedia"
            case MediaPlayer.EndOfMedia: return "EndOfMedia"
            case MediaPlayer.InvalidMedia: return "InvalidMedia"
            default: return "Unknown"
            }
        }

        function mediaPlayerErrorToString(input) {
            switch (input) {
            case MediaPlayer.NoError: return "NoError"
            case MediaPlayer.ResourceError: return "ResourceError"
            case MediaPlayer.FormatError: return "FormatError"
            case MediaPlayer.NetworkError: return "NetworkError"
            case MediaPlayer.AccessDeniedError: return "AccessDeniedError"
            default: return "Unknown"
            }
        }

        onActiveAudioTrackChanged: console.log("onActiveAudioTrackChanged: " + activeAudioTrack)

        onErrorOccurred: (error, string) => {
            console.log(
                "MediaPlayer.onErrorOccurred: "
                + "[" + mediaPlayerErrorToString(error) + "]"
                + " "
                + (string ? string : "(no message)"))
        }

        onMediaStatusChanged: {
            console.log("onMediaStatusChanged: " + mediaStatusToString(mediaStatus));
        }

        onPlaybackRateChanged: console.log("onPlaybackRateChanged: " + playbackRate)

        onTracksChanged: {
            console.log("onTracksChanged")
        }
    }


    AudioOutput {
        id: mainAudioOutput
        onDeviceChanged: {
            console.log(
                "Signaled AudioOutput.onDeviceChanged: " + (device ? device.description : "null"))
        }
    }
    VideoOutput {
        id: mainVideoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
        visible: true
    }

    Pane {
        anchors.top: parent.top
        anchors.right: parent.right
        opacity: root.overlayOpacity

        SettingsWindowButton {
            title: "Settings"
            previewItem: mainVideoOutput
            windowWidth: 480
            windowHeight: 640
            settings: Component {
                ColumnLayout {
                    TabBar {
                        id: tabBar
                        Layout.fillWidth: true
                        TabButton {
                            text: "Player"
                        }
                        TabButton {
                            text: "Devices"
                        }
                        TabButton {
                            text: "Backends"
                        }
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        currentIndex: tabBar.currentIndex

                        PlayerSettings {
                            mediaPlayer: mainMediaPlayer
                            audioOutput: mainAudioOutput
                            videoOutput: mainVideoOutput
                            mediaDevices: mainMediaDevices
                            recentSources: mainRecentSources
                        }

                        // Devices page
                        MediaDeviceInfo {
                            mediaDevices: mainMediaDevices
                            audioOutput: mainAudioOutput
                        }

                        BackendSettings {
                        }
                    }
                }
            }
        }
    }

    // Basic playback controls. Everything else is in the settings.
    Pane {
        id: playbackControls
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        opacity: root.overlayOpacity

        // Formats milliseconds as m:ss.
        function formatTime(ms) {
            const totalSeconds = Math.floor(ms / 1000)
            const seconds = totalSeconds % 60
            return Math.floor(totalSeconds / 60) + ":" + (seconds < 10 ? "0" : "") + seconds
        }

        RowLayout {
            anchors.fill: parent

            Button {
                text: mainMediaPlayer.playing ? "Pause" : "Play"
                onClicked: () => {
                    if (mainMediaPlayer.playing)
                        mainMediaPlayer.pause()
                    else
                        mainMediaPlayer.play()
                }
            }
            Button {
                text: "Stop"
                onClicked: () => {
                    mainMediaPlayer.stop()
                }
            }
            Label {
                text: playbackControls.formatTime(mainMediaPlayer.position)
            }
            Slider {
                Layout.fillWidth: true
                enabled: mainMediaPlayer.seekable
                value: mainMediaPlayer.position
                from: 0
                to: mainMediaPlayer.duration
                onMoved: () => {
                    mainMediaPlayer.position = value
                }
            }
            Label {
                text: playbackControls.formatTime(mainMediaPlayer.duration)
            }
        }
    }
}
