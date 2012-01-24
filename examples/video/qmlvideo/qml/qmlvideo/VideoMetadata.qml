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

Scene {
    id: root
    property string contentType: "video"

    Content {
        id: content
        anchors.centerIn: parent
        width: parent.contentWidth
        contentType: "video"
        source: parent.source1
        volume: parent.volume
        onInitialized: {
            if (!dummy)
                metadata.createObject(root)
        }
        onVideoFramePainted: root.videoFramePainted()
    }

    Component {
        id: metadata
        Column {
            anchors.fill: parent
            Text {
                color: "yellow"
                text: "Title:" + content.contentItem().metaData.title
            }
            Text {
                color: "yellow"
                text: "Size:" + content.contentItem().metaData.size
            }
            Text {
                color: "yellow"
                text: "Resolution:" + content.contentItem().metaData.resolution
            }
            Text {
                color: "yellow"
                text: "Media type:" + content.contentItem().metaData.mediaType
            }
            Text {
                color: "yellow"
                text: "Video codec:" + content.contentItem().metaData.videoCodec
            }
            Text {
                color: "yellow"
                text: "Video bit rate:" + content.contentItem().metaData.videoBitRate
            }
            Text {
                color: "yellow"
                text: "Video frame rate:" +content.contentItem().metaData.videoFrameRate
            }
            Text {
                color: "yellow"
                text: "Audio codec:" + content.contentItem().metaData.audioCodec
            }
            Text {
                color: "yellow"
                text: "Audio bit rate:" + content.contentItem().metaData.audioBitRate
            }
            Text {
                color: "yellow"
                text: "Date:" + content.contentItem().metaData.date
            }
            Text {
                color: "yellow"
                text: "Description:" + content.contentItem().metaData.description
            }
            Text {
                color: "yellow"
                text: "Copyright:" + content.contentItem().metaData.copyright
            }
            Text {
                color: "yellow"
                text: "Seekable:" + content.contentItem().metaData.seekable
            }
        }
    }

    Component.onCompleted: root.content = content
}
