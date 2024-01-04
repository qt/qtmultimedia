// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import frequencymonitor

Rectangle {
    id: root
    border.color: palette.window
    border.width: showBorder ? 1 : 0
    color: "transparent"
    property string contentType // "camera" or "video"
    property string source
    property real volume
    property bool dummy: false
    property bool autoStart: true
    property bool started: false
    property bool showFrameRate: false
    property bool showBorder: false
    property alias contentItem: contentLoader.item

    signal initialized
    signal error
    signal videoFramePainted

    Loader {
        id: contentLoader
    }

    Connections {
        id: framePaintedConnection
        function onFramePainted() {
            (frameRateLoader.item as FrequencyItem)?.notify()
            root.videoFramePainted()
        }
        ignoreUnknownSignals: true
    }

    Connections {
        id: errorConnection
        function onFatalError() {
            console.log("[qmlvideo] Content.onFatalError")
            root.stop()
            root.error()
        }
        ignoreUnknownSignals: true
    }

    Component {
        id: frequencyItem
        FrequencyItem {}
    }

    Loader {
        id: frameRateLoader
        sourceComponent: root.showFrameRate ? frequencyItem : undefined
        onLoaded: {
            item.parent = root
            item.anchors.top = root.top
            item.anchors.right = root.right
            item.anchors.margins = 10
        }
    }

    onWidthChanged: {
        if (root.contentItem)
            root.contentItem.width = width
    }

    onHeightChanged: {
        if (root.contentItem)
            root.contentItem.height = height
    }

    function initialize() {
        if ("video" == contentType) {
            contentLoader.source = "VideoItem.qml"
            if (Loader.Error == contentLoader.status) {
                contentLoader.source = "VideoDummy.qml"
                dummy = true
            }
            contentLoader.item.volume = volume
        } else if ("camera" == contentType) {
            contentLoader.source = "CameraItem.qml"
            if (Loader.Error == contentLoader.status) {
                contentLoader.source = "CameraDummy.qml"
                dummy = true
            }
        } else {
            console.log("[qmlvideo] Content.initialize: error: invalid contentType")
        }
        if (contentLoader.item) {
            contentLoader.item.sizeChanged.connect(updateRootSize)
            contentLoader.item.parent = root
            contentLoader.item.width = root.width
            framePaintedConnection.target = contentLoader.item
            errorConnection.target = contentLoader.item
            if (root.autoStart)
                root.start()
        }
        root.initialized()
    }

    function start() {
        if (contentLoader.item) {
            if (root.contentType == "video")
                contentLoader.item.mediaSource = root.source
            contentLoader.item.start()
            root.started = true
        }
    }

    // qmllint disable
    function stop() {
        if (contentLoader.item) {
            contentLoader.item.stop()
            if (root.contentType == "video")
                contentLoader.item.mediaSource = ""
            root.started = false
        }
    }
    // qmllint enable

    function updateRootSize() { root.height = (root.contentItem as Item).height }
}
