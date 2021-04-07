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
#include "avfcamerasession_p.h"
#include "avfcameraservice_p.h"
#include "avfcamera_p.h"
#include "avfcamerarenderer_p.h"
#include "avfcameraimagecapture_p.h"
#include "avfmediaencoder_p.h"
#include "avfcamerautility_p.h"
#include <private/avfvideosink_p.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qurl.h>
#include <QtCore/qelapsedtimer.h>

#include <QtCore/qdebug.h>

QT_USE_NAMESPACE

@interface AVFCameraSessionObserver : NSObject

- (AVFCameraSessionObserver *) initWithCameraSession:(AVFCameraSession*)session;
- (void) processRuntimeError:(NSNotification *)notification;
- (void) processSessionStarted:(NSNotification *)notification;
- (void) processSessionStopped:(NSNotification *)notification;

@end

@implementation AVFCameraSessionObserver
{
@private
    AVFCameraSession *m_session;
    AVCaptureSession *m_captureSession;
}

- (AVFCameraSessionObserver *) initWithCameraSession:(AVFCameraSession*)session
{
    if (!(self = [super init]))
        return nil;

    self->m_session = session;
    self->m_captureSession = session->captureSession();

    [m_captureSession retain];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(processRuntimeError:)
                                                 name:AVCaptureSessionRuntimeErrorNotification
                                               object:m_captureSession];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(processSessionStarted:)
                                                 name:AVCaptureSessionDidStartRunningNotification
                                               object:m_captureSession];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(processSessionStopped:)
                                                 name:AVCaptureSessionDidStopRunningNotification
                                               object:m_captureSession];

    return self;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AVCaptureSessionRuntimeErrorNotification
                                                  object:m_captureSession];

    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AVCaptureSessionDidStartRunningNotification
                                                  object:m_captureSession];

    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AVCaptureSessionDidStopRunningNotification
                                                  object:m_captureSession];
    [m_captureSession release];
    [super dealloc];
}

- (void) processRuntimeError:(NSNotification *)notification
{
    Q_UNUSED(notification);
    QMetaObject::invokeMethod(m_session, "processRuntimeError", Qt::AutoConnection);
}

- (void) processSessionStarted:(NSNotification *)notification
{
    Q_UNUSED(notification);
    QMetaObject::invokeMethod(m_session, "processSessionStarted", Qt::AutoConnection);
}

- (void) processSessionStopped:(NSNotification *)notification
{
    Q_UNUSED(notification);
    QMetaObject::invokeMethod(m_session, "processSessionStopped", Qt::AutoConnection);
}

@end

AVFCameraSession::AVFCameraSession(AVFCameraService *service, QObject *parent)
   : QObject(parent)
   , m_service(service)
   , m_defaultCodec(0)
{
    m_captureSession = [[AVCaptureSession alloc] init];
    m_observer = [[AVFCameraSessionObserver alloc] initWithCameraSession:this];
}

AVFCameraSession::~AVFCameraSession()
{
    if (m_videoSink) {
        m_videoSink->setLayer(nil);
    }

    if (m_videoInput) {
        [m_captureSession removeInput:m_videoInput];
        [m_videoInput release];
    }

    if (m_audioInput) {
        [m_captureSession removeInput:m_audioInput];
        [m_audioInput release];
    }

    if (m_audioOutput) {
        [m_captureSession removeOutput:m_audioOutput];
        [m_audioOutput release];
    }

    if (m_videoOutput)
        delete m_videoOutput;

    [m_observer release];
    [m_captureSession release];
}

void AVFCameraSession::setActiveCamera(const QCameraInfo &info)
{
    if (m_activeCameraInfo != info) {
        m_activeCameraInfo = info;
        [m_captureSession beginConfiguration];
        attachVideoInputDevice();
        if (!m_videoOutput)
            setVideoOutput(new AVFCameraRenderer(this));
        [m_captureSession commitConfiguration];
    }
}

void AVFCameraSession::setVideoOutput(AVFCameraRenderer *output)
{
    if (m_videoOutput == output)
        return;

    delete m_videoOutput;
    m_videoOutput = output;
    if (output)
        output->configureAVCaptureSession(this);
}

void AVFCameraSession::setAudioOutput()
{
    if (m_audioOutput) {
        [m_captureSession removeOutput:m_audioOutput];
        [m_audioOutput release];
        m_audioOutput = nullptr;
    }

    m_audioOutput = [[AVCaptureAudioDataOutput alloc] init];
    if (m_audioOutput && [m_captureSession canAddOutput:m_audioOutput]) {
        [m_captureSession addOutput:m_audioOutput];
    } else {
        qWarning() << Q_FUNC_INFO << "failed to add audio output";
    }
}

AVCaptureDevice *AVFCameraSession::videoCaptureDevice() const
{
    if (m_videoInput)
        return m_videoInput.device;

    return nullptr;
}

AVCaptureDevice *AVFCameraSession::audioCaptureDevice() const
{
    if (m_audioInput)
        return m_audioInput.device;

    return nullptr;
}

bool AVFCameraSession::isActive() const
{
    return m_active;
}

