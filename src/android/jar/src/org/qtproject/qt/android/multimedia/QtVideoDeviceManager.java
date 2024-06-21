// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.MediaCodecList;
import android.media.MediaCodecInfo;
import android.os.Build;
import android.util.Range;
import android.util.Size;
import android.util.Log;

import java.lang.String;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
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

    static private boolean isSoftwareCodec(String longCodecName) {
        longCodecName = longCodecName.toLowerCase();
        return longCodecName.startsWith("omx.google.") || longCodecName.startsWith("c2.android.")
               || !(longCodecName.startsWith("omx.") || longCodecName.startsWith("c2."));
    }

    private enum CODEC {
      DECODER,
      ENCODER
    }
    static private String[] getHWVideoCodecs(CODEC expectedType) {
        MediaCodecList mediaCodecList = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
        MediaCodecInfo[] mediaCodecInfo = mediaCodecList.getCodecInfos();
        Set<String> codecs = new HashSet<String>();

        for (MediaCodecInfo codecInfo : mediaCodecInfo) {
            CODEC currentType = codecInfo.isEncoder() ? CODEC.ENCODER : CODEC.DECODER;
            if (currentType == expectedType && !isSoftwareCodec(codecInfo.getName())) {
                String[] supportedTypes = codecInfo.getSupportedTypes();
                for (String type : supportedTypes) {
                    if (type.startsWith("video/"))
                        codecs.add(type.substring(6));
                }
            }
        }
        return codecs.toArray(new String[codecs.size()]);
    }

    static public String[] getHWVideoDecoders() { return getHWVideoCodecs(CODEC.DECODER); }
    static public String[] getHWVideoEncoders() { return getHWVideoCodecs(CODEC.ENCODER); }

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

    public float[] getZoomRange(String cameraId) {

        float[] zoomRange = { 1.0f, 1.0f };
        final CameraCharacteristics characteristics = getCameraCharacteristics(cameraId);
        if (characteristics == null)
            return zoomRange;

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R) {
            final Range<Float> range = characteristics.get(CameraCharacteristics.CONTROL_ZOOM_RATIO_RANGE);
            if (range != null) {
                zoomRange[0] = range.getLower();
                zoomRange[1] = range.getUpper();
            }
        }

        if (zoomRange[1] == 1.0f)
            zoomRange[1] = characteristics.get(CameraCharacteristics.SCALER_AVAILABLE_MAX_DIGITAL_ZOOM);

        return zoomRange;
    }

    public Rect getActiveArraySize(String cameraId) {
        Rect activeArraySize = new Rect();
        final CameraCharacteristics characteristics = getCameraCharacteristics(cameraId);
        if (characteristics != null)
            activeArraySize = characteristics.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);
        return activeArraySize;
    }

    static final int maxResolution = 3840*2160; // 4k resolution
    public String[] getStreamConfigurationsSizes(String cameraId, int imageFormat) {

        CameraCharacteristics characteristics = getCameraCharacteristics(cameraId);
        if (characteristics == null)
            return new String[0];

        StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
        Size[] sizes = map.getOutputSizes(imageFormat);
        if (sizes == null)
            return new String[0];

        ArrayList<String> stream = new ArrayList<>();

        for (int index = 0; index < sizes.length; index++) {
            if (sizes[index].getWidth() * sizes[index].getHeight() <= maxResolution)
                stream.add(sizes[index].toString());
        }

        return stream.toArray(new String[0]);
    }

    public int stringToControlAEMode(String mode) {
        switch (mode) {
            case "off":
                return CaptureRequest.CONTROL_AE_MODE_ON;
            case "auto":
                return CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH;
            case "on":
                return CaptureRequest.CONTROL_AE_MODE_ON_ALWAYS_FLASH;
            case "redeye":
                return CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE;
            case "external":
                return CaptureRequest.CONTROL_AE_MODE_ON_EXTERNAL_FLASH;
            default:
                return -1;
        }
    }

    public String controlAEModeToString(int mode) {
        switch (mode) {
            case CaptureRequest.CONTROL_AE_MODE_ON:
                return "off";
            case CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH:
                return "auto";
            case CaptureRequest.CONTROL_AE_MODE_ON_ALWAYS_FLASH:
                return "on";
            case CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE:
                return "redeye";
            case CaptureRequest.CONTROL_AE_MODE_ON_EXTERNAL_FLASH:
                return "external";
            case CaptureRequest.CONTROL_AE_MODE_OFF:
            default:
                return "unknown";
        }
    }

    public int[] getSupportedAfModes(String cameraId) {

        CameraCharacteristics characteristics = getCameraCharacteristics(cameraId);
        if (characteristics == null)
            return new int[0];

        return characteristics.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES);
    }

    public String[] getSupportedFlashModes(String cameraId) {

        CameraCharacteristics characteristics = getCameraCharacteristics(cameraId);
        if (characteristics == null)
            return new String[0];

        int supportedFlashModes[] = characteristics.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_MODES);
        ArrayList<String> supportedFlashModesList = new ArrayList<String>();
        for (int index = 0; index < supportedFlashModes.length; index++) {
            supportedFlashModesList.add(controlAEModeToString(supportedFlashModes[index]));
        }

        String[] ret = new String[ supportedFlashModesList.size() ];
        return supportedFlashModesList.toArray(ret);
    }

    static public boolean isEmulator()
    {
        return ((Build.BRAND.startsWith("generic") && Build.DEVICE.startsWith("generic"))
            || Build.FINGERPRINT.startsWith("generic")
            || Build.FINGERPRINT.startsWith("unknown")
            || Build.HARDWARE.contains("goldfish")
            || Build.HARDWARE.contains("ranchu")
            || Build.MODEL.contains("google_sdk")
            || Build.MODEL.contains("Emulator")
            || Build.MODEL.contains("Android SDK built for x86")
            || Build.MANUFACTURER.contains("Genymotion")
            || Build.PRODUCT.contains("sdk")
            || Build.PRODUCT.contains("vbox86p")
            || Build.PRODUCT.contains("emulator")
            || Build.PRODUCT.contains("simulator"));
    }

    public boolean isTorchModeSupported(String cameraId) {
        boolean ret = false;
        final CameraCharacteristics characteristics = getCameraCharacteristics(cameraId);
        if (characteristics != null)
            ret = characteristics.get(CameraCharacteristics.FLASH_INFO_AVAILABLE);
        return ret;
    }
}
