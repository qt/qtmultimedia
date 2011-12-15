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
    property int itemHeight: 25
    property string effectSource: ""

    signal clicked

    QtObject {
        id: d
        property Item selectedItem
    }

    ListModel {
        id: sources
        ListElement { name: "No effect"; source: "EffectPassThrough.qml" }
        ListElement { name: "Billboard"; source: "EffectBillboard.qml" }
        ListElement { name: "Black & white"; source: "EffectBlackAndWhite.qml" }
        ListElement { name: "Blur"; source: "EffectGaussianBlur.qml" }
        ListElement { name: "Edge detection"; source: "EffectSobelEdgeDetection1.qml" }
        //ListElement { name: "Edge detection (Sobel, #2)"; source: "EffectSobelEdgeDetection2.qml" }
        ListElement { name: "Emboss"; source: "EffectEmboss.qml" }
        ListElement { name: "Glow"; source: "EffectGlow.qml" }
        ListElement { name: "Isolate"; source: "EffectIsolate.qml" }
        ListElement { name: "Magnify"; source: "EffectMagnify.qml" }
        ListElement { name: "Page curl"; source: "EffectPageCurl.qml" }
        ListElement { name: "Pixelate"; source: "EffectPixelate.qml" }
        ListElement { name: "Posterize"; source: "EffectPosterize.qml" }
        ListElement { name: "Ripple"; source: "EffectRipple.qml" }
        ListElement { name: "Sepia"; source: "EffectSepia.qml" }
        ListElement { name: "Sharpen"; source: "EffectSharpen.qml" }
        ListElement { name: "Shockwave"; source: "EffectShockwave.qml" }
        ListElement { name: "Tilt shift"; source: "EffectTiltShift.qml" }
        ListElement { name: "Toon"; source: "EffectToon.qml" }
        ListElement { name: "Warhol"; source: "EffectWarhol.qml" }
        ListElement { name: "Wobble"; source: "EffectWobble.qml" }
        ListElement { name: "Vignette"; source: "EffectVignette.qml" }
    }

    Component {
        id: sourceDelegate
        Item {
            id: sourceDelegateItem
            width: root.width
            height: itemHeight

            Button {
                id: sourceSelectorItem
                anchors.centerIn: parent
                width: 0.9 * parent.width
                height: 0.8 * itemHeight
                text: name
                onClicked: {
                    if (d.selectedItem)
                        d.selectedItem.state = "baseState"
                    d.selectedItem = sourceDelegateItem
                    d.selectedItem.state = "selected"
                    effectSource = source
                    root.clicked()
                }
            }

            states: [
                State {
                    name: "selected"
                    PropertyChanges {
                        target: sourceSelectorItem
                        bgColor: "#ff8888"
                    }
                }
            ]

            Component.onCompleted: {
                if (name == "No effect") {
                    state = "selected"
                    d.selectedItem = sourceDelegateItem
                }
            }

            transitions: [
                Transition {
                    from: "*"
                    to: "*"
                    ColorAnimation {
                        properties: "color"
                        easing.type: Easing.OutQuart
                        duration: 500
                    }
                }
            ]
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: (itemHeight * sources.count) + layout.anchors.topMargin + layout.spacing
        clip: true

        Column {
            id: layout

            anchors {
                fill: parent
                topMargin: 10
            }

            Repeater {
                model: sources
                delegate: sourceDelegate
            }
        }
    }
}
