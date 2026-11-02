// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml
import QtMultimedia

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
        Layout.minimumWidth: 0
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
        Layout.minimumWidth: 0
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
        Layout.minimumWidth: 0
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
        Layout.minimumWidth: 0
        wrapMode: Text.WordWrap
        text: "Source rect: " + root.rectToString(root.videoOutput.sourceRect)
    }

    Label {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        wrapMode: Text.WordWrap
        text: "Content rect: " + root.rectToString(root.videoOutput.contentRect)
    }

    // Undefined until a frame has arrived.
    property var lastFrameInfo

    readonly property bool hasFrame: lastFrameInfo !== undefined && lastFrameInfo.valid

    function orNone(valueFn) {
        return hasFrame ? valueFn() : "-"
    }

    Label {
        Layout.topMargin: 8
        text: "Last frame"
        font.bold: true
    }

    GridLayout {
        columns: 2
        columnSpacing: 12
        rowSpacing: 2

        Label {
            text: "Start time"
            opacity: 0.7
        }
        Label {
            text: root.orNone(() => root.lastFrameInfo.startTime < 0
                ? "none"
                : (root.lastFrameInfo.startTime / 1000000).toFixed(3) + " s")
        }

        Label {
            text: "Resolution"
            opacity: 0.7
        }
        Label {
            text: root.orNone(() => root.lastFrameInfo.resolution.width + "x"
                + root.lastFrameInfo.resolution.height)
        }

        Label {
            text: "Pixel format"
            opacity: 0.7
        }
        Label {
            text: root.orNone(() => root.lastFrameInfo.pixelFormat)
        }

        Label {
            text: "Rotation"
            opacity: 0.7
        }
        Label {
            text: root.orNone(() => root.lastFrameInfo.rotation)
        }

        Label {
            text: "Mirrored"
            opacity: 0.7
        }
        Label {
            text: root.orNone(() => root.lastFrameInfo.mirrored)
        }

        Label {
            text: "Surface rotation"
            opacity: 0.7
        }
        Label {
            text: root.orNone(() => root.lastFrameInfo.surfaceFormatRotation)
        }

        Label {
            text: "Surface mirrored"
            opacity: 0.7
        }
        Label {
            text: root.orNone(() => root.lastFrameInfo.surfaceFormatMirrored)
        }

        Label {
            text: "Color space"
            opacity: 0.7
        }
        Label {
            text: root.orNone(() => root.lastFrameInfo.colorSpace)
        }

        Label {
            text: "Color transfer"
            opacity: 0.7
        }
        Label {
            text: root.orNone(() => root.lastFrameInfo.colorTransfer)
        }

        Label {
            text: "Color range"
            opacity: 0.7
        }
        Label {
            text: root.orNone(() => root.lastFrameInfo.colorRange)
        }
    }
}
