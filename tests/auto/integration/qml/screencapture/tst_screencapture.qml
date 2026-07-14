// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtTest

TestCase {
    id: testCase
    name: "ScreenCapture"

    Component {
        id: screenCaptureComponent
        ScreenCapture {
            property int maximumFrameRateChangedCount: 0
            onMaximumFrameRateChanged: () => {
                maximumFrameRateChangedCount++;
            }
        }
    }

    function test_maximumFrameRate_isMinusOne_byDefault() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);
        compare(capture.maximumFrameRate, -1);
    }

    function test_settingMaximumFrameRateToMinusOne_clearsMaximumFrameRate() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);

        capture.maximumFrameRate = 60;
        compare(capture.maximumFrameRate, 60);

        capture.maximumFrameRate = -1;
        compare(capture.maximumFrameRate, -1);
    }

    function test_settingInvalidMaximumFrameRate_isIgnored() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);

        capture.maximumFrameRate = 60;
        compare(capture.maximumFrameRate, 60);

        capture.maximumFrameRate = 0;
        compare(capture.maximumFrameRate, 60);

        capture.maximumFrameRate = -10;
        compare(capture.maximumFrameRate, 60);
    }

    function test_maximumFrameRate_isResettable() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);

        capture.maximumFrameRate = 60;
        compare(capture.maximumFrameRate, 60);

        // Resetting the property must return it to its default value (-1).
        capture.maximumFrameRate = undefined;
        compare(capture.maximumFrameRate, -1);
    }

    function test_onMaximumFrameRateChanged_isCalledWhenMaximumFrameRateChanges() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);

        capture.maximumFrameRate = 60;
        compare(capture.maximumFrameRate, 60);
        compare(capture.maximumFrameRateChangedCount, 1);

        // Setting the same value again must not invoke the handler.
        capture.maximumFrameRate = 60;
        compare(capture.maximumFrameRateChangedCount, 1);
    }
}
