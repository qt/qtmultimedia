// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: audioController

    property alias busy: slider.pressed
    //! [0]
    property alias muted: muteButton.checked
    property real volume: slider.value
    //! [0]
    property alias showSlider: slider.visible
    property int iconDimension: 24

    implicitHeight: 46
    implicitWidth: mainLayout.width

    RowLayout {
        id: mainLayout
        spacing: 10
        anchors.verticalCenter: parent.verticalCenter

        RoundButton {
            id: muteButton
            implicitHeight: 40
            implicitWidth: 40
            radius: 4
            icon.source: audioController.muted ? "../images/volume_mute.svg" : "../images/volume.svg"
            icon.width: audioController.iconDimension
            icon.height: audioController.iconDimension
            flat: true
            checkable: true
        }

        Slider {
            id: slider
            visible: !audioController.showSlider
            implicitWidth: 136
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            enabled: !audioController.muted
            value: 1
        }
    }
}
