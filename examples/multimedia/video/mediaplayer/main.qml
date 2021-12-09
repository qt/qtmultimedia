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
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia


Window {
    id: root
    width: 640
    height: 480
    visible: true
    title: qsTr("Multimedia Player")
    property alias source: mediaPlayer.source

    Popup {
        id: mediaError
        anchors.centerIn: Overlay.overlay
        Text {
            id: mediaErrorText
        }
    }

    MediaPlayer {
        id: mediaPlayer

        function updateMetadata() {
            metadataInfo.clear();
            metadataInfo.read(mediaPlayer.metaData);
            metadataInfo.read(mediaPlayer.audioTracks[mediaPlayer.activeAudioTrack]);
            metadataInfo.read(mediaPlayer.videoTracks[mediaPlayer.activeVideoTrack]);
        }

        videoOutput: videoOutput
        audioOutput: AudioOutput {
            id: audio
            muted: playbackControl.muted
            volume: playbackControl.volume
        }

        onErrorOccurred: { mediaErrorText.text = mediaPlayer.errorString; mediaError.open() }
        onMetaDataChanged: { updateMetadata() }
        onTracksChanged: {
            audioTracksInfo.read(mediaPlayer.audioTracks);
            audioTracksInfo.selectedTrack = mediaPlayer.activeAudioTrack;
            videoTracksInfo.read(mediaPlayer.videoTracks);
            videoTracksInfo.selectedTrack = mediaPlayer.activeVideoTrack;
            subtitleTracksInfo.read(mediaPlayer.subtitleTracks);
            subtitleTracksInfo.selectedTrack = mediaPlayer.activeSubtitleTrack;
            updateMetadata()
        }
        onActiveTracksChanged: { updateMetadata() }
    }

    PlayerMenuBar {
        id: menuBar

        anchors.left: parent.left
        anchors.right: parent.right

        visible: !videoOutput.fullScreen

        mediaPlayer: mediaPlayer
        videoOutput: videoOutput
        metadataInfo: metadataInfo
        audioTracksInfo: audioTracksInfo
        videoTracksInfo: videoTracksInfo
        subtitleTracksInfo: subtitleTracksInfo

        onClosePlayer: root.close()
    }


    VideoOutput {
        id: videoOutput

        property bool fullScreen: false

        anchors.top: fullScreen ? parent.top : menuBar.bottom
        anchors.bottom: playbackControl.top
        anchors.left: parent.left
        anchors.right: parent.right

        TapHandler {
            onDoubleTapped: {
                parent.fullScreen ?  showNormal() : showFullScreen()
                parent.fullScreen = !parent.fullScreen
            }
            onTapped: {
                metadataInfo.visible = false
                audioTracksInfo.visible = false
                videoTracksInfo.visible = false
                subtitleTracksInfo.visible = false
            }
        }
    }

    MetadataInfo {
        id: metadataInfo

        anchors.right: parent.right
        anchors.top: videoOutput.fullScreen ? parent.top : menuBar.bottom
        anchors.bottom: playbackControl.opacity ? playbackControl.bottom : parent.bottom

        visible: false
    }

    TracksInfo {
        id: audioTracksInfo

        anchors.right: parent.right
        anchors.top: videoOutput.fullScreen ? parent.top : menuBar.bottom
        anchors.bottom: playbackControl.opacity ? playbackControl.bottom : parent.bottom

        visible: false
        onSelectedTrackChanged:  mediaPlayer.activeAudioTrack = audioTracksInfo.selectedTrack
    }

    TracksInfo {
        id: videoTracksInfo

        anchors.right: parent.right
        anchors.top: videoOutput.fullScreen ? parent.top : menuBar.bottom
        anchors.bottom: playbackControl.opacity ? playbackControl.bottom : parent.bottom

        visible: false
        onSelectedTrackChanged: mediaPlayer.activeVideoTrack = videoTracksInfo.selectedTrack
    }

    TracksInfo {
        id: subtitleTracksInfo

        anchors.right: parent.right
        anchors.top: videoOutput.fullScreen ? parent.top : menuBar.bottom
        anchors.bottom: playbackControl.opacity ? playbackControl.bottom : parent.bottom

        visible: false
        onSelectedTrackChanged: mediaPlayer.activeSubtitleTrack = subtitleTracksInfo.selectedTrack
    }

    PlaybackControl {
        id: playbackControl

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        mediaPlayer: mediaPlayer
    }
}
