// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia;

import org.qtproject.qt.android.multimedia.QtVideoDeviceManager;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.Rect;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureFailure;
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
import java.util.ArrayList;
import java.util.List;

@TargetApi(23)
public class QtCamera2 {

    CameraDevice mCameraDevice = null;
    QtVideoDeviceManager mVideoDeviceManager = null;
    HandlerThread mBackgroundThread;
    Handler mBackgroundHandler;
    ImageReader mImageReader = null;
    CameraManager mCameraManager;
    CameraCaptureSession mCaptureSession;
    CaptureRequest.Builder mPreviewRequestBuilder;
    CaptureRequest mPreviewRequest;
    String mCameraId;
    List<Surface> mTargetSurfaces = new ArrayList<>();

    private Object mStartMutex = new Object();
    private boolean mIsStarted = false;
    private static int MaxNumberFrames = 10;
    private int mTorchMode = CameraMetadata.FLASH_MODE_OFF;

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


    native void onFrameAvailable(String cameraId, Image frame);

    ImageReader.OnImageAvailableListener mOnImageAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            QtCamera2.this.onFrameAvailable(mCameraId, reader.acquireLatestImage());
        }
    };

    public boolean addImageReader(int width, int height, int format) {

        if (mImageReader != null)
            removeSurface(mImageReader.getSurface());

        mImageReader = ImageReader.newInstance(width, height, format, MaxNumberFrames);
        mImageReader.setOnImageAvailableListener(mOnImageAvailableListener, mBackgroundHandler);
        addSurface(mImageReader.getSurface());

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
                for (Surface surface : mTargetSurfaces) {
                    mPreviewRequestBuilder.addTarget(surface);
                }

                mPreviewRequestBuilder.set(CaptureRequest.FLASH_MODE, mTorchMode);
                mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
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
