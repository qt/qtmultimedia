// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    required property CameraHelper camera
    required property MediaDevicesHelper mediaDevices
    required property var captureSession
    required property real backgroundOpacity
    readonly property bool clearCameraPropertiesOnDeviceChange: clearUponChangeCheckbox.checked

    id: top
    padding: 0

    ColumnLayout {
        anchors.fill: parent

        Pane {
            Layout.fillWidth: true
            background: null
            Button {
                anchors.fill: parent
                text: "Hide"
                onClicked: () => {
                    window.showingAdvancedPanel = !window.showingAdvancedPanel
                }
            }
        }

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            TabButton {
                text: "QCamera"
                width: implicitWidth + 20
            }
            TabButton {
                text: "Extras"
                width: implicitWidth + 20
            }
            TabButton {
                text: "Info"
                width: implicitWidth + 20
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Pane {
                background: null
                StackLayout {
                    currentIndex: tabBar.currentIndex
                    AllCameraControls {
                        captureSession: top.captureSession
                        camera: top.camera
                        mediaDevices: top.mediaDevices
                    }

                    // Extras
                    ScrollView {
                        CheckBox {
                            id: clearUponChangeCheckbox
                            text: "Clear properties on device change"
                        }
                    }

                    // Info
                    ScrollView {
                        Layout.fillWidth: true
                        InfoPage {
                            Layout.fillWidth: true
                            camera: top.camera
                            mediaDevices: top.mediaDevices
                        }
                    }
                }
            }
        }
    }
}

