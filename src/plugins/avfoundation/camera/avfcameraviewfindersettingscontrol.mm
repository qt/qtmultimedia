/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "avfcameraviewfindersettingscontrol.h"
#include "avfcamerarenderercontrol.h"
#include "avfcamerautility.h"
#include "avfcamerasession.h"
#include "avfcameraservice.h"
#include "avfcameradebug.h"

#include <QtMultimedia/qabstractvideosurface.h>

#include <QtCore/qvariant.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qvector.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>
#include <private/qmultimediautils_p.h>

#include <algorithm>

#include <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

namespace {

bool qt_framerates_sane(const QCameraViewfinderSettings &settings)
{
    const qreal minFPS = settings.minimumFrameRate();
    const qreal maxFPS = settings.maximumFrameRate();

    if (minFPS < 0. || maxFPS < 0.)
        return false;

    return !maxFPS || maxFPS >= minFPS;
}

void qt_set_framerate_limits(AVCaptureConnection *videoConnection,
                             const QCameraViewfinderSettings &settings)
{
    Q_ASSERT(videoConnection);

    if (!qt_framerates_sane(settings)) {
        qDebugCamera() << Q_FUNC_INFO << "invalid framerate (min, max):"
                       << settings.minimumFrameRate() << settings.maximumFrameRate();
        return;
    }

    const qreal maxFPS = settings.maximumFrameRate();
    CMTime minDuration = kCMTimeInvalid;
    if (maxFPS > 0.) {
        if (!videoConnection.supportsVideoMinFrameDuration)
            qDebugCamera() << Q_FUNC_INFO << "maximum framerate is not supported";
        else
            minDuration = CMTimeMake(1, maxFPS);
    }
    if (videoConnection.supportsVideoMinFrameDuration)
        videoConnection.videoMinFrameDuration = minDuration;

    const qreal minFPS = settings.minimumFrameRate();
#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_9, __IPHONE_5_0)
#if QT_OSX_DEPLOYMENT_TARGET_BELOW(__MAC_10_9)
    if (QSysInfo::MacintoshVersion < QSysInfo::MV_10_9) {
        if (minFPS > 0.)
            qDebugCamera() << Q_FUNC_INFO << "minimum framerate is not supported";
    } else
#endif
    {
        CMTime maxDuration = kCMTimeInvalid;
        if (minFPS > 0.) {
            if (!videoConnection.supportsVideoMaxFrameDuration)
                qDebugCamera() << Q_FUNC_INFO << "minimum framerate is not supported";
            else
                maxDuration = CMTimeMake(1, minFPS);
        }
        if (videoConnection.supportsVideoMaxFrameDuration)
            videoConnection.videoMaxFrameDuration = maxDuration;
    }
#else
    if (minFPS > 0.)
        qDebugCamera() << Q_FUNC_INFO << "minimum framerate is not supported";
#endif
}

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)

CMTime qt_adjusted_frame_duration(AVFrameRateRange *range, qreal fps)
{
    Q_ASSERT(range);
    Q_ASSERT(fps > 0.);

    if (range.maxFrameRate - range.minFrameRate < 0.1) {
        // Can happen on OS X.
        return range.minFrameDuration;
    }

    if (fps <= range.minFrameRate)
        return range.maxFrameDuration;
    if (fps >= range.maxFrameRate)
        return range.minFrameDuration;

    int n, d;
    qt_real_to_fraction(1. / fps, &n, &d);
    return CMTimeMake(n, d);
}

