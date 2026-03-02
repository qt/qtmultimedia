// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D.SpatialAudio
import QtTest

TestCase {
    name: "AmbientSound"

    AmbientSound { id: ambient }

    function test_instantiation() {
        verify(ambient !== undefined)
    }

    function test_volume_and_loops() {
        ambient.volume = 0.3
        compare(ambient.volume, 0.3)
        compare(ambient.loops, 1)
        ambient.loops = 2
        compare(ambient.loops, 2)
    }

    function test_autoplay_readWrite() {
        compare(ambient.autoPlay, true)
        ambient.autoPlay = false
        compare(ambient.autoPlay, false)
    }

    function
    test_source_and_playback() {
        var u = Qt.resolvedUrl("test.wav")
        ambient.source = u
        compare(ambient.source.toString(), u.toString())
        ambient.play()
        wait(700)
        compare(ambient.source.toString(), u.toString())
    }
}
