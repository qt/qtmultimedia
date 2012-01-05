/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
    color: "transparent"
    radius: 5
    property alias value: grip.value
    property color fillColor: "white"
    property color lineColor: "black"
    property alias gripSize: grip.width
    property alias enabled: mouseArea.enabled

    Rectangle {
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
        height: 3
        color: displayedColor()

        Rectangle {
            anchors { fill: parent; margins: 1 }
            color: root.fillColor
        }
    }

    Rectangle {
        id: grip
        property real value: 0.5
        x: (value * parent.width) - width/2
        anchors.verticalCenter: parent.verticalCenter
        width: 20
        height: width
        radius: width/2
        color: displayedColor()

        MouseArea {
            id: mouseArea
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

        Rectangle {
            anchors { fill: parent; margins: 1 }
            radius: width/2
            color: root.fillColor
        }
    }

    function displayedColor() {
        var tint = Qt.rgba(fillColor.r, fillColor.g, fillColor.b, 0.25)
        return enabled ? lineColor : Qt.tint(fillColor, tint)
    }
}
