// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia.qffmpeg;

import android.hardware.camera2.CameraDevice;

// All events in this class are invoked by the background thread.
// TODO: The events in this class access QtCamera2 members directly
// without synchronization
class CameraDeviceStateCallback extends CameraDevice.StateCallback {
    private QtCamera2 mMainCameraObject = null;

    CameraDeviceStateCallback(QtCamera2 mainCameraObject) {
        assert(mainCameraObject != null);
        mMainCameraObject = mainCameraObject;
    }

    @Override
    public void onOpened(CameraDevice cameraDevice) {
        if (mMainCameraObject.mCameraDevice != null)
            mMainCameraObject.mCameraDevice.close();
        mMainCameraObject.mCameraDevice = cameraDevice;
        mMainCameraObject.onCameraOpened(mMainCameraObject.mCameraId);
    }
    @Override
    public void onDisconnected(CameraDevice cameraDevice) {
        cameraDevice.close();
        if (mMainCameraObject.mCameraDevice == cameraDevice)
            mMainCameraObject.mCameraDevice = null;
        mMainCameraObject.onCameraDisconnect(mMainCameraObject.mCameraId);
    }
    @Override
    public void onError(CameraDevice cameraDevice, int error) {
        cameraDevice.close();
        if (mMainCameraObject.mCameraDevice == cameraDevice)
            mMainCameraObject.mCameraDevice = null;
        mMainCameraObject.onCameraError(mMainCameraObject.mCameraId, error);
    }
    @Override
    public void onClosed(CameraDevice cameraDevice) {
        java.util.concurrent.CountDownLatch latch = mMainCameraObject.mDeviceClosedLatch;
        if (latch != null)
            latch.countDown();
    }
}
