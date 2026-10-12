// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// Controls for picking the media backend and the Qt Quick graphics API. Both can only
// be chosen before startup, so the choice is saved and applied on the next launch.
ColumnLayout {
    id: root

    Label {
        text: "Media backend"
        font.bold: true
    }

    Label {
        text: "Current: " + QmlMediaPlayerAdvancedHelper.mediaBackendName
    }

    RowLayout {
        Label {
            text: "Preferred:"
        }
        ComboBox {
            // "Default" clears the preference and lets Qt pick the backend.
            textRole: "text"
            valueRole: "value"
            model: [{ text: "Default", value: "" }].concat(
                QmlMediaPlayerAdvancedHelper.availableMediaBackends.map((name) => {
                    return { text: name, value: name }
                }))
            enabled: !QmlMediaPlayerAdvancedHelper.mediaBackendOverriddenByEnvironment
            currentValue: QmlMediaPlayerAdvancedHelper.preferredMediaBackend
            onActivated: () => {
                QmlMediaPlayerAdvancedHelper.setPreferredMediaBackend(currentValue)
            }
        }
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: "orange"
        text: "Restart the application for the media backend change to take effect."
        visible: !QmlMediaPlayerAdvancedHelper.mediaBackendOverriddenByEnvironment
            && QmlMediaPlayerAdvancedHelper.preferredMediaBackend !== ""
            && QmlMediaPlayerAdvancedHelper.preferredMediaBackend
                !== QmlMediaPlayerAdvancedHelper.mediaBackendName
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: "red"
        text: "QT_MEDIA_BACKEND is set in the environment, or --media-backend on the command line; "
            + "the selection above is ignored."
        visible: QmlMediaPlayerAdvancedHelper.mediaBackendOverriddenByEnvironment
    }

    Label {
        Layout.topMargin: 20
        text: "Qt Quick graphics API"
        font.bold: true
    }

    Label {
        id: currentGraphicsApiLabel
        readonly property string apiName:
            QmlMediaPlayerAdvancedHelper.graphicsApiName(GraphicsInfo.api)
        text: "Current: " + apiName
    }

    RowLayout {
        Label {
            text: "Preferred:"
        }
        ComboBox {
            // "Default" clears the preference and lets Qt Quick pick the API.
            textRole: "text"
            valueRole: "value"
            model: [{ text: "Default", value: "" }].concat(
                QmlMediaPlayerAdvancedHelper.availableGraphicsApis.map((name) => {
                    return { text: name, value: name }
                }))
            enabled: !QmlMediaPlayerAdvancedHelper.graphicsApiOverriddenByEnvironment
            currentValue: QmlMediaPlayerAdvancedHelper.preferredGraphicsApi
            onActivated: () => {
                QmlMediaPlayerAdvancedHelper.setPreferredGraphicsApi(currentValue)
            }
        }
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: "orange"
        text: "Restart the application for the graphics API change to take effect."
        visible: !QmlMediaPlayerAdvancedHelper.graphicsApiOverriddenByEnvironment
            && QmlMediaPlayerAdvancedHelper.preferredGraphicsApi !== ""
            && QmlMediaPlayerAdvancedHelper.preferredGraphicsApi !== currentGraphicsApiLabel.apiName
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: "red"
        text: "QSG_RHI_BACKEND, QT_QUICK_BACKEND or QMLSCENE_DEVICE is set in the environment, or "
            + "--graphics-api on the command line; "
            + "the selection above is ignored."
        visible: QmlMediaPlayerAdvancedHelper.graphicsApiOverriddenByEnvironment
    }
}
