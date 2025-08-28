// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtMultimedia
import QtTest

TestCase {
    name: "playbackOptions"

    property playbackOptions options

    property playbackOptions optionsA
    property playbackOptions optionsB

    function test_networkTimeoutMs_returns5sec_byDefault() {
        compare(options.networkTimeoutMs, 5000)
    }

    function test_settingNetworkTimeoutMs_changesNetworkTimeoutMs() {
        options.networkTimeoutMs = 1000
        compare(options.networkTimeoutMs, 1000)
    }

    function test_resettingNetworkTimeoutMs_resetsToDefault() {
        options.networkTimeoutMs = 1000
        options.networkTimeoutMs = undefined
        compare(options.networkTimeoutMs, 5000)
    }

    function test_playbackIntent_returnsPlayBack_byDefault() {
        compare(options.playbackIntent, PlaybackOptions.PlaybackIntent.Playback)
    }

    function test_settingPlaybackIntent_changesPlaybackIntent() {
        options.playbackIntent = PlaybackOptions.PlaybackIntent.LowLatencyStreaming
        compare(options.playbackIntent, PlaybackOptions.PlaybackIntent.LowLatencyStreaming)
    }

    function test_resettingPlaybackIntent_resetsToDefault() {
        options.playbackIntent = PlaybackOptions.LowLatencyStreaming
        options.playbackIntent = undefined
        compare(options.playbackIntent, PlaybackOptions.PlaybackIntent.Playback)
    }

    function test_probeSize_returnsMinusOneBack_byDefault() {
        compare(options.probeSize, -1)
    }

    function test_settingProbeSize_changesProbeSize() {
        options.probeSize = 32
        compare(options.probeSize, 32)
    }

    function test_resettingProbeSize_resetsToDefault() {
        options.probeSize = 32
        options.probeSize = undefined
        compare(options.probeSize, -1)
    }

    function test_assignment() {
        optionsA.networkTimeoutMs = 1
        optionsB = optionsA
        compare(optionsB.networkTimeoutMs, 1)
    }
}
