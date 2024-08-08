// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtQuick.Dialogs

//! [0]
Item {
    id: playbackController

    required property MediaPlayer mediaPlayer
    required property MetadataInfo metadataInfo
    required property TracksInfo audioTracksInfo
    required property TracksInfo videoTracksInfo
    required property TracksInfo subtitleTracksInfo
    //! [0]

    property alias muted: audioControl.muted
    property alias volume: audioControl.volume

    property bool landscapePlaybackControls: root.width >= 668
    property bool busy: fileDialog.visible
                        || urlPopup.visible
                        || settingsPopup.visible
                        || audioControl.busy
                        || playbackSeekControl.busy

    implicitHeight: landscapePlaybackControls ? 168 : 208

    Behavior on opacity { NumberAnimation { duration: 300 } }

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        onAccepted: {
            playbackController.mediaPlayer.stop()
            playbackController.mediaPlayer.source = fileDialog.selectedFile
            playbackController.mediaPlayer.play()
        }
    }

    UrlPopup {
        id: urlPopup
        anchors.centerIn: Overlay.overlay
        mediaPlayer: playbackController.mediaPlayer
    }

    SettingsPopup {
        id: settingsPopup
        anchors.centerIn: Overlay.overlay

        metadataInfo: playbackController.metadataInfo
        mediaPlayer: playbackController.mediaPlayer
        audioTracksInfo: playbackController.audioTracksInfo
        videoTracksInfo: playbackController.videoTracksInfo
        subtitleTracksInfo: playbackController.subtitleTracksInfo
    }

    component CustomButton: RoundButton {
        implicitWidth: 40
        implicitHeight: 40
        radius: 4
        icon.width: 24
        icon.height: 24
        flat: true
    }

    component CustomRoundButton: RoundButton {
        property int diameter: 40
        Layout.preferredWidth: diameter
        Layout.preferredHeight: diameter
        radius: diameter / 2
        icon.width: 24
        icon.height: 24
    }

    //! [1]
    CustomButton {
        id: fileDialogButton
        icon.source: "../images/open_new.svg"
        flat: false
        onClicked: fileDialog.open()
    }

    CustomButton {
        id: openUrlButton
        icon.source: "../images/link.svg"
        flat: false
        onClicked: urlPopup.open()
    }
    //! [1]

    CustomButton {
        id: loopButton
        icon.source: "../images/loop.svg"
        icon.color: playbackController.mediaPlayer.loops === MediaPlayer.Once ? palette.buttonText : palette.accent
        onClicked: playbackController.mediaPlayer.loops = playbackController.mediaPlayer.loops === MediaPlayer.Once
                   ? MediaPlayer.Infinite
                   : MediaPlayer.Once
    }

    CustomButton {
        id: settingsButton
        icon.source: "../images/more.svg"
        onClicked: settingsPopup.open()
    }

    CustomButton {
        id: fullScreenButton
        icon.source: root.fullScreen ? "../images/zoom_minimize.svg"
                                     : "../images/zoom_maximize.svg"
        onClicked: {
            root.fullScreen ?  root.showNormal() : root.showFullScreen()
            root.fullScreen = !root.fullScreen
        }
    }

    RowLayout {
        id: controlButtons
        spacing: 16

        CustomRoundButton {
            id: backward10Button
            icon.source: "../images/backward10.svg"
            onClicked: {
                const pos = Math.max(0, playbackController.mediaPlayer.position - 10000)
                playbackController.mediaPlayer.setPosition(pos)
            }
        }

        //! [2]
        CustomRoundButton {
            id: playButton
            visible: playbackController.mediaPlayer.playbackState !== MediaPlayer.PlayingState
            icon.source: "../images/play_symbol.svg"
            onClicked: playbackController.mediaPlayer.play()
        }

        CustomRoundButton {
            id: pauseButton
            visible: playbackController.mediaPlayer.playbackState === MediaPlayer.PlayingState
            icon.source: "../images/pause_symbol.svg"
            onClicked: playbackController.mediaPlayer.pause()
        }
        //! [2]

        //! [3]
        CustomRoundButton {
            id: forward10Button
            icon.source: "../images/forward10.svg"
            onClicked: {
                const pos = Math.min(playbackController.mediaPlayer.duration,
                                   playbackController.mediaPlayer.position + 10000)
                playbackController.mediaPlayer.setPosition(pos)
            }
        }
        //! [3]
    } // RowLayout controlButtons

    AudioControl {
        id: audioControl
        showSlider: root.width >= 960
    }

    PlaybackSeekControl {
        id: playbackSeekControl
        Layout.fillWidth: true
        mediaPlayer: playbackController.mediaPlayer
    }

    Frame {
        id: landscapeLayout
        anchors.fill: parent
        padding: 32
        topPadding: 28
        visible: landscapePlaybackControls
        background: Rectangle {
            color: "#F6F6F6"
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 16

            Item {
                Layout.fillWidth: true
                implicitHeight: 40

                LayoutItemProxy {
                    id: fdbProxy
                    target: fileDialogButton
                    anchors.left: parent.left
                }

                LayoutItemProxy {
                    target: openUrlButton
                    anchors.left: fdbProxy.right
                    anchors.leftMargin: 12
                }

                LayoutItemProxy {
                    target: controlButtons
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                LayoutItemProxy {
                    target: loopButton
                    anchors.right: acProxy.left
                    anchors.rightMargin: 12
                }

                LayoutItemProxy {
                    id: acProxy
                    target: audioControl
                    anchors.right: sbProxy.left
                    anchors.rightMargin: 30
                    anchors.verticalCenter: parent.verticalCenter
                }

                LayoutItemProxy {
                    id: sbProxy
                    target: settingsButton
                    anchors.right: fbProxy.left
                    anchors.rightMargin: 12
                }

                LayoutItemProxy {
                    id: fbProxy
                    target: fullScreenButton
                    anchors.right: parent.right
                }
            } // Item

            LayoutItemProxy {
                target: playbackSeekControl
                Layout.topMargin: 16
                Layout.bottomMargin: 16
            }
        }
    } // Frame frame

    Frame {
        id: portraitLayout
        anchors.fill: parent
        padding: 32
        topPadding: 28
        visible: !landscapePlaybackControls
        background: Rectangle {
            color: "#F6F6F6"
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 16

            Item {
                Layout.fillWidth: true
                implicitHeight: 40

                LayoutItemProxy {
                    target: loopButton
                    anchors.right: cbProxy.left
                    anchors.rightMargin: 16
                }

                LayoutItemProxy {
                    id: cbProxy
                    target: controlButtons
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                LayoutItemProxy {
                    target: audioControl
                    anchors.left: cbProxy.right
                    anchors.leftMargin: 16
                }
            }
            Item {
                Layout.fillWidth: true
                implicitHeight: 40

                LayoutItemProxy {
                    id: fdbProxy_
                    target: fileDialogButton
                    anchors.left: parent.left
                }

                LayoutItemProxy {
                    target: openUrlButton
                    anchors.left: fdbProxy_.right
                    anchors.leftMargin: 12
                }

                LayoutItemProxy {
                    target: settingsButton
                    anchors.right: fbProxy_.left
                    anchors.rightMargin: 12
                }

                LayoutItemProxy {
                    id: fbProxy_
                    target: fullScreenButton
                    anchors.right: parent.right
                }
            }

            LayoutItemProxy {
                target: playbackSeekControl
                Layout.topMargin: 8
                Layout.bottomMargin: 8
            }
        }
    }
}
