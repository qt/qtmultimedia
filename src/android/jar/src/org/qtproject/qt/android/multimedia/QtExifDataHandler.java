// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia;

import android.hardware.camera2.CaptureResult;
import android.media.ExifInterface;
import android.os.Build;
import android.util.Log;

import java.io.IOException;

class QtExifDataHandler {

    private int mFlashFired = 0;
    private long mExposureTime = 0L;
    private float mFocalLength = 0;
    private static String mModel = Build.MANUFACTURER + " " + Build.MODEL;

    QtExifDataHandler(CaptureResult r)
    {
        Integer flash = r.get(CaptureResult.FLASH_STATE);
        if (flash != null && flash == CaptureResult.FLASH_STATE_FIRED)
            mFlashFired = 1;

        Long exposureTime = r.get(CaptureResult.SENSOR_EXPOSURE_TIME);
        if (exposureTime != null)
            mExposureTime = exposureTime/1000000000;
        mFocalLength = r.get(CaptureResult.LENS_FOCAL_LENGTH);
    }

    void save(String path)
    {
        ExifInterface exif;
        try {
            exif = new ExifInterface(path);
        } catch ( IOException e ) {
            Log.e("QtExifDataHandler", "Cannot open file: " + path + "\n" + e);
            return;
        }
        exif.setAttribute(ExifInterface.TAG_FLASH, String.valueOf(mFlashFired));
        exif.setAttribute(ExifInterface.TAG_EXPOSURE_TIME, String.valueOf(mExposureTime));
        exif.setAttribute(ExifInterface.TAG_FOCAL_LENGTH, String.valueOf(mFocalLength));
        exif.setAttribute(ExifInterface.TAG_MODEL, mModel);

        try {
            exif.saveAttributes();
        } catch ( IOException e ) {
            Log.e("QtExifDataHandler", "Cannot save file: " + path + "\n" + e);
        }
    }
}
