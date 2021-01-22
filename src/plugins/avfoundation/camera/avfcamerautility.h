/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef AVFCAMERAUTILITY_H
#define AVFCAMERAUTILITY_H

#include <QtCore/qglobal.h>
#include <QtCore/qvector.h>
#include <QtCore/qdebug.h>
#include <QtCore/qsize.h>
#include <QtCore/qpair.h>

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
    static void cleanup(NSObject *obj)
    {
        if (obj)
            [obj release];
    }
};

template<class T>
class AVFScopedPointer : public QScopedPointer<NSObject, AVFObjectDeleter>
{
public:
    AVFScopedPointer() {}
    explicit AVFScopedPointer(T *ptr) : QScopedPointer(ptr) {}
    operator T*() const
    {
        // Quite handy operator to enable Obj-C messages: [ptr someMethod];
        return data();
    }

    T *data() const
    {
        return static_cast<T *>(QScopedPointer::data());
    }

    T *take()
    {
        return static_cast<T *>(QScopedPointer::take());
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

QVector<AVCaptureDeviceFormat *> qt_unique_device_formats(AVCaptureDevice *captureDevice,
                                                          FourCharCode preferredFormat);
QSize qt_device_format_resolution(AVCaptureDeviceFormat *format);
QSize qt_device_format_high_resolution(AVCaptureDeviceFormat *format);
QSize qt_device_format_pixel_aspect_ratio(AVCaptureDeviceFormat *format);
QVector<AVFPSRange> qt_device_format_framerates(AVCaptureDeviceFormat *format);
AVCaptureDeviceFormat *qt_find_best_resolution_match(AVCaptureDevice *captureDevice, const QSize &res,
                                                     FourCharCode preferredFormat, bool stillImage = true);
AVCaptureDeviceFormat *qt_find_best_framerate_match(AVCaptureDevice *captureDevice,
                                                    FourCharCode preferredFormat,
                                                    Float64 fps);
AVFrameRateRange *qt_find_supported_framerate_range(AVCaptureDeviceFormat *format, Float64 fps);

bool qt_formats_are_equal(AVCaptureDeviceFormat *f1, AVCaptureDeviceFormat *f2);
bool qt_set_active_format(AVCaptureDevice *captureDevice, AVCaptureDeviceFormat *format, bool preserveFps);

AVFPSRange qt_current_framerates(AVCaptureDevice *captureDevice, AVCaptureConnection *videoConnection);
void qt_set_framerate_limits(AVCaptureDevice *captureDevice, AVCaptureConnection *videoConnection,
                             qreal minFPS, qreal maxFPS);

QT_END_NAMESPACE

#endif
