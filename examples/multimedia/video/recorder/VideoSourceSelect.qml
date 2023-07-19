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
    property WindowCapture selectedWindowCapture: windowAvailable ? windowCapture : null

    property bool sourceAvailable: typeof comboBox.currentValue !== 'undefined' &&
                                   comboBox.currentValue.type !== 'toggler' &&
                                   videoSourceSwitch.checked

    property bool cameraAvailable: sourceAvailable && comboBox.currentValue.type === 'camera'
    property bool screenAvailable: sourceAvailable && comboBox.currentValue.type === 'screen'
    property bool windowAvailable: sourceAvailable && comboBox.currentValue.type === 'window'

    Component.onCompleted: {
        videoSourceModel.populate()

        for (var i = 0; i < videoSourceModel.count; i++) {
            if (videoSourceModel.get(i).value.type !== 'toggler') {
                comboBox.currentIndex = i
                break
            }
        }
    }

    Camera {
        id: camera
        active: cameraAvailable
    }

    ScreenCapture {
        id: screenCapture
        active: screenAvailable
    }

    WindowCapture {
        id: windowCapture
        active: windowAvailable
    }

    MediaDevices { id: mediaDevices }

    Switch {
        id: videoSourceSwitch
        anchors.verticalCenter: parent.verticalCenter
        checked: true
    }

    ListModel {
        id: videoSourceModel
        property var enabledSources: {
            'camera': true,
            'screen': true,
            'window': false
        }

        function toggleEnabledSource(type) {
            enabledSources[type] = !enabledSources[type]
            populate()
        }

        function appendItem(text, value) {
            append({ text: text, value: value})
        }

        function appendToggler(name, sourceType) {
            appendItem((enabledSources[sourceType] ? "- Hide " : "+ Show ") + name,
                       { type: 'toggler', 'sourceType': sourceType })
        }

        function populate() {
            clear()

            appendToggler('Cameras', 'camera')
            if (enabledSources['camera'])
                for (var camera of mediaDevices.videoInputs)
                    appendItem(camera.description, { type: 'camera', camera: camera })

            appendToggler('Screens', 'screen')
            if (enabledSources['screen'])
                for (var screen of Application.screens)
                    appendItem(screen.name, { type: 'screen', screen: screen })

            appendToggler('Windows', 'window')
            if (enabledSources['window'])
                for (var window of windowCapture.capturableWindows())
                    appendItem(window.description, { type: 'window', window: window })
        }
    }

    ComboBox {
        id: comboBox
        width: Style.widthLong
        height: Style.height
        background: StyleRectangle { anchors.fill: parent }
        model: videoSourceModel
        displayText: typeof currentValue === 'undefined' ||
                     currentValue.type === 'toggler' ? "Unavailable" : currentText
        font.pointSize: Style.fontSize
        textRole: "text"
        valueRole: "value"
        onCurrentValueChanged: {
            if (typeof currentValue === 'undefined')
                return
            if (currentValue.type === 'screen')
                screenCapture.screen = currentValue.screen
            else if (currentValue.type === 'camera')
                camera.cameraDevice = currentValue.camera
            else if (currentValue.type === 'window')
                windowCapture.window = currentValue.window
            else if (currentValue.type === 'toggler')
                model.toggleEnabledSource(currentValue.sourceType)
        }

        delegate: ItemDelegate {
            property bool isToggler: value.type === 'toggler'
            text: model[comboBox.textRole]
            width: comboBox.width
            height: comboBox.height
            highlighted: comboBox.highlightedIndex === index && !isToggler
            font.italic: isToggler
            font.underline: isToggler
            font.bold: comboBox.currentIndex === index && !isToggler ||
                       isToggler && comboBox.highlightedIndex === index
            palette.text: isToggler ? 'blue' : comboBox.palette.text

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (isToggler)
                        videoSourceModel.toggleEnabledSource(value.sourceType)
                    else {
                        comboBox.currentIndex = index
                        comboBox.popup.close()
                    }
                }
            }

            required property var value
            required property int index
            required property var model
        }

    }
}
