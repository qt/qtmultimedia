/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
            anchors {
                left: parent.left
                bottom: parent.bottom
                right: effectSelectionPanel.left
                margins: 20
            }
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
                parameterPanel.model = content.effect.parameters
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

    FileBrowser {
        id: imageFileBrowser
        anchors.fill: root
        Component.onCompleted: fileSelected.connect(content.openImage)
    }

    FileBrowser {
        id: videoFileBrowser
        anchors.fill: root
        Component.onCompleted: fileSelected.connect(content.openVideo)
    }

    Component.onCompleted: {
        fileOpen.openImage.connect(openImage)
        fileOpen.openVideo.connect(openVideo)
        fileOpen.openCamera.connect(openCamera)
        fileOpen.close.connect(close)
    }

    function init() {
        console.log("[qmlvideofx] main.init")
        imageFileBrowser.folder = imagePath
        videoFileBrowser.folder = videoPath
        content.init()
        performanceLoader.init()
        if (fileName != "")
            content.openVideo(fileName)
    }

    function qmlFramePainted() {
        if (performanceLoader.item)
            performanceLoader.item.qmlFramePainted()
    }

    function openImage() {
        imageFileBrowser.show()
    }

    function openVideo() {
        videoFileBrowser.show()
    }

    function openCamera() {
        content.openCamera()
    }

    function close() {
        content.openImage("qrc:/images/qt-logo.png")
    }
}
