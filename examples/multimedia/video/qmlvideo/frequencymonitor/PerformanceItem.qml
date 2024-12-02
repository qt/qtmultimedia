// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    property bool logging: true
    property bool displayed: true
    property bool videoActive
    property int margins: 5
    property bool enabled: true

    color: "transparent"

    // This should ensure that the monitor is on top of all other content
    z: 999

    Column {
        id: column
        anchors {
            fill: root
            margins: 10
        }
        spacing: 10
    }

    QtObject {
        id: d
        property Item qmlFrameRateItem: null
        property Item videoFrameRateItem: null
    }

    Connections {
        id: videoFrameRateActiveConnections
        ignoreUnknownSignals: true
        function onActiveChanged() { root.videoActive = videoFrameRateActiveConnections.target.active }
    }

    states: [
        State {
            name: "hidden"
            PropertyChanges {
                root.opacity: 0
            }
        }
    ]

    transitions: [
        Transition {
            from: "*"
            to: "*"
            NumberAnimation {
                properties: "opacity"
                easing.type: Easing.OutQuart
                duration: 500
            }
        }
    ]

    state: enabled ? "baseState" : "hidden"

    function createQmlFrameRateItem() {
        let component = Qt.createComponent("frequencymonitor", "FrequencyItem")
        if (component.status === Component.Ready)
            d.qmlFrameRateItem = component.createObject(column, { label: qsTr("QML frame rate"),
                                                                  displayed: root.displayed,
                                                                  logging: root.logging
                                                                });
    }

    function createVideoFrameRateItem() {
        let component = Qt.createComponent("frequencymonitor", "FrequencyItem")
        if (component.status === Component.Ready)
            d.videoFrameRateItem = component.createObject(column, { label: qsTr("Video frame rate"),
                                                                    displayed: root.displayed,
                                                                    logging: root.logging
                                                                  });
        videoFrameRateActiveConnections.target = d.videoFrameRateItem
    }


    function init() {
        createQmlFrameRateItem()
        createVideoFrameRateItem()
    }

    function videoFramePainted() {
        d.videoFrameRateItem?.notify()
    }

    function qmlFramePainted() {
        d.qmlFrameRateItem?.notify()
    }

    onVideoActiveChanged: {
        if (d.videoFrameRateItem)
            d.videoFrameRateItem.active = root.videoActive
    }
}
