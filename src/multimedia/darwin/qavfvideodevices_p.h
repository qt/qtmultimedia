// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFVIDEODEVICES_H
#define QAVFVIDEODEVICES_H

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

#include <QtMultimedia/private/qplatformvideodevices_p.h>

#include <QtCore/private/qcore_mac_p.h>

#include <QtMultimedia/private/qavfcamerautility_p.h>
#include <QtMultimedia/qtmultimediaexports.h>

#include <functional>
#include <vector>

#import <AVFoundation/AVCaptureDevice.h>

QT_BEGIN_NAMESPACE

class QPlatformMediaIntegration;

// This class is always moved to the main-thread upon construction.
// All changes and events happens on the main-thread.
class Q_MULTIMEDIA_EXPORT QAVFVideoDevices : public QPlatformVideoDevices
{
public:
    // Takes a delegate to check whether a given CvPixelFormat is supported for a capture session.
    // If given a nullptr, it assumes all formats are supported.
    QAVFVideoDevices(
        QPlatformMediaIntegration *integration,
        std::function<bool(uint32_t)> &&isCvPixelFormatSupportedDelegate = nullptr);
    ~QAVFVideoDevices();

    // Returns true if the given CvPixelFormat is supported for camera capture session.
    [[nodiscard]] bool isCvPixelFormatSupported(uint32_t cvPixelFormat) const;

protected:
    QList<QCameraDevice> findVideoInputs() const override;

private:
    void onAvCaptureDevicesChanged();

private:
    // In rare cases, m_avDiscoverySession may be null. See QTBUG-144218.
    AVFScopedPointer<AVCaptureDeviceDiscoverySession> m_avDiscoverySession;
    // Note: Observer holds weak ref to AVCaptureDeviceDiscoverySession, and must
    // be destroyed before AVCaptureDeviceDiscoverySession.
    QMacKeyValueObserver m_avDiscoverySessionObserver;

    std::function<bool(uint32_t)> m_isCvPixelFormatSupportedDelegate;

    // We need to key-value observe the "suspended" value of all connected
    // AVCaptureDevices in order to determine whether they should be exposed as
    // QCameraDevice to the user.
    struct ObservedAVCaptureDevice {
        AVFScopedPointer<AVCaptureDevice> avCaptureDevice;
        // Note: Observer holds weak ref to avCaptureDevice, and must
        // be destroyed before avCaptureDevice.
        QMacKeyValueObserver observer;
    };

    void rebuildObserveredAvCaptureDevices();
    // All modifications and read of m_observedAvCaptureDevices happen by
    // posting jobs the QAVFVideoDevices' thread, and so this doesn't need a
    // lock.
    std::vector<ObservedAVCaptureDevice> m_observedAvCaptureDevices;
};

QT_END_NAMESPACE

#endif
