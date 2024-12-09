// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;
import android.util.Size;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.WindowManager;
import android.view.WindowMetrics;


class QtScreenGrabber {
    // Lock for synchronization
    private final Object mServiceLock = new Object();
    private QtScreenCaptureService mService = null;
    private final Context m_activity;
    static final String DATA = "data";
    static final String HEIGHT = "height";
    static final String ID = "id";
    static final String RESULT_CODE = "resultCode";
    static final String WIDTH = "width";
    private static final String QtTAG = "QtScreenGrabber";

    private ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            synchronized (mServiceLock) {
                QtScreenCaptureService.ScreenCaptureBinder binder =
                    (QtScreenCaptureService.ScreenCaptureBinder) service;
                mService = binder.getService();
                mServiceLock.notify();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            synchronized (mServiceLock) {
                mService = null;
            }
        }
    };

    QtScreenGrabber(Activity activity, int requestCode) {
        m_activity = activity;
        MediaProjectionManager mgr = (MediaProjectionManager) activity.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
        activity.startActivityForResult(mgr.createScreenCaptureIntent(), requestCode);
    }

    public static Size getScreenCaptureSize(Activity activity) {
        WindowManager windowManager = (WindowManager) activity.getSystemService(Context.WINDOW_SERVICE);
        if (windowManager == null) {
            Log.w(QtTAG, "WindowManager is null. Invalid screen size");
            return new Size(0, 0);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            WindowMetrics metrics = windowManager.getCurrentWindowMetrics();
            return new Size(metrics.getBounds().width(), metrics.getBounds().height());
        } else {
            Display display = windowManager.getDefaultDisplay();
            DisplayMetrics metrics = new DisplayMetrics();
            display.getRealMetrics(metrics);
            return new Size(metrics.widthPixels, metrics.heightPixels);
        }
    }

    public boolean startScreenCaptureService(int resultCode, long id, int width, int height, Intent data) {
        try {
            Intent serviceIntent = new Intent(m_activity, QtScreenCaptureService.class);
            serviceIntent.putExtra(RESULT_CODE, resultCode);
            serviceIntent.putExtra(DATA, data);
            serviceIntent.putExtra(ID, id);
            serviceIntent.putExtra(WIDTH, width);
            serviceIntent.putExtra(HEIGHT, height);
            m_activity.bindService(serviceIntent, mConnection, Context.BIND_AUTO_CREATE);
            m_activity.startService(serviceIntent);
        } catch (Exception e) {
            Log.w(QtTAG, "Cannot start QtScreenCaptureService: " + e);
            return false;
        }

        return true;
    }

    public boolean stopScreenCaptureService() {
        try {
            Intent serviceIntent = new Intent(m_activity, QtScreenCaptureService.class);
            synchronized (mServiceLock) {
                if (mService == null)
                    // Service wasn't started at all or was not bound yet.
                    // Lets wait to make sure that we will call stopScreenCapture if it is needed
                    mServiceLock.wait(1000);

                if (mService != null)
                    mService.stopScreenCapture();
                m_activity.unbindService(mConnection);
            }
            m_activity.stopService(serviceIntent);
        } catch (Exception e) {
            Log.w(QtTAG, "Cannot stop QtScreenCaptureService: " + e);
            return false;
        }

        return true;
    }
}
