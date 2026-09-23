// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlibcamera_controls_p.h"

#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/geometry.h>

QT_BEGIN_NAMESPACE

namespace QLibCamera {

using namespace libcamera;

namespace QLibCameraControls {

QCamera::Features detectSupportedFeatures(const ControlInfoMap &controls)
{
    QCamera::Features features;

    if (controls.find(&libcamera::controls::ColourTemperature) != controls.end())
        features |= QCamera::Feature::ColorTemperature;
    if (controls.find(&libcamera::controls::ExposureValue) != controls.end())
        features |= QCamera::Feature::ExposureCompensation;
    if (controls.find(&libcamera::controls::AnalogueGain) != controls.end())
        features |= QCamera::Feature::IsoSensitivity;
    if (controls.find(&libcamera::controls::ExposureTime) != controls.end())
        features |= QCamera::Feature::ManualExposureTime;
    if (controls.find(&libcamera::controls::LensPosition) != controls.end())
        features |= QCamera::Feature::FocusDistance;

    return features;
}

void setAeEnable(ControlList &pending, bool enable)
{
    pending.set(libcamera::controls::AeEnable, enable);
}

void setManualExposureTime(ControlList &pending, float seconds)
{
    // libcamera::controls::ExposureTime is in microseconds.
    pending.set(libcamera::controls::AeEnable, false);
    pending.set(libcamera::controls::ExposureTime, int32_t(seconds * 1'000'000.f));
}

void setExposureCompensation(ControlList &pending, float stops)
{
    pending.set(libcamera::controls::ExposureValue, stops);
}

void setAnalogueGainForIso(ControlList &pending, int iso)
{
    // ISO 100 is conventionally analogue gain 1.0.
    pending.set(libcamera::controls::AeEnable, false);
    pending.set(libcamera::controls::AnalogueGain, float(iso) / 100.f);
}

void setAwbEnable(ControlList &pending, bool enable)
{
    pending.set(libcamera::controls::AwbEnable, enable);
}

void setColorTemperature(ControlList &pending, int kelvin)
{
    pending.set(libcamera::controls::AwbEnable, false);
    pending.set(libcamera::controls::ColourTemperature, kelvin);
}

void setZoomFactor(ControlList &pending, const ControlInfoMap &controls, float zoomFactor)
{
    const auto it = controls.find(&libcamera::controls::ScalerCrop);
    if (it == controls.end())
        return;

    const Rectangle maxCrop = it->second.max().get<Rectangle>();
    if (maxCrop.isNull() || zoomFactor <= 1.f)
        return;

    const unsigned int width = std::max(1u, static_cast<unsigned int>(maxCrop.width / zoomFactor));
    const unsigned int height =
            std::max(1u, static_cast<unsigned int>(maxCrop.height / zoomFactor));
    const int x = maxCrop.x + int(maxCrop.width - width) / 2;
    const int y = maxCrop.y + int(maxCrop.height - height) / 2;

    pending.set(libcamera::controls::ScalerCrop, Rectangle(x, y, width, height));
}

} // namespace QLibCameraControls

} // namespace QLibCamera

QT_END_NAMESPACE
