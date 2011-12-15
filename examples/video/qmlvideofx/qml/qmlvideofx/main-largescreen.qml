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
    width: 900
    height: 600
    color: "grey"
    property string fileName
    property alias volume: content.volume
    property bool perfMonitorsLogging: false
    property bool perfMonitorsVisible: false

    QtObject {
        id: d
        property string openFileType
    }

    Rectangle {
        id: inner
        anchors.fill: parent
        color: "grey"

        Content {
            id: content
            anchors {
                top: parent.top
                left: parent.left
                margins: 5
            }
            width: 600
            height: 600
        }

        Loader {
            id: performanceLoader
            function init() {
                console.log("[qmlvideofx] performanceLoader.init logging " + root.perfMonitorsLogging + " visible " + root.perfMonitorsVisible)
                var enabled = root.perfMonitorsLogging || root.perfMonitorsVisible
                source = enabled ? "../performancemonitor/PerformanceItem.qml" : ""
            }
            onLoaded: {
                item.parent = content
                item.anchors.top = content.top
                item.anchors.left = content.left
                item.anchors.right = content.right
                item.height = 100
                item.anchors.margins = 5
                item.logging = root.perfMonitorsLogging
                item.displayed = root.perfMonitorsVisible
                item.init()
            }
        }

        ParameterPanel {
            id: parameterPanel
            enabled: numParameters >= 1
            numParameters: content.effect ? content.effect.numParameters : 0
            anchors {
                top: content.bottom
                left: parent.left
                bottom: parent.bottom
            }
            width: content.width
            onParam1ValueChanged: updateParameters()
        }

        EffectSelectionPanel {
            id: effectSelectionPanel
            anchors {
                top: parent.top
                left: content.right
                right: parent.right
                margins: 5
            }
            height: 420
            itemHeight: 40
             onEffectSourceChanged: {
                content.effectSource = effectSource
                updateParameters()
            }
        }

        FileOpen {
            id: fileOpen
            anchors {
                top: effectSelectionPanel.bottom
                left: content.right
                right: parent.right
                bottom: parent.bottom
                margins: 5
            }
            buttonHeight: 32
        }
    }

    Loader {
        id: fileBrowserLoader
    }

    Component.onCompleted: {
        fileOpen.openImage.connect(openImage)
        fileOpen.openVideo.connect(openVideo)
        fileOpen.openCamera.connect(openCamera)
        fileOpen.close.connect(close)
    }

    function init() {
        console.log("[qmlvideofx] main.init")
        content.init()
        performanceLoader.init()
        if (fileName != "") {
            d.openFileType = "video"
            openFile(fileName)
        }
    }

    function qmlFramePainted() {
        if (performanceLoader.item)
            performanceLoader.item.qmlFramePainted()
    }

    function updateParameters() {
        if (content.effect.numParameters >= 1)
            content.effect.param1Value = parameterPanel.param1Value
    }

    function openImage() {
        d.openFileType = "image"
        showFileBrowser("../../images")
    }

    function openVideo() {
        d.openFileType = "video"
        showFileBrowser("../../videos")
    }

    function openCamera() {
        content.openCamera()
    }

    function close() {
        content.openImage("qrc:/images/qt-logo.png")
    }

    function showFileBrowser(path) {
        fileBrowserLoader.source = "FileBrowser.qml"
        fileBrowserLoader.item.parent = root
        fileBrowserLoader.item.anchors.fill = root
        fileBrowserLoader.item.openFile.connect(root.openFile)
        fileBrowserLoader.item.folder = path
        inner.visible = false
    }

    function openFile(path) {
        fileBrowserLoader.source = ""
        if (path != "") {
            if (d.openFileType == "image")
                content.openImage(path)
            else if (d.openFileType == "video")
                content.openVideo(path)
        }
        inner.visible = true
    }
}
