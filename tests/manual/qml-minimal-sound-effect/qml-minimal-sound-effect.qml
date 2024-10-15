// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtMultimedia

ApplicationWindow {
    id: window
    width: 400
    height: 300
    visible: true
    title: qsTr("QmlMinimalSoundEffect")

    SoundEffect {
        id: effect
        source: "qrc:/double-drop.wav"
    }

    Button {
        text: "Play"
        width: 120
        height: 80
        anchors.centerIn: parent
        onClicked: effect.play()
    }
}
