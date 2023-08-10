// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia;

import android.hardware.camera2.CaptureResult;
import android.media.ExifInterface;
import android.os.Build;
import android.util.Log;

import java.io.IOException;

public class QtExifDataHandler {

    private int mFlashFired = 0;
    private Long mExposureTime = 0L;
    private float mFocalLength = 0;
    private static String mModel = Build.MANUFACTURER + " " + Build.MODEL;

    public QtExifDataHandler(CaptureResult r)
    {
        if (r.get(CaptureResult.FLASH_STATE) == CaptureResult.FLASH_STATE_FIRED)
            mFlashFired = 1;

        mExposureTime = r.get(CaptureResult.SENSOR_EXPOSURE_TIME)/1000000000;
        mFocalLength = r.get(CaptureResult.LENS_FOCAL_LENGTH);
    }

    public void save(String path)
    {
        ExifInterface exif;
        try {
            exif = new ExifInterface(path);
        } catch ( IOException e ) {
            Log.e("QtExifDataHandler", "Cannot open file: " + path + "\n" + e);
            return;
        }
        exif.setAttribute(ExifInterface.TAG_FLASH, String.valueOf(mFlashFired));

        if (mExposureTime != null)
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
