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

    Label {
        text: "Media backend"
        font.bold: true
    }

    RowLayout {
        Label {
            text: "Current: " + QtMultimediaPrivate.mediaBackendName
        }
    }

    RowLayout {
        Label {
            text: "Preferred:"
        }
        ComboBox {
            id: backendComboBox

            // Index 0 ("Default") clears the preference and lets Qt pick the backend.
            readonly property string defaultEntry: "Default"
            model: [defaultEntry].concat(QtMultimediaPrivate.availableBackends)
            enabled: !QtMultimediaPrivate.backendOverriddenByEnvironment

            function syncFromSettings() {
                const pref = QtMultimediaPrivate.preferredBackend
                currentIndex = pref === "" ? 0 : Math.max(0, model.indexOf(pref))
            }
            Component.onCompleted: syncFromSettings()

            onActivated: (index) => {
                QtMultimediaPrivate.setPreferredBackend(index === 0 ? "" : model[index])
            }
        }
    }

    Label {
        color: "orange"
        wrapMode: Text.WordWrap
        Layout.preferredWidth: 400
        text: "Restart the application for the backend change to take effect."
        // Show once the chosen backend differs from the one currently loaded.
        visible: !QtMultimediaPrivate.backendOverriddenByEnvironment
                 && QtMultimediaPrivate.preferredBackend !== ""
                 && QtMultimediaPrivate.preferredBackend !== QtMultimediaPrivate.mediaBackendName
    }

    Label {
        color: "red"
        wrapMode: Text.WordWrap
        Layout.preferredWidth: 400
        text: "QT_MEDIA_BACKEND is set in the environment; the selection above is ignored."
        visible: QtMultimediaPrivate.backendOverriddenByEnvironment
    }

    Label {
        text: "Media devices (" + mediaDevices.videoInputs.length + ")"
        font.bold: true
    }
    ListView {
        id: videoInputsListView
        clip: true
        Layout.preferredWidth: 400
        Layout.preferredHeight: 100
        model: mediaDevices.videoInputs
        delegate: ItemDelegate {
            required property var modelData
            required property int index

            highlighted: ListView.isCurrentItem

            // Item is QCameraDevice
            function buildString(item) {
                let string = ""

                if (String(item.id) === String(top.camera.cameraDevice.id))
                    string += "(Active) "

                if (item.description === "")
                    string += "No description"
                else
                    string += item.description

                if (item.isDefault)
                    string += " (Default)"

                return string
            }
            text: buildString(modelData)

            onClicked: videoInputsListView.currentIndex = index
        }
    }

    Label {
        readonly property var defaultCount:
            mediaDevices.videoInputs
            .reduce((count, item) => { return count + Number(item.isDefault) }, 0)
        color: "red"
        text: defaultCount > 1 ? "Multiple default devices" : "No default device"
        visible: defaultCount > 1 || defaultCount === 0
    }


    Label {
        // video-input might be undefined, in which case we get errors when dereferencing.
        text:
            videoInputsListView.model[videoInputsListView.currentIndex]
            ? ("Formats for "
              + videoInputsListView.model[videoInputsListView.currentIndex].description
              + "("
              + videoFormatsListView.model.length
              + ")")
            : "Formats"
        font.bold: true
    }
    ListView {
        id: videoFormatsListView
        clip: true
        Layout.fillWidth: true
        Layout.preferredHeight: 100
        // Chosen video input can be undefined if there is no device, in which case we get errors
        // when dereferencing.
        model:
            videoInputsListView.model[videoInputsListView.currentIndex]
            ? (videoInputsListView.model[videoInputsListView.currentIndex].videoFormats
               .slice()
               .sort((a, b) => top.camera.customCameraFormatSortDelegate(a, b)))
            : []

        delegate: Label {
            required property int index
            readonly property var item: videoFormatsListView.model[index]
            text: item !== undefined ? top.camera.cameraFormatToString(item) : ""
        }
    }

    Label {
        text: "Supported features (" + top.camera.supportedFeaturesList.length + ")"
        font.bold: true
    }
    ListView {
        id: supportedFeaturesListView
        clip: true
        Layout.preferredWidth: 250
        Layout.preferredHeight: 200
        model: top.camera.allFeatureFlags
        delegate: CheckBox {
            required property int index
            readonly property var item: supportedFeaturesListView.model[index]
            function buildString() {
                return top.camera.featureFlagToString(item)
            }
            text: buildString()
            checked: top.camera.supportedFeatures & item
            onClicked: () => {
                checked = top.camera.supportedFeatures & item
            }
        }
    }
}