void AVFCameraSession::setActive(bool active)
{
    if (m_active == active)
        return;

    qDebugCamera() << Q_FUNC_INFO << m_active << " -> " << active;

    if (active) {
        [m_captureSession beginConfiguration];

        if (m_service->audioCaptureDevice()) {
            attachAudioInputDevice();
            setAudioOutput();
        }
        if (!m_activeCameraInfo.isNull()) {
            attachVideoInputDevice();
            Q_EMIT readyToConfigureConnections();
            m_defaultCodec = 0;
            defaultCodec();
        }

        bool activeFormatSet = applyImageEncoderSettings();

        [m_captureSession commitConfiguration];

        if (activeFormatSet) {
        // According to the doc, the capture device must be locked before
        // startRunning to prevent the format we set to be overridden by the
        // session preset.
            [videoCaptureDevice() lockForConfiguration:nil];
        }
        [m_captureSession startRunning];

        if (activeFormatSet)
            [videoCaptureDevice() unlockForConfiguration];
    } else {
        [m_captureSession stopRunning];
    }
}

void AVFCameraSession::processRuntimeError()
{
    qWarning() << tr("Runtime camera error");
    m_active = false;
    Q_EMIT error(QCamera::CameraError, tr("Runtime camera error"));
}

void AVFCameraSession::processSessionStarted()
{
    qDebugCamera() << Q_FUNC_INFO;
    if (!m_active) {
        m_active = true;
        Q_EMIT activeChanged(m_active);
    }
}

void AVFCameraSession::processSessionStopped()
{
    qDebugCamera() << Q_FUNC_INFO;
    if (m_active) {
        m_active = false;
        Q_EMIT activeChanged(m_active);
    }
}

AVCaptureDevice *AVFCameraSession::createVideoCaptureDevice()
{
    AVCaptureDevice *device = nullptr;

    QByteArray deviceId = m_activeCameraInfo.id();
    if (!deviceId.isEmpty()) {
        device = [AVCaptureDevice deviceWithUniqueID:
                    [NSString stringWithUTF8String:
                        deviceId.constData()]];
    }

    return device;
}

void AVFCameraSession::attachVideoInputDevice()
{
    //Attach video input device:
    if (m_videoInput) {
        [m_captureSession removeInput:m_videoInput];
        [m_videoInput release];
        m_videoInput = nullptr;
    }

    AVCaptureDevice *videoDevice = createVideoCaptureDevice();
    if (!videoDevice) {
        m_activeCameraInfo = QCameraInfo();
        return;
    }

    NSError *error = nil;
    m_videoInput = [AVCaptureDeviceInput
            deviceInputWithDevice:videoDevice
            error:&error];

    if (!m_videoInput) {
        qWarning() << "Failed to create video device input";
    } else {
        if ([m_captureSession canAddInput:m_videoInput]) {
            [m_videoInput retain];
            [m_captureSession addInput:m_videoInput];
        } else {
            qWarning() << "Failed to connect video device input";
            m_activeCameraInfo = QCameraInfo();
        }
    }
}

void AVFCameraSession::attachAudioInputDevice()
{
    //Attach audio input device:
    if (m_audioInput) {
        [m_captureSession removeInput:m_audioInput];
        [m_audioInput release];
        m_audioInput = nullptr;
    }

    AVCaptureDevice *audioDevice = m_service->audioCaptureDevice();

    NSError *error = nil;
    m_audioInput = [AVCaptureDeviceInput
            deviceInputWithDevice:audioDevice
            error:&error];

    if (!m_audioInput) {
        qWarning() << "Failed to create audio device input";
    } else {
        if ([m_captureSession canAddInput:m_audioInput]) {
            [m_audioInput retain];
            [m_captureSession addInput:m_audioInput];
        } else {
            qWarning() << "Failed to connect audio device input";
        }
    }
}

bool AVFCameraSession::applyImageEncoderSettings()
{
    if (AVFCameraImageCapture *control = m_service->avfImageCaptureControl())
        return control->applySettings();

    return false;
}

bool AVFCameraSession::applyEncoderSettings()
{
    if (AVFMediaEncoder *encoder = m_service->recorderControl()) {
        encoder->applySettings();
        return true;
    }
    return false;
}

FourCharCode AVFCameraSession::defaultCodec()
{
    if (!m_defaultCodec) {
        if (AVCaptureDevice *device = videoCaptureDevice()) {
            AVCaptureDeviceFormat *format = device.activeFormat;
            if (!format || !format.formatDescription)
                return m_defaultCodec;
            m_defaultCodec = CMVideoFormatDescriptionGetCodecType(format.formatDescription);
        }
    }
    return m_defaultCodec;
}

void AVFCameraSession::setVideoSink(QVideoSink *sink)
{
    auto *videoSink = sink ? static_cast<AVFVideoSink *>(sink->platformVideoSink()) : nullptr;

    if (m_videoSink == videoSink)
        return;

    if (m_videoSink)
        m_videoSink->setLayer(nil);

    m_videoSink = videoSink;

    if (m_videoSink && m_videoOutput) {
        m_videoOutput->setVideoSink(videoSink);
        AVCaptureVideoPreviewLayer *previewLayer = [AVCaptureVideoPreviewLayer layerWithSession:m_captureSession];
        m_videoOutput->setLayer(previewLayer);
    }
}

#include "moc_avfcamerasession_p.cpp"
