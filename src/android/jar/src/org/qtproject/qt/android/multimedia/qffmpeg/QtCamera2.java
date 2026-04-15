// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.Rect;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.TotalCaptureResult;
import android.media.Image;
import android.media.ImageReader;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Range;
import android.view.Surface;
import java.lang.Thread;
import java.util.ArrayList;
import java.util.List;

import org.qtproject.qt.android.UsedFromNativeCode;

@TargetApi(23)
class QtCamera2 {
    // Should be called if an on-going still photo capture has failed to finish.
    // This lets us submit an appropriate error and notify QImageCapture that there will
    // be no photo emitted.
    // TODO: In the future we should send a more descriptive error message to QImageCapture, and
    // and pass it as a parameter here.
    native void onStillPhotoCaptureFailed(String cameraId);

    CameraDevice mCameraDevice = null;
    QtVideoDeviceManager mVideoDeviceManager = null;
    // Thread and handler to that allows us to receive callbacks and frames on a background thread.
    HandlerThread mBackgroundThread;
    Handler mBackgroundHandler;
    // This object allows us to receive frames with associated callbacks. This ImageReader
    // is used to emit preview/video frames to the C++ thread.
    ImageReader mPreviewImageReader = null;
    // Used to emit still photo images to the C++ thread.
    ImageReader mStillPhotoImageReader = null;
    CameraManager mCameraManager;
    CameraCaptureSession mCaptureSession;
    CaptureRequest.Builder mPreviewRequestBuilder;
    CaptureRequest mPreviewRequest;
    String mCameraId;
    List<Surface> mTargetSurfaces = new ArrayList<>();

    private static int MaxNumberFrames = 12;

    private static final int DEFAULT_FLASH_MODE = CaptureRequest.CONTROL_AE_MODE_ON;
    private static final int DEFAULT_TORCH_MODE = CameraMetadata.FLASH_MODE_OFF;
    // Default value in QPlatformCamera is FocusModeAuto, which maps to CONTINUOUS_PICTURE.
    private static final int DEFAULT_AF_MODE = CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE;
    private static final float DEFAULT_FOCUS_DISTANCE = 1.f;
    private static final float DEFAULT_ZOOM_FACTOR = 1.0f;

    class CameraSettings {
        // Not to be confused with QCamera::FlashMode.
        // This controls the currently desired CaptureRequest.CONTROL_AE_MODE,
        // but only for still photos.
        //
        // QCamera::FlashMode::FlashOff maps to CaptureRequest.CONTROL_AE_MODE_ON. This implies
        // regular automatic exposure.
        // QCamera::FlashMode::FlashAuto maps to CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH.
        // QCamera::FlashMode::FlashOn should ideally map to
        // CaptureRequest.CONTROL_AE_MODE_ON_ALWAYS_FLASH, but testing has shown that this is
        // unreliable. Instead we force flash on using CaptureRequest.FLASH_MODE_ON and
        // trigger auto-exposure using CaptureRequest.CONTROL_AE_MODE_ON.
        public int mStillPhotoFlashMode = DEFAULT_FLASH_MODE;

        // Not to be confused with QCamera::TorchMode.
        // This controls the currently desired CaptureRequest.FLASH_MODE
        // QCamera::TorchMode::TorchOff maps to CaptureRequest.FLASH_MODE_OFF
        // QCamera::TorchMode::TorchAuto is not supported.
        // QCamera::TorhcMode::TorchOn maps to CaptureRequest.FLASH_MODE_TORCH.
        public int mTorchMode = DEFAULT_TORCH_MODE;

        // Not to be confused with QCamera::FocusMode
        // This controls the currently desired CaptureRequest.CONTROL_AF_MODE
        // QCamera::FocusMode::FocusModeAuto maps to CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE
        //
        // This variable only controls the AF_MODE we desire to apply. If the device
        // does not support this AF_MODE this will not reflect what the camera is currently doing.
        public int mAFMode = DEFAULT_AF_MODE;

        // Not to be confused with CaptureRequest.LENS_FOCUS_DISTANCE. This variable stores the
        // current QCamera::focusDistance. Must be applied whenever the focus-mode is set to
        // Manual.
        // Must have the same default value as the one set in QPlatformCamera.
        public float mFocusDistance = DEFAULT_FOCUS_DISTANCE;

        // Not to be confused with CaptureRequest.CONTROL_ZOOM_RATIO
        // This matches the current QCamera::zoomFactor of the C++ QCamera object.
        public float mZoomFactor = DEFAULT_ZOOM_FACTOR;

        Range<Integer> mFpsRange = null;

        public CameraSettings() { }

        public CameraSettings(CameraSettings other) {
            this.mStillPhotoFlashMode = other.mStillPhotoFlashMode;
            this.mTorchMode = other.mTorchMode;
            this.mAFMode = other.mAFMode;
            this.mFocusDistance = other.mFocusDistance;
            this.mZoomFactor = other.mZoomFactor;
            if (other.mFpsRange != null) {
                this.mFpsRange = new Range<Integer>(other.mFpsRange.getLower(), other.mFpsRange.getUpper());
            }

        }
    }

    // The purpose of this class is to gather variables that are accessed across
    // the C++ QCamera's thread, and the background capture-processing thread.
    // It also acts as the mutex for these variables.
    // All access to these variables must happen after locking the instance.
    class SyncedMembers {
        private boolean mIsStarted = false;

