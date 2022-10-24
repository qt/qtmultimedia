// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtMultimedia

Row {
    id: root

    property AudioInput selected: available ? audioInput : null
    property bool available: (typeof comboBox.currentValue !== 'undefined') && audioSwitch.checked

    MediaDevices { id: mediaDevices }

    AudioInput { id: audioInput; muted: !audioSwitch.checked }

    Switch {
        id: audioSwitch;
        height: Style.height;
        checked: true
    }

    ComboBox {
        id: comboBox
        width: Style.widthLong
        height: Style.height
        background: StyleRectangle { anchors.fill: parent }
        model: mediaDevices.audioInputs
        textRole: "description"
        font.pointSize: Style.fontSize
        displayText: typeof currentValue === 'undefined' ? "unavailable" : currentValue.description
        onCurrentValueChanged: if (typeof comboBox.currentValue !== 'undefined') audioInput.device = currentValue
    }
}
