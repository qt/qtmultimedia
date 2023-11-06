// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Item {
    id: root

    required property MediaPlayer mediaPlayer

    RowLayout {
        anchors.fill: parent

        Slider {
            id: slider
            Layout.fillWidth: true
            snapMode: Slider.SnapOnRelease
            enabled: true
            from: 0.5
            to: 2.5
            stepSize: 0.5
            value: 1.0

            onMoved: { mediaPlayer.setPlaybackRate(value) }
        }
        Text {
            text: "Rate " + mediaPlayer.playbackRate + "x"
        }
    }
}
