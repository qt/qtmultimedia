// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// Asks for the permission needed to capture screens and windows, for showing in
// place of the captured video while the permission is missing.
ColumnLayout {
    id: root

    spacing: 12

    Label {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: "Screen Recording permission is missing. After granting it, restart the "
            + "application for it to take effect."
    }

    Button {
        Layout.alignment: Qt.AlignHCenter
        text: "Request permission"
        onClicked: () => {
            QmlScreencaptureAdvanced.requestScreenCapturePermission()
        }
    }
}
