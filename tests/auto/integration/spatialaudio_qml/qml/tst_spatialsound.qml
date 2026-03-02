// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D.SpatialAudio
import QtTest

TestCase {
    name: "SpatialSound"

    SpatialSound {
        id: sound
    }

    function test_instantiation() {
        verify(sound !== undefined)
    }

    function test_volume_readWrite() {
        sound.volume = 0.5
        compare(sound.volume, 0.5)
    }

    function test_directivity_defaults_and_rw() {
        compare(sound.directivity, 0)
        sound.directivity = 0.7
        compare(sound.directivity, 0.7)
        compare(sound.directivityOrder, 1)
        sound.directivityOrder = 2.0
        compare(sound.directivityOrder, 2.0)
    }

    function test_loops_and_autoplay() {
        compare(sound.loops, 1)
        compare(sound.autoPlay, true)
        sound.loops = 4
        compare(sound.loops, 4)
        sound.autoPlay = false
        compare(sound.autoPlay, false)
    }

    function test_other_properties_readWrite() {
        sound.size = 2.0
        compare(sound.size, 2.0)
        sound.distanceCutoff = 50
        compare(sound.distanceCutoff, 50)
        sound.manualAttenuation = 0.1
        compare(sound.manualAttenuation, 0.1)
        sound.occlusionIntensity = 0.3
        compare(sound.occlusionIntensity, 0.3)
        sound.nearFieldGain = 0.5
        compare(sound.nearFieldGain, 0.5)
    }

    function test_source_is_readWrite() {
        var url = Qt.resolvedUrl("test.wav")
        sound.source = url
        compare(sound.source.toString(), url.toString())
    }
}
