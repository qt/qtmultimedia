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
