// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Row {
    id: root
    spacing: Style.intraSpacing

    property alias label: label.text
    property alias from: slider.from
    property alias to: slider.to
    property alias value: slider.value
    property bool enabled: true

    signal moved(real currentValue)

    Text {
        id: label
        width: Style.valueWidth
        height: Style.height
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        color: root.enabled ? "black" : "gray"
    }

    Slider {
        id: slider
        anchors.verticalCenter: label.verticalCenter
        width: Style.widthMedium
        enabled: root.enabled
        onMoved: root.moved(value)
    }
}
