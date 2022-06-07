// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtMultimedia

Row {
    id: root
    height: Style.height
    property Camera selected: available ? camera : null
    property bool available: (typeof comboBox.currentValue !== 'undefined') && cameraSwitch.checked

    Camera {
        id: camera
        active: available && selected != null
    }

    MediaDevices { id: mediaDevices }

    Switch {
        id: cameraSwitch
        anchors.verticalCenter: parent.verticalCenter
        checked: true
    }

    ComboBox {
        id: comboBox
        width: Style.widthLong
        height: Style.height
        background: StyleRectangle { anchors.fill: parent }
        model: mediaDevices.videoInputs
        displayText: typeof currentValue === 'undefined' ? "Unavailable" : currentValue.description
        font.pointSize: Style.fontSize
        textRole: "description"
        onCurrentValueChanged: if (typeof comboBox.currentValue !== 'undefined') camera.cameraDevice = currentValue
    }
}
