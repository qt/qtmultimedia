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

#include "avfcameraviewfindersettingscontrol.h"
#include "avfcamerarenderercontrol.h"
#include "avfcamerautility.h"
#include "avfcamerasession.h"
#include "avfcameraservice.h"
#include "avfcameradebug.h"

#include <QtCore/qvariant.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qvector.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>

#include <algorithm>

#include <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

namespace {

QVector<QVideoFrame::PixelFormat> qt_viewfinder_pixel_formats(AVCaptureVideoDataOutput *videoOutput)
{
    Q_ASSERT(videoOutput);

    QVector<QVideoFrame::PixelFormat> qtFormats;

    NSArray *pixelFormats = [videoOutput availableVideoCVPixelFormatTypes];
    for (NSObject *obj in pixelFormats) {
        if (![obj isKindOfClass:[NSNumber class]])
            continue;

        NSNumber *formatAsNSNumber = static_cast<NSNumber *>(obj);
        // It's actually FourCharCode (== UInt32):
        const QVideoFrame::PixelFormat qtFormat(AVFCameraViewfinderSettingsControl2::
                                                QtPixelFormatFromCVFormat([formatAsNSNumber unsignedIntValue]));
        if (qtFormat != QVideoFrame::Format_Invalid)
            qtFormats << qtFormat;
    }

    return qtFormats;
}

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

    const qreal minFPS = settings.minimumFrameRate();
    const qreal maxFPS = settings.maximumFrameRate();

    CMTime minDuration = kCMTimeInvalid;
    CMTime maxDuration = kCMTimeInvalid;
    if (minFPS > 0. || maxFPS > 0.) {
        if (maxFPS) {
            if (!videoConnection.supportsVideoMinFrameDuration)
                qDebugCamera() << Q_FUNC_INFO << "maximum framerate is not supported";
            else
                minDuration = CMTimeMake(1, maxFPS);
        }

        if (minFPS) {
            if (!videoConnection.supportsVideoMaxFrameDuration)
                qDebugCamera() << Q_FUNC_INFO << "minimum framerate is not supported";
            else
                maxDuration = CMTimeMake(1, minFPS);
        }
    }

    if (videoConnection.supportsVideoMinFrameDuration)
        videoConnection.videoMinFrameDuration = minDuration;
    if (videoConnection.supportsVideoMaxFrameDuration)
        videoConnection.videoMaxFrameDuration = maxDuration;
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

    const AVFRational timeAsRational(qt_float_to_rational(1. / fps, 1000));
    return CMTimeMake(timeAsRational.first, timeAsRational.second);
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
#else
    if (CMTimeCompare(minFrameDuration, kCMTimeInvalid))
        [captureDevice setActiveVideoMinFrameDuration:minFrameDuration];
    if (CMTimeCompare(maxFrameDuration, kCMTimeInvalid))
        [captureDevice setActiveVideoMaxFrameDuration:maxFrameDuration];
#endif
}

#endif // Platform SDK >= 10.9, >= 7.0.

// 'Dispatchers':

AVFPSRange qt_current_framerates(AVCaptureDevice *captureDevice, AVCaptureConnection *videoConnection)
{
    Q_ASSERT(captureDevice);
    Q_ASSERT(videoConnection);

    AVFPSRange fps;
#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
    if (QSysInfo::MacintoshVersion >= qt_OS_limit(QSysInfo::MV_10_7, QSysInfo::MV_IOS_7_0)) {
        const CMTime minDuration = captureDevice.activeVideoMinFrameDuration;
        if (CMTimeCompare(minDuration, kCMTimeInvalid)) {
            if (const Float64 minSeconds = CMTimeGetSeconds(minDuration))
                fps.second = 1. / minSeconds; // Max FPS = 1 / MinDuration.
        }

        const CMTime maxDuration = captureDevice.activeVideoMaxFrameDuration;
        if (CMTimeCompare(maxDuration, kCMTimeInvalid)) {
            if (const Float64 maxSeconds = CMTimeGetSeconds(maxDuration))
                fps.first = 1. / maxSeconds; // Min FPS = 1 / MaxDuration.
        }
    } else {
#else
    {
#endif
        fps = qt_connection_framerates(videoConnection);
    }

    return fps;
}

void qt_set_framerate_limits(AVCaptureDevice *captureDevice, AVCaptureConnection *videoConnection,
                             const QCameraViewfinderSettings &settings)
{
    Q_ASSERT(captureDevice);
    Q_ASSERT(videoConnection);
#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
    if (QSysInfo::MacintoshVersion >= qt_OS_limit(QSysInfo::MV_10_9, QSysInfo::MV_IOS_7_0))
        qt_set_framerate_limits(captureDevice, settings);
    else
        qt_set_framerate_limits(videoConnection, settings);
#else
    qt_set_framerate_limits(videoConnection, settings);
#endif
}

} // Unnamed namespace.

