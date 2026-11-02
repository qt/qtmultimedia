// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 12
    id: root

    required property ImageCaptureHelper imageCapture
    required property CameraHelper camera

    readonly property var allQualities: [
        ImageCapture.VeryLowQuality,
        ImageCapture.LowQuality,
        ImageCapture.NormalQuality,
        ImageCapture.HighQuality,
        ImageCapture.VeryHighQuality]

    function qualityToString(quality) {
        return Qt.enumValueToString(ImageCapture.Quality, quality)
    }

    function sizeToString(size) {
        return size.width + "x" + size.height
    }

    function errorToString(error) {
        return Qt.enumValueToString(ImageCapture.Error, error)
    }

    readonly property url lastPhotoUrl: imageCapture.lastSavedFile === ""
        ? ""
        : (Qt.platform.os === "windows" ? "file:///" : "file://") + imageCapture.lastSavedFile

    // Read from the file, as it can differ from the requested resolution.
    readonly property var lastPhotoInfo: imageCapture.lastSavedFile === ""
        ? ({ valid: false })
        : ImageFileInspector.inspect(imageCapture.lastSavedFile)

    function fileSizeToString(bytes) {
        if (bytes >= 1024 * 1024)
            return (bytes / (1024 * 1024)).toFixed(2) + " MB"
        return (bytes / 1024).toFixed(1) + " kB"
    }

    // Invalid until set, which means the camera's default is used.
    readonly property bool hasResolution: imageCapture.resolution.width > 0
        && imageCapture.resolution.height > 0

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "State"

        GridLayout {
            anchors.fill: parent
            columns: 2
            columnSpacing: 12

            Label {
                text: "Ready"
                opacity: 0.7
            }
            Label {
                text: root.imageCapture.readyForCapture
            }

            Label {
                text: "Error"
                opacity: 0.7
            }
            Label {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                wrapMode: Text.WordWrap
                text: root.imageCapture.error === ImageCapture.NoError
                    ? "None"
                    : root.errorToString(root.imageCapture.error)
                        + ": " + root.imageCapture.errorString
                color: root.imageCapture.error === ImageCapture.NoError ? palette.windowText : "red"
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "Format"

        GridLayout {
            anchors.fill: parent
            columns: 3
            columnSpacing: 12

            Label {
                text: "Quality"
            }
            ComboBox {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.columnSpan: 2
                model: root.allQualities.map(item => {
                    return { value: item, text: root.qualityToString(item) }
                })
                valueRole: "value"
                textRole: "text"
                displayText: root.qualityToString(root.imageCapture.quality)
                currentIndex: model.findIndex(item => item.value === root.imageCapture.quality)
                onActivated: index => {
                    root.imageCapture.quality = currentValue
                }
            }

            Label {
                text: "File format"
            }
            ComboBox {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                model: root.imageCapture.supportedFormats.map(item => {
                    return { value: item, text: root.imageCapture.fileFormatToString(item) }
                })
                valueRole: "value"
                textRole: "text"
                displayText: root.imageCapture.fileFormatToString(root.imageCapture.fileFormat)
                currentIndex: model.findIndex(item => item.value === root.imageCapture.fileFormat)
                onActivated: index => {
                    root.imageCapture.fileFormat = currentValue
                    currentIndex = model.findIndex(
                        item => item.value === root.imageCapture.fileFormat)
                }
            }
            Button {
                text: "Default"
                onClicked: root.imageCapture.fileFormat = undefined
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "Resolution"

        ColumnLayout {
            anchors.fill: parent

            Label {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: root.hasResolution
                    ? "Requested: " + root.sizeToString(root.imageCapture.resolution)
                    : "Using the camera default"
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0

                ComboBox {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    model: root.camera.cameraDevice.photoResolutions
                        .slice()
                        .sort((a, b) => b.width * b.height - a.width * a.height)
                        .map(item => {
                            return { value: item, text: root.sizeToString(item) }
                        })
                    textRole: "text"
                    valueRole: "value"
                    displayText: "Supported resolutions"
                    currentIndex: root.hasResolution
                        ? model.findIndex(item => item.value === root.imageCapture.resolution)
                        : -1
                    onActivated: index => {
                        root.imageCapture.resolution = currentValue
                    }
                }
                Button {
                    text: "Default"
                    onClicked: root.imageCapture.resolution = undefined
                }
            }

            Label {
                text: "Custom"
                opacity: 0.7
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0

                SpinBox {
                    id: widthBox
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    editable: true
                    from: 1
                    to: 16384
                    value: root.hasResolution ? root.imageCapture.resolution.width : 0
                    onValueModified: root.applyResolution()
                }
                Label {
                    text: "x"
                }
                SpinBox {
                    id: heightBox
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    editable: true
                    from: 1
                    to: 16384
                    value: root.hasResolution ? root.imageCapture.resolution.height : 0
                    onValueModified: root.applyResolution()
                }
            }

            Label {
                text: "Try"
                opacity: 0.7
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0

                Repeater {
                    model: [Qt.size(1, 1), Qt.size(1024, 1024), Qt.size(8192, 8192)]

                    Button {
                        required property size modelData

                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: root.sizeToString(modelData)
                        onClicked: root.imageCapture.resolution = modelData
                    }
                }
            }
        }
    }

    // The boxes show 1 while no resolution is set, so editing one sets both.
    function applyResolution() {
        imageCapture.resolution = Qt.size(widthBox.value, heightBox.value)
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "Capture"

        ColumnLayout {
            anchors.fill: parent

            Button {
                text: "Snap"
                enabled: root.imageCapture.readyForCapture
                onClicked: root.imageCapture.captureToFile("")
            }

            Label {
                visible: root.imageCapture.lastSavedFile === ""
                text: "No photo taken yet"
                opacity: 0.6
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 12
                visible: root.imageCapture.lastSavedFile !== ""

                Frame {
                    padding: 1
                    implicitWidth: 100
                    implicitHeight: 100

                    Image {
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        source: root.lastPhotoUrl
                        sourceSize: Qt.size(240, 240)
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 2

                    Label {
                        text: "Resolution"
                        opacity: 0.7
                    }
                    Label {
                        text: root.lastPhotoInfo.valid
                            ? root.sizeToString(root.lastPhotoInfo.resolution)
                            : "-"
                    }

                    Label {
                        text: "Format"
                        opacity: 0.7
                    }
                    Label {
                        text: root.lastPhotoInfo.valid
                            ? root.lastPhotoInfo.format.toUpperCase()
                            : "-"
                    }

                    Label {
                        text: "Size"
                        opacity: 0.7
                    }
                    Label {
                        text: root.lastPhotoInfo.valid
                            ? root.fileSizeToString(root.lastPhotoInfo.fileSize)
                            : "-"
                    }

                    Label {
                        text: "File"
                        opacity: 0.7
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: root.imageCapture.lastSavedFile.split("/").pop()
                        elide: Text.ElideMiddle
                    }
                }
            }

            Button {
                text: "View last photo"
                enabled: root.imageCapture.lastSavedFile !== ""
                onClicked: photoPopup.open()
            }
        }
    }

    Popup {
        id: photoPopup

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: parent ? parent.width * 0.9 : 0
        height: parent ? parent.height * 0.9 : 0
        modal: true
        padding: 8
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        contentItem: Image {
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            // Avoids holding on to a large image while closed.
            source: photoPopup.visible ? root.lastPhotoUrl : ""

            MouseArea {
                anchors.fill: parent
                onClicked: photoPopup.close()
            }
        }
    }
}
