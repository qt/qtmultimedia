// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtTest
import QtMultimediaTest

TestCase {
    name: "SoundEffect"

    function initTestCase() {
        verify(
            TestHelper.isMediaBackendPluginLoaded,
            "No media backend plugin was loaded; the fallback integration was used.")
    }

    SoundEffect {
        id: effect
    }

    function test_playing_returnsFalse_byDefault() {
        verify(!effect.playing)
    }

}
