// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia

// CustomMediaDevices inherits MediaDevices but has some extra helper properties such as
// storing all media devices that have been connected so far.
MediaDevices {
    id: mediaDevices

    // Holds all camera devices reported so far in the app session.
    // DeviceID -> { connected: bool, device: cameraDevice }
    readonly property var allCameraDevicesSoFarDict: privateAllCameraDevicesSoFarDict

    property var privateAllCameraDevicesSoFarDict: ({})

    // Takes a list<cameraDevice>
    function privateUpdateAllCameraDevicesSoFar(currentDevices) {
        // We need to do a deep copy and reassign the property in order to trigger
        // changed signals.
        let newDeviceDict = ({});
        for (let key in allCameraDevicesSoFarDict) {
            newDeviceDict[key] = allCameraDevicesSoFarDict[key];
        }

        for (let device of currentDevices) {
            // If not here, insert the object
            newDeviceDict[String(device.id)] = {
                device: device,
                connected: true,
            };
        }

        // For any items in the Dict that are not in current devices, it means that device
        // is not connected.
        for (let deviceId in newDeviceDict) {
            newDeviceDict[deviceId].connected = currentDevices.some((device) => String(device.id) === String(deviceId));
        }
        privateAllCameraDevicesSoFarDict = newDeviceDict;
    }

    onVideoInputsChanged: () => {
        privateUpdateAllCameraDevicesSoFar(videoInputs)
    }

    function isCameraDeviceConnected(cameraDeviceId): bool {
        return videoInputs.some(device => String(device.id) === String(cameraDeviceId))
    }

    // Holds all audio input devices reported so far in the app session.
    // DeviceID -> { connected: bool, device: audioDevice }
    readonly property var allAudioInputsSoFarDict: privateAllAudioInputsSoFarDict

    property var privateAllAudioInputsSoFarDict: ({})

    // Takes a list<audioDevice>
    function privateUpdateAllAudioInputsSoFar(currentDevices) {
        // We need to do a deep copy and reassign the property in order to trigger
        // changed signals.
        let newDeviceDict = ({});
        for (let key in allAudioInputsSoFarDict) {
            newDeviceDict[key] = allAudioInputsSoFarDict[key];
        }
        for (let device of currentDevices) {
            // If not here, insert the object
            newDeviceDict[String(device.id)] = {
                device: device,
                connected: true,
            };
        }
        // For any items in the Dict that are not in current devices, it means that device
        // is not connected.
        for (let deviceId in newDeviceDict) {
            newDeviceDict[deviceId].connected = currentDevices.some((device) => String(device.id) === String(deviceId));
        }
        privateAllAudioInputsSoFarDict = newDeviceDict;
    }

    onAudioInputsChanged: () => {
        privateUpdateAllAudioInputsSoFar(audioInputs)
    }

    function isAudioInputConnected(audioDeviceId): bool {
        return audioInputs.some(device => String(device.id) === String(audioDeviceId))
    }

    // Holds all audio output devices reported so far in the app session.
    // DeviceID -> { connected: bool, device: audioDevice }
    readonly property var allAudioOutputsSoFarDict: privateAllAudioOutputsSoFarDict

    property var privateAllAudioOutputsSoFarDict: ({})

    // Takes a list<audioDevice>
    function privateUpdateAllAudioOutputsSoFar(currentDevices) {
        // We need to do a deep copy and reassign the property in order to trigger
        // changed signals.
        let newDeviceDict = ({});
        for (let key in allAudioOutputsSoFarDict) {
            newDeviceDict[key] = allAudioOutputsSoFarDict[key];
        }
        for (let device of currentDevices) {
            // If not here, insert the object
            newDeviceDict[String(device.id)] = {
                device: device,
                connected: true,
            };
        }
        // For any items in the Dict that are not in current devices, it means that device
        // is not connected.
        for (let deviceId in newDeviceDict) {
            newDeviceDict[deviceId].connected = currentDevices.some((device) => String(device.id) === String(deviceId));
        }
        privateAllAudioOutputsSoFarDict = newDeviceDict;
    }

    onAudioOutputsChanged: () => {
        privateUpdateAllAudioOutputsSoFar(audioOutputs)
    }

    function isAudioOutputConnected(audioDeviceId): bool {
        return audioOutputs.some(device => String(device.id) === String(audioDeviceId))
    }

    Component.onCompleted: () => {
        privateUpdateAllCameraDevicesSoFar(videoInputs);
        privateUpdateAllAudioInputsSoFar(audioInputs);
        privateUpdateAllAudioOutputsSoFar(audioOutputs);
    }
}
