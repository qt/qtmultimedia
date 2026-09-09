// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtMultimediaTest
import QtTest

TestCase {
    id: testCase
    name: "CameraFormat"

    Component {
        id: mediaDevicesComponent
        MediaDevices {}
    }

    Component {
        id: cameraComponent
        Camera {}
    }

    function initTestCase() {
        verify(TestHelper.mediaBackendName === "mock");
    }

    function test_mediaDevicesExposesAtleastTwoFormats() {
        let mediaDevices = createTemporaryObject(mediaDevicesComponent, testCase);
        let videoInputs = mediaDevices.videoInputs;
        verify(videoInputs.length >= 2)

        let cameraDevice = videoInputs[0];
        verify(cameraDevice.videoFormats.length >= 2)
    }

    function test_cameraFormatIsEqualityComparable() {
        let mediaDevices = createTemporaryObject(mediaDevicesComponent, testCase);
        let a = mediaDevices.videoInputs[0].videoFormats[0];
        verify(a === a)
        compare(a, a);
    }

    function test_cameraFormatIsInequalityComparable() {
        let mediaDevices = createTemporaryObject(mediaDevicesComponent, testCase);
        let a = mediaDevices.videoInputs[0].videoFormats[0];
        let b = mediaDevices.videoInputs[0].videoFormats[1];
        verify(a !== b);
    }

    function test_cameraFormatIsImplicitlyConvertibleToQCameraFormat() {
        let mediaDevices = createTemporaryObject(mediaDevicesComponent, testCase);
        let a = mediaDevices.videoInputs[0].videoFormats[0];
        verify(CameraFormatHelper.implicitlyConvertibleToQCameraFormat(a));
    }

    function test_readProperties_data() {
        return [
            {
                tag: "ARGB8888",
                index: 0,
                pixelFormat: 1,
                resolution: Qt.size(640, 480),
                minFrameRate: 0,
                maxFrameRate: 30
            },
            {
                tag: "YUV420P",
                index: 1,
                pixelFormat: 13,
                resolution: Qt.size(640, 480),
                minFrameRate: 0,
                maxFrameRate: 30
            }
        ];
    }

    // This test just confirms that we can indeed read the properties we expect
    // to be able to read from a CameraFormat instance.
    function test_readProperties(data) {
        let mediaDevices = createTemporaryObject(mediaDevicesComponent, testCase);
        let formats = mediaDevices.videoInputs[0].videoFormats;
        verify(formats.length > data.index);

        let format = formats[data.index];
        compare(format.resolution, data.resolution);
        compare(format.resolution.width, data.resolution.width);
        compare(format.resolution.height, data.resolution.height);
        compare(format.pixelFormat, data.pixelFormat);
        compare(format.minFrameRate, data.minFrameRate);
        compare(format.maxFrameRate, data.maxFrameRate);
    }

    // All CameraFormat properties are declared CONSTANT, so the QML engine
    // must reject any assignment and leave the value unchanged.
    //
    // QML engine will throw exception if we try to assign a readonly QML property.
    function test_propertiesAreReadOnly() {
        let mediaDevices = createTemporaryObject(mediaDevicesComponent, testCase);
        let cameraFormat = mediaDevices.videoInputs[0].videoFormats[0];

        let originalPixelFormat = cameraFormat.pixelFormat;
        try {
            cameraFormat.pixelFormat = 0;
        } catch (e) {}
        compare(cameraFormat.pixelFormat, originalPixelFormat);

        let originalMinFrameRate = cameraFormat.minFrameRate;
        try {
            cameraFormat.minFrameRate = 1;
        } catch (e) {}
        compare(cameraFormat.minFrameRate, originalMinFrameRate);

        let originalMaxFrameRate = cameraFormat.maxFrameRate;
        try {
            cameraFormat.maxFrameRate = 1;
        } catch (e) {}
        compare(cameraFormat.maxFrameRate, originalMaxFrameRate);

        let originalResolution = cameraFormat.resolution;
        try {
            cameraFormat.resolution = Qt.size(10, 10);
        } catch (e) {}
        compare(cameraFormat.resolution, originalResolution);
    }

    function test_defaultCameraFormatIsNull() {
        let camera = createTemporaryObject(cameraComponent, testCase);
        let format = camera.cameraFormat;
        compare(format.resolution, Qt.size(-1,-1));
        compare(format.pixelFormat, 0);
        compare(format.minFrameRate, 0);
        compare(format.maxFrameRate, 0);
    }

    function test_cameraFormatCanBeAssignedToCamera() {
        let mediaDevices = createTemporaryObject(mediaDevicesComponent, testCase);
        let cameraDevice = mediaDevices.videoInputs[0];
        let format = cameraDevice.videoFormats[0];
        let camera = createTemporaryObject(cameraComponent, testCase);
        camera.cameraDevice = cameraDevice;
        compare(camera.cameraDevice, cameraDevice);

        camera.cameraFormat = format;
        compare(camera.cameraFormat, format);
    }

    function test_canCreateCameraFormat() {
        let cameraFormat = CameraFormatHelper.createCameraFormat(
            Qt.size(1920, 1080),
            1,
            1.0,
            60.0);
        verify(cameraFormat);
    }

    // Fabricate two separate QML CameraFormat instances that have
    // equal properties, then check that they compare to be equal.
    function test_separateCameraFormatInstancesAreEqual() {
        let a = CameraFormatHelper.createCameraFormat(
            Qt.size(1920, 1080),
            1,
            1.0,
            60.0);
        let b = CameraFormatHelper.createCameraFormat(
            Qt.size(1920, 1080),
            1,
            1.0,
            60.0);
        compare(a, b);
    }
}
