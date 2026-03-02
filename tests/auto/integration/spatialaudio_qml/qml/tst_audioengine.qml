// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D.SpatialAudio
import QtTest

TestCase {
    name: "AudioEngine"

    AudioEngine { id: engine }

    function test_instantiation() {
        verify(engine !== undefined)
    }

    function test_masterVolume_and_distanceScale() {
        engine.masterVolume = 0.6
        compare(engine.masterVolume, 0.6)
        engine.distanceScale = 100
        compare(engine.distanceScale, 100)
    }

    function test_outputMode_enum_rw() {
        // use available enum values
        engine.outputMode = AudioEngine.Headphone
        compare(engine.outputMode, AudioEngine.Headphone)
        engine.outputMode = AudioEngine.Stereo
        compare(engine.outputMode, AudioEngine.Stereo)
    }
}
