// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia

VideoOutput {
    required property var camera

    fillMode: VideoOutput.PreserveAspectFit

    MediaDevices {
        id: mediaDevices
    }

    MouseArea {
        anchors.fill: parent

        // Threshold for determining a swipe
        property int swipeThreshold: 50

        // Variables to track the start position of the swipe
        property int startY: 0

        // Triggered when the swipe starts
        onPressed: mouse => {
            startY = mouse.y
        }

        function switchDevice(up) {
            if (mediaDevices.videoInputs.length <= 1)
                return

            let index = mediaDevices.videoInputs.findIndex(
                    item => item === myCamera.cameraDevice)
            if (index === null) {
                console.log("Error. Current camera device doesn't exist. Can't choose next device.")
                return
            }
            let newIndex = (up === true ? index + 1 : index - 1 + mediaDevices.videoInputs.length)
                % mediaDevices.videoInputs.length

            parent.camera.cameraDevice = mediaDevices.videoInputs[newIndex]
        }

        // Triggered when the swipe ends
        onReleased: mouse => {

            if (mediaDevices.videoInputs.length <= 1)
                return
            let deltaY = mouse.y - startY

            // Check if the swipe distance is greater than the threshold
            if (Math.abs(deltaY) > swipeThreshold) {
                switchDevice(deltaY < 0)
            }
        }
    }
}
