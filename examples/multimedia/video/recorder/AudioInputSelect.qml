// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtMultimedia

Row {
    id: root
    height: Style.height
    property AudioInput selected: available ? audioInput : null
    property bool available: (typeof comboBox.currentValue !== 'undefined') && audioSwitch.checked

    Component.onCompleted: {
        audioInputModel.populate()
        comboBox.currentIndex = 0
    }

    MediaDevices { id: mediaDevices }

    AudioInput { id: audioInput; muted: !audioSwitch.checked }

    Switch {
        id: audioSwitch;
        height: Style.height;
        checked: true
    }

    ListModel {
        id: audioInputModel
        property var audioInputs: mediaDevices.audioInputs

        function populate() {
            audioInputModel.clear()

            for (var audioDevice of audioInputs)
                audioInputModel.append({ text: audioDevice.description, value:
                                        { type: 'audioDevice', audioDevice: audioDevice } })
        }
    }
    ComboBox {
        id: comboBox
        width: Style.widthLong
        height: Style.height
        background: StyleRectangle { anchors.fill: parent }
        model: audioInputModel
        textRole: "text"
        font.pointSize: Style.fontSize
        displayText: typeof currentValue === 'undefined' ? "unavailable" : currentText
        valueRole: "value"
        onCurrentValueChanged: if (typeof comboBox.currentValue !== 'undefined') audioInput.device = currentValue.audioDevice
    }
}
