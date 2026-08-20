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
            property int screenChangedCount: 0
            onScreenChanged: () => {
                screenChangedCount++;
            }

            property int maximumFrameRateChangedCount: 0
            property real lastMaximumFrameRate: -1
            onMaximumFrameRateChanged: (newFrameRate) => {
                maximumFrameRateChangedCount++;
                lastMaximumFrameRate = newFrameRate;
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
            property int lastError: ScreenCapture.NoError
            property string lastErrorString: ""
            onErrorOccurred: (error, errorString) => {
                errorOccurredCount++;
                lastError = error;
                lastErrorString = errorString;
            }
        }
    }

    function createScreenCapture() {
        let capture = createTemporaryObject(screenCaptureComponent, testCase);
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

    function initTestCase() {
        verify(Qt.application.screens.length > 0, "The test requires at least one screen");
    }

    function test_screen_isQmlScreenInfo_byDefault() {
        let capture = createScreenCapture();

        // No screen has been selected yet, but the property must still expose a
        // QML screen object rather than null.
        verify(capture.screen);
        compare(typeof capture.screen.name, "string");
        compare(typeof capture.screen.width, "number");
    }

    function test_settingScreen_updatesScreenProperty() {
        let capture = createScreenCapture();
        let screen = Qt.application.screens[0];

        capture.screen = screen;
        verify(capture.screen);
        compare(capture.screen.name, screen.name);
        compare(capture.screen.width, screen.width);
        compare(capture.screen.height, screen.height);
    }

    function test_onScreenChanged_isCalledWhenScreenChanges() {
        let capture = createScreenCapture();
        compare(capture.screenChangedCount, 0);

        capture.screen = Qt.application.screens[0];
        compare(capture.screenChangedCount, 1);

        // Setting the same screen again must not invoke the handler.
        capture.screen = Qt.application.screens[0];
        compare(capture.screenChangedCount, 1);
    }

    function test_screenChanged_isConnectable() {
        let capture = createScreenCapture();

        let connectCount = 0;
        capture.screenChanged.connect(() => {
            connectCount++;
        });

        capture.screen = Qt.application.screens[0];
        compare(connectCount, 1);
    }

    function test_settingScreenToNull_clearsScreen() {
        let capture = createScreenCapture();
        capture.screen = Qt.application.screens[0];
        compare(capture.screenChangedCount, 1);

        capture.screen = null;
        compare(capture.screenChangedCount, 2);

        // A cleared screen is still reported as a QML screen object, but it does
        // not wrap any actual screen.
        verify(capture.screen);
        compare(capture.screen.name, "");
    }

    function test_maximumFrameRate_isMinusOne_byDefault() {
        let capture = createScreenCapture();

        // The C++ property is a std::optional, which QML must see as a plain
        // number, using -1 for 'not set'.
        compare(typeof capture.maximumFrameRate, "number");
        compare(capture.maximumFrameRate, -1);
    }

    function test_onMaximumFrameRateChanged_isCalledWhenMaximumFrameRateChanges() {
        let capture = createScreenCapture();

        capture.maximumFrameRate = 60;
        compare(capture.maximumFrameRate, 60);
        compare(capture.maximumFrameRateChangedCount, 1);
        compare(capture.lastMaximumFrameRate, 60);

        // Setting the same value again must not invoke the handler.
        capture.maximumFrameRate = 60;
        compare(capture.maximumFrameRateChangedCount, 1);
    }

    function test_maximumFrameRateChanged_carriesNewValueAsNumber() {
        let capture = createScreenCapture();
        let spy = createSpy(capture, "maximumFrameRateChanged");

        capture.maximumFrameRate = 60;
        compare(spy.count, 1);
        compare(typeof spy.signalArguments[0][0], "number");
        compare(spy.signalArguments[0][0], 60);

        // Clearing the frame rate reports the default value, not undefined.
        capture.maximumFrameRate = -1;
        compare(spy.count, 2);
        compare(typeof spy.signalArguments[1][0], "number");
        compare(spy.signalArguments[1][0], -1);
        compare(capture.lastMaximumFrameRate, -1);
    }

    function test_maximumFrameRateChanged_isConnectable() {
        let capture = createScreenCapture();

        let connectCount = 0;
        let lastFrameRate = null;
        capture.maximumFrameRateChanged.connect((newFrameRate) => {
            connectCount++;
            lastFrameRate = newFrameRate;
        });

        capture.maximumFrameRate = 60;
        compare(connectCount, 1);
        compare(lastFrameRate, 60);
    }

    function test_maximumFrameRate_isResettable() {
        let capture = createScreenCapture();
        capture.maximumFrameRate = 60;
        compare(capture.maximumFrameRate, 60);

        capture.maximumFrameRate = undefined;
        compare(capture.maximumFrameRate, -1);
        compare(capture.maximumFrameRateChangedCount, 2);
    }

    function test_settingInvalidMaximumFrameRate_isIgnored() {
        let capture = createScreenCapture();
        capture.maximumFrameRate = 60;

        capture.maximumFrameRate = 0;
        compare(capture.maximumFrameRate, 60);

        capture.maximumFrameRate = -10;
        compare(capture.maximumFrameRate, 60);

        compare(capture.maximumFrameRateChangedCount, 1);
    }

    function test_active_isFalse_byDefault() {
        let capture = createScreenCapture();
        compare(capture.active, false);
        compare(capture.activeChangedCount, 0);
    }

    function test_onActiveChanged_isCalledWhenActiveChanges() {
        let capture = createScreenCapture();

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
        let capture = createScreenCapture();
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
        let capture = createScreenCapture();

        // No error is expected here. Check that the signals reach QML, and
        // the error properties are readable.
        createSpy(capture, "errorChanged");
        createSpy(capture, "errorOccurred");

        compare(capture.error, ScreenCapture.NoError);
        compare(typeof capture.errorString, "string");
        compare(capture.errorChangedCount, 0);
        compare(capture.errorOccurredCount, 0);
    }

    function test_errorEnum_isExposedToQml() {
        compare(ScreenCapture.NoError, 0);
        compare(ScreenCapture.InternalError, 1);
        compare(ScreenCapture.CapturingNotSupported, 2);
        compare(ScreenCapture.CaptureFailed, 4);
        compare(ScreenCapture.NotFound, 5);
    }
}
