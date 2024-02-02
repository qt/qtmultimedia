// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Row {
    id: root
    spacing: Style.intraSpacing

    property alias label: label.text
    property alias model: comboBox.model
    property alias currentIndex: comboBox.currentIndex
    property alias currentValue: comboBox.currentValue
    property bool enabled: true
    signal activated(var currentValue)

    Text {
        id: label
        height: Style.height
        width: Style.widthShort
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        color: root.enabled ? palette.text : palette.mid
        font.pointSize: Style.fontSize
    }

    ComboBox {
        id: comboBox
        height: Style.height
        width: Style.widthLong
        enabled: root.enabled

        displayText: currentText
        textRole: "text"
        valueRole: "value"
        font.pointSize: Style.fontSize

        background: StyleRectangle { anchors.fill: parent }
        onActivated: root.activated(currentValue)
    }
}
