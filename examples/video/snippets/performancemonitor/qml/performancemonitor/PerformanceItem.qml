/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Rectangle {
    id: root
    property bool logging: true
    property bool displayed: true
    property bool videoActive
    property int samplingInterval: 500
    property color textColor: "yellow"
    property int textSize: 28
    property int margins: 5

    color: "transparent"

    // This should ensure that the monitor is on top of all other content
    z: 999

    Loader {
        id: videoFrameRateItemLoader
        function init() {
            source = "../frequencymonitor/FrequencyItem.qml"
            item.label = "videoFrameRate"
            item.parent = root
            item.anchors.left = root.left
            item.anchors.top = root.top
            item.anchors.margins = root.margins
            item.logging = root.logging
            item.displayed = root.displayed
            videoFrameRateActiveConnections.target = item
        }

        Connections {
            id: videoFrameRateActiveConnections
            ignoreUnknownSignals: true
            onActiveChanged: root.videoActive = videoFrameRateActiveConnections.target.active
        }
    }

    Loader {
        id: qmlFrameRateItemLoader
        function init() {
            source = "../frequencymonitor/FrequencyItem.qml"
            item.label = "qmlFrameRate"
            item.parent = root
            item.anchors.left = root.left
            item.anchors.bottom = root.bottom
            item.anchors.margins = root.margins
            item.logging = root.logging
            item.displayed = root.displayed
        }
    }

    function init() {
        videoFrameRateItemLoader.init()
        qmlFrameRateItemLoader.init()
    }

    function videoFramePainted() {
        videoFrameRateItemLoader.item.notify()
    }

    function qmlFramePainted() {
        qmlFrameRateItemLoader.item.notify()
    }

    onVideoActiveChanged: {
        videoFrameRateItemLoader.item.active = root.videoActive
    }
}
