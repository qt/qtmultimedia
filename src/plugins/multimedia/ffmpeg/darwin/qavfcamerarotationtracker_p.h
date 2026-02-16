// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFCAMERAROTATIONTRACKER_P_H
#define QAVFCAMERAROTATIONTRACKER_P_H

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

#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qdarwinhelpers.h>

#include <QtMultimedia/private/qavfcamerautility_p.h>

#include <os/availability.h>

#include <AVFoundation/AVCaptureDevice.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

// Gives us rotational information for an AVCaptureDevice
class AvfCameraRotationTracker
{
public:
    AvfCameraRotationTracker() = default;
    explicit AvfCameraRotationTracker(AVCaptureDevice* avCaptureDevice);
    AvfCameraRotationTracker(AvfCameraRotationTracker &&) noexcept;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(AvfCameraRotationTracker)
    ~AvfCameraRotationTracker();

    void swap(AvfCameraRotationTracker &other);

    // Guaranteed to return rotation in clockwise 90 degree increments.
    [[nodiscard]] int rotationDegrees() const;

    [[nodiscard]] AVCaptureDevice *avCaptureDevice() const { return m_avCaptureDevice.data(); }

private:
    void clear();

    AVFScopedPointer<AVCaptureDevice> m_avCaptureDevice;

    // If running iOS or macOS 14+, we use AVCaptureDeviceRotationCoordinator
    // to get the camera rotation directly from the camera-device.
    API_AVAILABLE(macos(14.0))
    AVFScopedPointer<AVCaptureDeviceRotationCoordinator> m_avRotationCoordinator;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QAVFCAMERAROTATIONTRACKER_P_H

