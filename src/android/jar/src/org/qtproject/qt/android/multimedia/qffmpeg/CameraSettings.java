// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia.qffmpeg;

import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureRequest;
import android.util.Range;

// Represents the QCamera settings we should use for
// a stream or still-photo.
class CameraSettings {
    private static final int DEFAULT_FLASH_MODE = CaptureRequest.CONTROL_AE_MODE_ON;
    private static final int DEFAULT_TORCH_MODE = CameraMetadata.FLASH_MODE_OFF;
    // Default value in QPlatformCamera is FocusModeAuto, which maps to CONTINUOUS_PICTURE.
    private static final int DEFAULT_AF_MODE = CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE;
    private static final float DEFAULT_FOCUS_DISTANCE = 1.f;
    private static final float DEFAULT_ZOOM_FACTOR = 1.0f;

    // Not to be confused with QCamera::FlashMode.
    // This controls the currently desired CaptureRequest.CONTROL_AE_MODE,
    // but only for still photos.
    //
    // QCamera::FlashMode::FlashOff maps to CaptureRequest.CONTROL_AE_MODE_ON. This implies
    // regular automatic exposure.
    // QCamera::FlashMode::FlashAuto maps to CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH.
    // QCamera::FlashMode::FlashOn should ideally map to
    // CaptureRequest.CONTROL_AE_MODE_ON_ALWAYS_FLASH, but testing has shown that this is
    // unreliable. Instead we force flash on using CaptureRequest.FLASH_MODE_ON and
    // trigger auto-exposure using CaptureRequest.CONTROL_AE_MODE_ON.
    public int mStillPhotoFlashMode = DEFAULT_FLASH_MODE;

    // Not to be confused with QCamera::TorchMode.
    // This controls the currently desired CaptureRequest.FLASH_MODE
    // QCamera::TorchMode::TorchOff maps to CaptureRequest.FLASH_MODE_OFF
    // QCamera::TorchMode::TorchAuto is not supported.
    // QCamera::TorhcMode::TorchOn maps to CaptureRequest.FLASH_MODE_TORCH.
    public int mTorchMode = DEFAULT_TORCH_MODE;

    // Not to be confused with QCamera::FocusMode
    // This controls the currently desired CaptureRequest.CONTROL_AF_MODE
    // QCamera::FocusMode::FocusModeAuto maps to CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE
    //
    // This variable only controls the AF_MODE we desire to apply. If the device
    // does not support this AF_MODE this will not reflect what the camera is currently doing.
    public int mAFMode = DEFAULT_AF_MODE;

    // Not to be confused with CaptureRequest.LENS_FOCUS_DISTANCE. This variable stores the
    // current QCamera::focusDistance. Must be applied whenever the focus-mode is set to
    // Manual.
    // Must have the same default value as the one set in QPlatformCamera.
    public float mFocusDistance = DEFAULT_FOCUS_DISTANCE;

    // Not to be confused with CaptureRequest.CONTROL_ZOOM_RATIO
    // This matches the current QCamera::zoomFactor of the C++ QCamera object.
    public float mZoomFactor = DEFAULT_ZOOM_FACTOR;

    Range<Integer> mFpsRange = null;

    public CameraSettings() { }

    public CameraSettings(CameraSettings other) {
        this.mStillPhotoFlashMode = other.mStillPhotoFlashMode;
        this.mTorchMode = other.mTorchMode;
        this.mAFMode = other.mAFMode;
        this.mFocusDistance = other.mFocusDistance;
        this.mZoomFactor = other.mZoomFactor;
        if (other.mFpsRange != null) {
            this.mFpsRange = new Range<Integer>(other.mFpsRange.getLower(), other.mFpsRange.getUpper());
        }
    }
}
