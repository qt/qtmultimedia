// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia

// Opens a separate window on desktop. Mobile platforms do not support multiple windows, so
// there the panel covers the whole screen instead.
Button {
    id: root

    required property string title
    required property Component settings

    readonly property bool previewActive: settingsPopup.visible

    // A capture session has a single video output, so the owner should make this the session's
    // output in place of its own while previewActive.
    readonly property VideoOutput previewOutput: previewVideo

    property bool scrollable: true

    property int windowWidth: 360
    property int windowHeight: 560

    readonly property bool useSeparateWindow: Qt.platform.os !== "android"
        && Qt.platform.os !== "ios"

    // Created on first use, and kept afterwards so it reopens where it was.
    property ApplicationWindow settingsWindow: null

    text: title

    function close() {
        if (root.settingsWindow)
            root.settingsWindow.close()
        settingsPopup.close()
    }

    onClicked: () => {
        if (!root.useSeparateWindow) {
            settingsPopup.open()
            return
        }

        if (!root.settingsWindow)
            root.settingsWindow = settingsWindowComponent.createObject(root)

        root.settingsWindow.show()
        root.settingsWindow.raise()
        root.settingsWindow.requestActivate()
    }

    Component {
        id: settingsWindowComponent

        ApplicationWindow {
            id: settingsWindow

            // Keeps the application from staying alive once the main window is closed.
            transientParent: root.Window.window
            title: root.title
            width: root.windowWidth
            height: root.windowHeight

            ScrollView {
                id: windowScrollView
                anchors.fill: parent
                padding: 4
                contentWidth: availableWidth
                visible: root.scrollable

                Loader {
                    width: windowScrollView.availableWidth
                    active: settingsWindow.visible && root.scrollable
                    sourceComponent: root.settings
                }
            }

            Loader {
                anchors.fill: parent
                anchors.margins: 4
                active: settingsWindow.visible && !root.scrollable
                sourceComponent: root.settings
            }
        }
    }

    Popup {
        id: settingsPopup

        parent: Overlay.overlay
        x: 0
        y: 0
        width: parent ? parent.width : 0
        height: parent ? parent.height : 0
        padding: 0

        modal: true
        dim: false

        focus: true
        closePolicy: Popup.CloseOnEscape

        // Popup does not receive the Android back button by itself. The shortcut stays enabled
        // during the exit transition, since Android backgrounds the application unless the
        // release of the back button is handled too.
        Shortcut {
            sequence: "Back"
            enabled: settingsPopup.visible
            onActivated: () => {
                settingsPopup.close()
            }
        }

        contentItem: Item {
            id: safeAreaItem

            Page {
                id: popupPage

                readonly property bool landscape: width > height

                anchors.fill: parent
                anchors.topMargin: safeAreaItem.SafeArea.margins.top
                anchors.bottomMargin: safeAreaItem.SafeArea.margins.bottom
                anchors.leftMargin: safeAreaItem.SafeArea.margins.left
                anchors.rightMargin: safeAreaItem.SafeArea.margins.right

                header: ToolBar {
                    RowLayout {
                        anchors.fill: parent

                        // iOS has no system back button.
                        ToolButton {
                            text: "‹ Back"
                            onClicked: () => {
                                settingsPopup.close()
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.title
                            font.bold: true
                            elide: Text.ElideRight
                        }
                    }
                }

                GridLayout {
                    anchors.fill: parent
                    columnSpacing: 0
                    rowSpacing: 0

                    VideoOutput {
                        id: previewVideo
                        fillMode: VideoOutput.PreserveAspectCrop

                        Layout.row: 0
                        Layout.column: popupPage.landscape ? 1 : 0
                        Layout.fillWidth: !popupPage.landscape
                        Layout.fillHeight: popupPage.landscape
                        Layout.preferredWidth: popupPage.width * 0.3
                        Layout.preferredHeight: popupPage.height * 0.3
                    }

                    ScrollView {
                        id: popupScrollView
                        padding: 4
                        contentWidth: availableWidth
                        visible: root.scrollable

                        Layout.row: popupPage.landscape ? 0 : 1
                        Layout.column: 0
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Loader {
                            width: popupScrollView.availableWidth
                            active: settingsPopup.visible && root.scrollable
                            sourceComponent: root.settings
                        }
                    }

                    Loader {
                        active: settingsPopup.visible && !root.scrollable
                        visible: active
                        sourceComponent: root.settings

                        Layout.row: popupPage.landscape ? 0 : 1
                        Layout.column: 0
                        Layout.margins: 4
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }
}
