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
    color: "transparent"
    height: view.model.count * sliderHeight
    property color lineColor: "black"
    property real gripSize: 25
    property real spacing: 10
    property real sliderHeight: 40

    property ListModel model: ListModel { }

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.5
        radius: 10
    }

    Component {
        id: editDelegate

        Rectangle {
            id: delegate
            width: parent.width
            height: root.sliderHeight
            color: "transparent"

            Text {
                id: text
                text: name
                color: "white"
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: parent.left
                }
                font.pixelSize: 0.5 * parent.height
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                width: 150
            }

            Slider {
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: text.right
                    leftMargin: 20
                    right: parent.right
                    rightMargin: 20
                }
                value: model.value
                onValueChanged: view.model.setProperty(index, "value", value)
            }
        }
    }

    ListView {
        id: view
        anchors.fill: parent
        model: root.model
        delegate: editDelegate
    }
}
