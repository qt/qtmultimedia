/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "avfcameradebug.h"
#include "avfcamerasession.h"
#include "avfcameraservice.h"
#include "avfcameracontrol.h"
#include "avfvideorenderercontrol.h"
#include "avfvideodevicecontrol.h"
#include "avfaudioinputselectorcontrol.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qurl.h>
#include <QtCore/qelapsedtimer.h>

#include <QtCore/qdebug.h>

QT_USE_NAMESPACE

int AVFCameraSession::m_defaultCameraIndex;
QList<AVFCameraInfo> AVFCameraSession::m_cameraDevices;

@interface AVFCameraSessionObserver : NSObject
{
@private
    AVFCameraSession *m_session;
    AVCaptureSession *m_captureSession;
}

- (AVFCameraSessionObserver *) initWithCameraSession:(AVFCameraSession*)session;
- (void) processRuntimeError:(NSNotification *)notification;
- (void) processSessionStarted:(NSNotification *)notification;
- (void) processSessionStopped:(NSNotification *)notification;

@end

@implementation AVFCameraSessionObserver

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
   , m_state(QCamera::UnloadedState)
   , m_active(false)
   , m_videoInput(nil)
{
    m_captureSession = [[AVCaptureSession alloc] init];
    m_observer = [[AVFCameraSessionObserver alloc] initWithCameraSession:this];

    //configuration is commited during transition to Active state
    [m_captureSession beginConfiguration];
}

AVFCameraSession::~AVFCameraSession()
{
    if (m_videoInput) {
        [m_captureSession removeInput:m_videoInput];
        [m_videoInput release];
    }

    [m_observer release];
    [m_captureSession release];
}

int AVFCameraSession::defaultCameraIndex()
{
    updateCameraDevices();
    return m_defaultCameraIndex;
}

const QList<AVFCameraInfo> &AVFCameraSession::availableCameraDevices()
{
    updateCameraDevices();
    return m_cameraDevices;
}

AVFCameraInfo AVFCameraSession::cameraDeviceInfo(const QByteArray &device)
{
    updateCameraDevices();

    Q_FOREACH (const AVFCameraInfo &info, m_cameraDevices) {
        if (info.deviceId == device)
            return info;
    }

    return AVFCameraInfo();
}

void AVFCameraSession::updateCameraDevices()
{
#ifdef Q_OS_IOS
    // Cameras can't change dynamically on iOS. Update only once.
    if (!m_cameraDevices.isEmpty())
        return;
#else
    // On OS X, cameras can be added or removed. Update the list every time, but not more than
    // once every 500 ms
    static QElapsedTimer timer;
    if (timer.isValid() && timer.elapsed() < 500) // ms
        return;
#endif

    m_defaultCameraIndex = -1;
    m_cameraDevices.clear();

    AVCaptureDevice *defaultDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    NSArray *videoDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice *device in videoDevices) {
        if (defaultDevice && [defaultDevice.uniqueID isEqualToString:device.uniqueID])
            m_defaultCameraIndex = m_cameraDevices.count();

        AVFCameraInfo info;
        info.deviceId = QByteArray([[device uniqueID] UTF8String]);
        info.description = QString::fromNSString([device localizedName]);

        // There is no API to get the camera sensor orientation, however, cameras are always
        // mounted in landscape on iDevices.
        //   - Back-facing cameras have the top side of the sensor aligned with the right side of
        //     the screen when held in portrait ==> 270 degrees clockwise angle
        //   - Front-facing cameras have the top side of the sensor aligned with the left side of
        //     the screen when held in portrait ==> 270 degrees clockwise angle
        // On Mac OS, the position will always be unspecified and the sensor orientation unknown.
        switch (device.position) {
        case AVCaptureDevicePositionBack:
            info.position = QCamera::BackFace;
            info.orientation = 270;
            break;
        case AVCaptureDevicePositionFront:
            info.position = QCamera::FrontFace;
            info.orientation = 270;
            break;
        default:
            info.position = QCamera::UnspecifiedPosition;
            info.orientation = 0;
            break;
        }

        m_cameraDevices.append(info);
    }

#ifndef Q_OS_IOS
    timer.restart();
#endif
}

void AVFCameraSession::setVideoOutput(AVFVideoRendererControl *output)
{
    m_videoOutput = output;
    if (output)
        output->configureAVCaptureSession(this);
}

AVCaptureDevice *AVFCameraSession::videoCaptureDevice() const
{
    if (m_videoInput)
        return m_videoInput.device;

    return 0;
}

QCamera::State AVFCameraSession::state() const
{
    if (m_active)
        return QCamera::ActiveState;

    return m_state == QCamera::ActiveState ? QCamera::LoadedState : m_state;
}

void AVFCameraSession::setState(QCamera::State newState)
{
    if (m_state == newState)
        return;

    qDebugCamera() << Q_FUNC_INFO << m_state << " -> " << newState;

    QCamera::State oldState = m_state;
    m_state = newState;

    //attach video input during Unloaded->Loaded transition
    if (oldState == QCamera::UnloadedState)
        attachVideoInputDevice();

    if (m_state == QCamera::ActiveState) {
        Q_EMIT readyToConfigureConnections();
        [m_captureSession commitConfiguration];
        [m_captureSession startRunning];
    }

    if (oldState == QCamera::ActiveState) {
        [m_captureSession stopRunning];
        [m_captureSession beginConfiguration];
    }

    Q_EMIT stateChanged(m_state);
}

void AVFCameraSession::processRuntimeError()
{
    qWarning() << tr("Runtime camera error");
    Q_EMIT error(QCamera::CameraError, tr("Runtime camera error"));
}

void AVFCameraSession::processSessionStarted()
{
    qDebugCamera() << Q_FUNC_INFO;
    if (!m_active) {
        m_active = true;
        Q_EMIT activeChanged(m_active);
        Q_EMIT stateChanged(state());
    }
}

void AVFCameraSession::processSessionStopped()
{
    qDebugCamera() << Q_FUNC_INFO;
    if (m_active) {
        m_active = false;
        Q_EMIT activeChanged(m_active);
        Q_EMIT stateChanged(state());
    }
}

void AVFCameraSession::attachVideoInputDevice()
{
    //Attach video input device:
    if (m_service->videoDeviceControl()->isDirty()) {
        if (m_videoInput) {
            [m_captureSession removeInput:m_videoInput];
            [m_videoInput release];
            m_videoInput = 0;
        }

        AVCaptureDevice *videoDevice = m_service->videoDeviceControl()->createCaptureDevice();

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
            }
        }
    }
}

#include "moc_avfcamerasession.cpp"
