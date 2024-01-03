// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Item {
    id: seekController
    required property MediaPlayer mediaPlayer
    property alias busy: slider.pressed

    implicitHeight: 20

    function formatToMinutes(milliseconds) {
        const min = Math.floor(milliseconds / 60000)
        const sec = ((milliseconds - min * 60000) / 1000).toFixed(1)
        return `${min}:${sec.padStart(4, 0)}`
    }

    RowLayout {
        anchors.fill: parent
        spacing: 22

        //! [0]
        Text {
            id: currentTime
            Layout.preferredWidth: 45
            text: seekController.formatToMinutes(seekController.mediaPlayer.position)
            horizontalAlignment: Text.AlignLeft
            font.pixelSize: 11
        }
        //! [0]

        Slider {
            id: slider
            Layout.fillWidth: true
            //! [2]
            enabled: seekController.mediaPlayer.seekable
            value: seekController.mediaPlayer.position / seekController.mediaPlayer.duration
            //! [2]
            onMoved: seekController.mediaPlayer.setPosition(value * seekController.mediaPlayer.duration)
        }

        //! [1]
        Text {
            id: remainingTime
            Layout.preferredWidth: 45
            text: seekController.formatToMinutes(seekController.mediaPlayer.duration - seekController.mediaPlayer.position)
            horizontalAlignment: Text.AlignRight
            font.pixelSize: 11
        }
        //! [1]
    }
}
