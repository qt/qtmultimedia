// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Window {
    id: root
    visible: true
    title: "Media recorder"
    width: Style.screenWidth
    height: Style.screenHeigth

    onWidthChanged:{
        Style.calculateRatio(root.width, root.height)
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        visible: !playback.playing
    }

    Popup {
        id: recorderError
        anchors.centerIn: Overlay.overlay
        Text { id: recorderErrorText }
    }

    CaptureSession {
        id: captureSession
        recorder: recorder
        audioInput: controls.audioInput
        camera: controls.camera
        screenCapture: controls.screenCapture
        windowCapture: controls.windowCapture
        videoOutput: videoOutput
    }

    MediaRecorder {
        id: recorder
        onRecorderStateChanged:
            (state) => {
                if (state === MediaRecorder.StoppedState) {
                    root.contentOrientation = Qt.PrimaryOrientation
                    mediaList.append()
                } else if (state === MediaRecorder.RecordingState && captureSession.camera) {
                    // lock orientation while recording and create a preview image
                    root.contentOrientation = root.screen.orientation;
                    videoOutput.grabToImage(function(res) { mediaList.mediaThumbnail = res.url })
                }
            }
        onActualLocationChanged: (url) => { mediaList.mediaUrl = url }
        onErrorOccurred: { recorderErrorText.text = recorder.errorString; recorderError.open(); }
    }

    Playback {
        id: playback
        anchors {
            fill: parent
            margins: 50
        }
        active: controls.capturesVisible
    }

    Frame {
        id: mediaListFrame
        height: 150
        width: parent.width
        anchors.bottom: controlsFrame.top
        x: controls.capturesVisible ? 0 : parent.width
        background: Rectangle {
            anchors.fill: parent
            color: "white"
            opacity: 0.8
        }

        Behavior on x { NumberAnimation { duration: 200 } }

        MediaList {
            id: mediaList
            anchors.fill: parent
            playback: playback
        }
    }

    Frame {
        id: controlsFrame

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        height: controls.height + Style.interSpacing * 2 + (settingsEncoder.visible? settingsEncoder.height : 0) +(settingsMetaData.visible? settingsMetaData.height : 0)

        background: Rectangle {
            anchors.fill: parent
            color: "white"
            opacity: 0.8
        }

        Behavior on height { NumberAnimation { duration: 100 } }

        ColumnLayout {
            anchors.fill: parent

            Controls {
                Layout.alignment: Qt.AlignHCenter
                id: controls
                recorder: recorder
            }

            StyleRectangle {
                Layout.alignment: Qt.AlignHCenter
                visible: controls.settingsVisible
                width: controls.width
                height: 1
            }

            SettingsEncoder {

                id:settingsEncoder
                Layout.alignment: Qt.AlignHCenter
                visible: controls.settingsVisible
                padding: Style.interSpacing
                recorder: recorder
            }

            SettingsMetaData {
                id: settingsMetaData
                Layout.alignment: Qt.AlignHCenter
                visible: !Style.isMobile() && controls.settingsVisible
                recorder: recorder
           }
        }
    }
}
