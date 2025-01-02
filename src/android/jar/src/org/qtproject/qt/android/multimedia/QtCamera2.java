// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia;

import org.qtproject.qt.android.multimedia.QtVideoDeviceManager;
import org.qtproject.qt.android.multimedia.QtExifDataHandler;

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
import android.graphics.ImageFormat;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.Surface;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import java.lang.Thread;
import java.util.ArrayList;
import java.util.List;

@TargetApi(23)
public class QtCamera2 {

    CameraDevice mCameraDevice = null;
    QtVideoDeviceManager mVideoDeviceManager = null;
    HandlerThread mBackgroundThread;
    Handler mBackgroundHandler;
    ImageReader mImageReader = null;
    ImageReader mCapturedPhotoReader = null;
    CameraManager mCameraManager;
    CameraCaptureSession mCaptureSession;
    CaptureRequest.Builder mPreviewRequestBuilder;
    CaptureRequest mPreviewRequest;
    String mCameraId;
    List<Surface> mTargetSurfaces = new ArrayList<>();

    private static final int STATE_PREVIEW = 0;
    private static final int STATE_WAITING_LOCK = 1;
    private static final int STATE_WAITING_PRECAPTURE = 2;
    private static final int STATE_WAITING_NON_PRECAPTURE = 3;
    private static final int STATE_PICTURE_TAKEN = 4;

