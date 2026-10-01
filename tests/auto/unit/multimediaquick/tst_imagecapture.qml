// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtMultimediaTest
import QtTest

TestCase {
    id: testCase
    name: "ImageCapture"

    property int bindingWidth: 0

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    Component {
        id: captureSessionComponent
        CaptureSession {}
    }

    Component {
        id: imageCaptureComponent
        ImageCapture {}
    }

    function createObject(component) {
        let object = createTemporaryObject(component, testCase);
        verify(object);
        return object;
    }

    function createSpy(target, signalName) {
        let spy = createObject(signalSpyComponent);
        spy.target = target;
        spy.signalName = signalName;
        verify(spy.valid);
        return spy;
    }

    function initTestCase() {
        verify(TestHelper.mediaBackendName === "mock");
    }

    function test_resolution_isInvalidByDefault() {
        let imageCapture = createObject(imageCaptureComponent);
        compare(imageCapture.resolution, Qt.size(-1, -1));
    }

    function test_settingResolution_isReflectedAndNotified_data() {
        return [
            { tag: "small", size: Qt.size(320, 240) },
            { tag: "hd", size: Qt.size(1280, 720) },
            { tag: "uhd", size: Qt.size(3840, 2160) }
        ];
    }

    function test_settingResolution_isReflectedAndNotified(data) {
        let imageCapture = createObject(imageCaptureComponent);
        let spy = createSpy(imageCapture, "resolutionChanged");

        imageCapture.resolution = data.size;

        compare(imageCapture.resolution, data.size);
        compare(spy.count, 1);
    }

    function test_settingSameResolution_isNotNotified() {
        let imageCapture = createObject(imageCaptureComponent);
        imageCapture.resolution = Qt.size(640, 480);
        let spy = createSpy(imageCapture, "resolutionChanged");

        imageCapture.resolution = Qt.size(640, 480);

        compare(imageCapture.resolution, Qt.size(640, 480));
        compare(spy.count, 0);
    }

    function test_resettingResolution_restoresDefault() {
        let imageCapture = createObject(imageCaptureComponent);
        imageCapture.resolution = Qt.size(640, 480);
        let spy = createSpy(imageCapture, "resolutionChanged");

        imageCapture.resolution = undefined;

        compare(imageCapture.resolution, Qt.size(-1, -1));
        compare(spy.count, 1);
    }

    function test_changingResolution_isNotifiedEachTime() {
        let imageCapture = createObject(imageCaptureComponent);
        let spy = createSpy(imageCapture, "resolutionChanged");

        imageCapture.resolution = Qt.size(640, 480);
        imageCapture.resolution = Qt.size(1280, 720);

        compare(imageCapture.resolution, Qt.size(1280, 720));
        compare(spy.count, 2);
    }

    function test_resolution_isKeptWhenMovedToNewCaptureSession() {
        let session1 = createObject(captureSessionComponent);
        let session2 = createObject(captureSessionComponent);
        let imageCapture = createObject(imageCaptureComponent);

        session1.imageCapture = imageCapture;
        imageCapture.resolution = Qt.size(640, 480);
        session2.imageCapture = imageCapture;

        compare(imageCapture.resolution, Qt.size(640, 480));
    }

    function test_resolutionBinding_updatesImageCapture() {
        let imageCapture = createObject(imageCaptureComponent);
        testCase.bindingWidth = 640;
        imageCapture.resolution = Qt.binding(() => Qt.size(testCase.bindingWidth, 480));
        compare(imageCapture.resolution, Qt.size(640, 480));

        testCase.bindingWidth = 800;

        compare(imageCapture.resolution, Qt.size(800, 480));
    }
}
