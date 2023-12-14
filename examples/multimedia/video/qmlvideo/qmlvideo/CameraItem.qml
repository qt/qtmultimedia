// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtMultimedia

Item {
    id: root
    height: width

    signal fatalError
    signal sizeChanged

    onHeightChanged: root.sizeChanged()

    CaptureSession {
        camera: Camera {
            id: camera

            onErrorOccurred: function(error, errorString) {
                if (Camera.NoError !== error) {
                    console.log("[qmlvideo] CameraItem.onError error " + error + " errorString " + errorString)
                    root.fatalError()
                }
            }
        }
        imageCapture: ImageCapture {
            id: imageCapture
        }

        recorder: MediaRecorder {
            id: recorder
//             resolution: "640x480"
//             frameRate: 30
        }
        videoOutput: videoOutput
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
    }


    function start() { camera.start() }
    function stop() { camera.stop() }
}
