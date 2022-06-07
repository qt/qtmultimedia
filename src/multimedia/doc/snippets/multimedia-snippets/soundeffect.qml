// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtMultimedia

//! [complete snippet]
Text {
    text: "Click Me!";
    font.pointSize: 24;
    width: 150; height: 50;

    //! [play sound on click]
    SoundEffect {
        id: playSound
        source: "soundeffect.wav"
    }
    MouseArea {
        id: playArea
        anchors.fill: parent
        onPressed: { playSound.play() }
    }
    //! [play sound on click]
}
//! [complete snippet]
