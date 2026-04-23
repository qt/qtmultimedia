// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia.qffmpeg;

import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.TotalCaptureResult;
import android.util.Log;

// Used for finalizing a still photo capture. Will reset mState and preview-request back to
// default when capture is done. This should be used for a singular capture-call, not a
// repeating request.
//
// All the events here are invoked from the background processing thread.
class CameraStillPhotoFinalizerCallback extends CameraCaptureSession.CaptureCallback {
    QtCamera2 mMainCameraObject = null;

    CameraStillPhotoFinalizerCallback(QtCamera2 mainCameraObject)
    {
        assert(mainCameraObject != null);
        mMainCameraObject = mainCameraObject;
    }

    // TODO: Implement failure case where we tell QImageCapture that cancel this pending
    // image and then try to reset our camera to preview if applicable.

    // If capture fails, try to return to previewing.
    @Override
    public void onCaptureFailed(
        CameraCaptureSession session,
        CaptureRequest request,
        CaptureFailure failure)
    {
        mMainCameraObject.onStillPhotoCaptureFailed(mMainCameraObject.mCameraId);
        synchronized (mMainCameraObject.mSyncedMembers) {
            mMainCameraObject.mSyncedMembers.mIsTakingStillPhoto = false;
        }
        try {
            mMainCameraObject.setRepeatingRequestToPreview();
        } catch (CameraAccessException e) {
            // TODO: If we fail here, we can clean up the camera session and set the QCamera
            // to unactive.
        }
    }

    @Override
    public void onCaptureCompleted(
        CameraCaptureSession session,
        CaptureRequest request,
        TotalCaptureResult result)
    {
        try {
            mMainCameraObject.mExifDataHandler = new QtExifDataHandler(result);
            synchronized (mMainCameraObject.mSyncedMembers) {
                // If mIsStarted is true, it's an indication the QCamera is active and wants
                // to keep receiving preview frames.
                if (mMainCameraObject.mSyncedMembers.mIsStarted) {
                    mMainCameraObject.setRepeatingRequestToPreview();
                }

                // TODO: If we implement queueing of multiple photos, we should start the
                // process of capturing the next photo here.
                mMainCameraObject.mSyncedMembers.mIsTakingStillPhoto = false;
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException e) {
            // See QTBUG-130901:
            // It should not be possible for mCaptureSession to be null here
            // because we always call .close() on mCaptureSession and then set it to null.
            // Calling .close() should flush all pending callbacks, including this one.
            // Either way, user has evidence this is happening, and catching this exception
            // stops us from crashing the program.
            Log.e(
                "QtCamera2",
                "Null-pointer access exception thrown when finalizing still photo capture. " +
                "This should not be possible.");
            e.printStackTrace();
        } catch (IllegalStateException e) {
            // See QTBUG-136227:
            // According to the Bug description, it may happen that we are trying to call
            // setRepeatingRequest on not active session
            Log.w("QtCamera2", "Session is no longer active.");
            e.printStackTrace();
        }
    }
}
