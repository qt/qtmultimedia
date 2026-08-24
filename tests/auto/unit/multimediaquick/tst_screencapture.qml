// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtTest
import QtMultimediaTest

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
            property ScreenInfo lastScreen: null
            onScreenChanged: (newScreen) => {
                screenChangedCount++;
                lastScreen = newScreen;
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

    function test_screenChanged_carriesQmlScreen() {
        let capture = createScreenCapture();
        let spy = createSpy(capture, "screenChanged");
        let screen = Qt.application.screens[0];

        capture.screen = screen;
        compare(spy.count, 1);

        // The signal argument must be the QML screen object exposed by the
        // 'screen' property, not the underlying C++ QScreen.
        let argument = spy.signalArguments[0][0];
        verify(argument);
        compare(argument.name, screen.name);
        compare(argument, capture.screen);
        compare(capture.lastScreen, capture.screen);
    }

    function test_screenChanged_isConnectable() {
        let capture = createScreenCapture();

        let connectCount = 0;
        let lastScreen = null;
        capture.screenChanged.connect((newScreen) => {
            connectCount++;
            lastScreen = newScreen;
        });

        capture.screen = Qt.application.screens[0];
        compare(connectCount, 1);
        compare(lastScreen, capture.screen);
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

    function test_maximumFrameRateChanged_isEmittedWithoutArguments() {
        let capture = createScreenCapture();
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
        let capture = createScreenCapture();

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

    function test_screenCapture_convertsToCppScreenCapture() {
        let capture = createScreenCapture();

        // The helper methods take a QScreenCapture pointer, so a successful
        // call proves the QML element converts to the C++ type.
        verify(ScreenCaptureHelper.isScreenCapture(capture));
        verify(ScreenCaptureHelper.isQuickScreenCapture(capture));
    }

    function test_screenAssignedFromQml_isVisibleInCpp() {
        let capture = createScreenCapture();
        verify(!ScreenCaptureHelper.hasScreen(capture));

        capture.screen = Qt.application.screens[0];
        verify(ScreenCaptureHelper.hasScreen(capture));
        compare(ScreenCaptureHelper.screenName(capture), Qt.application.screens[0].name);

        capture.screen = null;
        verify(!ScreenCaptureHelper.hasScreen(capture));
    }

    function test_screenProperty_isTheObjectExposedToCpp() {
        let capture = createScreenCapture();
        capture.screen = Qt.application.screens[0];

        // Reading the property repeatedly must hand out the same object, and it
        // must be the very object the C++ side keeps.
        compare(capture.screen, capture.screen);
        compare(ScreenCaptureHelper.qmlScreen(capture), capture.screen);
    }

    function test_reassigningEquivalentScreen_keepsScreenObject_data() {
        return [
            { tag: "Null screen", useScreen: false },
            { tag: "Primary screen", useScreen: true },
        ];
    }

    function test_reassigningEquivalentScreen_keepsScreenObject(data) {
        let capture = createScreenCapture();
        let screen = data.useScreen ? Qt.application.screens[0] : null;

        capture.screen = screen;
        let firstScreenObject = capture.screen;
        verify(firstScreenObject);

        let spy = createSpy(capture, "screenChanged");

        // Assigning a different QML screen object that wraps the same underlying
        // screen must neither emit a signal nor replace the exposed object.
        capture.screen = screen;
        compare(spy.count, 0);
        compare(capture.screen, firstScreenObject);
    }

    function test_settingScreenFromCpp_updatesQmlScreen_data() {
        return [
            { tag: "Null screen", useScreen: false },
            { tag: "Primary screen", useScreen: true },
        ];
    }

    function test_settingScreenFromCpp_updatesQmlScreen(data) {
        let capture = createScreenCapture();

        if (data.useScreen)
            ScreenCaptureHelper.clearScreen(capture);
        else
            ScreenCaptureHelper.setPrimaryScreen(capture);

        let oldScreenObject = capture.screen;
        verify(oldScreenObject);

        ScreenCaptureHelper.rememberQmlScreen(capture);
        verify(!ScreenCaptureHelper.rememberedQmlScreenIsDestroyed());

        // Note that 'screenChanged' shadows the inherited C++ signal of the same
        // name, so the signal seen from QML carries the QML screen object rather
        // than the underlying QScreen.
        let spy = createSpy(capture, "screenChanged");
        let handlerCountBefore = capture.screenChangedCount;

        // Act: change the screen through the C++ API.
        if (data.useScreen)
            ScreenCaptureHelper.setPrimaryScreen(capture);
        else
            ScreenCaptureHelper.clearScreen(capture);

        compare(spy.count, 1);
        compare(capture.screenChangedCount, handlerCountBefore + 1);

        // The exposed object is replaced with a new one, and the old one is
        // destroyed rather than leaked.
        verify(capture.screen);
        verify(capture.screen !== oldScreenObject);
        verify(ScreenCaptureHelper.rememberedQmlScreenIsDestroyed());
        compare(spy.signalArguments[0][0], capture.screen);
        compare(capture.lastScreen, capture.screen);
        compare(capture.screen.name,
            data.useScreen ? ScreenCaptureHelper.primaryScreenName : "");
    }

    function test_settingScreenFromDeletedQmlScreen_stillExposesScreen_data() {
        return [
            { tag: "Null screen", useScreen: false },
            { tag: "Primary screen", useScreen: true },
        ];
    }

    function test_settingScreenFromDeletedQmlScreen_stillExposesScreen(data) {
        let capture = createScreenCapture();

        // The helper assigns the screen using a QQuickScreenInfo that is
        // destroyed before it returns. The capture must not be left holding a
        // dangling screen object.
        ScreenCaptureHelper.setScreenFromTemporaryQmlScreen(capture, data.useScreen);

        verify(capture.screen);
        compare(capture.screen.name,
                data.useScreen ? ScreenCaptureHelper.primaryScreenName : "");
        compare(ScreenCaptureHelper.hasScreen(capture), data.useScreen);
    }

    function test_maximumFrameRateSetFromQml_isVisibleInCpp() {
        let capture = createScreenCapture();

        // The C++ property is a std::optional, which must be unset by default.
        verify(!ScreenCaptureHelper.hasMaximumFrameRate(capture));

        capture.maximumFrameRate = 60;
        verify(ScreenCaptureHelper.hasMaximumFrameRate(capture));
        compare(ScreenCaptureHelper.maximumFrameRate(capture), 60);

        capture.maximumFrameRate = -1;
        verify(!ScreenCaptureHelper.hasMaximumFrameRate(capture));
    }

    function test_settingMaximumFrameRateFromCpp_updatesQmlProperty() {
        let capture = createScreenCapture();
        let spy = createSpy(capture, "maximumFrameRateChanged");

        ScreenCaptureHelper.setMaximumFrameRate(capture, 60);
        compare(capture.maximumFrameRate, 60);
        compare(capture.maximumFrameRateChangedCount, 1);
        compare(capture.lastMaximumFrameRate, 60);
        compare(spy.count, 1);
    }

    function test_clearingMaximumFrameRateFromCpp_reportsMinusOneToQml() {
        let capture = createScreenCapture();
        ScreenCaptureHelper.setMaximumFrameRate(capture, 60);

        let spy = createSpy(capture, "maximumFrameRateChanged");

        // An unset std::optional must reach QML as -1, not as undefined.
        ScreenCaptureHelper.clearMaximumFrameRate(capture);
        compare(typeof capture.maximumFrameRate, "number");
        compare(capture.maximumFrameRate, -1);
        compare(spy.count, 1);
        compare(capture.lastMaximumFrameRate, -1);
    }

    function test_activeSetFromQml_isVisibleInCpp() {
        let capture = createScreenCapture();
        verify(!ScreenCaptureHelper.isActive(capture));

        capture.active = true;
        verify(ScreenCaptureHelper.isActive(capture));

        capture.active = false;
        verify(!ScreenCaptureHelper.isActive(capture));
    }

    function test_settingActiveFromCpp_updatesQmlProperty() {
        let capture = createScreenCapture();
        let spy = createSpy(capture, "activeChanged");

        ScreenCaptureHelper.setActive(capture, true);
        compare(capture.active, true);
        compare(capture.activeChangedCount, 1);
        compare(capture.lastActive, true);
        compare(spy.count, 1);
        compare(spy.signalArguments[0][0], true);

        ScreenCaptureHelper.setActive(capture, false);
        compare(capture.active, false);
        compare(capture.activeChangedCount, 2);
        compare(spy.count, 2);
        compare(spy.signalArguments[1][0], false);
    }
}
