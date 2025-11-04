// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma Singleton
import QtCore
import QtQuick

// This helper class allows us to reuse a single Permission component
// instance throughout the app without explicitly passing it, or making
// additional instances in multiple files. This is useful because
// permission components have a quirk where the statusChanged signal is
// only emitted on the instance on which we called .request().
Item {

    CameraPermission {
        id: cameraPermissionInternal
    }

    function requestCamera() {
        cameraPermissionInternal.request()
    }

    MicrophonePermission {
        id: microphonePermissionInternal
    }

    function requestMicrophone() {
        microphonePermissionInternal.request()
    }

    readonly property int cameraStatus: cameraPermissionInternal.status

    readonly property int microphoneStatus: microphonePermissionInternal.status
}
