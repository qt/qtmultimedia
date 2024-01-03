// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia

Popup {
    id: settingsController
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: "#F6F6F6"
    }


    required property MetadataInfo metadataInfo
    required property MediaPlayer mediaPlayer
    required property TracksInfo audioTracksInfo
    required property TracksInfo videoTracksInfo
    required property TracksInfo subtitleTracksInfo

    property int vPadding: 20
    property int hPadding: 26
    property bool landscapeSettingsPopup: root.width >= settingsLayout.width + metadataLayout.width + 2 * hPadding + 20 + 24

    padding: {
        top: vPadding
        bottom: vPadding
        left: hPadding
        right: hPadding
    }

    Flickable {
        id: flickable
        implicitWidth: mainLayout.width
        implicitHeight: landscapeSettingsPopup ? 200 : 340
        contentWidth: mainLayout.width
        contentHeight: mainLayout.height
        flickableDirection: Flickable.VerticalFlick
        clip: true

        GridLayout {
            id: mainLayout

            columns: landscapeSettingsPopup ? 2 : 1
            columnSpacing: 24
            rowSpacing: 24

            ColumnLayout {
                id: settingsLayout
                spacing: 16
                Layout.alignment: Qt.AlignTop

                Label {
                    id: settingsLabel
                    text: qsTr("Settings")
                    font.pixelSize: 16
                    font.bold: true
                }

                GridLayout {
                    id: gridLayout
                    columns: 2
                    rowSpacing: 16
                    columnSpacing: 16

                    component CustomComboBox: ComboBox {
                        required property TracksInfo tracksInfo

                        model: tracksInfo.model
                        enabled: model.count > 0
                        textRole: "data"
                        currentIndex: model.count > 0 ? 0 : -1

                        onActivated: {
                            //! [1]
                            settingsController.mediaPlayer.pause()
                            tracksInfo.selectedTrack = currentIndex
                            settingsController.mediaPlayer.play()
                            //! [1]
                        }
                    }

                    Label {
                        text: qsTr("Playback Speed")
                        Layout.fillWidth: true
                        font.pixelSize: 14
                    }

                    ComboBox {
                        id: rateCb
                        model: ["0.25", "0.5", "0.75", "Normal", "1.25", "1.5", "1.75", "2"]
                        currentIndex: 3

                        onCurrentIndexChanged: {
                            //! [0]
                            settingsController.mediaPlayer.playbackRate = (currentIndex + 1) * 0.25
                            //! [0]
                        }
                    }

                    Label {
                        text: qsTr("Audio Tracks")
                        enabled: audioCb.enabled
                        Layout.fillWidth: true
                        font.pixelSize: 14
                    }

                    CustomComboBox {
                        id: audioCb
                        tracksInfo: settingsController.audioTracksInfo

                    }

                    Label {
                        text: qsTr("Video Tracks")
                        enabled: videoCb.enabled
                        Layout.fillWidth: true
                        font.pixelSize: 14
                    }

                    CustomComboBox {
                        id: videoCb
                        tracksInfo: settingsController.videoTracksInfo
                    }

                    Label {
                        text: qsTr("Subtitle Tracks")
                        enabled: subtitlesCb.enabled
                        font.pixelSize: 14
                    }

                    CustomComboBox {
                        id: subtitlesCb
                        tracksInfo: settingsController.subtitleTracksInfo
                    }
                }
            }

            ColumnLayout {
                id: metadataLayout
                spacing: 16
                Layout.alignment: Qt.AlignTop

                Label {
                   id: metadataLabel
                    text: qsTr("Metadata")
                    font.pixelSize: 16
                    font.bold: true
                }

                Rectangle {
                    id: metadataRect
                    implicitWidth: 240
                    implicitHeight: metadataList.height
                    border.color: "#8E8E93"
                    radius: 6

                    Column {
                        id: metadataList
                        visible: settingsController.metadataInfo.count > 0

                        padding: 10
                        Repeater {
                            Row {
                                spacing: metadataList.padding
                                Text {
                                    text: model.name
                                    font.bold: true
                                    width: (metadataRect.width - 3 * metadataList.padding) / 2
                                    horizontalAlignment: Text.AlignRight
                                    wrapMode: Text.WordWrap
                                    font.pixelSize: 12
                                }

                                Text {
                                    text: model.value
                                    width: (metadataRect.width - 3 * metadataList.padding) / 2
                                    horizontalAlignment: Text.AlignLeft
                                    anchors.verticalCenter: parent.verticalCenter
                                    wrapMode: Text.WrapAnywhere
                                    font.pixelSize: 12
                                }
                            }
                            model: settingsController.metadataInfo.metadata
                        }
                    }

                    Text {
                        id: metadataNoList
                        visible: settingsController.metadataInfo.count === 0
                        anchors.centerIn: parent
                        text: qsTr("No metadata present")
                        font.pixelSize: 14
                    }
                }
            }
        }
    }

}
