// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// A button that opens a panel, such as settings, in front of everything else.
//
// On desktop, the panel opens in a separate window, so it can stay open while using
// the main window, and the effect of changing a setting is visible right away.
// Mobile platforms do not support multiple windows, so the panel covers the whole
// screen instead, and closes on the back button on Android or through its own back
// button.
Button {
    id: root

    required property string title
    required property Component settings

    // Shown as a live preview above the settings on mobile, where the settings panel
    // covers the rest of the application.
    property Item previewItem: null

    // Settings are placed in a scrollable column. Content that should instead fill
    // the whole panel, like a video, can turn this off.
    property bool scrollable: true

    property int windowWidth: 360
    property int windowHeight: 560

    readonly property bool useSeparateWindow: Qt.platform.os !== "android"
        && Qt.platform.os !== "ios"

    // Created on first use, and kept afterwards so it reopens where it was.
    property ApplicationWindow settingsWindow: null

    text: title

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

            // A window with a transient parent is a secondary window, so it does
            // not keep the application running once the main window is closed.
            transientParent: root.Window.window
            title: root.title
            width: root.windowWidth
            height: root.windowHeight

            ScrollView {
                id: windowScrollView
                anchors.fill: parent
                padding: 12
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
                anchors.margins: 12
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

        // Popup does not receive the back button on Android by itself, so close it
        // explicitly. The shortcut stays enabled during the exit transition, since
        // Android moves the application to the background unless the release of
        // the back button is handled too.
        Shortcut {
            sequence: "Back"
            enabled: settingsPopup.visible
            onActivated: () => {
                settingsPopup.close()
            }
        }

        // The popup covers the whole window, including the areas behind the
        // system bars, so keep the actual content inside the safe area.
        contentItem: Item {
            id: safeAreaItem

            Page {
                anchors.fill: parent
                anchors.topMargin: safeAreaItem.SafeArea.margins.top
                anchors.bottomMargin: safeAreaItem.SafeArea.margins.bottom
                anchors.leftMargin: safeAreaItem.SafeArea.margins.left
                anchors.rightMargin: safeAreaItem.SafeArea.margins.right

                header: ToolBar {
                    RowLayout {
                        anchors.fill: parent

                        // iOS has no back button, so the panel needs a way to
                        // close on its own.
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

                // Scales the preview to fit, preserving the aspect ratio of the
                // previewed item.
                Item {
                    id: previewArea
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: visible ? 12 : 0
                    height: visible ? parent.height / 3 : 0
                    visible: root.previewItem !== null

                    readonly property real previewScale: root.previewItem
                        && root.previewItem.width > 0 && root.previewItem.height > 0
                        ? Math.min(width / root.previewItem.width,
                                   height / root.previewItem.height)
                        : 0

                    ShaderEffectSource {
                        anchors.centerIn: parent
                        width: root.previewItem ? root.previewItem.width * previewArea.previewScale : 0
                        height: root.previewItem ? root.previewItem.height * previewArea.previewScale : 0
                        sourceItem: settingsPopup.visible ? root.previewItem : null
                        live: true
                    }
                }

                ScrollView {
                    id: popupScrollView
                    anchors.top: previewArea.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    padding: 12
                    contentWidth: availableWidth
                    visible: root.scrollable

                    Loader {
                        width: popupScrollView.availableWidth
                        active: settingsPopup.visible && root.scrollable
                        sourceComponent: root.settings
                    }
                }

                Loader {
                    anchors.top: previewArea.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 12
                    active: settingsPopup.visible && !root.scrollable
                    sourceComponent: root.settings
                }
            }
        }
    }
}
