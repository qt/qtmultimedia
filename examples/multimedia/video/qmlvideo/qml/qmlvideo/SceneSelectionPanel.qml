/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Rectangle {
    id: root
    property int itemHeight: 25
    property string sceneSource: ""

    ListModel {
        id: list
        ListElement { name: "multi"; source: "SceneMulti.qml" }
        ListElement { name: "video"; source: "VideoBasic.qml" }
        ListElement { name: "video-drag"; source: "VideoDrag.qml" }
        ListElement { name: "video-fillmode"; source: "VideoFillMode.qml" }
        ListElement { name: "video-fullscreen"; source: "VideoFullScreen.qml" }
        ListElement { name: "video-fullscreen-inverted"; source: "VideoFullScreenInverted.qml" }
        ListElement { name: "video-metadata"; source: "VideoMetadata.qml" }
        ListElement { name: "video-move"; source: "VideoMove.qml" }
        ListElement { name: "video-overlay"; source: "VideoOverlay.qml" }
        ListElement { name: "video-playbackrate"; source: "VideoPlaybackRate.qml" }
        ListElement { name: "video-resize"; source: "VideoResize.qml" }
        ListElement { name: "video-rotate"; source: "VideoRotate.qml" }
        ListElement { name: "video-spin"; source: "VideoSpin.qml" }
        ListElement { name: "video-seek"; source: "VideoSeek.qml" }
        ListElement { name: "camera"; source: "CameraBasic.qml" }
        ListElement { name: "camera-drag"; source: "CameraDrag.qml" }
        ListElement { name: "camera-fullscreen"; source: "CameraFullScreen.qml" }
        ListElement { name: "camera-fullscreen-inverted"; source: "CameraFullScreenInverted.qml" }
        ListElement { name: "camera-move"; source: "CameraMove.qml" }
        ListElement { name: "camera-overlay"; source: "CameraOverlay.qml" }
        ListElement { name: "camera-resize"; source: "CameraResize.qml" }
        ListElement { name: "camera-rotate"; source: "CameraRotate.qml" }
        ListElement { name: "camera-spin"; source: "CameraSpin.qml" }
    }

    Component {
        id: delegate
        Item {
            id: delegateItem
            width: root.width
            height: itemHeight

            Button {
                id: selectorItem
                anchors.centerIn: parent
                width: 0.9 * parent.width
                height: 0.8 * itemHeight
                text: name
                onClicked: root.sceneSource = source
            }
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: (itemHeight * list.count) + layout.anchors.topMargin + layout.spacing
        clip: true

        Column {
            id: layout

            anchors {
                fill: parent
                topMargin: 10
            }

            Repeater {
                model: list
                delegate: delegate
            }
        }
    }
}
