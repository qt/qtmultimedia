// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtTest

TestCase {
    id: testCase
    name: "CaptureSession"

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    Component {
        id: captureSessionComponent
        CaptureSession {}
    }

    Component {
        id: cameraComponent
        Camera {}
    }

    Component {
        id: screenCaptureComponent
        ScreenCapture {}
    }

    Component {
        id: windowCaptureComponent
        WindowCapture {}
    }

    Component {
        id: imageCaptureComponent
        ImageCapture {}
    }

    Component {
        id: mediaRecorderComponent
        MediaRecorder {}
    }

    Component {
        id: audioInputComponent
        AudioInput {}
    }

    Component {
        id: audioOutputComponent
        AudioOutput {}
    }

    Component {
        id: videoOutputComponent
        VideoOutput {}
    }

    Component {
        id: inlineCaptureSessionComponent
        CaptureSession {
            camera: Camera {}
            screenCapture: ScreenCapture {}
            windowCapture: WindowCapture {}
            imageCapture: ImageCapture {}
            recorder: MediaRecorder {}
            audioInput: AudioInput {}
            audioOutput: AudioOutput {}
            videoOutput: VideoOutput {}
        }
    }

    // Counts the QML signal handlers, which resolve the signals by name and can
    // therefore behave differently from a connection made from a SignalSpy.
    Component {
        id: handlerCaptureSessionComponent
        CaptureSession {
            property int cameraChangedCount: 0
            onCameraChanged: () => {
                cameraChangedCount++;
            }

            property int screenCaptureChangedCount: 0
            onScreenCaptureChanged: () => {
                screenCaptureChangedCount++;
            }

            property int windowCaptureChangedCount: 0
            onWindowCaptureChanged: () => {
                windowCaptureChangedCount++;
            }

            property int imageCaptureChangedCount: 0
            onImageCaptureChanged: () => {
                imageCaptureChangedCount++;
            }

            property int recorderChangedCount: 0
            onRecorderChanged: () => {
                recorderChangedCount++;
            }

            property int audioInputChangedCount: 0
            onAudioInputChanged: () => {
                audioInputChangedCount++;
            }

            property int audioOutputChangedCount: 0
            onAudioOutputChanged: () => {
                audioOutputChangedCount++;
            }

            property int videoOutputChangedCount: 0
            onVideoOutputChanged: () => {
                videoOutputChangedCount++;
            }
        }
    }

    function createSession() {
        let session = createTemporaryObject(captureSessionComponent, testCase);
        verify(session);
        return session;
    }

    function createObject(component) {
        let object = createTemporaryObject(component, testCase);
        verify(object);
        return object;
    }

    function test_captureObjects_areNull_byDefault() {
        let session = createSession();

        compare(session.camera, null);
        compare(session.screenCapture, null);
        compare(session.windowCapture, null);
        compare(session.imageCapture, null);
        compare(session.recorder, null);
        compare(session.audioInput, null);
        compare(session.audioOutput, null);
        compare(session.videoOutput, null);
    }

    function test_assigningCaptureObjects_isVisibleOnTheSession() {
        let session = createSession();

        let camera = createObject(cameraComponent);
        session.camera = camera;
        compare(session.camera, camera);

        let screenCapture = createObject(screenCaptureComponent);
        session.screenCapture = screenCapture;
        compare(session.screenCapture, screenCapture);

        let windowCapture = createObject(windowCaptureComponent);
        session.windowCapture = windowCapture;
        compare(session.windowCapture, windowCapture);

        let imageCapture = createObject(imageCaptureComponent);
        session.imageCapture = imageCapture;
        compare(session.imageCapture, imageCapture);

        let recorder = createObject(mediaRecorderComponent);
        session.recorder = recorder;
        compare(session.recorder, recorder);

        let audioInput = createObject(audioInputComponent);
        session.audioInput = audioInput;
        compare(session.audioInput, audioInput);

        let audioOutput = createObject(audioOutputComponent);
        session.audioOutput = audioOutput;
        compare(session.audioOutput, audioOutput);

        let videoOutput = createObject(videoOutputComponent);
        session.videoOutput = videoOutput;
        compare(session.videoOutput, videoOutput);
    }

    function test_captureObjectsDeclaredInline_areAssigned() {
        // Declaring the objects inside the CaptureSession is the documented way
        // of setting up a session.
        let session = createTemporaryObject(inlineCaptureSessionComponent, testCase);
        verify(session);

        verify(session.camera);
        verify(session.screenCapture);
        verify(session.windowCapture);
        verify(session.imageCapture);
        verify(session.recorder);
        verify(session.audioInput);
        verify(session.audioOutput);
        verify(session.videoOutput);
    }

    function test_clearingCaptureObjects() {
        let session = createTemporaryObject(inlineCaptureSessionComponent, testCase);
        verify(session);

        session.camera = null;
        session.screenCapture = null;
        session.windowCapture = null;
        session.imageCapture = null;
        session.recorder = null;
        session.audioInput = null;
        session.audioOutput = null;
        session.videoOutput = null;

        compare(session.camera, null);
        compare(session.screenCapture, null);
        compare(session.windowCapture, null);
        compare(session.imageCapture, null);
        compare(session.recorder, null);
        compare(session.audioInput, null);
        compare(session.audioOutput, null);
        compare(session.videoOutput, null);
    }

    function test_assigningScreenCapture_isNotified() {
        let session = createSession();
        let spy = createTemporaryObject(
            signalSpyComponent, testCase,
            { target: session, signalName: "screenCaptureChanged" });
        verify(spy);
        verify(spy.valid, "The signal 'screenCaptureChanged' is not exposed to QML");

        let screenCapture = createObject(screenCaptureComponent);
        session.screenCapture = screenCapture;
        compare(spy.count, 1);

        // Assigning the same object again is not a change.
        session.screenCapture = screenCapture;
        compare(spy.count, 1);

        session.screenCapture = null;
        compare(spy.count, 2);
    }

    function test_assigningWindowCapture_isNotified() {
        let session = createSession();
        let spy = createTemporaryObject(
            signalSpyComponent, testCase,
            { target: session, signalName: "windowCaptureChanged" });
        verify(spy);
        verify(spy.valid, "The signal 'windowCaptureChanged' is not exposed to QML");

        let windowCapture = createObject(windowCaptureComponent);
        session.windowCapture = windowCapture;
        compare(spy.count, 1);

        // Assigning the same object again is not a change.
        session.windowCapture = windowCapture;
        compare(spy.count, 1);

        session.windowCapture = null;
        compare(spy.count, 2);
    }

    function test_captureObjectHandlers_areCalledWhenObjectsAreAssigned() {
        let session = createTemporaryObject(handlerCaptureSessionComponent, testCase);
        verify(session);

        session.camera = createObject(cameraComponent);
        compare(session.cameraChangedCount, 1);

        session.screenCapture = createObject(screenCaptureComponent);
        compare(session.screenCaptureChangedCount, 1);

        session.windowCapture = createObject(windowCaptureComponent);
        compare(session.windowCaptureChangedCount, 1);

        session.imageCapture = createObject(imageCaptureComponent);
        compare(session.imageCaptureChangedCount, 1);

        session.recorder = createObject(mediaRecorderComponent);
        compare(session.recorderChangedCount, 1);

        session.audioInput = createObject(audioInputComponent);
        compare(session.audioInputChangedCount, 1);

        session.audioOutput = createObject(audioOutputComponent);
        compare(session.audioOutputChangedCount, 1);

        session.videoOutput = createObject(videoOutputComponent);
        compare(session.videoOutputChangedCount, 1);
    }

    function test_onScreenCaptureChanged_isCalledForEachChange() {
        let session = createTemporaryObject(handlerCaptureSessionComponent, testCase);
        verify(session);

        let screenCapture = createObject(screenCaptureComponent);
        session.screenCapture = screenCapture;
        compare(session.screenCaptureChangedCount, 1);

        // Assigning the same object again is not a change.
        session.screenCapture = screenCapture;
        compare(session.screenCaptureChangedCount, 1);

        session.screenCapture = null;
        compare(session.screenCaptureChangedCount, 2);
    }

    function test_onWindowCaptureChanged_isCalledForEachChange() {
        let session = createTemporaryObject(handlerCaptureSessionComponent, testCase);
        verify(session);

        let windowCapture = createObject(windowCaptureComponent);
        session.windowCapture = windowCapture;
        compare(session.windowCaptureChangedCount, 1);

        // Assigning the same object again is not a change.
        session.windowCapture = windowCapture;
        compare(session.windowCaptureChangedCount, 1);

        session.windowCapture = null;
        compare(session.windowCaptureChangedCount, 2);
    }

    function test_assigningScreenCaptureToAnotherSession_removesItFromTheFirst() {
        let first = createSession();
        let second = createSession();
        let screenCapture = createObject(screenCaptureComponent);

        first.screenCapture = screenCapture;
        second.screenCapture = screenCapture;

        // A capture object can only be used by one session at a time.
        compare(second.screenCapture, screenCapture);
        compare(first.screenCapture, null);
    }

    function test_assigningWindowCaptureToAnotherSession_removesItFromTheFirst() {
        let first = createSession();
        let second = createSession();
        let windowCapture = createObject(windowCaptureComponent);

        first.windowCapture = windowCapture;
        second.windowCapture = windowCapture;

        // A capture object can only be used by one session at a time.
        compare(second.windowCapture, windowCapture);
        compare(first.windowCapture, null);
    }

    function test_destroyingScreenCapture_clearsItFromTheSession() {
        let session = createSession();
        let screenCapture = createObject(screenCaptureComponent);
        session.screenCapture = screenCapture;

        // The session must not be left holding a destroyed object.
        screenCapture.destroy();
        wait(0);

        compare(session.screenCapture, null);
    }

    function test_destroyingWindowCapture_clearsItFromTheSession() {
        let session = createSession();
        let windowCapture = createObject(windowCaptureComponent);
        session.windowCapture = windowCapture;

        // The session must not be left holding a destroyed object.
        windowCapture.destroy();
        wait(0);

        compare(session.windowCapture, null);
    }
}
