/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Rectangle {
    id: root
    anchors.fill: parent
    color: "transparent"
    property alias value: grip.value
    property alias lineWidth: line.width
    property alias gripSize: grip.width

    Rectangle {
        id: grip
        property real value: 0.5
        x: (value * parent.width) - width/2
        anchors.top: parent.top
        width: 20
        height: width
        radius: width/2
        color: "red"

        MouseArea {
            anchors.fill:  parent

            drag {
                target: grip
                axis: Drag.XAxis
                minimumX: -parent.width/2
                maximumX: root.width - parent.width/2
            }

            onPositionChanged:  {
                if (drag.active)
                    updatePosition()
            }

            onReleased: {
                updatePosition()
            }

            function updatePosition() {
                value = (grip.x + grip.width/2) / grip.parent.width
            }
        }
    }

    Rectangle {
        id: line
        anchors { top: parent.top; bottom: parent.bottom }
        x: value * parent.width - (width / 2)
        width: 2
        color: "red"
    }
}
