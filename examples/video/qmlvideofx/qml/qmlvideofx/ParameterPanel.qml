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
    color: "transparent"
    height: numParameters * sliderHeight + (numParameters + 1) * spacing
    visible: numParameters > 0
    property color lineColor: "black"
    property int numParameters: 1
    property alias param1Value: slider1.value
    property alias param2Value: slider2.value
    property bool enabled: true
    property real gripSize: 25
    property real spacing: 10
    property real sliderHeight: 40

    Slider {
        id: slider1
        color: "white"
        enabled: parent.enabled
        gripSize: root.gripSize
        height: sliderHeight
        visible: enabled
        anchors {
            left: parent.left
            right: parent.right
            bottom: (root.numParameters == 1) ? root.bottom : slider2.top
            margins: root.spacing
        }
    }

    Slider {
        id: slider2
        color: "white"
        enabled: parent.enabled && root.numParameters >= 2
        gripSize: root.gripSize
        height: sliderHeight
        visible: enabled
        anchors {
            left: parent.left
            right: parent.right
            bottom: root.bottom
            margins: root.spacing
        }
    }
}
