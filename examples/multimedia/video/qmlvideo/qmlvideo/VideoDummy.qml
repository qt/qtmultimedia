// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

// Item which is loaded by VideoItem if Qt Multimedia is not available
Rectangle {
    id: root
    color: "grey"
    height: width
    property int duration: 0
    property int position: 0
    property string source
    property real volume: 1.0
    property real playbackRate: 1.0

    signal fatalError
    signal sizeChanged
    signal framePainted

    Label {
        anchors.fill: parent
        anchors.margins: 10
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Failed to create Video item\n\nCheck that Qt Multimedia is installed")
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
    }

    onWidthChanged: height = width
    onHeightChanged: root.sizeChanged()

    function start() { }
    function stop() { }
    function seek() { }
}
