// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtCore
import QtQuick
import QtMultimedia

Item {
//! [Camera selection]
MediaDevices {
    id: mediaDevices
}
CaptureSession {
    camera: Camera {
        cameraDevice: mediaDevices.defaultVideoInput
    }
}
//! [Camera selection]

//! [Camera zoom]
Camera {
    zoomFactor: maximumZoomFactor // zoom in as much as possible
}
//! [Camera zoom]

//! [Camera image whitebalance]
Camera {
    whiteBalanceMode: Camera.WhiteBalanceManual
    colorTemperature: 5600
}
//! [Camera image whitebalance]

//! [Camera permission]
CameraPermission {
    id: cameraPermission
}

Camera {
    active: cameraPermission.status === Qt.PermissionStatus.Granted
}

Component.onCompleted: cameraPermission.request()
//! [Camera permission]
}
