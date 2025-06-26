// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import android.content.Context;
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

import java.lang.String;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;

import org.qtproject.qt.android.UsedFromNativeCode;

class QtVideoDeviceManager {

    CameraManager mCameraManager;
    Map<String, CameraCharacteristics> cache;

    @UsedFromNativeCode
    QtVideoDeviceManager(Context context) {
        mCameraManager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
        cache = new WeakHashMap<String, CameraCharacteristics>();
    }

    CameraCharacteristics getCameraCharacteristics(String cameraId) {

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

    static String[] getHWVideoDecoders() { return getHWVideoCodecs(CODEC.DECODER); }
    static String[] getHWVideoEncoders() { return getHWVideoCodecs(CODEC.ENCODER); }

    @UsedFromNativeCode
    String[] getCameraIdList() {
        try {
            return mCameraManager.getCameraIdList();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    @UsedFromNativeCode
    int getSensorOrientation(String cameraId) {
        CameraCharacteristics characteristics =  getCameraCharacteristics(cameraId);
        if (characteristics == null)
            return 0;
        return characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
    }

    // Returns -1 when camera is not available.
    @UsedFromNativeCode
    int getLensFacing(String cameraId) {
        final CameraCharacteristics characteristics = getCameraCharacteristics(cameraId);
        if (characteristics == null)
            return -1;

        // The docs guarantees this is not null:
        // https://developer.android.com/reference/android/hardware/camera2/CameraCharacteristics#LENS_FACING
        return characteristics.get(CameraCharacteristics.LENS_FACING);
    }

    @UsedFromNativeCode
    String[] getFpsRange(String cameraId) {

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

    @UsedFromNativeCode
    float[] getZoomRange(String cameraId) {

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

    Rect getActiveArraySize(String cameraId) {
        Rect activeArraySize = new Rect();
        final CameraCharacteristics characteristics = getCameraCharacteristics(cameraId);
        if (characteristics != null)
            activeArraySize = characteristics.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);
        return activeArraySize;
    }

    static final int maxResolution = 3840*2160; // 4k resolution
    @UsedFromNativeCode
    String[] getStreamConfigurationsSizes(String cameraId, int imageFormat) {

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

    int stringToControlAEMode(String mode) {
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

    String controlAEModeToString(int mode) {
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

    // Returns all available modes exposed by the physical device, regardless
    // of whether we have implemented them.
    //
    // Guaranteed to not return null. Will instead return array of size zero.
    @UsedFromNativeCode
    int[] getAllAvailableAfModes(String cameraId) {
        if (cameraId.isEmpty())
            return new int[0];

        CameraCharacteristics characteristics = getCameraCharacteristics(cameraId);
        if (characteristics == null)
            return new int[0];

        final int[] characteristicsValue = characteristics.get(
            CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES);
        return characteristicsValue != null ? characteristicsValue : new int[0];
    }

    boolean isAfModeAvailable(String cameraId, int afMode) {
        if (cameraId == null || cameraId.isEmpty())
            return false;
        return Arrays
            .stream(getAllAvailableAfModes(cameraId))
            .anyMatch(value -> value == afMode);
    }

    // Returns supported QCamera::FocusModes as strings. I.e FocusModeAuto becomes "FocusModeAuto".
    // This method will return only those focus-modes for which we have an implementation, and
    // is also reported as available by the physical device. This method will never return null.
    // It is guaranteed to return an empty list if no focus modes are found.
    //
    // Note: These returned strings MUST match that of QCamera::FocusMode.
    @UsedFromNativeCode
    String[] getSupportedQCameraFocusModesAsStrings(String cameraId) {
        ArrayList<String> outList = new ArrayList<String>();

        // FocusModeAuto maps to the CONTINUOUS_PICTURE mode.
        if (isAfModeAvailable(cameraId, CameraCharacteristics.CONTROL_AF_MODE_CONTINUOUS_PICTURE)) {
            outList.add("FocusModeAuto");
        }

        if (isAfModeAvailable(cameraId, CameraCharacteristics.CONTROL_AF_MODE_OFF)
            && isManualFocusDistanceSupported(cameraId))
            outList.add("FocusModeManual");

        String[] ret = new String[ outList.size() ];
        return outList.toArray(ret);
    }

    @UsedFromNativeCode
    String[] getSupportedFlashModes(String cameraId) {

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

    // Returns the CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE for the given cameraId.
    // for the given cameraId. If this returns 0, it means the camera is fixed-focus and can't be
    // adjusted and so should not be applied as manual focus distance.
    // Returns -1 if setting focus distance is not supported.
    float getLensInfoMinimumFocusDistance(String cameraId) {
        final CameraCharacteristics characteristics = getCameraCharacteristics(cameraId);
        if (characteristics == null)
            return -1;
        final Float value = characteristics.get(
            CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE);
        if (value == null)
            return -1;
        return value;
    }

    // Returns true if the camera is able to manually set
    // lens focus distance. This is required to support
    // QCamera::FocusModeManual and QCamera::Feature::FocusDistance.
    //
    // Docs require LENS_INFO_MINIMUM_FOCUS_DISTANCE to be higher than 0 in
    // order for manual focus distance to be supported.
    boolean isManualFocusDistanceSupported(String cameraId) {
        return getLensInfoMinimumFocusDistance(cameraId) > 0;
    }

    static boolean isEmulator()
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

    @UsedFromNativeCode
    boolean isTorchModeSupported(String cameraId) {
        boolean ret = false;
        final CameraCharacteristics characteristics = getCameraCharacteristics(cameraId);
        if (characteristics != null)
            ret = characteristics.get(CameraCharacteristics.FLASH_INFO_AVAILABLE);
        return ret;
    }
}
