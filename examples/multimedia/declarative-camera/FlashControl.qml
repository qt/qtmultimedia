// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtMultimedia

Item {
    id: topItem

    height: column.height

    //! [0]
    required property Camera camera

    property bool mIsFlashSupported: camera.isFlashModeSupported(Camera.FlashOn)
    property bool mIsTorchSupported: camera.isTorchModeSupported(Camera.TorchOn)
    //! [0]

    // Because the function 'camera.isFlashModeSupported()' is not a reactive binding
    // we must explicitly check if the flash mode is still supported when we change
    // the camera-device.
    Connections {
        target: topItem.camera
        function onCameraDeviceChanged() {
            topItem.mIsFlashSupported = topItem.camera.isFlashModeSupported(Camera.FlashOn)
            topItem.mIsTorchSupported = topItem.camera.isTorchModeSupported(Camera.TorchOn)
        }
    }

    Column {
        id: column

        //! [1]
        Switch {
            id: flashModeControl
            visible: topItem.mIsFlashSupported
            checked: topItem.camera.flashMode === Camera.FlashOn
            opacity: checked ? 0.75 : 0.25
            text: "Flash"

            contentItem: Text {
                text: flashModeControl.text
                color: "white"
                leftPadding: flashModeControl.indicator.width + flashModeControl.spacing
            }

            onClicked: topItem.camera.flashMode = checked ? Camera.FlashOn : Camera.FlashOff
        }
        //! [1]

        Switch {
            id: torchModeControl
            visible: topItem.mIsTorchSupported
            checked: topItem.camera.torchMode === Camera.FlashOn
            opacity: checked ? 0.75 : 0.25
            text: "Torch"

            contentItem: Text {
                text: torchModeControl.text
                color: "white"
                leftPadding: torchModeControl.indicator.width + torchModeControl.spacing
            }

            onClicked: topItem.camera.torchMode = checked ? Camera.TorchOn : Camera.TorchOff
        }
    }
}
