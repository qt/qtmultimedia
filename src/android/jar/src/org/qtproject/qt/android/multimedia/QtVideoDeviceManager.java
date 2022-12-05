// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.util.Range;
import android.util.Size;
import android.util.Log;

import java.lang.String;
import java.util.ArrayList;
import java.util.Map;
import java.util.WeakHashMap;

public class QtVideoDeviceManager {

    CameraManager mCameraManager;
    Map<String, CameraCharacteristics> cache;

    public QtVideoDeviceManager(Context context) {
        mCameraManager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
        cache = new WeakHashMap<String, CameraCharacteristics>();
    }

    public CameraCharacteristics getCameraCharacteristics(String cameraId) {

        if (cache.containsKey(cameraId))
            return cache.get(cameraId);

        try {
            CameraCharacteristics cameraCharacteristics = mCameraManager.getCameraCharacteristics(cameraId);
            cache.put(cameraId, cameraCharacteristics);
            return cameraCharacteristics;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public String[] getCameraIdList() {
        try {
            return mCameraManager.getCameraIdList();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public int getSensorOrientation(String cameraId) {
        CameraCharacteristics characteristics =  getCameraCharacteristics(cameraId);
        if (characteristics == null)
            return 0;
        return characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
    }

    public int getLensFacing(String cameraId) {
        CameraCharacteristics characteristics =  getCameraCharacteristics(cameraId);
        if (characteristics == null)
            return 0;
        return characteristics.get(CameraCharacteristics.LENS_FACING);
    }

    public String[] getFpsRange(String cameraId) {

        CameraCharacteristics characteristics =  getCameraCharacteristics(cameraId);
        if (characteristics == null)
            return new String[0];

        Range<Integer>[] ranges = characteristics.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);

        String[] fps = new String[ranges.length];

        for (int index = 0; index < ranges.length; index++) {
            fps[index] = ranges[index].toString();
        }

        return fps;
    }

    public String[] getStreamConfigurationsSizes(String cameraId, int imageFormat) {

        CameraCharacteristics characteristics = getCameraCharacteristics(cameraId);
        if (characteristics == null)
            return new String[0];

        StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
        Size[] sizes = map.getOutputSizes(imageFormat);

        String[] stream = new String[sizes.length];

        for (int index = 0; index < sizes.length; index++) {
            stream[index] = sizes[index].toString();
        }

        return stream;
    }
}
