/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef AVFCAMERAUTILITY_H
#define AVFCAMERAUTILITY_H

#include <QtCore/qsysinfo.h>
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
    AVFScopedPointer() : m_queue(0) {}
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

    void reset(dispatch_queue_t q = 0)
    {
        if (m_queue)
            dispatch_release(m_queue);
        m_queue = q;
    }

private:
    dispatch_queue_t m_queue;

    Q_DISABLE_COPY(AVFScopedPointer);
};

inline QSysInfo::MacVersion qt_OS_limit(QSysInfo::MacVersion osxVersion,
                                        QSysInfo::MacVersion iosVersion)
{
#ifdef Q_OS_OSX
    Q_UNUSED(iosVersion)
    return osxVersion;
#else
    Q_UNUSED(osxVersion)
    return iosVersion;
#endif
}

typedef QPair<qreal, qreal> AVFPSRange;
AVFPSRange qt_connection_framerates(AVCaptureConnection *videoConnection);

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)

QVector<AVCaptureDeviceFormat *> qt_unique_device_formats(AVCaptureDevice *captureDevice,
                                                          FourCharCode preferredFormat);
QSize qt_device_format_resolution(AVCaptureDeviceFormat *format);
QSize qt_device_format_high_resolution(AVCaptureDeviceFormat *format);
QSize qt_device_format_pixel_aspect_ratio(AVCaptureDeviceFormat *format);
QVector<AVFPSRange> qt_device_format_framerates(AVCaptureDeviceFormat *format);
AVCaptureDeviceFormat *qt_find_best_resolution_match(AVCaptureDevice *captureDevice, const QSize &res,
                                                     FourCharCode preferredFormat);
AVCaptureDeviceFormat *qt_find_best_framerate_match(AVCaptureDevice *captureDevice,
                                                    FourCharCode preferredFormat,
                                                    Float64 fps);
AVFrameRateRange *qt_find_supported_framerate_range(AVCaptureDeviceFormat *format, Float64 fps);

bool qt_formats_are_equal(AVCaptureDeviceFormat *f1, AVCaptureDeviceFormat *f2);
bool qt_set_active_format(AVCaptureDevice *captureDevice, AVCaptureDeviceFormat *format, bool preserveFps);

#endif

AVFPSRange qt_current_framerates(AVCaptureDevice *captureDevice, AVCaptureConnection *videoConnection);
void qt_set_framerate_limits(AVCaptureDevice *captureDevice, AVCaptureConnection *videoConnection,
                             qreal minFPS, qreal maxFPS);

QT_END_NAMESPACE

#endif
