/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfcameradebug_p.h"
#include "avfcameracontrol_p.h"
#include "avfcamerasession_p.h"
#include "avfcameraservice_p.h"
#include "avfcamerautility_p.h"
#include "avfcamerarenderercontrol_p.h"
#include "qabstractvideosurface.h"

QT_USE_NAMESPACE

AVFCameraControl::AVFCameraControl(AVFCameraService *service, QObject *parent)
   : QCameraControl(parent)
   , m_session(service->session())
   , m_service(service)
   , m_state(QCamera::UnloadedState)
   , m_lastStatus(QCamera::UnloadedStatus)
   , m_captureMode(QCamera::CaptureStillImage)
{
    Q_UNUSED(service);
    connect(m_session, SIGNAL(stateChanged(QCamera::State)), SLOT(updateStatus()));
    connect(this, &AVFCameraControl::captureModeChanged, m_session, &AVFCameraSession::onCaptureModeChanged);
}

AVFCameraControl::~AVFCameraControl()
{
}

QCamera::State AVFCameraControl::state() const
{
    return m_state;
}

void AVFCameraControl::setState(QCamera::State state)
{
    if (m_state == state)
        return;
    m_state = state;
    m_session->setState(state);

    Q_EMIT stateChanged(m_state);
    updateStatus();
}

QCamera::Status AVFCameraControl::status() const
{
    static QCamera::Status statusTable[3][3] = {
        { QCamera::UnloadedStatus, QCamera::UnloadingStatus, QCamera::StoppingStatus }, //Unloaded state
        { QCamera::LoadingStatus,  QCamera::LoadedStatus,    QCamera::StoppingStatus }, //Loaded state
        { QCamera::LoadingStatus,  QCamera::StartingStatus,  QCamera::ActiveStatus } //ActiveState
    };

    return statusTable[m_state][m_session->state()];
}

void AVFCameraControl::setCamera(const QCameraInfo &camera)
{
    m_session->setActiveCamera(camera);
}

void AVFCameraControl::updateStatus()
{
    QCamera::Status newStatus = status();

    if (m_lastStatus != newStatus) {
        qDebugCamera() << "Camera status changed: " << m_lastStatus << " -> " << newStatus;
        m_lastStatus = newStatus;
        Q_EMIT statusChanged(m_lastStatus);
    }
}

QCamera::CaptureModes AVFCameraControl::captureMode() const
{
    return m_captureMode;
}

void AVFCameraControl::setCaptureMode(QCamera::CaptureModes mode)
{
    if (m_captureMode == mode)
        return;

    m_captureMode = mode;
    Q_EMIT captureModeChanged(mode);
}

bool AVFCameraControl::isCaptureModeSupported(QCamera::CaptureModes /*mode*/) const
{
    return true;
}

bool AVFCameraControl::canChangeProperty(QCameraControl::PropertyChangeType changeType, QCamera::Status status) const
{
    Q_UNUSED(changeType);
    Q_UNUSED(status);

    return true;
}

QCamera::LockTypes AVFCameraControl::supportedLocks() const
{
    return {};
}

QCamera::LockStatus AVFCameraControl::lockStatus(QCamera::LockType) const
{
    return QCamera::Unlocked;
}

void AVFCameraControl::searchAndLock(QCamera::LockTypes locks)
{
    Q_UNUSED(locks);
}

void AVFCameraControl::unlock(QCamera::LockTypes locks)
{
    Q_UNUSED(locks);
}


namespace {

bool qt_framerates_sane(const QCameraViewfinderSettings &settings)
{
    const qreal minFPS = settings.minimumFrameRate();
    const qreal maxFPS = settings.maximumFrameRate();

    if (minFPS < 0. || maxFPS < 0.)
        return false;

    return !maxFPS || maxFPS >= minFPS;
}

}

QList<QCameraViewfinderSettings> AVFCameraControl::supportedViewfinderSettings() const
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

    return supportedSettings;
}

QCameraViewfinderSettings AVFCameraControl::viewfinderSettings() const
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

    const AVFPSRange fps = qt_current_framerates(captureDevice, videoConnection());
    settings.setMinimumFrameRate(fps.first);
    settings.setMaximumFrameRate(fps.second);

    AVCaptureVideoDataOutput *videoOutput = m_service->videoOutput() ? m_service->videoOutput()->videoDataOutput() : nullptr;
    if (videoOutput) {
        NSObject *obj = [videoOutput.videoSettings objectForKey:(id)kCVPixelBufferPixelFormatTypeKey];
        if (obj && [obj isKindOfClass:[NSNumber class]]) {
            NSNumber *nsNum = static_cast<NSNumber *>(obj);
            settings.setPixelFormat(QtPixelFormatFromCVFormat([nsNum unsignedIntValue]));
        }
    }

    return settings;
}

void AVFCameraControl::setViewfinderSettings(const QCameraViewfinderSettings &settings)
{
    if (m_settings == settings)
        return;

    m_settings = settings;
#if defined(Q_OS_IOS)
    bool active = m_service->session()->state() == QCamera::ActiveState;
    if (active)
        [m_service->session()->captureSession() beginConfiguration];
    applySettings(m_settings);
    if (active)
        [m_service->session()->captureSession() commitConfiguration];
#else
    applySettings(m_settings);
#endif
}

QVideoFrame::PixelFormat AVFCameraControl::QtPixelFormatFromCVFormat(unsigned avPixelFormat)
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
    case kCVPixelFormatType_422YpCbCr8:
        return QVideoFrame::Format_UYVY;
    case kCVPixelFormatType_422YpCbCr8_yuvs:
        return QVideoFrame::Format_YUYV;
    case kCMVideoCodecType_JPEG:
    case kCMVideoCodecType_JPEG_OpenDML:
        return QVideoFrame::Format_Jpeg;
    default:
        return QVideoFrame::Format_Invalid;
    }
}