AVFCameraViewfinderSettingsControl2::AVFCameraViewfinderSettingsControl2(AVFCameraService *service)
    : m_service(service),
      m_captureDevice(0),
      m_videoOutput(0),
      m_videoConnection(0)
{
    Q_ASSERT(service);
}

QList<QCameraViewfinderSettings> AVFCameraViewfinderSettingsControl2::supportedViewfinderSettings() const
{
    QList<QCameraViewfinderSettings> supportedSettings;

    if (!updateAVFoundationObjects()) {
        qDebugCamera() << Q_FUNC_INFO << "no capture device or video output found";
        return supportedSettings;
    }

    QVector<AVFPSRange> framerates;

    QVector<QVideoFrame::PixelFormat> pixelFormats(qt_viewfinder_pixel_formats(m_videoOutput));
    if (!pixelFormats.size())
        pixelFormats << QVideoFrame::Format_Invalid; // The default value.
#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
    if (QSysInfo::MacintoshVersion >= qt_OS_limit(QSysInfo::MV_10_7, QSysInfo::MV_IOS_7_0)) {
        if (!m_captureDevice.formats || !m_captureDevice.formats.count) {
            qDebugCamera() << Q_FUNC_INFO << "no capture device formats found";
            return supportedSettings;
        }

        const QVector<AVCaptureDeviceFormat *> formats(qt_unique_device_formats(m_captureDevice,
                                                       m_session->defaultCodec()));
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
        // TODO: resolution and PAR.
        framerates << qt_connection_framerates(m_videoConnection);
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

    return supportedSettings;
}

QCameraViewfinderSettings AVFCameraViewfinderSettingsControl2::viewfinderSettings() const
{
    QCameraViewfinderSettings settings;

    if (!updateAVFoundationObjects()) {
        qDebugCamera() << Q_FUNC_INFO << "no capture device or video output found";
        return settings;
    }

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
    if (QSysInfo::MacintoshVersion >= qt_OS_limit(QSysInfo::MV_10_7, QSysInfo::MV_IOS_7_0)) {
        if (!m_captureDevice.activeFormat) {
            qDebugCamera() << Q_FUNC_INFO << "no active capture device format";
            return settings;
        }

        const QSize res(qt_device_format_resolution(m_captureDevice.activeFormat));
        const QSize par(qt_device_format_pixel_aspect_ratio(m_captureDevice.activeFormat));
        if (res.isNull() || !res.isValid() || par.isNull() || !par.isValid()) {
            qDebugCamera() << Q_FUNC_INFO << "failed to obtain resolution/pixel aspect ratio";
            return settings;
        }

        settings.setResolution(res);
        settings.setPixelAspectRatio(par);
    }
#endif
    // TODO: resolution and PAR before 7.0.
    const AVFPSRange fps = qt_current_framerates(m_captureDevice, m_videoConnection);
    settings.setMinimumFrameRate(fps.first);
    settings.setMaximumFrameRate(fps.second);

    if (NSObject *obj = [m_videoOutput.videoSettings objectForKey:(id)kCVPixelBufferPixelFormatTypeKey]) {
        if ([obj isKindOfClass:[NSNumber class]]) {
            NSNumber *nsNum = static_cast<NSNumber *>(obj);
            settings.setPixelFormat(QtPixelFormatFromCVFormat([nsNum unsignedIntValue]));
        }
    }

    return settings;
}

void AVFCameraViewfinderSettingsControl2::setViewfinderSettings(const QCameraViewfinderSettings &settings)
{
    if (settings.isNull()) {
        qDebugCamera() << Q_FUNC_INFO << "empty viewfinder settings";
        return;
    }

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
#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
    if (QSysInfo::MacintoshVersion >= qt_OS_limit(QSysInfo::MV_10_7, QSysInfo::MV_IOS_7_0)) {
        Q_ASSERT(m_captureDevice);
        Q_ASSERT(m_session);

        const QSize &resolution = settings.resolution();
        if (!resolution.isNull() && resolution.isValid()) {
            // Either the exact match (including high resolution for images on iOS)
            // or a format with a resolution close to the requested one.
            return qt_find_best_resolution_match(m_captureDevice, resolution,
                                                 m_session->defaultCodec());
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
            return qt_find_best_framerate_match(m_captureDevice, maxFPS ? maxFPS : minFPS,
                                                m_session->defaultCodec());
        // Ignore PAR for the moment (PAR without resolution can
        // pick a format with really bad resolution).
        // No need to test pixel format, just return settings.
    }
#endif
    return nil;
}

bool AVFCameraViewfinderSettingsControl2::convertPixelFormatIfSupported(QVideoFrame::PixelFormat qtFormat, unsigned &avfFormat)const
{
    Q_ASSERT(m_videoOutput);

    unsigned conv = 0;
    if (!CVPixelFormatFromQtFormat(qtFormat, conv))
        return false;

    NSArray *formats = [m_videoOutput availableVideoCVPixelFormatTypes];
    if (!formats || !formats.count)
        return false;

    for (NSObject *obj in formats) {
        if (![obj isKindOfClass:[NSNumber class]])
            continue;
        NSNumber *nsNum = static_cast<NSNumber *>(obj);
        if ([nsNum unsignedIntValue] == conv) {
            avfFormat = conv;
            return true;
        }
    }

    return false;
}

void AVFCameraViewfinderSettingsControl2::applySettings()
{
    if (m_settings.isNull())
        return;

    if (!updateAVFoundationObjects())
        return;

    if (m_session->state() != QCamera::LoadedState &&
        m_session->state() != QCamera::ActiveState) {
        return;
    }

    NSMutableDictionary *videoSettings = [NSMutableDictionary dictionaryWithCapacity:1];
#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_7_0)
    AVCaptureDeviceFormat *match = findBestFormatMatch(m_settings);
    if (match) {
        if (match != m_captureDevice.activeFormat) {
            const AVFConfigurationLock lock(m_captureDevice);
            if (!lock) {
                qDebugCamera() << Q_FUNC_INFO << "failed to lock for configuration";
                return;
            }

            m_captureDevice.activeFormat = match;
        }
    } else {
        qDebugCamera() << Q_FUNC_INFO << "matching device format not found";
        // We still can update the pixel format at least.
    }
#endif

    unsigned avfPixelFormat = 0;
    if (m_settings.pixelFormat() != QVideoFrame::Format_Invalid  &&
        convertPixelFormatIfSupported(m_settings.pixelFormat(), avfPixelFormat)) {
        [videoSettings setObject:[NSNumber numberWithUnsignedInt:avfPixelFormat]
                          forKey:(id)kCVPixelBufferPixelFormatTypeKey];
    } else {
        // We have to set the pixel format, otherwise AVFoundation can change it to something we do not support.
        if (NSObject *oldFormat = [m_videoOutput.videoSettings objectForKey:(id)kCVPixelBufferPixelFormatTypeKey]) {
            [videoSettings setObject:oldFormat forKey:(id)kCVPixelBufferPixelFormatTypeKey];
        } else {
            [videoSettings setObject:[NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA]
                              forKey:(id)kCVPixelBufferPixelFormatTypeKey];
        }
    }

    if (videoSettings.count)
        m_videoOutput.videoSettings = videoSettings;

    qt_set_framerate_limits(m_captureDevice, m_videoConnection, m_settings);
}

QCameraViewfinderSettings AVFCameraViewfinderSettingsControl2::requestedSettings() const
{
    return m_settings;
}

bool AVFCameraViewfinderSettingsControl2::updateAVFoundationObjects() const
{
    m_session = 0;
    m_captureDevice = 0;
    m_videoOutput = 0;
    m_videoConnection = 0;

    if (!m_service->session())
        return false;

    if (!m_service->session()->videoCaptureDevice())
        return false;

    if (!m_service->videoOutput() || !m_service->videoOutput()->videoDataOutput())
        return false;

    AVCaptureVideoDataOutput *output = m_service->videoOutput()->videoDataOutput();
    AVCaptureConnection *connection = [output connectionWithMediaType:AVMediaTypeVideo];
    if (!connection)
        return false;

    m_session = m_service->session();
    m_captureDevice = m_session->videoCaptureDevice();
    m_videoOutput = output;
    m_videoConnection = connection;

    return true;
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
