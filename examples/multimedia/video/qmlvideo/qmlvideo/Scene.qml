// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    color: palette.window
    property string source1
    property string source2
    property int contentWidth: parent.width / 2
    property real volume: 0.25
    property int margins: 5
    property QtObject content

    signal close
    signal videoFramePainted

    Button {
        id: closeButton
        anchors {
            top: parent.top
            right: parent.right
            margins: root.margins
        }
        z: 2.0
        text: qsTr("Back")
        onClicked: root.close()
    }
}
