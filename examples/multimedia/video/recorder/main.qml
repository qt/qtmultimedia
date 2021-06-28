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
import QtMultimedia

Window {
    id: root
    width: 800
    height: 600
    visible: true
    title: "Media recorder"

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
    }

    Popup {
        id: recorderError
        anchors.centerIn: Overlay.overlay
        Text { id: recorderErrorText }
    }

    CaptureSession {
        id: captureSession
        encoder: recorder
        audioInput: controls.audioInput
        camera: controls.camera
        videoOutput: playback.active ? null : videoOutput
    }

    MediaRecorder {
        id: recorder
        onRecorderStateChanged:
            (state) => {
                if (state === MediaRecorder.StoppedState)
                    mediaList.append()
                else if (state === MediaRecorder.RecordingState && captureSession.camera)
                    videoOutput.grabToImage(function(res) { mediaList.mediaThumbnail = res.url })
            }
        onActualLocationChanged: (url) => { mediaList.mediaUrl = url }
        onErrorOccurred: { recorderErrorText.text = recorder.errorString; recorderError.open(); }
    }

    Playback {
        id: playback
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: controls.capturesVisible ? mediaListFrame.top : controlsFrame.top
            margins: 50
        }
        videoOutput: videoOutput
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
        height: controlsAndSettings.height
        background: Rectangle {
            anchors.fill: parent
            color: "white"
            opacity: 0.8
        }

        Behavior on height { NumberAnimation { duration: 100 } }

        Column {
            id: controlsAndSettings
            anchors {
                left: parent.left
                right: parent.right
            }
            padding: Style.interSpacing
            spacing: Style.interSpacing

            property real widthWithPadding: width - padding * 2

            Controls {
                id: controls
                width: parent.widthWithPadding
                recorder: recorder
            }

            StyleRectangle {
                visible: controls.settingsVisible
                width: parent.widthWithPadding
                height: 1
            }

            Row {
                visible: controls.settingsVisible
                width: parent.widthWithPadding
                spacing: Style.interSpacing

                SettingsEncoder {
                    recorder: recorder
                }

                SettingsMetaData {
                    height: parent.height
                    recorder: recorder
                }
            }
        }
    }
}