        private boolean mIsTakingStillPhoto = false;

        private CameraSettings mCameraSettings = new CameraSettings();
    }
    private final SyncedMembers mSyncedMembers = new SyncedMembers();

    // Resets the control properties of this camera to their default values.
    @UsedFromNativeCode
    public void resetControlProperties() {
        synchronized (mSyncedMembers) {
            mSyncedMembers.mCameraSettings = new CameraSettings();
        }
    }

    // Returns a deep copy of the CameraSettings instance. Thread-safe.
    private CameraSettings atomicCameraSettingsCopy() {
        synchronized (mSyncedMembers) {
            return new CameraSettings(mSyncedMembers.mCameraSettings);
        }
    }

    private QtExifDataHandler mExifDataHandler = null;

    native void onCameraOpened(String cameraId);
    native void onCameraDisconnect(String cameraId);
    native void onCameraError(String cameraId, int error);
    CameraDevice.StateCallback mStateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(CameraDevice cameraDevice) {
            if (mCameraDevice != null)
                mCameraDevice.close();
            mCameraDevice = cameraDevice;
            onCameraOpened(mCameraId);
        }
        @Override
        public void onDisconnected(CameraDevice cameraDevice) {
            cameraDevice.close();
            if (mCameraDevice == cameraDevice)
                mCameraDevice = null;
            onCameraDisconnect(mCameraId);
        }
        @Override
        public void onError(CameraDevice cameraDevice, int error) {
            cameraDevice.close();
            if (mCameraDevice == cameraDevice)
                mCameraDevice = null;
            onCameraError(mCameraId, error);
        }
    };

    native void onCaptureSessionConfigured(String cameraId);
    native void onCaptureSessionConfigureFailed(String cameraId);
    CameraCaptureSession.StateCallback mCaptureStateCallback = new CameraCaptureSession.StateCallback() {
        @Override
        public void onConfigured(CameraCaptureSession cameraCaptureSession) {
            mCaptureSession = cameraCaptureSession;
            onCaptureSessionConfigured(mCameraId);
        }

        @Override
        public void onConfigureFailed(CameraCaptureSession cameraCaptureSession) {
            onCaptureSessionConfigureFailed(mCameraId);
        }

        @Override
        public void onActive(CameraCaptureSession cameraCaptureSession) {
           super.onActive(cameraCaptureSession);
           onSessionActive(mCameraId);
        }

        @Override
        public void onClosed(CameraCaptureSession cameraCaptureSession) {
            super.onClosed(cameraCaptureSession);
            onSessionClosed(mCameraId);
        }
    };

    native void onSessionActive(String cameraId);
    native void onSessionClosed(String cameraId);
    native void onCaptureSessionFailed(String cameraId, int reason, long frameNumber);

    // This callback is used when doing normal preview. The only purpose is to detect if something
    // goes wrong, so we can report back to QCamera.
    class PreviewCaptureSessionCallback extends CameraCaptureSession.CaptureCallback {
        @Override
        public void onCaptureFailed(
            CameraCaptureSession session,
            CaptureRequest request,
            CaptureFailure failure)
        {
            super.onCaptureFailed(session, request, failure);
            onCaptureSessionFailed(mCameraId, failure.getReason(), failure.getFrameNumber());
        }
    }

    // Callback that is being used for error-handling when doing preview.
    // TODO: The variable can be removed in the future, and instead just recreate the object
    // every time we go into previewing.
    PreviewCaptureSessionCallback mPreviewCaptureCallback = new PreviewCaptureSessionCallback();

    QtCamera2(Context context) {
        mCameraManager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
        mVideoDeviceManager = new QtVideoDeviceManager(context);
        startBackgroundThread();
    }

    void startBackgroundThread() {
        mBackgroundThread = new HandlerThread("CameraBackground");
        mBackgroundThread.start();
        mBackgroundHandler = new Handler(mBackgroundThread.getLooper());
    }

    @UsedFromNativeCode
    void stopBackgroundThread() {
        mBackgroundThread.quitSafely();
        try {
            mBackgroundThread.join();
            mBackgroundThread = null;
            mBackgroundHandler = null;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @SuppressLint("MissingPermission")
    boolean open(String cameraId) {
        try {
            mCameraId = cameraId;
            mCameraManager.openCamera(cameraId,mStateCallback,mBackgroundHandler);
            return true;
        } catch (Exception e){
            Log.w("QtCamera2", "Failed to open camera:" + e);
        }

        return false;
    }

    native void onStillPhotoAvailable(String cameraId, Image frame);

    // Callback for when we receive a finalized still photo in mStillPhotoImageReader.
    ImageReader.OnImageAvailableListener mOnStillPhotoAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            QtCamera2.this.onStillPhotoAvailable(mCameraId, reader.acquireLatestImage());
        }
    };

    native void onPreviewFrameAvailable(String cameraId, Image frame);

    // Callback for when we receive a preview/video frame in the associated mPreviewImageReader.
    ImageReader.OnImageAvailableListener mOnPreviewImageAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            try {
                Image img = reader.acquireLatestImage();
                if (img != null)
                    QtCamera2.this.onPreviewFrameAvailable(mCameraId, img);
            } catch (IllegalStateException e) {
                // It seems that ffmpeg is processing images for too long (and does not close it)
                // Give it a little more time. Restarting the camera session if it doesn't help
                Log.e("QtCamera2", "Image processing taking too long. Let's wait 0,5s more " + e);
                try {
                    Thread.sleep(500);
                    QtCamera2.this.onPreviewFrameAvailable(mCameraId, reader.acquireLatestImage());
                } catch (IllegalStateException | InterruptedException e2) {
                    Log.e("QtCamera2", "Will not wait anymore. Restart camera session. " + e2);
                    // Remember current used camera ID, because stopAndClose will clear the value
                    String cameraId = mCameraId;
                    stopAndClose();
                    addImageReader(
                        mPreviewImageReader.getWidth(),
                        mPreviewImageReader.getHeight(),
                        mPreviewImageReader.getImageFormat());
                    open(cameraId);
                }
            }
        }
    };

    @UsedFromNativeCode
    void prepareCamera(int width, int height, int format, int minFps, int maxFps) {

        addImageReader(width, height, format);
        setFrameRate(minFps, maxFps);
    }

    private void addImageReader(int width, int height, int format) {

        if (mPreviewImageReader != null)
            removeSurface(mPreviewImageReader.getSurface());

        if (mStillPhotoImageReader != null)
            removeSurface(mStillPhotoImageReader.getSurface());

        mPreviewImageReader = ImageReader.newInstance(width, height, format, MaxNumberFrames);
        mPreviewImageReader.setOnImageAvailableListener(mOnPreviewImageAvailableListener, mBackgroundHandler);
        addSurface(mPreviewImageReader.getSurface());

        mStillPhotoImageReader = ImageReader.newInstance(width, height, format, MaxNumberFrames);
        mStillPhotoImageReader.setOnImageAvailableListener(mOnStillPhotoAvailableListener, mBackgroundHandler);
        addSurface(mStillPhotoImageReader.getSurface());
    }

    private void setFrameRate(int minFrameRate, int maxFrameRate) {
        synchronized (mSyncedMembers) {
            if (minFrameRate <= 0 || maxFrameRate <= 0)
                mSyncedMembers.mCameraSettings.mFpsRange = null;
            else
                mSyncedMembers.mCameraSettings.mFpsRange = new Range<>(minFrameRate, maxFrameRate);
        }
    }

    boolean addSurface(Surface surface) {
        if (mTargetSurfaces.contains(surface))
            return true;

        return mTargetSurfaces.add(surface);
    }

    boolean removeSurface(Surface surface) {
        return  mTargetSurfaces.remove(surface);
    }

    @UsedFromNativeCode
    void clearSurfaces() {
        mTargetSurfaces.clear();
    }

    @UsedFromNativeCode
    boolean createSession() {
        if (mCameraDevice == null)
            return false;

        try {
            // TODO: This API is deprecated and we should transition to the more modern method
            // overload. See QTBUG-134750.
            mCameraDevice.createCaptureSession(mTargetSurfaces, mCaptureStateCallback, mBackgroundHandler);
            return true;
        } catch (Exception exception) {
            Log.w("QtCamera2", "Failed to create a capture session:" + exception);
        }
        return false;
    }

    @UsedFromNativeCode
    boolean start() {
        if (mCameraDevice == null)
            return false;

        if (mCaptureSession == null)
            return false;

        try {
            synchronized (mSyncedMembers) {
                setRepeatingRequestToPreview();
                mSyncedMembers.mIsStarted = true;
            }
            return true;
        } catch (CameraAccessException exception) {
            Log.w("QtCamera2", "Failed to start preview:" + exception);
        }
        return false;
    }

    @UsedFromNativeCode
    void stopAndClose() {
        synchronized (mSyncedMembers) {
            try {
                if (null != mCaptureSession) {
                    mCaptureSession.close();
                    mCaptureSession = null;
                }
                if (null != mCameraDevice) {
                    mCameraDevice.close();
                    mCameraDevice = null;
                }
                mCameraId = "";
                mTargetSurfaces.clear();
            } catch (Exception exception) {
                Log.w("QtCamera2", "Failed to stop and close:" + exception);
            }
            mSyncedMembers.mIsStarted = false;
            mSyncedMembers.mIsTakingStillPhoto = false;
        }
    }

    enum StillPhotoPrecaptureOperation {
        FINALIZE_CAPTURE,
        RESUBMIT_WITH_FORCED_FLASH,
        WAIT
    }

    // This callback class is meant to be used a repeating request during still photo capture, when
    // waiting for the auto-focus and auto-exposure calibration to lock in. Once done,
    // it will finalize the still photo and then return the camera to previewing.
    class StillPhotoPrecaptureCallback extends CameraCaptureSession.CaptureCallback {
        StillPhotoPrecaptureCallback(
            CameraSettings cameraSettings,
            boolean waitForAutoFocus,
            boolean waitForAutoExposure)
        {
            this.mCameraSettings = cameraSettings;
            this.mWaitForAutoFocus = waitForAutoFocus;
            this.mWaitForAutoExposure = waitForAutoExposure;
        }

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

        boolean capturingWithAutoFlash() {
            return mWaitForAutoExposure
                && mCameraSettings.mStillPhotoFlashMode == CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH;
        }

        private void onCaptureFailureEvent() {
            mShouldProcessIncomingEvents = false;

            onStillPhotoCaptureFailed(mCameraId);

            synchronized (mSyncedMembers) {
                mSyncedMembers.mIsTakingStillPhoto = false;
            }

            // Try to reset our camera to regular preview
            try {
                setRepeatingRequestToPreview();
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
        private StillPhotoPrecaptureOperation determinePrecaptureOperation(CaptureResult result) {
            Integer afState = result.get(CaptureResult.CONTROL_AF_STATE);
            Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);

            // If we are calibrating with auto flash and we receive FLASH_REQUIRED,
            // we don't have to care about auto focus. Just transition straight to resubmitting
            // the still photo request with flash forced on.
            if (capturingWithAutoFlash() && aeState == CaptureResult.CONTROL_AE_STATE_FLASH_REQUIRED) {
                return StillPhotoPrecaptureOperation.RESUBMIT_WITH_FORCED_FLASH;
            }

            // If we're not waiting for anything, finalize still photo immediately
            if (!mWaitForAutoFocus && !mWaitForAutoExposure) {
                return StillPhotoPrecaptureOperation.FINALIZE_CAPTURE;
            }

            // Wait for focus only
            if (mWaitForAutoFocus && QtCamera2.afStateIsReadyForCapture(afState)
                && !mWaitForAutoExposure)
            {
                return StillPhotoPrecaptureOperation.FINALIZE_CAPTURE;
            }

            // Wait for exposure only
            if (!mWaitForAutoFocus
                && mWaitForAutoExposure && QtCamera2.aeStateIsReadyForCapture(aeState))
            {
                return StillPhotoPrecaptureOperation.FINALIZE_CAPTURE;
            }

            // Wait for focus and exposure
            if (mWaitForAutoFocus && QtCamera2.afStateIsReadyForCapture(afState)
                && mWaitForAutoExposure && QtCamera2.aeStateIsReadyForCapture(aeState))
            {
                return StillPhotoPrecaptureOperation.FINALIZE_CAPTURE;
            }

            return StillPhotoPrecaptureOperation.WAIT;
        }

        @Override
        public void onCaptureCompleted(
            CameraCaptureSession s,
            CaptureRequest r,
            TotalCaptureResult result)
        {
            if (!mShouldProcessIncomingEvents)
                return;

            final StillPhotoPrecaptureOperation operation = determinePrecaptureOperation(result);
            try {
                switch (operation) {
                    case FINALIZE_CAPTURE:
                        finalizeStillPhoto(mCameraSettings);
                        break;
                    case RESUBMIT_WITH_FORCED_FLASH:
                        // Submit a new still photo capture as if we were forcing flash on.
                        CameraSettings newCameraSettings = new CameraSettings(mCameraSettings);
                        newCameraSettings.mStillPhotoFlashMode = CaptureRequest.CONTROL_AE_MODE_ON_ALWAYS_FLASH;
                        submitNewStillPhotoCapture(newCameraSettings);
                        break;
                    default:
                        // Do nothing; wait for next result
                        break;
                }
            } catch (CameraAccessException e) {
                onCaptureFailureEvent();
            }

            if (operation != StillPhotoPrecaptureOperation.WAIT) {
                mShouldProcessIncomingEvents = false;
            }
        }
    }

    // Used for finalizing a still photo capture. Will reset mState and preview-request back to
    // default when capture is done. This should be used for a singular capture-call, not a
    // repeating request.
    class StillPhotoFinalizerCallback extends CameraCaptureSession.CaptureCallback {
        // TODO: Implement failure case where we tell QImageCapture that cancel this pending
        // image and then try to reset our camera to preview if applicable.

        // If capture fails, try to return to previewing.
        @Override
        public void onCaptureFailed(
            CameraCaptureSession session,
            CaptureRequest request,
            CaptureFailure failure)
        {
            onStillPhotoCaptureFailed(mCameraId);
            synchronized (mSyncedMembers) {
                mSyncedMembers.mIsTakingStillPhoto = false;
            }
            try {
                setRepeatingRequestToPreview();
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
                mExifDataHandler = new QtExifDataHandler(result);
                synchronized (mSyncedMembers) {
                    // If mIsStarted is true, it's an indication the QCamera is active and wants
                    // to keep receiving preview frames.
                    if (mSyncedMembers.mIsStarted) {
                        setRepeatingRequestToPreview();
                    }

                    // TODO: If we implement queueing of multiple photos, we should start the
                    // process of capturing the next photo here.
                    mSyncedMembers.mIsTakingStillPhoto = false;
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

    // Can by StillPhotoPrecaptureCallback on background thread in order to finalize a still photo
    // capture.
    private void finalizeStillPhoto(CameraSettings cameraSettings) throws CameraAccessException
    {
        final CaptureRequest.Builder requestBuilder =
            mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
        requestBuilder.addTarget(mStillPhotoImageReader.getSurface());
        requestBuilder.set(
            CaptureRequest.CONTROL_CAPTURE_INTENT,
            CaptureRequest.CONTROL_CAPTURE_INTENT_STILL_CAPTURE);

        applyStillPhotoSettingsToCaptureRequestBuilder(
            requestBuilder,
            cameraSettings);

        mCaptureSession.capture(
            requestBuilder.build(),
            new StillPhotoFinalizerCallback(),
            mBackgroundHandler);
    }

    // This function starts the process of taking a still photo. The final outputted image will
    // be returned in the still photo image reader.
    //
    // Capturing a still photo requires the following steps:
    // -    We need to submit two requests: One repeating request and one single instant request.
    //      The single request triggers the calibration of auto-focus and/or auto-exposure,
    //      depending on what settings are set for the camera. This calibration takes some time,
    //      and we need to wait for these to settle. The logic for waiting for these to settle
    //      happens in the repeating request using the class StillPhotoPrecaptureCallback.
    // -    When the auto-focus and/or auto-exposure has settled, the StillPhotoPrecaptureCallback
    //      will submit a final request to finalize the photo. When the photo is finalized,
    //      we transition back into regular previewing.
    // -    Supporting auto flash is a corner case. When running auto-flash, we might get the result
    //      that no flash is needed, in which case we continue to finalizing the still photo
    //      as usual. If we receive the result that flash is required, we resubmit the still photo
    //      capture commands all over again as if we are using QCamera::FlashModeOn.
    @UsedFromNativeCode
    void beginStillPhotoCapture() {
        synchronized (mSyncedMembers) {
            if (mSyncedMembers.mIsTakingStillPhoto) {
                // Queuing multiple still photos is not implemented.
                // TODO: We might have to signal to QImageCapture here that capturing failed.
                Log.w(
                    "QtCamera2",
                    "beginStillPhotoCapture() was called on camera backend while there " +
                    "is already a still photo in progress. This is not supported. Likely Qt " +
                    "developer bug.");
                return;
            }
        }

        final CameraSettings cameraSettings = atomicCameraSettingsCopy();
        try {
            submitNewStillPhotoCapture(cameraSettings);
        } catch (CameraAccessException e) {
            Log.w("QtCamera2", "Cannot get access to the camera: " + e);
            e.printStackTrace();
            onStillPhotoCaptureFailed(mCameraId);
            // TODO: Try to go back to previewing if applicable. If that fails too, shut down
            // camera session and report QCamera as inactive.
        }
    }

    void submitNewStillPhotoCapture(CameraSettings cameraSettings) throws CameraAccessException
    {
        CaptureRequest.Builder requestBuilder = mCameraDevice.createCaptureRequest(
            CameraDevice.TEMPLATE_STILL_CAPTURE);
        // Any in-between frames gathered while waiting for still photo, can be sent into
        // the preview ImageReader.
        requestBuilder.addTarget(mPreviewImageReader.getSurface());

        applyStillPhotoSettingsToCaptureRequestBuilder(
            requestBuilder,
            cameraSettings);

        // We need to trigger the auto-focus and auto-exposure mechanism in a single capture
        // request, but waiting for it to settle happens in the repeating request.
        // If configuration ended up with AF_MODE_AUTO, this implies we should trigger the
        // auto focus to lock in.
        final boolean triggerAutoFocus = requestBuilder.get(CaptureRequest.CONTROL_AF_MODE)
            == CaptureResult.CONTROL_AF_MODE_AUTO;

        final Integer aeMode = requestBuilder.get(CaptureRequest.CONTROL_AE_MODE);
        boolean triggerAutoExposure = aeMode != null
            && aeMode != CaptureResult.CONTROL_AE_MODE_OFF;

        final StillPhotoPrecaptureCallback precaptureCallback = new StillPhotoPrecaptureCallback(
            cameraSettings,
            triggerAutoFocus,
            triggerAutoExposure);

        mCaptureSession.setRepeatingRequest(
            requestBuilder.build(),
            precaptureCallback,
            mBackgroundHandler);

        // Once we have prepared the repeating request that will wait, we re-use the
        // request-builder and modify it to include the trigger commands, and then submit
        // it as a one-time request.
        if (triggerAutoFocus) {
            requestBuilder.set(
                CaptureRequest.CONTROL_AF_TRIGGER,
                CaptureRequest.CONTROL_AF_TRIGGER_START);
        }
        if (triggerAutoExposure) {
            requestBuilder.set(
                CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER,
                CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER_START);
        }

        // TODO: We should have a callback here that can track if still photo fails
        mCaptureSession.capture(
            requestBuilder.build(),
            null,
            mBackgroundHandler);

        synchronized (mSyncedMembers) {
            mSyncedMembers.mIsTakingStillPhoto = true;
        }
    }

    @UsedFromNativeCode
    void saveExifToFile(String path)
    {
        if (mExifDataHandler != null)
            mExifDataHandler.save(path);
        else
            Log.e("QtCamera2", "No Exif data that could be saved to " + path);
    }

    private Rect getScalerCropRegion(float zoomFactor)
    {
        Rect activePixels = mVideoDeviceManager.getActiveArraySize(mCameraId);
        float zoomRatio = 1.0f;
        if (zoomFactor != 0.0f)
            zoomRatio = 1.0f / zoomFactor;

        int croppedWidth = activePixels.width() - (int)(activePixels.width() * zoomRatio);
        int croppedHeight = activePixels.height() - (int)(activePixels.height() * zoomRatio);
        return new Rect(croppedWidth/2, croppedHeight/2, activePixels.width() - croppedWidth/2,
                             activePixels.height() - croppedHeight/2);
    }

    private void applyZoomSettingsToRequestBuilder(CaptureRequest.Builder requBuilder, float zoomFactor)
    {
        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.R) {
            requBuilder.set(CaptureRequest.SCALER_CROP_REGION, getScalerCropRegion(zoomFactor));
        } else {
            requBuilder.set(CaptureRequest.CONTROL_ZOOM_RATIO, zoomFactor);
        }
    }

    @UsedFromNativeCode
    void zoomTo(float factor)
    {
        synchronized (mSyncedMembers) {
            mSyncedMembers.mCameraSettings.mZoomFactor = factor;

            if (!mSyncedMembers.mIsStarted) {
                // Camera capture has not begun. Zoom will be applied during start().
                return;
            }

            // TODO: In the future we can call .setRepeatingRequestToPreview() directly,
            // which will recreate the request builder as necessary and apply it with all
            // settings consistently.
            applyZoomSettingsToRequestBuilder(mPreviewRequestBuilder, factor);
            mPreviewRequest = mPreviewRequestBuilder.build();

            if (mSyncedMembers.mIsTakingStillPhoto) {
                // Don't set any request if we are in the middle of taking a still photo.
                // The setting will be applied to the preview after the still photo routine is done.
                return;
            }
            try {
                mCaptureSession.setRepeatingRequest(
                    mPreviewRequest,
                    mPreviewCaptureCallback,
                    mBackgroundHandler);
            } catch (Exception exception) {
                Log.w("QtCamera2", "Failed to set zoom:" + exception);
            }
        }
    }

    // As described in QPlatformCamera::setFocusMode, this function must apply the focus-distance
    // whenever the new QCamera::focusMode is set to Manual.
    // For now, the QtCamera2 implementation only supports Auto and Manual FocusModes.
    @UsedFromNativeCode
    void setFocusMode(int newFocusMode)
    {
        // TODO: In the future, not all QCamera::FocusModes will have a 1:1 mapping to the
        // CONTROL_AF_MODE values. We will need a general solution to translate between
        // QCamera::FocusModes and the relevant Android Camera2 properties.

        // Expand with more values in the future.
        // Translate into the corresponding CONTROL_AF_MODE.
        int newAfMode = 0;
        if (newFocusMode == 0) // FocusModeAuto
            newAfMode = CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        else if (newFocusMode == 5) // FocusModeManual
            newAfMode = CaptureRequest.CONTROL_AF_MODE_OFF;
        else {
            Log.d(
                "QtCamera2",
                "received a QCamera::FocusMode from native code that is not recognized. " +
                "Likely Qt developer bug. Ignoring.");
            return;
        }

        // TODO: Ideally this should check if newAfMode is supported through isAfModeAvailable()
        // but in some situation, mCameraId will be null and therefore isAfModeAvailable will always
        // return false. One example of this is during QCamera::setCameraDevice.
        /*
        if (!isAfModeAvailable(newAfMode)) {
            Log.d(
                "QtCamera2",
                "received a QCamera::FocusMode from native code that is not reported as supported. " +
                "Likely Qt developer bug. Ignoring.");
            return;
        }
         */

        synchronized (mSyncedMembers) {
            mSyncedMembers.mCameraSettings.mAFMode = newAfMode;

            // If the camera is not in the started state yet, we skip activating focus-mode here.
            // Instead it will get applied when the camera is initialized.
            if (!mSyncedMembers.mIsStarted)
                return;

            applyFocusSettingsToCaptureRequestBuilder(
                mPreviewRequestBuilder,
                mSyncedMembers.mCameraSettings,
                false);
            mPreviewRequest = mPreviewRequestBuilder.build();

            if (mSyncedMembers.mIsTakingStillPhoto) {
                // Don't set any request if we are in the middle of taking a still photo.
                // The setting will be applied to the preview after the still photo routine is done.
                return;
            }
            try {
                mCaptureSession.setRepeatingRequest(
                    mPreviewRequest,
                    mPreviewCaptureCallback,
                    mBackgroundHandler);
            } catch (Exception exception) {
                Log.w("QtCamera2", "Failed to set focus mode:" + exception);
            }
        }
    }

    @UsedFromNativeCode
    void setFlashMode(String flashMode)
    {
        synchronized (mSyncedMembers) {
            int flashModeValue = mVideoDeviceManager.stringToControlAEMode(flashMode);
            if (flashModeValue < 0) {
                Log.w("QtCamera2", "Unknown flash mode");
                return;
            }
            mSyncedMembers.mCameraSettings.mStillPhotoFlashMode = flashModeValue;
        }
    }

    // Sets the focus distance of the camera. Input is the same that accepted by the
    // QCamera public API. Accepts a float in the range 0,1. Where 0 means as close as possible,
    // and 1 means infinity.
    //
    // This should never be called if the device specifies focus-distance as unsupported.
    @UsedFromNativeCode
    public void setFocusDistance(float distanceInput)
    {
        if (distanceInput < 0.f || distanceInput > 1.f) {
            Log.w(
                "QtCamera2",
                "received out-of-bounds value when setting camera focus-distance. " +
                "Likely Qt developer bug. Ignoring.");
            return;
        }

        // TODO: Add error handling to check if current mCameraId supports setting focus-distance.
        // See setFocusMode relevant issue.

        synchronized (mSyncedMembers) {
            mSyncedMembers.mCameraSettings.mFocusDistance = distanceInput;

            // If the camera is not in the started state yet, we skip applying any camera-controls
            // here. It will get applied once the camera is ready.
            if (!mSyncedMembers.mIsStarted)
                return;

            // If we are currently in QCamera::FocusModeManual, we apply the focus distance
            // immediately. Otherwise, we store the value and apply it during setFocusMode(Manual).
            if (mSyncedMembers.mCameraSettings.mAFMode == CaptureRequest.CONTROL_AF_MODE_OFF) {
                applyFocusSettingsToCaptureRequestBuilder(
                    mPreviewRequestBuilder,
                    mSyncedMembers.mCameraSettings,
                    false);

                mPreviewRequest = mPreviewRequestBuilder.build();

                if (mSyncedMembers.mIsTakingStillPhoto) {
                    // Don't set any request if we are in the middle of taking a still photo.
                    // The setting will be applied to the preview after the still photo routine is done.
                    return;
                }

                try {
                    mCaptureSession.setRepeatingRequest(
                        mPreviewRequest,
                        mPreviewCaptureCallback,
                        mBackgroundHandler);
                } catch (Exception exception) {
                    Log.w("QtCamera2", "Failed to set focus distance:" + exception);
                }
            }
        }
    }

    private int getTorchModeValue(boolean mode)
    {
        return mode ? CameraMetadata.FLASH_MODE_TORCH : CameraMetadata.FLASH_MODE_OFF;
    }

    @UsedFromNativeCode
    void setTorchMode(boolean torchMode)
    {
        synchronized (mSyncedMembers) {
            mSyncedMembers.mCameraSettings.mTorchMode = getTorchModeValue(torchMode);

            if (mSyncedMembers.mIsStarted) {
                mPreviewRequestBuilder.set(CaptureRequest.FLASH_MODE, mSyncedMembers.mCameraSettings.mTorchMode);
                mPreviewRequest = mPreviewRequestBuilder.build();

                if (mSyncedMembers.mIsTakingStillPhoto) {
                    // Don't set any request if we are in the middle of taking a still photo.
                    // The setting will be applied to the preview after the still photo routine is done.
                    return;
                }

                try {
                    mCaptureSession.setRepeatingRequest(
                        mPreviewRequest,
                        mPreviewCaptureCallback,
                        mBackgroundHandler);
                } catch (Exception exception) {
                    Log.w("QtCamera2", "Failed to set flash mode:" + exception);
                }
            }
        }
    }

    // Called indirectly from C++ when the QCamera goes active.
    // Called again from Java camera background thread when a still photo is done.
    @UsedFromNativeCode
    private void setRepeatingRequestToPreview() throws CameraAccessException {
        synchronized (mSyncedMembers) {
            mPreviewRequestBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
            mPreviewRequestBuilder.addTarget(mPreviewImageReader.getSurface());

            applyPreviewSettingsToCaptureRequestBuilder(
                mPreviewRequestBuilder,
                mSyncedMembers.mCameraSettings);

            mPreviewRequest = mPreviewRequestBuilder.build();
            mCaptureSession.setRepeatingRequest(
                mPreviewRequest,
                mPreviewCaptureCallback,
                mBackgroundHandler);
        }
    }

    private void applyStillPhotoSettingsToCaptureRequestBuilder(
        CaptureRequest.Builder requestBuilder,
        CameraSettings cameraSettings)
    {
        requestBuilder.set(
            CaptureRequest.CONTROL_CAPTURE_INTENT,
            CaptureRequest.CONTROL_CAPTURE_INTENT_STILL_CAPTURE);
        // Hint for the camera to use automatic modes for auto-focus, auto-exposure and
        // white-balance where applicable.
        requestBuilder.set(
            CaptureRequest.CONTROL_MODE,
            CaptureRequest.CONTROL_MODE_AUTO);
        // TODO: We don't support any other exposure modes (such as manual control) yet. Will need
        // to modify this in the future if we do.
        requestBuilder.set(
            CaptureRequest.CONTROL_AE_MODE,
            CaptureRequest.CONTROL_AE_MODE_ON);

        applyZoomSettingsToRequestBuilder(requestBuilder, cameraSettings.mZoomFactor);

        applyFocusSettingsToCaptureRequestBuilder(
            requestBuilder,
            cameraSettings,
            true);

        // Ideally we would pass AE_MODE_ON_ALWAYS_FLASH straight to the camera and let it
        // control the flash unit. This has proven unreliable during testing. Instead we use
        // regular CONTROL_AE_MODE_ON and force the flash on.
        if (cameraSettings.mStillPhotoFlashMode == CaptureRequest.CONTROL_AE_MODE_ON_ALWAYS_FLASH) {
            requestBuilder.set(
                CaptureRequest.CONTROL_AE_MODE,
                CaptureRequest.CONTROL_AE_MODE_ON);
            // Ideally, this should be set to SINGLE, only when we are finalizing the capture.
            // However, this causes an issue on some Motorola devices where the flash will have
            // wrong timing compared to the capture, and we will end up with no flash in the final
            // photo.
            requestBuilder.set(
                CaptureRequest.FLASH_MODE,
                CaptureRequest.FLASH_MODE_TORCH);
        } else if (cameraSettings.mStillPhotoFlashMode == CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH) {
            requestBuilder.set(
                CaptureRequest.CONTROL_AE_MODE,
                CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH);
        }
    }

    private void applyPreviewSettingsToCaptureRequestBuilder(
        CaptureRequest.Builder requestBuilder,
        CameraSettings cameraSettings)
    {
        requestBuilder.set(CaptureRequest.CONTROL_CAPTURE_INTENT, CameraMetadata.CONTROL_CAPTURE_INTENT_VIDEO_RECORD);

        applyFocusSettingsToCaptureRequestBuilder(
            requestBuilder,
            cameraSettings,
            false);

        // TODO: Check if AE_MODE_ON is available
        requestBuilder.set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON);
        requestBuilder.set(CaptureRequest.FLASH_MODE, cameraSettings.mTorchMode);

        applyZoomSettingsToRequestBuilder(requestBuilder, cameraSettings.mZoomFactor);
        if (cameraSettings.mFpsRange != null) {
            requestBuilder.set(
                CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE,
                cameraSettings.mFpsRange);
        }

        // TODO: This should likely not be set because trigger-events should only be submitted
        // once. Meanwhile, this request will be used for preview which is repeating.
        mPreviewRequestBuilder.set(
            CaptureRequest.CONTROL_AF_TRIGGER,
            CameraMetadata.CONTROL_AF_TRIGGER_IDLE);
        mPreviewRequestBuilder.set(
            CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER,
            CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER_IDLE);
    }

    // If taking still photo, remember to trigger auto-focus calibration if CONTROL_AF_MODE is set
    // to CONTROL_AF_MODE_AUTO.
    void applyFocusSettingsToCaptureRequestBuilder(
         CaptureRequest.Builder requestBuilder,
         CameraSettings cameraSettings,
         boolean stillPhoto)
    {
        int desiredAfMode = cameraSettings.mAFMode;
        // During still photo, If the camera settings is set to CONTINUOUS_PICTURE, this is an
        // indication that we are in QCamera::FocusModeAuto. In which case we should be using
        // AF_MODE_AUTO, which lets us lock in focus once and keep it there until still photo
        // is done.
        if (stillPhoto && desiredAfMode == CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
            desiredAfMode = CaptureRequest.CONTROL_AF_MODE_AUTO;
        }

        if (!isAfModeAvailable(desiredAfMode)) {
            // If we don't support our desired AF_MODE, fallback to AF_MODE_OFF if that is
            // available. Otherwise don't set any focus-mode, leave it as default and
            // undefined state. Note: Setting CONTROL_AF_MODE to null is illegal and will cause an
            // exception thrown.
            if (isAfModeAvailable(CaptureRequest.CONTROL_AF_MODE_OFF)) {
                requestBuilder.set(
                    CaptureRequest.CONTROL_AF_MODE,
                    CaptureRequest.CONTROL_AF_MODE_OFF);
            }

            requestBuilder.set(CaptureRequest.LENS_FOCUS_DISTANCE, null);
            return;
        }

        requestBuilder.set(CaptureRequest.CONTROL_AF_MODE, desiredAfMode);

        // Set correct lens focus distance if we are in QCamera::FocusModeManual
        if (desiredAfMode == CaptureRequest.CONTROL_AF_MODE_OFF) {
            final float lensFocusDistance = calcLensFocusDistanceFromQCameraFocusDistance(
                cameraSettings.mFocusDistance);
            if (lensFocusDistance < 0) {
                Log.w(
                    "QtCamera2",
                    "Tried to apply FocusModeManual on a camera that doesn't support "
                    + "setting lens distance. Likely Qt developer bug. Ignoring.");
            } else {
                requestBuilder.set(CaptureRequest.LENS_FOCUS_DISTANCE, lensFocusDistance);
            }
        } else {
            requestBuilder.set(CaptureRequest.LENS_FOCUS_DISTANCE, null);
        }
    }

    // Calculates the CaptureRequest.LENS_FOCUS_DISTANCE equivalent given a QCamera::focusDistance
    // value. Returns -1 on failure, such as if camera does not support setting manual focus
    // distance.
    private float calcLensFocusDistanceFromQCameraFocusDistance(float qCameraFocusDistance) {
        float lensMinimumFocusDistance =
            mVideoDeviceManager.getLensInfoMinimumFocusDistance(mCameraId);
        if (lensMinimumFocusDistance <= 0)
            return -1;

        // Input is 0 to 1, with 0 meaning as close as possible.
        // Android Camera2 expects it to be in the range [0, minimumFocusDistance]
        // where higher values means closer to the camera and 0 means as far away as possible.
        // We need to map to this range.
        return (1.f - qCameraFocusDistance) * lensMinimumFocusDistance;
    }

    // Helper function to check if a given CaptureRequest.CONTROL_AF_MODE is supported on this
    // device
    private boolean isAfModeAvailable(int afMode) {
        if (mVideoDeviceManager == null || mCameraId == null || mCameraId.isEmpty())
            return false;
        return mVideoDeviceManager.isAfModeAvailable(mCameraId, afMode);
    }

    // AF_STATE_NOT_FOCUSED_LOCKED implies we tried to calibrate the auto-focus, but failed
    // to establish focus and the hardware has now given up and locked the focus.
    static boolean afStateIsReadyForCapture(Integer afState) {
        return afState == null
            || afState == CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED
            || afState == CaptureResult.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
    }

    static boolean aeStateIsReadyForCapture(Integer aeState) {
        return aeState == null
            || aeState == CaptureResult.CONTROL_AE_STATE_CONVERGED
            || aeState == CaptureResult.CONTROL_AE_STATE_FLASH_REQUIRED;
    }
}
