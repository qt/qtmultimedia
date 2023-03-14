// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtMultimedia

Row {
    id: root
    height: Style.height
    property Camera selectedCamera: cameraAvailable ? camera : null
    property ScreenCapture selectedScreenCapture: screenAvailable ? screenCapture : null
    property bool cameraAvailable: (comboBox.currentValue === 'camera') && cameraSwitch.checked
    property bool screenAvailable: (comboBox.currentValue === 'screen') && cameraSwitch.checked
    Component.onCompleted: {
        videoSourceModel.populate()
        comboBox.currentIndex = 0
    }

    Camera {
        id: camera
        active: cameraAvailable
    }

    ScreenCapture {
        id: screenCapture
        active: screenAvailable
    }

    MediaDevices { id: mediaDevices }

    Switch {
        id: cameraSwitch
        anchors.verticalCenter: parent.verticalCenter
        checked: true
    }

    ListModel {
        id: videoSourceModel
        property var cameras: mediaDevices.videoInputs
        property var screens: Application.screens

        function populate() {
            videoSourceModel.clear()

            for (var camera of cameras)
                videoSourceModel.append({ text: camera.description, value: 'camera' })

            for (var screen of screens)
                videoSourceModel.append({ text: screen.name, value: 'screen' })
        }
    }

    ComboBox {
        id: comboBox
        width: Style.widthLong
        height: Style.height
        background: StyleRectangle { anchors.fill: parent }
        model: videoSourceModel
        displayText: typeof currentValue === 'undefined' ? "Unavailable" : currentText
        font.pointSize: Style.fontSize
        textRole: "text"
        valueRole: "value"
        onCurrentValueChanged: {
            if (currentValue === 'screen')
                screenCapture.screen = videoSourceModel.screens[currentIndex - videoSourceModel.cameras.length]
            else if (currentValue === 'camera')
                camera.cameraDevice = videoSourceModel.cameras[currentIndex]
        }
    }
}
