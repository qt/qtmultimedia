// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.TotalCaptureResult;
import android.media.Image;
import android.media.ImageReader;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import java.util.List;

@TargetApi(23)
public class QtCamera2 {

    CameraDevice mCameraDevice = null;
    HandlerThread mBackgroundThread;
    Handler mBackgroundHandler;
    ImageReader mImageReader;
    CameraManager mCameraManager;
    CameraCaptureSession mCaptureSession;
    CaptureRequest.Builder mPreviewRequestBuilder;
    CaptureRequest mPreviewRequest;
    String mCameraId;

    native void onFrameAvailable(String cameraId, Image image);

    ImageReader.OnImageAvailableListener mOnImageAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            QtCamera2.this.onFrameAvailable(mCameraId, reader.acquireLatestImage());
        }
    };

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
    };

    native void onCaptureSessionStarted(String cameraId, long timestamp, long frameNumber);
    native void onCaptureSessionCompleted(String cameraId, long frameNumber);
    native void onCaptureSessionFailed(String cameraId, int reason, long frameNumber);
    CameraCaptureSession.CaptureCallback mCaptureCallback = new CameraCaptureSession.CaptureCallback() {
        @Override
        public void onCaptureStarted(CameraCaptureSession session, CaptureRequest request, long timestamp, long frameNumber) {
            super.onCaptureStarted(session, request, timestamp, frameNumber);
            onCaptureSessionStarted(mCameraId,timestamp,frameNumber);
        }
        public void onCaptureCompleted(CameraCaptureSession session, CaptureRequest request,  TotalCaptureResult result) {
            super.onCaptureCompleted(session,request,result);
            onCaptureSessionCompleted(mCameraId,result.getFrameNumber());
        }

        public void onCaptureFailed(CameraCaptureSession session,  CaptureRequest request,  CaptureFailure failure) {
            super.onCaptureFailed(session, request, failure);
            onCaptureSessionFailed(mCameraId, failure.getReason(), failure.getFrameNumber());
        }
    };

    public QtCamera2(Context context) {
        mCameraManager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
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

    public boolean open(String cameraId, int width, int height) {

        try {
            mImageReader = ImageReader.newInstance(width, height, ImageFormat.JPEG, /*maxImages*/10);
            mImageReader.setOnImageAvailableListener(mOnImageAvailableListener, mBackgroundHandler);

            mCameraId = cameraId;
            mCameraManager.openCamera(cameraId,mStateCallback,mBackgroundHandler);
            return true;
        } catch (Exception e){
            Log.w("QtCamera2", "Failed to open camera:" + e);
        }

        return false;
    }

    public boolean createSession() {
        if (mCameraDevice == null)
            return false;

        try {
            mCameraDevice.createCaptureSession(List.of(mImageReader.getSurface()), mCaptureStateCallback, mBackgroundHandler);
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

        try {
            mPreviewRequestBuilder = mCameraDevice.createCaptureRequest(template);
            mPreviewRequestBuilder.addTarget(mImageReader.getSurface());
            mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
            mPreviewRequest = mPreviewRequestBuilder.build();
            mCaptureSession.setRepeatingRequest(mPreviewRequest, mCaptureCallback, mBackgroundHandler);
            return true;

        } catch (Exception exception) {
            Log.w("QtCamera2", "Failed to start preview:" + exception);
        }
        return false;
    }

    public void stopAndClose() {
        try {
            if (null != mCaptureSession) {
                mCaptureSession.close();
                mCaptureSession = null;
            }
            if (null != mCameraDevice) {
                mCameraDevice.close();
                mCameraDevice = null;
            }
            if (null != mImageReader) {
                mImageReader.close();
                mImageReader = null;
            }
            mCameraId = "";
        } catch (Exception exception) {
            Log.w("QtCamera2", "Failed to stop and close:" + exception);
        }
    }
}
