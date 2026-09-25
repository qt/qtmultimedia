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

    required property int preferredLeftWidth

    Label {
        text: "Media backend"
        font.bold: true
    }

    Label {
        text: "Current: " + QmlScreencaptureAdvanced.mediaBackendName
    }

    RowLayout {
        Label {
            text: "Preferred:"
        }
        ComboBox {
            // Entry 0 ("Default") clears the preference and lets Qt pick the backend.
            model: ["Default"].concat(QmlScreencaptureAdvanced.availableMediaBackends)
            enabled: !QmlScreencaptureAdvanced.mediaBackendOverriddenByEnvironment
            currentIndex: count > 0
                ? Math.max(0, model.indexOf(QmlScreencaptureAdvanced.preferredMediaBackend))
                : -1
            onActivated: (index) => {
                QmlScreencaptureAdvanced.setPreferredMediaBackend(index === 0 ? "" : model[index])
            }
        }
    }

    Label {
        Layout.preferredWidth: root.preferredLeftWidth * 2
        wrapMode: Text.WordWrap
        color: "orange"
        text: "Restart the application for the media backend change to take effect."
        visible: !QmlScreencaptureAdvanced.mediaBackendOverriddenByEnvironment
            && QmlScreencaptureAdvanced.preferredMediaBackend !== ""
            && QmlScreencaptureAdvanced.preferredMediaBackend !== QmlScreencaptureAdvanced.mediaBackendName
    }

    Label {
        Layout.preferredWidth: root.preferredLeftWidth * 2
        wrapMode: Text.WordWrap
        color: "red"
        text: "QT_MEDIA_BACKEND is set in the environment; the selection above is ignored."
        visible: QmlScreencaptureAdvanced.mediaBackendOverriddenByEnvironment
    }

    Label {
        Layout.topMargin: 20
        text: "Qt Quick graphics API"
        font.bold: true
    }

    Label {
        id: currentGraphicsApiLabel
        readonly property string apiName: QmlScreencaptureAdvanced.graphicsApiName(GraphicsInfo.api)
        text: "Current: " + apiName
    }

    RowLayout {
        Label {
            text: "Preferred:"
        }
        ComboBox {
            // Entry 0 ("Default") clears the preference and lets Qt Quick pick the API.
            model: ["Default"].concat(QmlScreencaptureAdvanced.availableGraphicsApis)
            enabled: !QmlScreencaptureAdvanced.graphicsApiOverriddenByEnvironment
            currentIndex: count > 0
                ? Math.max(0, model.indexOf(QmlScreencaptureAdvanced.preferredGraphicsApi))
                : -1
            onActivated: (index) => {
                QmlScreencaptureAdvanced.setPreferredGraphicsApi(index === 0 ? "" : model[index])
            }
        }
    }

    Label {
        Layout.preferredWidth: root.preferredLeftWidth * 2
        wrapMode: Text.WordWrap
        color: "orange"
        text: "Restart the application for the graphics API change to take effect."
        visible: !QmlScreencaptureAdvanced.graphicsApiOverriddenByEnvironment
            && QmlScreencaptureAdvanced.preferredGraphicsApi !== ""
            && QmlScreencaptureAdvanced.preferredGraphicsApi !== currentGraphicsApiLabel.apiName
    }

    Label {
        Layout.preferredWidth: root.preferredLeftWidth * 2
        wrapMode: Text.WordWrap
        color: "red"
        text: "QSG_RHI_BACKEND, QT_QUICK_BACKEND or QMLSCENE_DEVICE is set in the environment; "
            + "the selection above is ignored."
        visible: QmlScreencaptureAdvanced.graphicsApiOverriddenByEnvironment
    }
}
