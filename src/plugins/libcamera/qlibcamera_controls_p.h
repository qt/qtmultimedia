// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QLIBCAMERA_CONTROLS_P_H
#define QLIBCAMERA_CONTROLS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qcamera.h>

namespace libcamera {
class ControlInfoMap;
class ControlList;
} // namespace libcamera

QT_BEGIN_NAMESPACE

namespace QLibCamera {

namespace QLibCameraControls {

QCamera::Features detectSupportedFeatures(const libcamera::ControlInfoMap &controls);

void setAeEnable(libcamera::ControlList &pending, bool enable);
void setManualExposureTime(libcamera::ControlList &pending, float seconds);
void setExposureCompensation(libcamera::ControlList &pending, float stops);
void setAnalogueGainForIso(libcamera::ControlList &pending, int iso);
void setAwbEnable(libcamera::ControlList &pending, bool enable);
void setColorTemperature(libcamera::ControlList &pending, int kelvin);
// crop is in the [0, 1] normalized zoom-window sense: a full-frame crop (zoomFactor 1) has
// zoomFactor == 1; higher values crop a smaller, centered window of the sensor's pixel array.
void setZoomFactor(libcamera::ControlList &pending, const libcamera::ControlInfoMap &controls,
                   float zoomFactor);

} // namespace QLibCameraControls

} // namespace QLibCamera

QT_END_NAMESPACE

#endif // QLIBCAMERA_CONTROLS_P_H
