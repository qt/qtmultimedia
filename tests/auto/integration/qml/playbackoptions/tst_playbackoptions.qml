// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtMultimedia
import QtTest

TestCase {
    name: "playbackOptions"

    property playbackOptions options

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

}
