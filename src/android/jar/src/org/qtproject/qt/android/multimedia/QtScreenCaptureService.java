// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.graphics.PixelFormat;
import android.media.Image;
import android.media.ImageReader;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.Binder;
import android.os.IBinder;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.DisplayMetrics;
import android.util.Log;


public class QtScreenCaptureService extends Service {
    // Lock for synchronization
    private final Object mServiceStopLock = new Object();
    private boolean mServiceStopped = false;
    private static final String QtTAG = "QtScreenCaptureService";
    private static final String CHANNEL_ID = "ScreenCaptureChannel";
    private static final String VIRTUAL_DISPLAY_NAME = "ScreenCapture";
    private VirtualDisplay mVirtualDisplay = null;
    private ImageReader mImageReader = null;
    private static int MAX_FRAMES_NUMBER = 12;
    private MediaProjection mMediaProjection = null;
    private Handler mBackgroundHandler = null;
    private HandlerThread mBackgroundThread = null;
    private long mId;
    private int mScreenWidth;
    private int mScreenHeight;

    static native void onScreenFrameAvailable(Image frame, long id);
    static native void onErrorUpdate(String errorString, long id);

    @Override
    public void onCreate() {
        super.onCreate();

        NotificationChannel channel = new NotificationChannel(CHANNEL_ID, "Screen Capture",
                                                            NotificationManager.IMPORTANCE_LOW);
        NotificationManager manager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        if (manager != null)
            manager.createNotificationChannel(channel);

        Notification notification = new Notification.Builder(this, CHANNEL_ID)
                .setSmallIcon(android.R.drawable.ic_notification_overlay)
                .build();

        // Start the service in the foreground with the created notification
        startForeground(1, notification);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        synchronized (mServiceStopLock) {
            if (mServiceStopped)
                return START_STICKY;
            return onStartCommandInternal(intent, flags, startId);
        }
    }

    private int onStartCommandInternal(Intent intent, int flags, int startId) {
        if (mServiceStopped)
            return START_STICKY;

        if (intent == null)
            return START_STICKY;

        int resultCode = intent.getIntExtra(QtScreenGrabber.RESULT_CODE, Activity.RESULT_CANCELED);
        if (resultCode != Activity.RESULT_OK)
            return START_STICKY;

        Intent data = Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU ?
                    intent.getParcelableExtra(QtScreenGrabber.DATA) :
                    intent.getParcelableExtra(QtScreenGrabber.DATA, Intent.class);
        mId = intent.getLongExtra(QtScreenGrabber.ID, -1);
        if (data == null || mId == -1) {
            onErrorUpdate("Cannot parse Intent. Screen capture not started", mId);
            return START_STICKY;
        }

        mScreenWidth = intent.getIntExtra(QtScreenGrabber.WIDTH, 0);
        mScreenHeight = intent.getIntExtra(QtScreenGrabber.HEIGHT, 0);
        if (mScreenWidth <= 0 || mScreenHeight <= 0) {
            onErrorUpdate("Wrong Screen size. Screen capture not started", mId);
            return START_STICKY;
        }

        try {
            MediaProjectionManager mgr = (MediaProjectionManager) getSystemService(MEDIA_PROJECTION_SERVICE);
            mMediaProjection = mgr.getMediaProjection(resultCode, data);
            mMediaProjection.registerCallback(new MediaProjection.Callback() {
                @Override
                public void onStop() {
                    stopScreenCapture();
                    super.onStop();
                }
            }, null);
            startScreenCapture();
        } catch (IllegalStateException | SecurityException e) {
            onErrorUpdate("Cannot start MediaProjection: " + e.toString(), mId);
        }

        return START_STICKY;
    }

    ImageReader.OnImageAvailableListener mScreenFrameListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            try {
                Image image = reader.acquireLatestImage();
                if (image != null)
                    onScreenFrameAvailable(image, mId);
                else
                    Log.w(QtTAG, "Null frame acquired. Skip it");
            } catch (Exception e) {
                Log.w(QtTAG, "The frame cannot be acquired: " + e);
            }
        }
    };

    private void startScreenCapture()
    {
        if (mMediaProjection == null)
            return;

        mBackgroundThread = new HandlerThread("ScreenCaptureThread");
        mBackgroundThread.start();
        mBackgroundHandler = new Handler(mBackgroundThread.getLooper());
        mImageReader = ImageReader.newInstance(mScreenWidth, mScreenHeight, PixelFormat.RGBA_8888, MAX_FRAMES_NUMBER);

        mVirtualDisplay = mMediaProjection.createVirtualDisplay(VIRTUAL_DISPLAY_NAME,
                            mScreenWidth, mScreenHeight, DisplayMetrics.DENSITY_MEDIUM,
                            DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC, mImageReader.getSurface(),
                            null, mBackgroundHandler);
        mImageReader.setOnImageAvailableListener(mScreenFrameListener, mBackgroundHandler);
    }

    void stopScreenCapture()
    {
        synchronized (mServiceStopLock) {
            if (mServiceStopped)
               return;
            mServiceStopped = true;

            if (mImageReader != null)
                mImageReader.setOnImageAvailableListener(null, mBackgroundHandler);

            if (mVirtualDisplay != null) {
                mVirtualDisplay.release();
                mVirtualDisplay = null;
            }
            if (mBackgroundHandler != null) {
                mBackgroundHandler.getLooper().quitSafely();
                mBackgroundHandler = null;
            }

            if (mBackgroundThread != null) {
                mBackgroundThread.quitSafely();
                try {
                    mBackgroundThread.join();
                } catch (InterruptedException e) {
                    Log.w(QtTAG, "The thread is interrupted. Cannot join: " + e);
                }
                mBackgroundThread = null;
            }
        }
    }

    private final IBinder binder = new ScreenCaptureBinder();

    class ScreenCaptureBinder extends Binder {
        QtScreenCaptureService getService() {
            return QtScreenCaptureService.this;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        stopScreenCapture();
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU)
            stopForeground(true);
        else
            stopForeground(STOP_FOREGROUND_REMOVE);

        if (mMediaProjection != null) {
            mMediaProjection.stop();
            mMediaProjection = null;
        }
    }
}
