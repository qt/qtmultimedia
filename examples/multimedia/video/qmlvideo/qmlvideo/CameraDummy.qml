// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

// Item which is loaded by CameraItem if Qt Multimedia is not available
Rectangle {
    id: root
    color: "grey"
    height: width

    signal fatalError
    signal sizeChanged
    signal framePainted

    Text {
        anchors.fill: parent
        anchors.margins: 10
        color: "white"
        horizontalAlignment: Text.AlignHCenter
        text: "Failed to create Camera item\n\nCheck that Qt Multimedia is installed"
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
    }

    onWidthChanged: height = width
    onHeightChanged: root.sizeChanged()

    function start() { }
    function stop() { }
}
