// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia.qffmpeg;

import android.hardware.camera2.CameraCaptureSession;

class CameraCaptureSessionStateCallback extends CameraCaptureSession.StateCallback {
    private QtCamera2 mMainCameraObject = null;

    CameraCaptureSessionStateCallback(QtCamera2 mainCameraObject) {
        assert(mainCameraObject != null);
        mMainCameraObject = mainCameraObject;
    }

    private boolean isSuperseded() {
        return mMainCameraObject.mCurrentSessionCallback != this;
    }

    @Override
    public void onConfigured(CameraCaptureSession cameraCaptureSession) {
        if (isSuperseded())
            return;
        mMainCameraObject.mCaptureSession = cameraCaptureSession;
        mMainCameraObject.onCaptureSessionConfigured(mMainCameraObject.mCameraId);
        mMainCameraObject.onSessionReconfigured(true);
    }

    @Override
    public void onConfigureFailed(CameraCaptureSession cameraCaptureSession) {
        if (isSuperseded())
            return;
        mMainCameraObject.onSessionConfigureFailed();
    }

    @Override
    public void onActive(CameraCaptureSession cameraCaptureSession) {
       super.onActive(cameraCaptureSession);
       if (isSuperseded())
           return;
       mMainCameraObject.onSessionActive(mMainCameraObject.mCameraId);
    }

    @Override
    public void onClosed(CameraCaptureSession cameraCaptureSession) {
        super.onClosed(cameraCaptureSession);
        if (isSuperseded())
            return;
        mMainCameraObject.onSessionClosed(mMainCameraObject.mCameraId);
    }
}
