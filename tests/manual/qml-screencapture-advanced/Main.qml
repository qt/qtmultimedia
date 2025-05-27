// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml
import QtMultimedia

ApplicationWindow {
    id: topWindow

    title: "qml-screencapture-advanced"
    width: 800
    height: 600
    visible: true

    Pane {
        anchors.fill: parent
        padding: 0

        ColumnLayout {
            anchors {
                fill: parent
            }
            spacing: 0

            TabBar {
                id: tabBar
                Layout.fillWidth: true
                TabButton {
                    text: "WindowCapture"
                    width: implicitWidth + 20
                }
                TabButton {
                    text: "ScreenCapture"
                    width: implicitWidth + 20
                }
            }

            Pane {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.fillHeight: true

                ScrollView {
                    id: outerScrollView
                    anchors.fill: parent
                    clip: true
                    contentWidth: availableWidth
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    StackLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        currentIndex: tabBar.currentIndex

                        WindowCapturePanel {
                            topWindow: topWindow
                            useLandscapeLayout: outerScrollView.width / outerScrollView.height > 0.75 ? true : false
                        }

                        ScreenCapturePanel {
                            useLandscapeLayout: outerScrollView.width / outerScrollView.height > 0.75 ? true : false
                        }
                    }
                }
            }
        }
    }
}
