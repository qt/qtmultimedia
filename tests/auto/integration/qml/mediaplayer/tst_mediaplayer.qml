// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtTest

TestCase {
    name: "MediaPlayer"

    MediaPlayer {
        id: player
    }

    function test_playing_returnsFalse_byDefault() {
        verify(!player.playing)
    }

    function test_playbackOptions_isReadWriteProperty() {
        // Test that the property exists by trying to use one of its members
        var defaultValue = player.playbackOptions.networkTimeoutMs
        player.playbackOptions.networkTimeoutMs = defaultValue + 1
        compare(player.playbackOptions.networkTimeoutMs, defaultValue + 1)
    }

    function test_playbackOptions_isResettable() {
        player.playbackOptions.networkTimeoutMs = 0
        player.playbackOptions = undefined
        verify(player.playbackOptions.networkTimeoutMs != 0)
    }

}
