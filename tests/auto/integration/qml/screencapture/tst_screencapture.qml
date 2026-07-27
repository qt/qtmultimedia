// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtTest

TestCase {
    id: testCase
    name: "ScreenCapture"

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    Component {
        id: screenCaptureComponent
        ScreenCapture {
            property int maximumFrameRateChangedCount: 0
            property real lastMaximumFrameRate: 0
            onMaximumFrameRateChanged: (maximumFrameRate) => {
                maximumFrameRateChangedCount++;
                lastMaximumFrameRate = maximumFrameRate;
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

    function test_maximumFrameRateChanged_isAccessibleAndCarriesNewValue() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);

        let spy = createTemporaryObject(
            signalSpyComponent,
            testCase,
            { target: capture, signalName: "maximumFrameRateChanged" });
        verify(spy);
        verify(spy.valid);

        capture.maximumFrameRate = 60;
        compare(spy.count, 1);
        compare(spy.signalArguments[0][0], 60);
        compare(capture.lastMaximumFrameRate, 60);

        // Resetting reports the default value (-1) as the signal argument.
        capture.maximumFrameRate = -1;
        compare(spy.count, 2);
        compare(spy.signalArguments[1][0], -1);
        compare(capture.lastMaximumFrameRate, -1);
    }

    function test_maximumFrameRateChanged_isConnectable() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);

        // The signal must be connectable via the signal.connect() syntax,
        // and must deliver the new value to the connected handler.
        let connectCount = 0;
        let lastValue = 0;
        capture.maximumFrameRateChanged.connect((maximumFrameRate) => {
            connectCount++;
            lastValue = maximumFrameRate;
        });

        capture.maximumFrameRate = 60;
        compare(connectCount, 1);
        compare(lastValue, 60);

        capture.maximumFrameRate = -1;
        compare(connectCount, 2);
        compare(lastValue, -1);
    }
}
