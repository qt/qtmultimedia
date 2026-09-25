// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml
import QtMultimedia

// Controls for configuring how a VideoOutput presents the captured frames.
ColumnLayout {
    id: root

    required property VideoOutput videoOutput

    function rectToString(rect) {
        return rect.width.toFixed(0) + "x" + rect.height.toFixed(0)
            + " at (" + rect.x.toFixed(0) + ", " + rect.y.toFixed(0) + ")"
    }

    Label {
        text: "Fill mode:"
    }
    ComboBox {
        Layout.fillWidth: true
        textRole: "text"
        valueRole: "value"
        model: [
            { text: "Stretch", value: VideoOutput.Stretch },
            { text: "Preserve aspect fit", value: VideoOutput.PreserveAspectFit },
            { text: "Preserve aspect crop", value: VideoOutput.PreserveAspectCrop }
        ]
        currentIndex: model.findIndex((item) => item.value === root.videoOutput.fillMode)
        onActivated: () => {
            root.videoOutput.fillMode = currentValue
        }
    }

    Label {
        text: "End of stream policy:"
    }
    ComboBox {
        Layout.fillWidth: true
        textRole: "text"
        valueRole: "value"
        model: [
            { text: "Clear output", value: VideoOutput.ClearOutput },
            { text: "Keep last frame", value: VideoOutput.KeepLastFrame }
        ]
        currentIndex: model.findIndex((item) => item.value === root.videoOutput.endOfStreamPolicy)
        onActivated: () => {
            root.videoOutput.endOfStreamPolicy = currentValue
        }
    }

    Label {
        text: "Orientation:"
    }
    ComboBox {
        Layout.fillWidth: true
        model: [0, 90, 180, 270]
        currentIndex: model.indexOf(root.videoOutput.orientation)
        onActivated: (index) => {
            root.videoOutput.orientation = model[index]
        }
    }

    CheckBox {
        text: "Mirrored"
        checked: root.videoOutput.mirrored
        onToggled: () => {
            root.videoOutput.mirrored = checked
        }
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: "Source rect: " + root.rectToString(root.videoOutput.sourceRect)
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: "Content rect: " + root.rectToString(root.videoOutput.contentRect)
    }
}
