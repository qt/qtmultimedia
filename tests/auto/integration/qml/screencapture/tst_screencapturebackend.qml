// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtTest
import QtMultimediaTest

TestCase {
    id: testCase
    name: "ScreenCapture"

    function initTestCase() {
        verify(
            TestHelper.isMediaBackendPluginLoaded,
            "No media backend plugin was loaded; the fallback integration was used.")
    }

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    Component {
        id: screenCaptureComponent
        ScreenCapture {
            property int maximumFrameRateChangedCount: 0
            property real lastMaximumFrameRate: 0
            onMaximumFrameRateChanged: () => {
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

    function test_maximumFrameRateChanged_isAccessibleAndNotifiesNewValue() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);

        let spy = createTemporaryObject(
            signalSpyComponent,
            testCase,
            { target: capture, signalName: "maximumFrameRateChanged" });
        verify(spy);
        verify(spy.valid);

        // The signal carries no argument, so the new value is read from the
        // property in the handler.
        capture.maximumFrameRate = 60;
        compare(spy.count, 1);
        compare(spy.signalArguments[0].length, 0);
        compare(capture.lastMaximumFrameRate, 60);

        // Resetting notifies the default value (-1), not undefined.
        capture.maximumFrameRate = -1;
        compare(spy.count, 2);
        compare(spy.signalArguments[1].length, 0);
        compare(capture.lastMaximumFrameRate, -1);
    }

    function test_maximumFrameRateChanged_isConnectable() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
        verify(capture);

        // The signal must be connectable via the signal.connect() syntax,
        // and the new value must be readable from the property when it fires.
        let connectCount = 0;
        let lastValue = 0;
        capture.maximumFrameRateChanged.connect(() => {
            connectCount++;
            lastValue = capture.maximumFrameRate;
        });

        capture.maximumFrameRate = 60;
        compare(connectCount, 1);
        compare(lastValue, 60);

        capture.maximumFrameRate = -1;
        compare(connectCount, 2);
        compare(lastValue, -1);
    }
}
