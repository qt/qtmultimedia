// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import FrequencyMonitor 1.0

Rectangle {
    id: root
    property bool logging: true
    property bool displayed: true
    property bool enabled: logging || displayed
    property alias active: monitor.active
    property int samplingInterval: 500
    property color textColor: "yellow"
    property int textSize: 20
    property alias label: monitor.label

    border.width: 1
    border.color: "yellow"
    width: 5.5 * root.textSize
    height: 3.0 * root.textSize
    color: "black"
    opacity: 0.5
    radius: 10
    visible: displayed && active

    // This should ensure that the monitor is on top of all other content
    z: 999

    function notify() {
        monitor.notify()
    }

    FrequencyMonitor {
        id: monitor
        samplingInterval: root.enabled ? root.samplingInterval : 0
        onAverageFrequencyChanged: {
            if (root.logging) trace()
            averageFrequencyText.text = monitor.averageFrequency.toFixed(2)
        }
    }

    Text {
        id: labelText
        anchors {
            left: parent.left
            top: parent.top
            margins: 10
        }
        color: root.textColor
        font.pixelSize: 0.6 * root.textSize
        text: root.label
        width: root.width - 2*anchors.margins
        elide: Text.ElideRight
    }

    Text {
        id: averageFrequencyText
        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: 10
        }
        color: root.textColor
        font.pixelSize: root.textSize
    }
}
