// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D.SpatialAudio
import QtTest

TestCase {
    name: "AudioRoom"

    AudioRoom { id: room }

    function test_dimensions_and_materials() {
        verify(room !== undefined)
        room.dimensions = Qt.vector3d(100,100,100)
        compare(room.dimensions.x, 100)
        room.leftMaterial = AudioRoom.Material.Transparent
        compare(room.leftMaterial, AudioRoom.Material.Transparent)
    }

    function test_reverb_and_reflection_properties() {
        room.reverbGain = 0.4
        compare(room.reverbGain, 0.4)
        room.reflectionGain = 0.7
        compare(room.reflectionGain, 0.7)
    }
}
