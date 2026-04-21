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

    @Override
    public void onConfigured(CameraCaptureSession cameraCaptureSession) {
        mMainCameraObject.mCaptureSession = cameraCaptureSession;
        mMainCameraObject.onCaptureSessionConfigured(mMainCameraObject.mCameraId);
    }

    @Override
    public void onConfigureFailed(CameraCaptureSession cameraCaptureSession) {
        mMainCameraObject.onCaptureSessionConfigureFailed(mMainCameraObject.mCameraId);
    }

    @Override
    public void onActive(CameraCaptureSession cameraCaptureSession) {
       super.onActive(cameraCaptureSession);
       mMainCameraObject.onSessionActive(mMainCameraObject.mCameraId);
    }

    @Override
    public void onClosed(CameraCaptureSession cameraCaptureSession) {
        super.onClosed(cameraCaptureSession);
        mMainCameraObject.onSessionClosed(mMainCameraObject.mCameraId);
    }
}
