// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.SpatialAudio
import QtQuick3D.Helpers

import QtMultimedia

Window {
    visible: true
    width: 640
    height: 480
    title: "Quick3d Spatial Audio Test"

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "black"
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: Texture {
                textureData: ProceduralSkyTextureData {}
            }
        }

        AudioEngine {
            id: engine
            distanceScale: 100 // 1 - 1 meter
            masterVolume: 1
            outputDevice:  mediaDevices.defaultAudioOutput

        }

        MediaDevices {
            id: mediaDevices
        }

        DirectionalLight {}

        PerspectiveCamera {
            id: camera
            clipNear: 0.1
            position: Qt.vector3d(200, 0, 0)
            AudioListener {
            }
        }

        Model {
            id: soundNode
            // This position should be reflected in the audio
            position: Qt.vector3d(200, 0, -5)

            source: "#Sphere"
            scale: Qt.vector3d(0.01, 0.01, 0.01)

            materials: PrincipledMaterial {}

            SpatialSound {
                source: "nokia-tune.mp3"
                loops: -1
                volume: 1
            }
        }

        WasdController {
            controlledObject: camera
            speed: 0.01
        }
    }
}
