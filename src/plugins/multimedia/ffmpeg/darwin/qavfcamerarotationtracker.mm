// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qavfcamerarotationtracker_p.h>

#include <QtCore/qassert.h>

#include <AVFoundation/AVFoundation.h>

#ifdef Q_OS_IOS
#include <UIKit/UIKit.h>
#endif // Q_OS_IOS

#include <cmath>

QT_BEGIN_NAMESPACE

namespace {

#ifdef Q_OS_IOS
[[nodiscard]] int uiDeviceOrientationToRotationDegrees(UIDeviceOrientation orientation)
{
    switch (orientation) {
    case UIDeviceOrientationLandscapeLeft: return 0;
    case UIDeviceOrientationPortrait: return 90;
    case UIDeviceOrientationLandscapeRight: return 180;
    case UIDeviceOrientationPortraitUpsideDown: return 270;
    default:
        Q_ASSERT(false);
        Q_UNREACHABLE_RETURN(0);
    }
}
#endif

}

namespace QFFmpeg { // namespace QFFmpeg start

AvfCameraRotationTracker::AvfCameraRotationTracker(AVCaptureDevice *avCaptureDevice)
{
    Q_ASSERT(avCaptureDevice != nullptr);

    m_avCaptureDevice = [avCaptureDevice retain];

    // Use RotationCoordinator if we can.
    if (@available(macOS 14.0, iOS 17.0, *)) {
        m_avRotationCoordinator = [[AVCaptureDeviceRotationCoordinator alloc]
            initWithDevice:m_avCaptureDevice
              previewLayer:nil];
    }
#ifdef Q_OS_IOS
    else {
        // If we're running iOS 16 or older, we need to register for UIDeviceOrientation changes.
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
        m_receivingUiDeviceOrientationNotifications = true;
    }
#endif
}

AvfCameraRotationTracker::AvfCameraRotationTracker(AvfCameraRotationTracker&& other) noexcept :
    m_avCaptureDevice(std::exchange(other.m_avCaptureDevice, nullptr))
#ifdef Q_OS_IOS
    , m_receivingUiDeviceOrientationNotifications(
        std::exchange(other.m_receivingUiDeviceOrientationNotifications, false))
#endif
{
    if (@available(macOS 14.0, iOS 17.0, *)) {
        m_avRotationCoordinator = std::exchange(other.m_avRotationCoordinator, nullptr);
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

#ifdef Q_OS_IOS
    std::swap(
        m_receivingUiDeviceOrientationNotifications,
        other.m_receivingUiDeviceOrientationNotifications);
#endif
}

void AvfCameraRotationTracker::clear()
{
    if (m_avCaptureDevice != nullptr) {
        [m_avCaptureDevice release];
        m_avCaptureDevice = nullptr;
    }

    if (@available(macOS 14.0, iOS 17.0, *)) {
        if (m_avRotationCoordinator != nullptr) {
            [m_avRotationCoordinator release];
            m_avRotationCoordinator = nullptr;
        }
    }

#ifdef Q_OS_IOS
    if (m_receivingUiDeviceOrientationNotifications)
        [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
    m_receivingUiDeviceOrientationNotifications = false;
#endif
}

int AvfCameraRotationTracker::rotationDegrees() const
{
    if (m_avCaptureDevice == nullptr)
        return 0;

    if (@available(macOS 14.0, iOS 17.0, *)) {
        // This code assumes that AVCaptureDeviceRotationCoordinator
        // .videoRotationAngleForHorizonLevelCapture returns degrees that are divisible by 90.
        // This has been the case during testing.
        //
        // TODO: Some rotations are not valid for preview on some devices (such as
        // iPhones not being allowed to have an upside-down window). This usage of the
        // rotation coordinator will still return it as a valid preview rotation, and
        // might cause bugs on iPhone previews.
        if (m_avRotationCoordinator != nullptr)
            return std::lround(m_avRotationCoordinator.videoRotationAngleForHorizonLevelCapture);
    }
#ifdef Q_OS_IOS
    if (m_receivingUiDeviceOrientationNotifications) {
        // TODO: The new orientation can be FlatFaceDown or FlatFaceUp, neither of
        // which should trigger a camera re-orientation. We can't store the previously
        // valid orientation because this method has to be const. Currently
        // this means orientation of the camera might be incorrect when laying the device
        // down flat.
        const UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];

        const AVCaptureDevicePosition captureDevicePosition = m_avCaptureDevice.position;

        // If the position is set to PositionUnspecified, it's a good indication that
        // this is an external webcam. In which case, don't apply any rotation.
        if (captureDevicePosition == AVCaptureDevicePositionBack)
            return uiDeviceOrientationToRotationDegrees(orientation);
        else if (captureDevicePosition == AVCaptureDevicePositionFront)
            return 360 - uiDeviceOrientationToRotationDegrees(orientation);
    }
#endif

    return 0;
}

} // namespace QFFmpeg end

QT_END_NAMESPACE
