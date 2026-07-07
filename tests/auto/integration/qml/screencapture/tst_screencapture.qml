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
            property int frameRateChangedCount: 0
            onFrameRateChanged: () => {
                frameRateChangedCount++;
            }
        }
    }

    function test_frameRate_isMinusOne_byDefault() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);
        compare(capture.frameRate, -1);
    }

    function test_settingFrameRateToMinusOne_clearsFrameRate() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);

        capture.frameRate = 60;
        compare(capture.frameRate, 60);

        capture.frameRate = -1;
        compare(capture.frameRate, -1);
    }

    function test_settingInvalidFrameRate_isIgnored() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);

        capture.frameRate = 60;
        compare(capture.frameRate, 60);

        capture.frameRate = 0;
        compare(capture.frameRate, 60);

        capture.frameRate = -10;
        compare(capture.frameRate, 60);
    }

    function test_frameRate_isResettable() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);

        capture.frameRate = 60;
        compare(capture.frameRate, 60);

        // Resetting the property must return it to its default value (-1).
        capture.frameRate = undefined;
        compare(capture.frameRate, -1);
    }

    function test_onFrameRateChanged_isCalledWhenFrameRateChanges() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);

        capture.frameRate = 60;
        compare(capture.frameRate, 60);
        compare(capture.frameRateChangedCount, 1);

        // Setting the same value again must not invoke the handler.
        capture.frameRate = 60;
        compare(capture.frameRateChangedCount, 1);
    }
}