void qt_set_framerate_limits(AVCaptureDevice *captureDevice,
                             const QCameraViewfinderSettings &settings)
{
    Q_ASSERT(captureDevice);
    if (!captureDevice.activeFormat) {
        qDebugCamera() << Q_FUNC_INFO << "no active capture device format";
        return;
    }

    const qreal minFPS = settings.minimumFrameRate();
    const qreal maxFPS = settings.maximumFrameRate();
    if (!qt_framerates_sane(settings)) {
        qDebugCamera() << Q_FUNC_INFO << "invalid framerates (min, max):"
                       << minFPS << maxFPS;
        return;
    }

    CMTime minFrameDuration = kCMTimeInvalid;
    CMTime maxFrameDuration = kCMTimeInvalid;
    if (maxFPS || minFPS) {
        AVFrameRateRange *range = qt_find_supported_framerate_range(captureDevice.activeFormat,
                                                                    maxFPS ? maxFPS : minFPS);
        if (!range) {
            qDebugCamera() << Q_FUNC_INFO << "no framerate range found, (min, max):"
                           << minFPS << maxFPS;
            return;
        }

        if (maxFPS)
            minFrameDuration = qt_adjusted_frame_duration(range, maxFPS);
        if (minFPS)
            maxFrameDuration = qt_adjusted_frame_duration(range, minFPS);
    }

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qDebugCamera() << Q_FUNC_INFO << "failed to lock for configuration";
        return;
    }

    // While Apple's docs say kCMTimeInvalid will end in default
    // settings for this format, kCMTimeInvalid on OS X ends with a runtime
    // exception:
    // "The activeVideoMinFrameDuration passed is not supported by the device."
#ifdef Q_OS_IOS
    [captureDevice setActiveVideoMinFrameDuration:minFrameDuration];
    [captureDevice setActiveVideoMaxFrameDuration:maxFrameDuration];
#else // Q_OS_OSX

    if (CMTimeCompare(minFrameDuration, kCMTimeInvalid))
        [captureDevice setActiveVideoMinFrameDuration:minFrameDuration];

#if QT_OSX_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_9)
#if QT_OSX_DEPLOYMENT_TARGET_BELOW(__MAC_10_9)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_9)
#endif
    {
        if (CMTimeCompare(maxFrameDuration, kCMTimeInvalid))
            [captureDevice setActiveVideoMaxFrameDuration:maxFrameDuration];
    }
#endif // QT_OSX_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_9)
#endif // Q_OS_OSX
}

#endif // Platform SDK >= 10.9, >= 7.0.

// 'Dispatchers':

AVFPSRange qt_current_framerates(AVCaptureDevice *captureDevice, AVCaptureConnection *videoConnection)
{
    Q_ASSERT(captureDevice);

    AVFPSRange fps;
#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
    if (QSysInfo::MacintoshVersion >= qt_OS_limit(QSysInfo::MV_10_7, QSysInfo::MV_IOS_7_0)) {
        const CMTime minDuration = captureDevice.activeVideoMinFrameDuration;
        if (CMTimeCompare(minDuration, kCMTimeInvalid)) {
            if (const Float64 minSeconds = CMTimeGetSeconds(minDuration))
                fps.second = 1. / minSeconds; // Max FPS = 1 / MinDuration.
        }

#if QT_OSX_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_9)
#if QT_OSX_DEPLOYMENT_TARGET_BELOW(__MAC_10_9)
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_9)
#endif
        {
            const CMTime maxDuration = captureDevice.activeVideoMaxFrameDuration;
            if (CMTimeCompare(maxDuration, kCMTimeInvalid)) {
                if (const Float64 maxSeconds = CMTimeGetSeconds(maxDuration))
                    fps.first = 1. / maxSeconds; // Min FPS = 1 / MaxDuration.
            }
        }
#endif // QT_OSX_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_9)

    } else {
#else // OSX < 10.7 or iOS < 7.0
    {
#endif // QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
        if (videoConnection)
            fps = qt_connection_framerates(videoConnection);
    }

    return fps;
}

void qt_set_framerate_limits(AVCaptureDevice *captureDevice, AVCaptureConnection *videoConnection,
                             const QCameraViewfinderSettings &settings)
{
    Q_ASSERT(captureDevice);
#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
    if (QSysInfo::MacintoshVersion >= qt_OS_limit(QSysInfo::MV_10_9, QSysInfo::MV_IOS_7_0))
        qt_set_framerate_limits(captureDevice, settings);
    else
#endif
    if (videoConnection)
        qt_set_framerate_limits(videoConnection, settings);

}

} // Unnamed namespace.

