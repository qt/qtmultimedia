// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qavfcamerarotationtracker_p.h>

#include <QtCore/qassert.h>

#include <AVFoundation/AVFoundation.h>

#include <cmath>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

AvfCameraRotationTracker::AvfCameraRotationTracker(AVCaptureDevice *avCaptureDevice)
{
    Q_ASSERT(avCaptureDevice);

    m_avCaptureDevice = AVFScopedPointer{ [avCaptureDevice retain] };

    // Use RotationCoordinator if we can.
    if (@available(macOS 14.0, iOS 17.0, *)) {
        m_avRotationCoordinator = AVFScopedPointer{ [[AVCaptureDeviceRotationCoordinator alloc]
            initWithDevice:m_avCaptureDevice
              previewLayer:nil] };
    }
}

AvfCameraRotationTracker::AvfCameraRotationTracker(AvfCameraRotationTracker &&other) noexcept
    : m_avCaptureDevice{ std::exchange(other.m_avCaptureDevice, {}) }
{
    if (@available(macOS 14.0, *)) {
        m_avRotationCoordinator = std::exchange(other.m_avRotationCoordinator, {});
    }
}

AvfCameraRotationTracker::~AvfCameraRotationTracker()
{
    clear();
}

void AvfCameraRotationTracker::swap(AvfCameraRotationTracker &other)
{
    std::swap(m_avCaptureDevice, other.m_avCaptureDevice);
    if (@available(macOS 14.0, iOS 17.0, *)) {
        std::swap(m_avRotationCoordinator, other.m_avRotationCoordinator);
    }
}

void AvfCameraRotationTracker::clear()
{
    if (m_avCaptureDevice)
        m_avCaptureDevice.reset();

    if (@available(macOS 14.0, *)) {
        if (m_avRotationCoordinator) {
            m_avRotationCoordinator.reset();
        }
    }
}

int AvfCameraRotationTracker::rotationDegrees() const
{
    if (m_avCaptureDevice == nullptr)
        return 0;

    if (@available(macOS 14.0, *)) {
        // This code assumes that AVCaptureDeviceRotationCoordinator
        // .videoRotationAngleForHorizonLevelCapture returns degrees that are divisible by 90.
        // This has been the case during testing.
        //
        // TODO: Some rotations are not valid for preview on some devices (such as
        // iPhones not being allowed to have an upside-down window). This usage of the
        // rotation coordinator will still return it as a valid preview rotation, and
        // might cause bugs on iPhone previews.
        if (m_avRotationCoordinator != nullptr)
            return std::lround(
                m_avRotationCoordinator.data().videoRotationAngleForHorizonLevelCapture);
    }

    return 0;
}

} // namespace QFFmpeg end

QT_END_NAMESPACE