bool AVFCameraControl::CVPixelFormatFromQtFormat(QVideoFrame::PixelFormat qtFormat, unsigned &conv)
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
    case QVideoFrame::Format_UYVY:
        conv = kCVPixelFormatType_422YpCbCr8;
        break;
    case QVideoFrame::Format_YUYV:
        conv = kCVPixelFormatType_422YpCbCr8_yuvs;
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

AVCaptureDeviceFormat *AVFCameraControl::findBestFormatMatch(const QCameraViewfinderSettings &settings) const
{
    AVCaptureDevice *captureDevice = m_service->session()->videoCaptureDevice();
    if (!captureDevice || settings.isNull())
        return nil;

    const QSize &resolution = settings.resolution();
    if (!resolution.isNull() && resolution.isValid()) {
        // Either the exact match (including high resolution for images on iOS)
        // or a format with a resolution close to the requested one.
        return qt_find_best_resolution_match(captureDevice, resolution,
                                             m_service->session()->defaultCodec(), false);
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
        return qt_find_best_framerate_match(captureDevice,
                                            m_service->session()->defaultCodec(),
                                            maxFPS ? maxFPS : minFPS);
    // Ignore PAR for the moment (PAR without resolution can
    // pick a format with really bad resolution).
    // No need to test pixel format, just return settings.

    return nil;
}

QVector<QVideoFrame::PixelFormat> AVFCameraControl::viewfinderPixelFormats() const
{
    QVector<QVideoFrame::PixelFormat> qtFormats;

    AVCaptureVideoDataOutput *videoOutput = m_service->videoOutput() ? m_service->videoOutput()->videoDataOutput() : nullptr;
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

bool AVFCameraControl::convertPixelFormatIfSupported(QVideoFrame::PixelFormat qtFormat,
                                                                        unsigned &avfFormat)const
{
    AVCaptureVideoDataOutput *videoOutput = m_service->videoOutput() ? m_service->videoOutput()->videoDataOutput() : nullptr;
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
        QAbstractVideoBuffer::HandleType h = m_service->videoOutput()->supportsTextures()
                                           ? QAbstractVideoBuffer::GLTextureHandle
                                           : QAbstractVideoBuffer::NoHandle;
        if (!surface->supportedPixelFormats(h).contains(qtFormat))
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

bool AVFCameraControl::applySettings(const QCameraViewfinderSettings &settings)
{
    if (m_service->session()->state() != QCamera::LoadedState &&
        m_service->session()->state() != QCamera::ActiveState) {
        return false;
    }

    AVCaptureDevice *captureDevice = m_service->session()->videoCaptureDevice();
    if (!captureDevice)
        return false;

    bool activeFormatChanged = false;

    AVCaptureDeviceFormat *match = findBestFormatMatch(settings);
    if (match) {
        activeFormatChanged = qt_set_active_format(captureDevice, match, false);
    } else {
        qDebugCamera() << Q_FUNC_INFO << "matching device format not found";
        // We still can update the pixel format at least.
    }

    AVCaptureVideoDataOutput *videoOutput = m_service->videoOutput() ? m_service->videoOutput()->videoDataOutput() : nullptr;
    if (videoOutput) {
        unsigned avfPixelFormat = 0;
        if (!convertPixelFormatIfSupported(settings.pixelFormat(), avfPixelFormat)) {
            // If the the pixel format is not specified or invalid, pick the preferred video surface
            // format, or if no surface is set, the preferred capture device format

            const QVector<QVideoFrame::PixelFormat> deviceFormats = viewfinderPixelFormats();
            QAbstractVideoSurface *surface = m_service->videoOutput()->surface();
            QVideoFrame::PixelFormat pickedFormat = deviceFormats.first();
            if (surface) {
                pickedFormat = QVideoFrame::Format_Invalid;
                QAbstractVideoBuffer::HandleType h = m_service->videoOutput()->supportsTextures()
                                                   ? QAbstractVideoBuffer::GLTextureHandle
                                                   : QAbstractVideoBuffer::NoHandle;
                QList<QVideoFrame::PixelFormat> surfaceFormats = surface->supportedPixelFormats(h);
                for (int i = 0; i < surfaceFormats.count(); ++i) {
                    const QVideoFrame::PixelFormat surfaceFormat = surfaceFormats.at(i);
                    if (deviceFormats.contains(surfaceFormat)) {
                        pickedFormat = surfaceFormat;
                        break;
                    }
                }
            }

            CVPixelFormatFromQtFormat(pickedFormat, avfPixelFormat);
        }

        NSMutableDictionary *videoSettings = [NSMutableDictionary dictionaryWithCapacity:1];
        [videoSettings setObject:[NSNumber numberWithUnsignedInt:avfPixelFormat]
                                  forKey:(id)kCVPixelBufferPixelFormatTypeKey];

        const AVFConfigurationLock lock(captureDevice);
        if (!lock)
            qWarning("Failed to set active format (lock failed)");

        videoOutput.videoSettings = videoSettings;
    }

    qt_set_framerate_limits(captureDevice, videoConnection(), settings.minimumFrameRate(), settings.maximumFrameRate());

    return activeFormatChanged;
}

QCameraViewfinderSettings AVFCameraControl::requestedSettings() const
{
    return m_settings;
}

AVCaptureConnection *AVFCameraControl::videoConnection() const
{
    if (!m_service->videoOutput() || !m_service->videoOutput()->videoDataOutput())
        return nil;

    return [m_service->videoOutput()->videoDataOutput() connectionWithMediaType:AVMediaTypeVideo];
}

#include "moc_avfcameracontrol_p.cpp"