AVFCameraViewfinderSettingsControl2::AVFCameraViewfinderSettingsControl2(AVFCameraService *service)
    : m_service(service)
{
    Q_ASSERT(service);
}

QList<QCameraViewfinderSettings> AVFCameraViewfinderSettingsControl2::supportedViewfinderSettings() const
{
    QList<QCameraViewfinderSettings> supportedSettings;

    AVCaptureDevice *captureDevice = m_service->session()->videoCaptureDevice();
    if (!captureDevice) {
        qDebugCamera() << Q_FUNC_INFO << "no capture device found";
        return supportedSettings;
    }

    QVector<AVFPSRange> framerates;

    QVector<QVideoFrame::PixelFormat> pixelFormats(viewfinderPixelFormats());

    if (!pixelFormats.size())
        pixelFormats << QVideoFrame::Format_Invalid; // The default value.

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
    if (QSysInfo::MacintoshVersion >= qt_OS_limit(QSysInfo::MV_10_7, QSysInfo::MV_IOS_7_0)) {
        if (!captureDevice.formats || !captureDevice.formats.count) {
            qDebugCamera() << Q_FUNC_INFO << "no capture device formats found";
            return supportedSettings;
        }

        const QVector<AVCaptureDeviceFormat *> formats(qt_unique_device_formats(captureDevice,
                                                       m_service->session()->defaultCodec()));
        for (int i = 0; i < formats.size(); ++i) {
            AVCaptureDeviceFormat *format = formats[i];

            const QSize res(qt_device_format_resolution(format));
            if (res.isNull() || !res.isValid())
                continue;
            const QSize par(qt_device_format_pixel_aspect_ratio(format));
            if (par.isNull() || !par.isValid())
                continue;

            framerates = qt_device_format_framerates(format);
            if (!framerates.size())
                framerates << AVFPSRange(); // The default value.

            for (int i = 0; i < pixelFormats.size(); ++i) {
                for (int j = 0; j < framerates.size(); ++j) {
                    QCameraViewfinderSettings newSet;
                    newSet.setResolution(res);
                    newSet.setPixelAspectRatio(par);
                    newSet.setPixelFormat(pixelFormats[i]);
                    newSet.setMinimumFrameRate(framerates[j].first);
                    newSet.setMaximumFrameRate(framerates[j].second);
                    supportedSettings << newSet;
                }
            }
        }
    } else {
#else
    {
#endif
        AVCaptureConnection *connection = videoConnection();
        if (connection) {
            // TODO: resolution and PAR.
            framerates << qt_connection_framerates(connection);
            for (int i = 0; i < pixelFormats.size(); ++i) {
                for (int j = 0; j < framerates.size(); ++j) {
                    QCameraViewfinderSettings newSet;
                    newSet.setPixelFormat(pixelFormats[i]);
                    newSet.setMinimumFrameRate(framerates[j].first);
                    newSet.setMaximumFrameRate(framerates[j].second);
                    supportedSettings << newSet;
                }
            }
        }
    }

    return supportedSettings;
}

QCameraViewfinderSettings AVFCameraViewfinderSettingsControl2::viewfinderSettings() const
{
    QCameraViewfinderSettings settings = m_settings;

    AVCaptureDevice *captureDevice = m_service->session()->videoCaptureDevice();
    if (!captureDevice) {
        qDebugCamera() << Q_FUNC_INFO << "no capture device found";
        return settings;
    }

    if (m_service->session()->state() != QCamera::LoadedState &&
        m_service->session()->state() != QCamera::ActiveState) {
        return settings;
    }

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
    if (QSysInfo::MacintoshVersion >= qt_OS_limit(QSysInfo::MV_10_7, QSysInfo::MV_IOS_7_0)) {
        if (!captureDevice.activeFormat) {
            qDebugCamera() << Q_FUNC_INFO << "no active capture device format";
            return settings;
        }

        const QSize res(qt_device_format_resolution(captureDevice.activeFormat));
        const QSize par(qt_device_format_pixel_aspect_ratio(captureDevice.activeFormat));
        if (res.isNull() || !res.isValid() || par.isNull() || !par.isValid()) {
            qDebugCamera() << Q_FUNC_INFO << "failed to obtain resolution/pixel aspect ratio";
            return settings;
        }

        settings.setResolution(res);
        settings.setPixelAspectRatio(par);
    }
#endif
    // TODO: resolution and PAR before 7.0.
    const AVFPSRange fps = qt_current_framerates(captureDevice, videoConnection());
    settings.setMinimumFrameRate(fps.first);
    settings.setMaximumFrameRate(fps.second);

    AVCaptureVideoDataOutput *videoOutput = m_service->videoOutput() ? m_service->videoOutput()->videoDataOutput() : 0;
    if (videoOutput) {
        NSObject *obj = [videoOutput.videoSettings objectForKey:(id)kCVPixelBufferPixelFormatTypeKey];
        if (obj && [obj isKindOfClass:[NSNumber class]]) {
            NSNumber *nsNum = static_cast<NSNumber *>(obj);
            settings.setPixelFormat(QtPixelFormatFromCVFormat([nsNum unsignedIntValue]));
        }
    }

    return settings;
}

void AVFCameraViewfinderSettingsControl2::setViewfinderSettings(const QCameraViewfinderSettings &settings)
{
    if (m_settings == settings)
        return;

    m_settings = settings;
    applySettings();
}

QVideoFrame::PixelFormat AVFCameraViewfinderSettingsControl2::QtPixelFormatFromCVFormat(unsigned avPixelFormat)
{
    // BGRA <-> ARGB "swap" is intentional:
    // to work correctly with GL_RGBA, color swap shaders
    // (in QSG node renderer etc.).
    switch (avPixelFormat) {
    case kCVPixelFormatType_32ARGB:
        return QVideoFrame::Format_BGRA32;
    case kCVPixelFormatType_32BGRA:
        return QVideoFrame::Format_ARGB32;
    case kCVPixelFormatType_24RGB:
        return QVideoFrame::Format_RGB24;
    case kCVPixelFormatType_24BGR:
        return QVideoFrame::Format_BGR24;
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
    case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
        return QVideoFrame::Format_NV12;
    default:
        return QVideoFrame::Format_Invalid;
    }
}

bool AVFCameraViewfinderSettingsControl2::CVPixelFormatFromQtFormat(QVideoFrame::PixelFormat qtFormat, unsigned &conv)
{
    // BGRA <-> ARGB "swap" is intentional:
    // to work correctly with GL_RGBA, color swap shaders
    // (in QSG node renderer etc.).
    switch (qtFormat) {
    case QVideoFrame::Format_ARGB32:
        conv = kCVPixelFormatType_32BGRA;
        break;
    case QVideoFrame::Format_BGRA32:
        conv = kCVPixelFormatType_32ARGB;
        break;
    case QVideoFrame::Format_NV12:
        conv = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
        break;
    // These two formats below are not supported
    // by QSGVideoNodeFactory_RGB, so for now I have to
    // disable them.
    /*
    case QVideoFrame::Format_RGB24:
        conv = kCVPixelFormatType_24RGB;
        break;
    case QVideoFrame::Format_BGR24:
        conv = kCVPixelFormatType_24BGR;
        break;
    */
    default:
        return false;
    }

    return true;
}

AVCaptureDeviceFormat *AVFCameraViewfinderSettingsControl2::findBestFormatMatch(const QCameraViewfinderSettings &settings) const
{
    AVCaptureDevice *captureDevice = m_service->session()->videoCaptureDevice();
    if (!captureDevice || settings.isNull())
        return nil;

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
    if (QSysInfo::MacintoshVersion >= qt_OS_limit(QSysInfo::MV_10_7, QSysInfo::MV_IOS_7_0)) {

        const QSize &resolution = settings.resolution();
        if (!resolution.isNull() && resolution.isValid()) {
            // Either the exact match (including high resolution for images on iOS)
            // or a format with a resolution close to the requested one.
            return qt_find_best_resolution_match(captureDevice, resolution,
                                                 m_service->session()->defaultCodec());
        }

        // No resolution requested, what about framerates?
        if (!qt_framerates_sane(settings)) {
            qDebugCamera() << Q_FUNC_INFO << "invalid framerate requested (min/max):"
                           << settings.minimumFrameRate() << settings.maximumFrameRate();
            return nil;
        }

        const qreal minFPS(settings.minimumFrameRate());
        const qreal maxFPS(settings.maximumFrameRate());
        if (minFPS || maxFPS)
            return qt_find_best_framerate_match(captureDevice, maxFPS ? maxFPS : minFPS,
                                                m_service->session()->defaultCodec());
        // Ignore PAR for the moment (PAR without resolution can
        // pick a format with really bad resolution).
        // No need to test pixel format, just return settings.
    }
#endif

    return nil;
}

QVector<QVideoFrame::PixelFormat> AVFCameraViewfinderSettingsControl2::viewfinderPixelFormats() const
{
    QVector<QVideoFrame::PixelFormat> qtFormats;

    AVCaptureVideoDataOutput *videoOutput = m_service->videoOutput() ? m_service->videoOutput()->videoDataOutput() : 0;
    if (!videoOutput) {
        qDebugCamera() << Q_FUNC_INFO << "no video output found";
        return qtFormats;
    }

    NSArray *pixelFormats = [videoOutput availableVideoCVPixelFormatTypes];

    for (NSObject *obj in pixelFormats) {
        if (![obj isKindOfClass:[NSNumber class]])
            continue;

        NSNumber *formatAsNSNumber = static_cast<NSNumber *>(obj);
        // It's actually FourCharCode (== UInt32):
        const QVideoFrame::PixelFormat qtFormat(QtPixelFormatFromCVFormat([formatAsNSNumber unsignedIntValue]));
        if (qtFormat != QVideoFrame::Format_Invalid
                && !qtFormats.contains(qtFormat)) { // Can happen, for example, with 8BiPlanar existing in video/full range.
            qtFormats << qtFormat;
        }
    }

    return qtFormats;
}

bool AVFCameraViewfinderSettingsControl2::convertPixelFormatIfSupported(QVideoFrame::PixelFormat qtFormat,
                                                                        unsigned &avfFormat)const
{
    AVCaptureVideoDataOutput *videoOutput = m_service->videoOutput() ? m_service->videoOutput()->videoDataOutput() : 0;
    if (!videoOutput)
        return false;

    unsigned conv = 0;
    if (!CVPixelFormatFromQtFormat(qtFormat, conv))
        return false;

    NSArray *formats = [videoOutput availableVideoCVPixelFormatTypes];
    if (!formats || !formats.count)
        return false;

    if (m_service->videoOutput()->surface()) {
        const QAbstractVideoSurface *surface = m_service->videoOutput()->surface();
        if (!surface->supportedPixelFormats().contains(qtFormat))
            return false;
    }

    bool found = false;
    for (NSObject *obj in formats) {
        if (![obj isKindOfClass:[NSNumber class]])
            continue;

        NSNumber *nsNum = static_cast<NSNumber *>(obj);
        if ([nsNum unsignedIntValue] == conv) {
            avfFormat = conv;
            found = true;
        }
    }

    return found;
}

bool AVFCameraViewfinderSettingsControl2::applySettings()
{
    if (m_service->session()->state() != QCamera::LoadedState &&
        m_service->session()->state() != QCamera::ActiveState) {
        return false;
    }

    AVCaptureDevice *captureDevice = m_service->session()->videoCaptureDevice();
    if (!captureDevice)
        return false;

    bool activeFormatChanged = false;

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
    AVCaptureDeviceFormat *match = findBestFormatMatch(m_settings);
    if (match) {
        if (match != captureDevice.activeFormat) {
            const AVFConfigurationLock lock(captureDevice);
            if (lock) {
                captureDevice.activeFormat = match;
                activeFormatChanged = true;
            } else {
                qDebugCamera() << Q_FUNC_INFO << "failed to lock for configuration";
            }
        }
    } else {
        qDebugCamera() << Q_FUNC_INFO << "matching device format not found";
        // We still can update the pixel format at least.
    }
#endif

    AVCaptureVideoDataOutput *videoOutput = m_service->videoOutput() ? m_service->videoOutput()->videoDataOutput() : 0;
    if (videoOutput) {
        unsigned avfPixelFormat = 0;
        if (!convertPixelFormatIfSupported(m_settings.pixelFormat(), avfPixelFormat)) {
            // If the the pixel format is not specified or invalid, pick the preferred video surface
            // format, or if no surface is set, the preferred capture device format

            const QVector<QVideoFrame::PixelFormat> deviceFormats = viewfinderPixelFormats();
            QVideoFrame::PixelFormat pickedFormat = deviceFormats.first();

            QAbstractVideoSurface *surface = m_service->videoOutput()->surface();
            if (surface) {
                if (m_service->videoOutput()->supportsTextures()) {
                    pickedFormat = QVideoFrame::Format_ARGB32;
                } else {
                    QList<QVideoFrame::PixelFormat> surfaceFormats = surface->supportedPixelFormats();

                    for (int i = 0; i < surfaceFormats.count(); ++i) {
                        const QVideoFrame::PixelFormat surfaceFormat = surfaceFormats.at(i);
                        if (deviceFormats.contains(surfaceFormat)) {
                            pickedFormat = surfaceFormat;
                            break;
                        }
                    }
                }
            }

            CVPixelFormatFromQtFormat(pickedFormat, avfPixelFormat);
        }

        if (avfPixelFormat != 0) {
            NSMutableDictionary *videoSettings = [NSMutableDictionary dictionaryWithCapacity:1];
            [videoSettings setObject:[NSNumber numberWithUnsignedInt:avfPixelFormat]
                                      forKey:(id)kCVPixelBufferPixelFormatTypeKey];

            videoOutput.videoSettings = videoSettings;
        }
    }

    qt_set_framerate_limits(captureDevice, videoConnection(), m_settings);

    return activeFormatChanged;
}

QCameraViewfinderSettings AVFCameraViewfinderSettingsControl2::requestedSettings() const
{
    return m_settings;
}

AVCaptureConnection *AVFCameraViewfinderSettingsControl2::videoConnection() const
{
    if (!m_service->videoOutput() || !m_service->videoOutput()->videoDataOutput())
        return nil;

    return [m_service->videoOutput()->videoDataOutput() connectionWithMediaType:AVMediaTypeVideo];
}

AVFCameraViewfinderSettingsControl::AVFCameraViewfinderSettingsControl(AVFCameraService *service)
    : m_service(service)
{
    // Legacy viewfinder settings control.
    Q_ASSERT(service);
    initSettingsControl();
}

bool AVFCameraViewfinderSettingsControl::isViewfinderParameterSupported(ViewfinderParameter parameter) const
{
    return parameter == Resolution
           || parameter == PixelAspectRatio
           || parameter == MinimumFrameRate
           || parameter == MaximumFrameRate
           || parameter == PixelFormat;
}

QVariant AVFCameraViewfinderSettingsControl::viewfinderParameter(ViewfinderParameter parameter) const
{
    if (!isViewfinderParameterSupported(parameter)) {
        qDebugCamera() << Q_FUNC_INFO << "parameter is not supported";
        return QVariant();
    }

    if (!initSettingsControl()) {
        qDebugCamera() << Q_FUNC_INFO << "initialization failed";
        return QVariant();
    }

    const QCameraViewfinderSettings settings(m_settingsControl->viewfinderSettings());
    if (parameter == Resolution)
        return settings.resolution();
    if (parameter == PixelAspectRatio)
        return settings.pixelAspectRatio();
    if (parameter == MinimumFrameRate)
        return settings.minimumFrameRate();
    if (parameter == MaximumFrameRate)
        return settings.maximumFrameRate();
    if (parameter == PixelFormat)
        return QVariant::fromValue(settings.pixelFormat());

    return QVariant();
}

void AVFCameraViewfinderSettingsControl::setViewfinderParameter(ViewfinderParameter parameter, const QVariant &value)
{
    if (!isViewfinderParameterSupported(parameter)) {
        qDebugCamera() << Q_FUNC_INFO << "parameter is not supported";
        return;
    }

    if (parameter == Resolution)
        setResolution(value);
    if (parameter == PixelAspectRatio)
        setAspectRatio(value);
    if (parameter == MinimumFrameRate)
        setFrameRate(value, false);
    if (parameter == MaximumFrameRate)
        setFrameRate(value, true);
    if (parameter == PixelFormat)
        setPixelFormat(value);
}

void AVFCameraViewfinderSettingsControl::setResolution(const QVariant &newValue)
{
    if (!newValue.canConvert<QSize>()) {
        qDebugCamera() << Q_FUNC_INFO << "QSize type expected";
        return;
    }

    if (!initSettingsControl()) {
        qDebugCamera() << Q_FUNC_INFO << "initialization failed";
        return;
    }

    const QSize res(newValue.toSize());
    if (res.isNull() || !res.isValid()) {
        qDebugCamera() << Q_FUNC_INFO << "invalid resolution:" << res;
        return;
    }

    QCameraViewfinderSettings settings(m_settingsControl->viewfinderSettings());
    settings.setResolution(res);
    m_settingsControl->setViewfinderSettings(settings);
}

void AVFCameraViewfinderSettingsControl::setAspectRatio(const QVariant &newValue)
{
    if (!newValue.canConvert<QSize>()) {
        qDebugCamera() << Q_FUNC_INFO << "QSize type expected";
        return;
    }

    if (!initSettingsControl()) {
        qDebugCamera() << Q_FUNC_INFO << "initialization failed";
        return;
    }

    const QSize par(newValue.value<QSize>());
    if (par.isNull() || !par.isValid()) {
        qDebugCamera() << Q_FUNC_INFO << "invalid pixel aspect ratio:" << par;
        return;
    }

    QCameraViewfinderSettings settings(m_settingsControl->viewfinderSettings());
    settings.setPixelAspectRatio(par);
    m_settingsControl->setViewfinderSettings(settings);
}

void AVFCameraViewfinderSettingsControl::setFrameRate(const QVariant &newValue, bool max)
{
    if (!newValue.canConvert<qreal>()) {
        qDebugCamera() << Q_FUNC_INFO << "qreal type expected";
        return;
    }

    if (!initSettingsControl()) {
        qDebugCamera() << Q_FUNC_INFO << "initialization failed";
        return;
    }

    const qreal fps(newValue.toReal());
    QCameraViewfinderSettings settings(m_settingsControl->viewfinderSettings());
    max ? settings.setMaximumFrameRate(fps) : settings.setMinimumFrameRate(fps);
    m_settingsControl->setViewfinderSettings(settings);
}

void AVFCameraViewfinderSettingsControl::setPixelFormat(const QVariant &newValue)
{
    if (!newValue.canConvert<QVideoFrame::PixelFormat>()) {
        qDebugCamera() << Q_FUNC_INFO
                       << "QVideoFrame::PixelFormat type expected";
        return;
    }

    if (!initSettingsControl()) {
        qDebugCamera() << Q_FUNC_INFO << "initialization failed";
        return;
    }

    QCameraViewfinderSettings settings(m_settingsControl->viewfinderSettings());
    settings.setPixelFormat(newValue.value<QVideoFrame::PixelFormat>());
    m_settingsControl->setViewfinderSettings(settings);
}

bool AVFCameraViewfinderSettingsControl::initSettingsControl()const
{
    if (!m_settingsControl)
        m_settingsControl = m_service->viewfinderSettingsControl2();

    return !m_settingsControl.isNull();
}

QT_END_NAMESPACE

#include "moc_avfcameraviewfindersettingscontrol.cpp"
