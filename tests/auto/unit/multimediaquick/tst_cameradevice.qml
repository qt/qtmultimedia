// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia
import QtMultimediaTest
import QtTest

TestCase {
    id: testCase
    name: "CameraDevice"

    Component {
        id: mediaDevicesComponent
        MediaDevices {}
    }

    function initTestCase() {
        verify(TestHelper.mediaBackendName === "mock");
    }

    function test_videoFormats_haveExpectedCount_data() {
        return [
            { tag: "default", index: 0, count: 2 },
            { tag: "front", index: 1, count: 1 },
            { tag: "back, none reported", index: 2, count: 0 }
        ];
    }

    function test_videoFormats_haveExpectedCount(data) {
        let mediaDevices = createTemporaryObject(mediaDevicesComponent, testCase);
        let videoInputs = mediaDevices.videoInputs;
        verify(videoInputs.length > data.index);
        compare(videoInputs[data.index].videoFormats.length, data.count);
    }

    function test_photoResolutions_areReadable_data() {
        return [
            {
                tag: "default",
                index: 0,
                resolutions: [Qt.size(640, 480), Qt.size(1920, 1080)]
            },
            {
                tag: "front",
                index: 1,
                resolutions: [Qt.size(1280, 720)]
            },
            {
                tag: "back, none reported",
                index: 2,
                resolutions: []
            }
        ];
    }

    function test_photoResolutions_areReadable(data) {
        let mediaDevices = createTemporaryObject(mediaDevicesComponent, testCase);
        let videoInputs = mediaDevices.videoInputs;
        verify(videoInputs.length > data.index);

        let resolutions = videoInputs[data.index].photoResolutions;
        compare(resolutions.length, data.resolutions.length);
        for (let i = 0; i < data.resolutions.length; ++i) {
            compare(resolutions[i], data.resolutions[i]);
            compare(resolutions[i].width, data.resolutions[i].width);
            compare(resolutions[i].height, data.resolutions[i].height);
        }
    }

    function test_photoResolutions_isReadOnly() {
        let mediaDevices = createTemporaryObject(mediaDevicesComponent, testCase);
        let device = mediaDevices.videoInputs[0];
        let original = device.photoResolutions;
        try {
            device.photoResolutions = [Qt.size(10, 10)];
        } catch (e) {}
        compare(device.photoResolutions, original);
    }
}