    private int mState = STATE_PREVIEW;
    private Object mStartMutex = new Object();
    private boolean mIsStarted = false;
    private static int MaxNumberFrames = 10;
    private int mFlashMode = CaptureRequest.CONTROL_AE_MODE_ON;
    private int mTorchMode = CameraMetadata.FLASH_MODE_OFF;
    private int mAFMode = CaptureRequest.CONTROL_AF_MODE_OFF;
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
    CameraCaptureSession.CaptureCallback mCaptureCallback = new CameraCaptureSession.CaptureCallback() {
        public void onCaptureFailed(CameraCaptureSession session,  CaptureRequest request,  CaptureFailure failure) {
            super.onCaptureFailed(session, request, failure);
            onCaptureSessionFailed(mCameraId, failure.getReason(), failure.getFrameNumber());
        }

        private void process(CaptureResult result) {
            switch (mState) {
                case STATE_WAITING_LOCK: {
                    Integer afState = result.get(CaptureResult.CONTROL_AF_STATE);
                    if (CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED == afState ||
                        CaptureResult.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED == afState) {
                        Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);
                        if (aeState == null ||
                                aeState == CaptureResult.CONTROL_AE_STATE_CONVERGED) {
                            mState = STATE_PICTURE_TAKEN;
                            capturePhoto();
                        } else {
                            try {
                                mPreviewRequestBuilder.set(
                                    CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER,
                                    CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER_START);
                                mState = STATE_WAITING_PRECAPTURE;
                                mCaptureSession.capture(mPreviewRequestBuilder.build(),
                                                        mCaptureCallback,
                                                        mBackgroundHandler);
                            } catch (CameraAccessException e) {
                                Log.w("QtCamera2", "Cannot get access to the camera: " + e);
                            }
                        }
                    }
                    break;
                }
                case STATE_WAITING_PRECAPTURE: {
                    Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);
                    if (aeState == null || aeState == CaptureResult.CONTROL_AE_STATE_PRECAPTURE) {
                        mState = STATE_WAITING_NON_PRECAPTURE;
                    }
                    break;
                }
                case STATE_WAITING_NON_PRECAPTURE: {
                    Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);
                    if (aeState == null || aeState != CaptureResult.CONTROL_AE_STATE_PRECAPTURE) {
                        mState = STATE_PICTURE_TAKEN;
                        capturePhoto();
                    }
                    break;
                }
                default:
                    break;
            }
        }

        @Override
        public void onCaptureProgressed(CameraCaptureSession s, CaptureRequest r, CaptureResult partialResult) {
            process(partialResult);
        }

        @Override
        public void onCaptureCompleted(CameraCaptureSession s, CaptureRequest r, TotalCaptureResult result) {
            process(result);
        }
    };

    public QtCamera2(Context context) {
        mCameraManager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
        mVideoDeviceManager = new QtVideoDeviceManager(context);
        startBackgroundThread();
    }

    void startBackgroundThread() {
        mBackgroundThread = new HandlerThread("CameraBackground");
        mBackgroundThread.start();
        mBackgroundHandler = new Handler(mBackgroundThread.getLooper());
    }

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
    public boolean open(String cameraId) {
        try {
            mCameraId = cameraId;
            mCameraManager.openCamera(cameraId,mStateCallback,mBackgroundHandler);
            return true;
        } catch (Exception e){
            Log.w("QtCamera2", "Failed to open camera:" + e);
        }

        return false;
    }

    native void onPhotoAvailable(String cameraId, Image frame);

    ImageReader.OnImageAvailableListener mOnPhotoAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            QtCamera2.this.onPhotoAvailable(mCameraId, reader.acquireLatestImage());
        }
    };

    native void onFrameAvailable(String cameraId, Image frame);

    ImageReader.OnImageAvailableListener mOnImageAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            try {
                Image img = reader.acquireLatestImage();
                if (img != null)
                    QtCamera2.this.onFrameAvailable(mCameraId, img);
            } catch (IllegalStateException e) {
                // It seems that ffmpeg is processing images for too long (and does not close it)
                // Give it a little more time. Restarting the camera session if it doesn't help
                Log.e("QtCamera2", "Image processing taking too long. Let's wait 0,5s more " + e);
                try {
                    Thread.sleep(500);
                    QtCamera2.this.onFrameAvailable(mCameraId, reader.acquireLatestImage());
                } catch (IllegalStateException | InterruptedException e2) {
                    Log.e("QtCamera2", "Will not wait anymore. Restart camera session. " + e2);
                    // Remember current used camera ID, because stopAndClose will clear the value
                    String cameraId = mCameraId;
                    stopAndClose();
                    addImageReader(mImageReader.getWidth(), mImageReader.getHeight(),
                                   mImageReader.getImageFormat());
                    open(cameraId);
                }
            }
        }
    };

    public boolean addImageReader(int width, int height, int format) {

        if (mImageReader != null)
            removeSurface(mImageReader.getSurface());

        if (mCapturedPhotoReader != null)
            removeSurface(mCapturedPhotoReader.getSurface());

        mImageReader = ImageReader.newInstance(width, height, format, MaxNumberFrames);
        mImageReader.setOnImageAvailableListener(mOnImageAvailableListener, mBackgroundHandler);
        addSurface(mImageReader.getSurface());

        mCapturedPhotoReader = ImageReader.newInstance(width, height, format, MaxNumberFrames);
        mCapturedPhotoReader.setOnImageAvailableListener(mOnPhotoAvailableListener, mBackgroundHandler);
        addSurface(mCapturedPhotoReader.getSurface());

        return true;
    }

    public boolean addSurface(Surface surface) {
        if (mTargetSurfaces.contains(surface))
            return true;

        return mTargetSurfaces.add(surface);
    }

    public boolean removeSurface(Surface surface) {
        return  mTargetSurfaces.remove(surface);
    }

    public void clearSurfaces() {
        mTargetSurfaces.clear();
    }

    public boolean createSession() {
        if (mCameraDevice == null)
            return false;

        try {
            mCameraDevice.createCaptureSession(mTargetSurfaces, mCaptureStateCallback, mBackgroundHandler);
            return true;
        } catch (Exception exception) {
            Log.w("QtCamera2", "Failed to create a capture session:" + exception);
        }
        return false;
    }

    public boolean start(int template) {

        if (mCameraDevice == null)
            return false;

        if (mCaptureSession == null)
            return false;

        synchronized (mStartMutex) {
            try {
                mPreviewRequestBuilder = mCameraDevice.createCaptureRequest(template);
                mPreviewRequestBuilder.addTarget(mImageReader.getSurface());
                mAFMode = CaptureRequest.CONTROL_AF_MODE_OFF;
                for (int mode : mVideoDeviceManager.getSupportedAfModes(mCameraId)) {
                    if (mode == CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
                        mAFMode = mode;
                        break;
                    }
                }

                mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, mFlashMode);
                mPreviewRequestBuilder.set(CaptureRequest.FLASH_MODE, mTorchMode);
                mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_IDLE);
                mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, mAFMode);
                mPreviewRequestBuilder.set(CaptureRequest.CONTROL_CAPTURE_INTENT, CameraMetadata.CONTROL_CAPTURE_INTENT_VIDEO_RECORD);

                mPreviewRequest = mPreviewRequestBuilder.build();
                mCaptureSession.setRepeatingRequest(mPreviewRequest, mCaptureCallback, mBackgroundHandler);
                mIsStarted = true;
                return true;

            } catch (Exception exception) {
                Log.w("QtCamera2", "Failed to start preview:" + exception);
            }
            return false;
        }
    }

    public void stopAndClose() {
        synchronized (mStartMutex) {
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
            mIsStarted = false;
        }
    }

    private void capturePhoto() {
        try {
            final CaptureRequest.Builder captureBuilder =
                   mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            captureBuilder.addTarget(mCapturedPhotoReader.getSurface());
            captureBuilder.set(CaptureRequest.CONTROL_AE_MODE, mFlashMode);

            CameraCaptureSession.CaptureCallback captureCallback
                        = new CameraCaptureSession.CaptureCallback() {
            @Override
            public void onCaptureCompleted(CameraCaptureSession session, CaptureRequest request,
                                           TotalCaptureResult result) {
                    try {
                        mExifDataHandler = new QtExifDataHandler(result);
                        // Reset the focus/flash and go back to the normal state of preview.
                        mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER,
                                                   CameraMetadata.CONTROL_AF_TRIGGER_IDLE);
                        mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER,
                                                   CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER_IDLE);
                        mPreviewRequest = mPreviewRequestBuilder.build();
                        mState = STATE_PREVIEW;
                        mCaptureSession.setRepeatingRequest(mPreviewRequest,
                                                            mCaptureCallback,
                                                            mBackgroundHandler);
                    } catch (CameraAccessException e) {
                        e.printStackTrace();
                    }
                }
            };

            mCaptureSession.capture(captureBuilder.build(), captureCallback, mBackgroundHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    public void takePhoto() {
        try {
            if (mAFMode == CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
                mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_START);
                mState = STATE_WAITING_LOCK;
                mCaptureSession.capture(mPreviewRequestBuilder.build(), mCaptureCallback, mBackgroundHandler);
            } else {
                capturePhoto();
            }
        } catch (CameraAccessException e) {
            Log.w("QtCamera2", "Cannot get access to the camera: " + e);
        }
    }

    public void saveExifToFile(String path)
    {
        if (mExifDataHandler != null)
            mExifDataHandler.save(path);
        else
            Log.e("QtCamera2", "No Exif data that could be saved to " + path);
    }

    public void zoomTo(float factor)
    {
        synchronized (mStartMutex) {
            if (!mIsStarted) {
                Log.w("QtCamera2", "Cannot set zoom on invalid camera");
                return;
            }

            Rect activePixels = mVideoDeviceManager.getActiveArraySize(mCameraId);
            float zoomRatio = 1/factor;
            int croppedWidth = activePixels.width() - (int)(activePixels.width() * zoomRatio);
            int croppedHeight = activePixels.height() - (int)(activePixels.height() * zoomRatio);
            Rect zoom = new Rect(croppedWidth/2, croppedHeight/2, activePixels.width() - croppedWidth/2,
                                 activePixels.height() - croppedHeight/2);
            mPreviewRequestBuilder.set(CaptureRequest.SCALER_CROP_REGION, zoom);
            mPreviewRequest = mPreviewRequestBuilder.build();

            try {
                mCaptureSession.setRepeatingRequest(mPreviewRequest, mCaptureCallback, mBackgroundHandler);
            } catch (Exception exception) {
                Log.w("QtCamera2", "Failed to set zoom:" + exception);
            }
        }
    }
    public void setFlashMode(String flashMode)
    {
        synchronized (mStartMutex) {

            int flashModeValue = mVideoDeviceManager.stringToControlAEMode(flashMode);
            if (flashModeValue < 0) {
                Log.w("QtCamera2", "Unknown flash mode");
                return;
            }
            mFlashMode = flashModeValue;

            if (!mIsStarted)
                return;

            mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, mFlashMode);
            mPreviewRequest = mPreviewRequestBuilder.build();

            try {
                mCaptureSession.setRepeatingRequest(mPreviewRequest, mCaptureCallback, mBackgroundHandler);
            } catch (Exception exception) {
                Log.w("QtCamera2", "Failed to set flash mode:" + exception);
            }
        }
    }

    private int getTorchModeValue(boolean mode)
    {
        return mode ? CameraMetadata.FLASH_MODE_TORCH : CameraMetadata.FLASH_MODE_OFF;
    }

    public void setTorchMode(boolean torchMode)
    {
        synchronized (mStartMutex) {
            mTorchMode = getTorchModeValue(torchMode);

            if (mIsStarted) {
                mPreviewRequestBuilder.set(CaptureRequest.FLASH_MODE, mTorchMode);
                mPreviewRequest = mPreviewRequestBuilder.build();

                try {
                    mCaptureSession.setRepeatingRequest(mPreviewRequest, mCaptureCallback, mBackgroundHandler);
                } catch (Exception exception) {
                    Log.w("QtCamera2", "Failed to set flash mode:" + exception);
                }
            }
        }
    }
}
