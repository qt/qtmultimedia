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

// Mouse area which flashes to indicate its location on the screen

import QtQuick 2.0

MouseArea {
    property alias hintColor: hintRect.color
    property bool hintEnabled: true

    Rectangle {
        id: hintRect
        anchors.fill: parent
        color: "yellow"
        opacity: 0

        states: [
            State {
                name: "high"
                PropertyChanges {
                    target: hintRect
                    opacity: 0.8
                }
            },
            State {
                name: "low"
                PropertyChanges {
                    target: hintRect
                    opacity: 0.4
                }
            }
        ]

        transitions: [
            Transition {
                from: "low"
                to: "high"
                SequentialAnimation {
                    NumberAnimation {
                        properties: "opacity"
                        easing.type: Easing.InOutSine
                        duration: 500
                    }
                    ScriptAction { script: hintRect.state = "low" }
                }
            },
            Transition {
                from: "*"
                to: "low"
                SequentialAnimation {
                    NumberAnimation {
                        properties: "opacity"
                        easing.type: Easing.InOutSine
                        duration: 500
                    }
                    ScriptAction { script: hintRect.state = "high" }
                }
            },
            Transition {
                from: "*"
                to: "baseState"
                NumberAnimation {
                    properties: "opacity"
                    easing.type: Easing.InOutSine
                    duration: 500
                }
            }
        ]
    }

    onHintEnabledChanged: hintRect.state = hintEnabled ? "low" : "baseState"

    Component.onCompleted: hintRect.state = "low"
}
