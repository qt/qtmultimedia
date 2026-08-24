// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtTest

TestCase {
    id: testCase
    name: "WindowCapture"

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    Component {
        id: windowCaptureComponent
        WindowCapture {
            property int windowChangedCount: 0
            onWindowChanged: () => {
                windowChangedCount++;
            }

            property int maximumFrameRateChangedCount: 0
            property real lastMaximumFrameRate: -1
            onMaximumFrameRateChanged: () => {
                maximumFrameRateChangedCount++;
                lastMaximumFrameRate = maximumFrameRate;
            }

            property int activeChangedCount: 0
            property bool lastActive: false
            onActiveChanged: (newActive) => {
                activeChangedCount++;
                lastActive = newActive;
            }

            property int errorChangedCount: 0
            onErrorChanged: () => {
                errorChangedCount++;
            }

            property int errorOccurredCount: 0
            property int lastError: WindowCapture.NoError
            property string lastErrorString: ""
            onErrorOccurred: (error, errorString) => {
                errorOccurredCount++;
                lastError = error;
                lastErrorString = errorString;
            }
        }
    }

    function createWindowCapture() {
        let capture = createTemporaryObject(windowCaptureComponent, testCase);
        verify(capture);
        return capture;
    }

    function createSpy(capture, signalName) {
        let spy = createTemporaryObject(
            signalSpyComponent, testCase, { target: capture, signalName: signalName });
        verify(spy);
        verify(spy.valid, "The signal '" + signalName + "' is not exposed to QML");
        return spy;
    }

    function test_window_isCapturableWindow_byDefault() {
        let capture = createWindowCapture();

        // No window has been selected yet, but the property must still expose a
        // capturableWindow value type rather than undefined.
        verify(capture.window);
        compare(typeof capture.window.description, "string");
        compare(typeof capture.window.isValid, "boolean");
    }

    function test_window_isInvalid_byDefault() {
        let capture = createWindowCapture();

        compare(capture.window.isValid, false);
        compare(capture.window.description, "");
        compare(capture.windowChangedCount, 0);
    }

    function test_windowChanged_isExposedToQml() {
        let capture = createWindowCapture();
        createSpy(capture, "windowChanged");

        let connectCount = 0;
        capture.windowChanged.connect(() => {
            connectCount++;
        });
        compare(connectCount, 0);
    }

    function test_capturableWindows_isCallableFromQml() {
        let capture = createWindowCapture();

        let windows = capture.capturableWindows();
        verify(windows);
        compare(typeof windows.length, "number");
        compare(windows.length, 0);
    }

    function test_maximumFrameRate_isMinusOne_byDefault() {
        let capture = createWindowCapture();

        compare(typeof capture.maximumFrameRate, "number");
        compare(capture.maximumFrameRate, -1);
    }

    function test_onMaximumFrameRateChanged_isCalledWhenMaximumFrameRateChanges() {
        let capture = createWindowCapture();

        capture.maximumFrameRate = 60;
        compare(capture.maximumFrameRate, 60);
        compare(capture.maximumFrameRateChangedCount, 1);
        compare(capture.lastMaximumFrameRate, 60);

        // Setting the same value again must not invoke the handler.
        capture.maximumFrameRate = 60;
        compare(capture.maximumFrameRateChangedCount, 1);
    }

    function test_maximumFrameRateChanged_isEmittedWithoutArguments() {
        let capture = createWindowCapture();
        let spy = createSpy(capture, "maximumFrameRateChanged");

        // The signal is a plain property notification, so the new value is read
        // from the property rather than from a signal argument.
        capture.maximumFrameRate = 60;
        compare(spy.count, 1);
        compare(spy.signalArguments[0].length, 0);
        compare(capture.lastMaximumFrameRate, 60);

        // Clearing the frame rate reports the default value, not undefined.
        capture.maximumFrameRate = -1;
        compare(spy.count, 2);
        compare(spy.signalArguments[1].length, 0);
        compare(typeof capture.maximumFrameRate, "number");
        compare(capture.lastMaximumFrameRate, -1);
    }

    function test_maximumFrameRateChanged_isConnectable() {
        let capture = createWindowCapture();

        let connectCount = 0;
        let lastFrameRate = -1;
        capture.maximumFrameRateChanged.connect(() => {
            connectCount++;
            lastFrameRate = capture.maximumFrameRate;
        });

        capture.maximumFrameRate = 60;
        compare(connectCount, 1);
        compare(lastFrameRate, 60);
    }

    function test_maximumFrameRate_isResettable() {
        let capture = createWindowCapture();
        capture.maximumFrameRate = 60;
        compare(capture.maximumFrameRate, 60);

        capture.maximumFrameRate = undefined;
        compare(capture.maximumFrameRate, -1);
        compare(capture.maximumFrameRateChangedCount, 2);
    }

    function test_settingInvalidMaximumFrameRate_isIgnored() {
        let capture = createWindowCapture();
        capture.maximumFrameRate = 60;

        capture.maximumFrameRate = 0;
        compare(capture.maximumFrameRate, 60);

        capture.maximumFrameRate = -10;
        compare(capture.maximumFrameRate, 60);

        compare(capture.maximumFrameRateChangedCount, 1);
    }

    function test_active_isFalse_byDefault() {
        let capture = createWindowCapture();
        compare(capture.active, false);
        compare(capture.activeChangedCount, 0);
    }

    function test_onActiveChanged_isCalledWhenActiveChanges() {
        let capture = createWindowCapture();

        capture.active = true;
        compare(capture.active, true);
        compare(capture.activeChangedCount, 1);
        compare(capture.lastActive, true);

        capture.active = false;
        compare(capture.active, false);
        compare(capture.activeChangedCount, 2);
        compare(capture.lastActive, false);
    }

    function test_startAndStop_areCallableFromQml() {
        let capture = createWindowCapture();
        let spy = createSpy(capture, "activeChanged");

        capture.start();
        compare(capture.active, true);
        compare(spy.count, 1);
        compare(spy.signalArguments[0][0], true);

        capture.stop();
        compare(capture.active, false);
        compare(spy.count, 2);
        compare(spy.signalArguments[1][0], false);
    }

    function test_errorSignals_areExposedToQml() {
        let capture = createWindowCapture();

        // No error is expected here. Check that the signals reach QML, and
        // the error properties are readable.
        createSpy(capture, "errorChanged");
        createSpy(capture, "errorOccurred");

        compare(capture.error, WindowCapture.NoError);
        compare(typeof capture.errorString, "string");
        compare(capture.errorChangedCount, 0);
        compare(capture.errorOccurredCount, 0);
    }

    function test_errorEnum_isExposedToQml() {
        compare(WindowCapture.NoError, 0);
        compare(WindowCapture.InternalError, 1);
        compare(WindowCapture.CapturingNotSupported, 2);
        compare(WindowCapture.CaptureFailed, 4);
        compare(WindowCapture.NotFound, 5);
    }
}
