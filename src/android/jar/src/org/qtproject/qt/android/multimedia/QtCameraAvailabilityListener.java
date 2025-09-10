// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import android.app.Activity;
import android.hardware.camera2.CameraManager;
import android.os.Handler;
import android.util.Log;

import org.qtproject.qt.android.UsedFromNativeCode;

import java.util.concurrent.CountDownLatch;

// This class implements the camera-device availability callbacks for Android CameraManager.
// This class should only be used with QAndroidVideoDevices.
// In the future it might make sense to a central QtVideoDeviceManager that everything references,
// and to build this class' functionality into that.
class QtCameraAvailabilityListener extends CameraManager.AvailabilityCallback {

    static final String LOG_TAG = "QtCameraAvailabilityListener";

    CameraManager mCameraManager = null;
    // Holds the pointer to the QAndroidVideoDevices instance.
    long mVideoDevicesNativePtr = 0;

    Handler mAvailabilityCallbackHandler = null;

    @UsedFromNativeCode
    QtCameraAvailabilityListener(Activity activity, long videoDevicesNativePtr) {
        mCameraManager = (CameraManager)activity.getSystemService(Activity.CAMERA_SERVICE);
        mVideoDevicesNativePtr = videoDevicesNativePtr;

        // We use the Android main UI thread for receiving callbacks.
        mAvailabilityCallbackHandler = new Handler(activity.getMainLooper());

        // TODO: This will call C++ updateNativeVideoDevices() for each currently available
        // camera when registered, possibly leading to us enumerating video-devices unncessarily
        // often, and also signalling QMediaDevices::videoInputsChanged more than necessary.
        // This should generally not be a problem assuming the multimedia backend is initialized
        // before user-code.
        //
        // A future solution could be to post a job on the handler and wait for it, or
        // to dynamically add/remove devices from an internal list which is then returned in
        // QAndroidVideoDevices::findVideoInputs()
        mCameraManager.registerAvailabilityCallback(this, mAvailabilityCallbackHandler);
    }

    @UsedFromNativeCode
    void cleanup() {
        // Perform cleanup on the same thread that we receive callbacks, then wait for cleanup job
        // to finish. No need for locks.
        final CountDownLatch latch = new CountDownLatch(1);
        final boolean postSuccess = mAvailabilityCallbackHandler.post(() -> {
            mCameraManager.unregisterAvailabilityCallback(this);
            mVideoDevicesNativePtr = 0;
            latch.countDown(); // Signal that the task is done
        });

        if (!postSuccess) {
            Log.w(
                LOG_TAG,
                "Unable to post cleanup job to corresponding thread during " +
                "QAndroidVideoDevices cleanup.");
            return;
        }

        try {
            latch.await();
        } catch (InterruptedException e) {
            Log.w(
                LOG_TAG,
                "Unable to wait for cleanup job to finish on corresponding thread during" +
                "QAndroidVideoDevices cleanup.");
        }

        // After waiting, QAndroidVideoDevices can now be safely destroyed.
    }

    native void onCameraAvailableNative(long videoDevicesNativePtr);

    // Will be called once for each connected camera device when availability-listener
    // is attached to the Android CameraManager.
    @Override
    public void onCameraAvailable(String cameraId) {
        assert(mVideoDevicesNativePtr != 0);
        onCameraAvailableNative(mVideoDevicesNativePtr);
    }

    native void onCameraUnavailableNative(long videoDevicesNativePtr);

    @Override
    public void onCameraUnavailable(String cameraId) {
        assert(mVideoDevicesNativePtr != 0);
        onCameraUnavailableNative(mVideoDevicesNativePtr);
    }

    @Override
    public void onCameraAccessPrioritiesChanged() {
        // TODO: In the future we should handle camera priorities, such as when the camera-device
        // is connected but is in use by another application, making the current application unable
        // to use it until the device is released by the other application.
    }
}
