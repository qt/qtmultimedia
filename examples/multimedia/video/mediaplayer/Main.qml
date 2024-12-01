// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import QtMultimedia
import "controls"

//! [0]
ApplicationWindow {
    id: root
    title: qsTr("Multimedia Player")
    width: 1280
    height: 720
    //! [0]
    minimumWidth: 960
    minimumHeight: 540
    visible: true
    color: "black"

    property alias source: mediaPlayer.source
    property alias playbackRate: mediaPlayer.playbackRate
    property bool fullScreen: false

    MessageDialog {
        id: mediaError
        buttons: MessageDialog.Ok
    }

    MouseArea {
        // an activity listener to hide the playback contols when idle
        id: activityListener
        anchors.fill: parent
        z: 1
        propagateComposedEvents: true
        hoverEnabled: true

        property bool inactiveMouse: false

        Timer {
            id: timer
            interval: 1500 // milliseconds
            onTriggered: activityListener.inactiveMouse = true
        }

        function activityHandler(mouse) {
            if (activityListener.inactiveMouse)
                activityListener.inactiveMouse = false
            timer.restart()
            timer.start()
            mouse.accepted = false
        }

        onPositionChanged: mouse => activityHandler(mouse)
        onPressed: mouse => activityHandler(mouse)
        onDoubleClicked: mouse => mouse.accepted = false
    }

    MetadataInfo {
        id: metadataInfo
    }

    TracksInfo {
        id: audioTracksInfo
        onSelectedTrackChanged: {
            mediaPlayer.activeAudioTrack = selectedTrack
            mediaPlayer.updateMetadata()
        }
    }

    TracksInfo {
        id: videoTracksInfo
        onSelectedTrackChanged: {
            mediaPlayer.activeVideoTrack = selectedTrack
            mediaPlayer.updateMetadata()
        }
    }

    TracksInfo {
        id: subtitleTracksInfo
        onSelectedTrackChanged: {
            mediaPlayer.activeSubtitleTrack = selectedTrack
            mediaPlayer.updateMetadata()
        }
    }

    MediaDevices {
        id: mediaDevices
        onAudioOutputsChanged: {
            audio.device = mediaDevices.defaultAudioOutput
        }
    }

    //! [1]
    MediaPlayer {
        id: mediaPlayer
        //! [1]
        function updateMetadata() {
            metadataInfo.clear()
            metadataInfo.read(mediaPlayer.metaData)
            metadataInfo.read(mediaPlayer.audioTracks[mediaPlayer.activeAudioTrack])
            metadataInfo.read(mediaPlayer.videoTracks[mediaPlayer.activeVideoTrack])
            metadataInfo.read(mediaPlayer.subtitleTracks[mediaPlayer.activeSubtitleTrack])
        }
        //! [2]
        videoOutput: videoOutput
        audioOutput: AudioOutput {
            id: audio
            muted: playbackController.muted
            volume: playbackController.volume
        }
        //! [2]
        //! [4]
        onErrorOccurred: {
            mediaError.text = mediaPlayer.errorString
            mediaError.open()
        }
        //! [4]
        onMetaDataChanged: { updateMetadata() }
        //! [6]
        onTracksChanged: {
            audioTracksInfo.read(mediaPlayer.audioTracks)
            videoTracksInfo.read(mediaPlayer.videoTracks)
            subtitleTracksInfo.read(mediaPlayer.subtitleTracks, 6) /* QMediaMetaData::Language = 6 */
            updateMetadata()
            mediaPlayer.play()
        }
        //! [6]
        source: new URL("https://download.qt.io/learning/videos/media-player-example/Qt_LogoMergeEffect.mp4")
    }

    //! [3]
    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        visible: mediaPlayer.mediaStatus > 0

        TapHandler {
            onDoubleTapped: {
                root.fullScreen ?  root.showNormal() : root.showFullScreen()
                root.fullScreen = !root.fullScreen
            }
        }
    }
    //! [3]

    Rectangle {
        anchors.fill: parent
        visible: mediaPlayer.mediaStatus === 0
        color: "black"

        TapHandler {
            onDoubleTapped: {
                root.fullScreen ? root.showNormal() : root.showFullScreen()
                root.fullScreen = !root.fullScreen
            }
        }
    }

    //! [5]
    PlaybackControl {
        id: playbackController
        //! [5]

        property bool showControls: !activityListener.inactiveMouse || busy
        opacity: showControls
        // onOpacityChanged can't be used as it is animated and therefore not immediate
        onShowControlsChanged: activityListener.cursorShape = showControls ?
                              Qt.ArrowCursor : Qt.BlankCursor

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        //! [6]
        mediaPlayer: mediaPlayer
        audioTracksInfo: audioTracksInfo
        videoTracksInfo: videoTracksInfo
        subtitleTracksInfo: subtitleTracksInfo
        metadataInfo: metadataInfo
    }
    //! [6]
}
