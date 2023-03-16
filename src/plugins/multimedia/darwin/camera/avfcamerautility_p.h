// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFCAMERAUTILITY_H
#define AVFCAMERAUTILITY_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qsize.h>

#include "qcameradevice.h"

#include <AVFoundation/AVFoundation.h>

// In case we have SDK below 10.7/7.0:
@class AVCaptureDeviceFormat;

QT_BEGIN_NAMESPACE

class AVFConfigurationLock
{
public:
    explicit AVFConfigurationLock(AVCaptureDevice *captureDevice)
        : m_captureDevice(captureDevice),
          m_locked(false)
    {
        Q_ASSERT(m_captureDevice);
        NSError *error = nil;
        m_locked = [m_captureDevice lockForConfiguration:&error];
    }

    ~AVFConfigurationLock()
    {
        if (m_locked)
            [m_captureDevice unlockForConfiguration];
    }

    operator bool() const
    {
        return m_locked;
    }

private:
    Q_DISABLE_COPY(AVFConfigurationLock)

    AVCaptureDevice *m_captureDevice;
    bool m_locked;
};

struct AVFObjectDeleter {
    void operator()(NSObject *obj)
    {
        if (obj)
            [obj release];
    }
};

template<class T>
class AVFScopedPointer : public std::unique_ptr<NSObject, AVFObjectDeleter>
{
public:
    AVFScopedPointer() {}
    explicit AVFScopedPointer(T *ptr) : std::unique_ptr<NSObject, AVFObjectDeleter>(ptr) {}
    operator T*() const
    {
        // Quite handy operator to enable Obj-C messages: [ptr someMethod];
        return data();
    }

    T *data() const
    {
        return static_cast<T *>(get());
    }

    T *take()
    {
        return static_cast<T *>(release());
    }
};

template<>
class AVFScopedPointer<dispatch_queue_t>
{
public:
    AVFScopedPointer() : m_queue(nullptr) {}
    explicit AVFScopedPointer(dispatch_queue_t q) : m_queue(q) {}

    ~AVFScopedPointer()
    {
        if (m_queue)
            dispatch_release(m_queue);
    }

    operator dispatch_queue_t() const
    {
        // Quite handy operator to enable Obj-C messages: [ptr someMethod];
        return m_queue;
    }

    dispatch_queue_t data() const
    {
        return m_queue;
    }

    void reset(dispatch_queue_t q = nullptr)
    {
        if (m_queue)
            dispatch_release(m_queue);
        m_queue = q;
    }

private:
    dispatch_queue_t m_queue;

    Q_DISABLE_COPY(AVFScopedPointer)
};

typedef QPair<qreal, qreal> AVFPSRange;
AVFPSRange qt_connection_framerates(AVCaptureConnection *videoConnection);

AVCaptureDeviceFormat *qt_convert_to_capture_device_format(
        AVCaptureDevice *captureDevice, const QCameraFormat &format,
        const std::function<bool(uint32_t)> &cvFormatValidator = nullptr);
QList<AVCaptureDeviceFormat *> qt_unique_device_formats(AVCaptureDevice *captureDevice,
                                                        FourCharCode preferredFormat);
QSize qt_device_format_resolution(AVCaptureDeviceFormat *format);
QSize qt_device_format_high_resolution(AVCaptureDeviceFormat *format);
QSize qt_device_format_pixel_aspect_ratio(AVCaptureDeviceFormat *format);
QList<AVFPSRange> qt_device_format_framerates(AVCaptureDeviceFormat *format);
AVCaptureDeviceFormat *qt_find_best_resolution_match(AVCaptureDevice *captureDevice, const QSize &res,
                                                     FourCharCode preferredFormat, bool stillImage = true);
AVCaptureDeviceFormat *qt_find_best_framerate_match(AVCaptureDevice *captureDevice,
                                                    FourCharCode preferredFormat,
                                                    Float64 fps);
AVFrameRateRange *qt_find_supported_framerate_range(AVCaptureDeviceFormat *format, Float64 fps);
bool qt_format_supports_framerate(AVCaptureDeviceFormat *format, qreal fps);

bool qt_formats_are_equal(AVCaptureDeviceFormat *f1, AVCaptureDeviceFormat *f2);
bool qt_set_active_format(AVCaptureDevice *captureDevice, AVCaptureDeviceFormat *format, bool preserveFps);

AVFPSRange qt_current_framerates(AVCaptureDevice *captureDevice, AVCaptureConnection *videoConnection);
void qt_set_framerate_limits(AVCaptureDevice *captureDevice, AVCaptureConnection *videoConnection,
                             qreal minFPS, qreal maxFPS);

QList<AudioValueRange> qt_supported_sample_rates_for_format(int codecId);
QList<AudioValueRange> qt_supported_bit_rates_for_format(int codecId);
std::optional<QList<UInt32>> qt_supported_channel_counts_for_format(int codecId);
QList<UInt32> qt_supported_channel_layout_tags_for_format(int codecId, int noChannels);

QT_END_NAMESPACE

#endif
