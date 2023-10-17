// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtMultimedia

Item {
    id: flashControl

    height: column.height

    property Camera cameraDevice
    property bool mIsFlashSupported: (cameraDevice && cameraDevice.active) ? cameraDevice.isFlashModeSupported(Camera.FlashOn) : false
    property bool mIsTorchSupported: (cameraDevice && cameraDevice.active) ? cameraDevice.isTorchModeSupported(Camera.TorchOn) : false

    Column {
        id: column

        Switch {
            id: flashModeControl
            visible: flashControl.mIsFlashSupported
            opacity: checked ? 0.75 : 0.25
            text: "Flash"
            contentItem: Text {
                text: flashModeControl.text
                color: "white"
                leftPadding: flashModeControl.indicator.width + flashModeControl.spacing
            }

            onPositionChanged: {
                if (position) {
                    if (torchModeControl.checked)
                        torchModeControl.toggle();
                    flashControl.cameraDevice.flashMode = Camera.FlashOn

                } else {
                    flashControl.cameraDevice.flashMode = Camera.FlashOff
                }
            }
        }

        Switch {
            id: torchModeControl
            visible: flashControl.mIsTorchSupported
            opacity: checked ? 0.75 : 0.25
            text: "Torch"
            contentItem: Text {
                text: torchModeControl.text
                color: "white"
                leftPadding: torchModeControl.indicator.width + torchModeControl.spacing
            }

            onPositionChanged: {
                if (position) {
                    if (flashModeControl.checked)
                        flashModeControl.toggle();
                    flashControl.cameraDevice.torchMode = Camera.TorchOn
                } else {
                    flashControl.cameraDevice.torchMode = Camera.TorchOff
                }
            }
        }
    }
}
