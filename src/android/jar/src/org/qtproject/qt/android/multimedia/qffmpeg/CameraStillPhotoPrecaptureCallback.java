// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia.qffmpeg;

import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.TotalCaptureResult;

// This callback class is meant to be used a repeating request during still photo capture, when
// waiting for the auto-focus and auto-exposure calibration to lock in. Once done,
// it will finalize the still photo and then return the camera to previewing.
//
// All the events here are invoked from the background processing thread.
class CameraStillPhotoPrecaptureCallback extends CameraCaptureSession.CaptureCallback {
    QtCamera2 mMainCameraObject = null;

    // Holds a copy of the camera settings that were to be used when the still photo
    // was started.
    CameraSettings mCameraSettings = null;
    boolean mWaitForAutoFocus = false;
    boolean mWaitForAutoExposure = false;
    // Testing has showed that this repeating request will keep on invoking methods on this
    // callback object, even after we have submitted a new request. This can make it hard
    // to differentiate between callbacks to this object, or new instances that have been
    // resubmitted for capture. This boolean tracks whether we should be processing incoming
    // events.
    boolean mShouldProcessIncomingEvents = true;

    enum PrecaptureOperation {
        FINALIZE_CAPTURE,
        RESUBMIT_WITH_FORCED_FLASH,
        WAIT
    }

    CameraStillPhotoPrecaptureCallback(
        QtCamera2 mainCameraObject,
        CameraSettings cameraSettings,
        boolean waitForAutoFocus,
        boolean waitForAutoExposure)
    {
        assert(mainCameraObject != null);
        assert(cameraSettings != null);

        mMainCameraObject = mainCameraObject;
        mCameraSettings = cameraSettings;
        mWaitForAutoFocus = waitForAutoFocus;
        mWaitForAutoExposure = waitForAutoExposure;
    }

    boolean capturingWithAutoFlash() {
        return mWaitForAutoExposure
            && mCameraSettings.mStillPhotoFlashMode == CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH;
    }

    private void onCaptureFailureEvent() {
        mShouldProcessIncomingEvents = false;

        mMainCameraObject.onStillPhotoCaptureFailed(mMainCameraObject.mCameraId);

        synchronized (mMainCameraObject.mSyncedMembers) {
            mMainCameraObject.mSyncedMembers.mIsTakingStillPhoto = false;
        }

        // Try to reset our camera to regular preview
        try {
            mMainCameraObject.setRepeatingRequestToPreview();
        } catch (CameraAccessException e) {
            // TODO: If we fail to go back into preview, we can clean up the camera session and
            // set the QCamera to inactive.
        }
    }

    @Override
    public void onCaptureFailed(
        CameraCaptureSession session,
        CaptureRequest request,
        CaptureFailure failure)
    {
        onCaptureFailureEvent();
    }

    // Returns the operation we should do as a result of processing the result.
    private PrecaptureOperation determinePrecaptureOperation(CaptureResult result) {
        Integer afState = result.get(CaptureResult.CONTROL_AF_STATE);
        Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);

        // If we are calibrating with auto flash and we receive FLASH_REQUIRED,
        // we don't have to care about auto focus. Just transition straight to resubmitting
        // the still photo request with flash forced on.
        if (capturingWithAutoFlash() && aeState == CaptureResult.CONTROL_AE_STATE_FLASH_REQUIRED) {
            return PrecaptureOperation.RESUBMIT_WITH_FORCED_FLASH;
        }

        // If we're not waiting for anything, finalize still photo immediately
        if (!mWaitForAutoFocus && !mWaitForAutoExposure) {
            return PrecaptureOperation.FINALIZE_CAPTURE;
        }

        // Wait for focus only
        if (mWaitForAutoFocus && QtCamera2.afStateIsReadyForCapture(afState)
            && !mWaitForAutoExposure)
        {
            return PrecaptureOperation.FINALIZE_CAPTURE;
        }

        // Wait for exposure only
        if (!mWaitForAutoFocus
            && mWaitForAutoExposure && QtCamera2.aeStateIsReadyForCapture(aeState))
        {
            return PrecaptureOperation.FINALIZE_CAPTURE;
        }

        // Wait for focus and exposure
        if (mWaitForAutoFocus && QtCamera2.afStateIsReadyForCapture(afState)
            && mWaitForAutoExposure && QtCamera2.aeStateIsReadyForCapture(aeState))
        {
            return PrecaptureOperation.FINALIZE_CAPTURE;
        }

        return PrecaptureOperation.WAIT;
    }

    @Override
    public void onCaptureCompleted(
        CameraCaptureSession s,
        CaptureRequest r,
        TotalCaptureResult result)
    {
        if (!mShouldProcessIncomingEvents)
            return;

        final PrecaptureOperation operation = determinePrecaptureOperation(result);
        try {
            switch (operation) {
                case FINALIZE_CAPTURE:
                    mMainCameraObject.finalizeStillPhoto(mCameraSettings);
                    break;
                case RESUBMIT_WITH_FORCED_FLASH:
                    // Submit a new still photo capture as if we were forcing flash on.
                    CameraSettings newCameraSettings = new CameraSettings(mCameraSettings);
                    newCameraSettings.mStillPhotoFlashMode = CaptureRequest.CONTROL_AE_MODE_ON_ALWAYS_FLASH;
                    mMainCameraObject.submitNewStillPhotoCapture(newCameraSettings);
                    break;
                default:
                    // Do nothing; wait for next result
                    break;
            }
        } catch (CameraAccessException e) {
            onCaptureFailureEvent();
        }

        if (operation != PrecaptureOperation.WAIT) {
            mShouldProcessIncomingEvents = false;
        }
    }
}
