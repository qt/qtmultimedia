// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtMultimedia
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    id: top
    required property CameraHelper camera
    required property MediaDevicesHelper mediaDevices

    readonly property var selectedDevice: videoInputsListView.currentIndex >= 0
        ? mediaDevices.videoInputs[videoInputsListView.currentIndex]
        : undefined

    readonly property int defaultDeviceCount: mediaDevices.videoInputs
        .reduce((count, item) => count + Number(item.isDefault), 0)

    spacing: 12

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "Media backend"

        ColumnLayout {
            anchors.fill: parent

            GridLayout {
                columns: 2
                columnSpacing: 12

                Label {
                    text: "Current"
                    opacity: 0.7
                }
                Label {
                    text: QtMultimediaPrivate.mediaBackendName
                    font.bold: true
                }

                Label {
                    text: "Preferred"
                    opacity: 0.7
                }
                ComboBox {
                    id: backendComboBox

                    model: ["Default"].concat(QtMultimediaPrivate.availableBackends)
                    enabled: !QtMultimediaPrivate.backendOverriddenByEnvironment
                    currentIndex: count > 0
                        ? Math.max(0, model.indexOf(QtMultimediaPrivate.preferredBackend))
                        : -1
                    onActivated: (index) => {
                        QtMultimediaPrivate.setPreferredBackend(index === 0 ? "" : model[index])
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                wrapMode: Text.WordWrap
                color: "orange"
                text: "Restart the application for the backend change to take effect."
                visible: !QtMultimediaPrivate.backendOverriddenByEnvironment
                    && QtMultimediaPrivate.preferredBackend !== ""
                    && QtMultimediaPrivate.preferredBackend !== QtMultimediaPrivate.mediaBackendName
            }

            Label {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                wrapMode: Text.WordWrap
                color: "red"
                text: "QT_MEDIA_BACKEND is set in the environment; the selection above is ignored."
                visible: QtMultimediaPrivate.backendOverriddenByEnvironment
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "Qt Quick graphics API (RHI)"

        ColumnLayout {
            anchors.fill: parent

            GridLayout {
                columns: 2
                columnSpacing: 12

                Label {
                    text: "Current"
                    opacity: 0.7
                }
                Label {
                    id: currentGraphicsApiLabel
                    text: QtMultimediaPrivate.graphicsApiName(GraphicsInfo.api)
                    font.bold: true
                }

                Label {
                    text: "Preferred"
                    opacity: 0.7
                }
                ComboBox {
                    model: ["Default"].concat(QtMultimediaPrivate.availableGraphicsApis)
                    enabled: !QtMultimediaPrivate.graphicsApiOverriddenByEnvironment
                    currentIndex: count > 0
                        ? Math.max(0, model.indexOf(QtMultimediaPrivate.preferredGraphicsApi))
                        : -1
                    onActivated: (index) => {
                        QtMultimediaPrivate.setPreferredGraphicsApi(index === 0 ? "" : model[index])
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                wrapMode: Text.WordWrap
                color: "orange"
                text: "Restart the application for the graphics API change to take effect."
                visible: !QtMultimediaPrivate.graphicsApiOverriddenByEnvironment
                    && QtMultimediaPrivate.preferredGraphicsApi !== ""
                    && QtMultimediaPrivate.preferredGraphicsApi !== currentGraphicsApiLabel.text
            }

            Label {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                wrapMode: Text.WordWrap
                color: "red"
                text: "QSG_RHI_BACKEND, QT_QUICK_BACKEND or QMLSCENE_DEVICE is set in the "
                    + "environment; the selection above is ignored."
                visible: QtMultimediaPrivate.graphicsApiOverriddenByEnvironment
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "Video inputs (" + top.mediaDevices.videoInputs.length + ")"

        ColumnLayout {
            anchors.fill: parent

            Frame {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: 130
                padding: 1

                ListView {
                    id: videoInputsListView
                    anchors.fill: parent
                    clip: true
                    currentIndex: 0
                    model: top.mediaDevices.videoInputs
                    ScrollBar.vertical: ScrollBar {}

                    delegate: ItemDelegate {
                        id: deviceDelegate

                        required property var modelData
                        required property int index

                        width: ListView.view.width
                        highlighted: ListView.isCurrentItem
                        onClicked: videoInputsListView.currentIndex = index

                        readonly property bool isActive:
                            String(modelData.id) === String(top.camera.cameraDevice.id)

                        contentItem: RowLayout {
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                elide: Text.ElideRight
                                text: deviceDelegate.modelData.description === ""
                                    ? "No description"
                                    : deviceDelegate.modelData.description
                            }
                            Label {
                                visible: deviceDelegate.modelData.isDefault
                                text: "Default"
                                font.pixelSize: 11
                                opacity: 0.7
                            }
                            Label {
                                visible: deviceDelegate.isActive
                                text: "Active"
                                font.pixelSize: 11
                                font.bold: true
                                color: "limegreen"
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: videoInputsListView.count === 0
                        text: "No video inputs"
                        opacity: 0.6
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                wrapMode: Text.WordWrap
                color: "red"
                text: top.defaultDeviceCount > 1 ? "Multiple default devices" : "No default device"
                visible: top.mediaDevices.videoInputs.length > 0 && top.defaultDeviceCount !== 1
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: top.selectedDevice
            ? "Formats for " + top.selectedDevice.description
                + " (" + videoFormatsListView.count + ")"
            : "Formats"

        Frame {
            anchors.fill: parent
            implicitHeight: 160
            padding: 1

            ListView {
                id: videoFormatsListView
                anchors.fill: parent
                clip: true
                ScrollBar.vertical: ScrollBar {}

                model: top.selectedDevice
                    ? top.selectedDevice.videoFormats
                        .slice()
                        .sort((a, b) => top.camera.customCameraFormatSortDelegate(a, b))
                    : []

                delegate: ItemDelegate {
                    required property var modelData

                    width: ListView.view.width
                    height: implicitHeight - 8
                    text: top.camera.cameraFormatToString(modelData)
                    hoverEnabled: false
                }

                Label {
                    anchors.centerIn: parent
                    visible: videoFormatsListView.count === 0
                    text: "No formats"
                    opacity: 0.6
                }
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: top.selectedDevice
            ? "Photo resolutions for " + top.selectedDevice.description
                + " (" + photoResolutionsListView.count + ")"
            : "Photo resolutions"

        Frame {
            anchors.fill: parent
            implicitHeight: 160
            padding: 1

            ListView {
                id: photoResolutionsListView
                anchors.fill: parent
                clip: true
                ScrollBar.vertical: ScrollBar {}

                model: top.selectedDevice
                    ? top.selectedDevice.photoResolutions
                        .slice()
                        .sort((a, b) => b.width * b.height - a.width * a.height)
                    : []

                delegate: ItemDelegate {
                    required property var modelData

                    width: ListView.view.width
                    height: implicitHeight - 8
                    text: modelData.width + "x" + modelData.height
                    hoverEnabled: false
                }

                Label {
                    anchors.centerIn: parent
                    visible: photoResolutionsListView.count === 0
                    text: "No photo resolutions"
                    opacity: 0.6
                }
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        title: "Supported features (" + top.camera.supportedFeaturesList.length + ")"

        ColumnLayout {
            anchors.fill: parent
            spacing: 4

            Repeater {
                model: top.camera.allFeatureFlags

                delegate: RowLayout {
                    id: featureRow

                    required property int modelData
                    readonly property bool supported:
                        (top.camera.supportedFeatures & modelData) !== 0

                    spacing: 8

                    // Some platforms' default fonts lack check mark glyphs.
                    Rectangle {
                        implicitWidth: 12
                        implicitHeight: 12
                        radius: 6
                        color: featureRow.supported ? "limegreen" : "transparent"
                        border.color: featureRow.supported ? "limegreen" : "gray"
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: top.camera.featureFlagToString(featureRow.modelData)
                        opacity: featureRow.supported ? 1.0 : 0.5
                    }
                }
            }
        }
    }
}
