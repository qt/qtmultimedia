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


#include "avfmediarecordercontrol_ios.h"
#include "avfcamerarenderercontrol.h"
#include "avfcamerasession.h"
#include "avfcameracontrol.h"
#include "avfcameraservice.h"
#include "avfcameradebug.h"

#include <QtCore/qdebug.h>

QT_USE_NAMESPACE

namespace {

bool qt_is_writable_file_URL(NSURL *fileURL)
{
    Q_ASSERT(fileURL);

    if (![fileURL isFileURL])
        return false;

    if (NSString *path = [[fileURL path] stringByExpandingTildeInPath]) {
        return [[NSFileManager defaultManager]
                isWritableFileAtPath:[path stringByDeletingLastPathComponent]];
    }

    return false;
}

bool qt_file_exists(NSURL *fileURL)
{
    Q_ASSERT(fileURL);

    if (NSString *path = [[fileURL path] stringByExpandingTildeInPath])
        return [[NSFileManager defaultManager] fileExistsAtPath:path];

    return false;
}

}

AVFMediaRecorderControlIOS::AVFMediaRecorderControlIOS(AVFCameraService *service, QObject *parent)
    : QMediaRecorderControl(parent)
    , m_service(service)
    , m_state(QMediaRecorder::StoppedState)
    , m_lastStatus(QMediaRecorder::UnloadedStatus)
{
    Q_ASSERT(service);

    m_writerQueue.reset(dispatch_queue_create("asset-writer-queue", DISPATCH_QUEUE_SERIAL));
    if (!m_writerQueue) {
        qDebugCamera() << Q_FUNC_INFO << "failed to create an asset writer's queue";
        return;
    }

    m_writer.reset([[QT_MANGLE_NAMESPACE(AVFMediaAssetWriter) alloc] initWithQueue:m_writerQueue delegate:this]);
    if (!m_writer) {
        qDebugCamera() << Q_FUNC_INFO << "failed to create an asset writer";
        return;
    }

    AVFCameraControl *cameraControl = m_service->cameraControl();
    if (!cameraControl) {
        qDebugCamera() << Q_FUNC_INFO << "camera control is nil";
        return;
    }

    connect(cameraControl, SIGNAL(captureModeChanged(QCamera::CaptureModes)),
                           SLOT(captureModeChanged(QCamera::CaptureModes)));
    connect(cameraControl, SIGNAL(statusChanged(QCamera::Status)),
                           SLOT(cameraStatusChanged(QCamera::Status)));
}

AVFMediaRecorderControlIOS::~AVFMediaRecorderControlIOS()
{
    [m_writer abort];
}

QUrl AVFMediaRecorderControlIOS::outputLocation() const
{
    return m_outputLocation;
}

bool AVFMediaRecorderControlIOS::setOutputLocation(const QUrl &location)
{
    m_outputLocation = location;
    return location.scheme() == QLatin1String("file") || location.scheme().isEmpty();
}

QMediaRecorder::State AVFMediaRecorderControlIOS::state() const
{
    return m_state;
}

QMediaRecorder::Status AVFMediaRecorderControlIOS::status() const
{
    return m_lastStatus;
}

qint64 AVFMediaRecorderControlIOS::duration() const
{
    return m_writer.data()->m_durationInMs.load();
}

bool AVFMediaRecorderControlIOS::isMuted() const
{
    return false;
}

qreal AVFMediaRecorderControlIOS::volume() const
{
    return 1.;
}

void AVFMediaRecorderControlIOS::applySettings()
{
}

void AVFMediaRecorderControlIOS::setState(QMediaRecorder::State state)
{
    Q_ASSERT(m_service->session()
             && m_service->session()->captureSession());

    if (!m_writer) {
        qDebugCamera() << Q_FUNC_INFO << "Invalid recorder";
        return;
    }

    if (state == m_state)
        return;

    switch (state) {
    case QMediaRecorder::RecordingState:
    {
        AVFCameraControl *cameraControl = m_service->cameraControl();
        Q_ASSERT(cameraControl);

        if (!(cameraControl->captureMode() & QCamera::CaptureVideo)) {
            qDebugCamera() << Q_FUNC_INFO << "wrong capture mode, CaptureVideo expected";
            Q_EMIT error(QMediaRecorder::ResourceError, tr("Failed to start recording"));
            return;
        }

        if (cameraControl->status() != QCamera::ActiveStatus) {
            qDebugCamera() << Q_FUNC_INFO << "can not start record while camera is not active";
            Q_EMIT error(QMediaRecorder::ResourceError, tr("Failed to start recording"));
            return;
        }

        const QString path(m_outputLocation.scheme() == QLatin1String("file") ?
                           m_outputLocation.path() : m_outputLocation.toString());
        const QUrl fileURL(QUrl::fromLocalFile(m_storageLocation.generateFileName(path, QCamera::CaptureVideo,
                           QLatin1String("clip_"), QLatin1String("mp4"))));

        NSURL *nsFileURL = fileURL.toNSURL();
        if (!nsFileURL) {
            qWarning() << Q_FUNC_INFO << "invalid output URL:" << fileURL;
            Q_EMIT error(QMediaRecorder::ResourceError, tr("Invalid output file URL"));
            return;
        }
        if (!qt_is_writable_file_URL(nsFileURL)) {
            qWarning() << Q_FUNC_INFO << "invalid output URL:" << fileURL
                       << "(the location is not writable)";
            Q_EMIT error(QMediaRecorder::ResourceError, tr("Non-writeable file location"));
            return;
        }
        if (qt_file_exists(nsFileURL)) {
            // We test for/handle this error here since AWAssetWriter will raise an
            // Objective-C exception, which is not good at all.
            qWarning() << Q_FUNC_INFO << "invalid output URL:" << fileURL
                       << "(file already exists)";
            Q_EMIT error(QMediaRecorder::ResourceError, tr("File already exists"));
            return;
        }

        AVCaptureSession *session = m_service->session()->captureSession();
        // We stop session now so that no more frames for renderer's queue
        // generated, will restart in assetWriterStarted.
        [session stopRunning];

        if ([m_writer setupWithFileURL:nsFileURL cameraService:m_service]) {
            m_state = QMediaRecorder::RecordingState;
            m_lastStatus = QMediaRecorder::StartingStatus;

            Q_EMIT actualLocationChanged(fileURL);
            Q_EMIT stateChanged(m_state);
            Q_EMIT statusChanged(m_lastStatus);

            dispatch_async(m_writerQueue, ^{
                [m_writer start];
            });
        } else {
            [session startRunning];
            Q_EMIT error(QMediaRecorder::FormatError, tr("Failed to start recording"));
        }
    } break;
    case QMediaRecorder::PausedState:
    {
        Q_EMIT error(QMediaRecorder::FormatError, tr("Recording pause not supported"));
        return;
    } break;
    case QMediaRecorder::StoppedState:
    {
        // Do not check the camera status, we can stop if we started.
        stopWriter();
    }
    }
}

void AVFMediaRecorderControlIOS::setMuted(bool muted)
{
    Q_UNUSED(muted)
    qDebugCamera() << Q_FUNC_INFO << "not implemented";
}

void AVFMediaRecorderControlIOS::setVolume(qreal volume)
{
    Q_UNUSED(volume);
    qDebugCamera() << Q_FUNC_INFO << "not implemented";
}

void AVFMediaRecorderControlIOS::assetWriterStarted()
{
    m_lastStatus = QMediaRecorder::RecordingStatus;
    Q_EMIT statusChanged(QMediaRecorder::RecordingStatus);
}

void AVFMediaRecorderControlIOS::assetWriterFinished()
{
    AVFCameraControl *cameraControl = m_service->cameraControl();
    Q_ASSERT(cameraControl);

    const QMediaRecorder::Status lastStatus = m_lastStatus;

    if (cameraControl->captureMode() & QCamera::CaptureVideo)
        m_lastStatus = QMediaRecorder::LoadedStatus;
    else
        m_lastStatus = QMediaRecorder::UnloadedStatus;

    m_service->videoOutput()->resetCaptureDelegate();
    [m_service->session()->captureSession() startRunning];

    if (m_lastStatus != lastStatus)
        Q_EMIT statusChanged(m_lastStatus);
}

void AVFMediaRecorderControlIOS::captureModeChanged(QCamera::CaptureModes newMode)
{
    AVFCameraControl *cameraControl = m_service->cameraControl();
    Q_ASSERT(cameraControl);

    const QMediaRecorder::Status lastStatus = m_lastStatus;

    if (newMode & QCamera::CaptureVideo) {
        if (cameraControl->status() == QCamera::ActiveStatus)
            m_lastStatus = QMediaRecorder::LoadedStatus;
    } else {
        if (m_lastStatus == QMediaRecorder::RecordingStatus)
           return stopWriter();
        else
            m_lastStatus = QMediaRecorder::UnloadedStatus;
    }

    if (m_lastStatus != lastStatus)
        Q_EMIT statusChanged(m_lastStatus);
}

void AVFMediaRecorderControlIOS::cameraStatusChanged(QCamera::Status newStatus)
{
    AVFCameraControl *cameraControl = m_service->cameraControl();
    Q_ASSERT(cameraControl);

    const QMediaRecorder::Status lastStatus = m_lastStatus;
    const bool isCapture = cameraControl->captureMode() & QCamera::CaptureVideo;
    if (newStatus == QCamera::StartingStatus) {
        if (isCapture && m_lastStatus == QMediaRecorder::UnloadedStatus)
            m_lastStatus = QMediaRecorder::LoadingStatus;
    } else if (newStatus == QCamera::ActiveStatus) {
        if (isCapture && m_lastStatus == QMediaRecorder::LoadingStatus)
            m_lastStatus = QMediaRecorder::LoadedStatus;
    } else {
        if (m_lastStatus == QMediaRecorder::RecordingStatus)
            return stopWriter();
        if (newStatus == QCamera::UnloadedStatus)
            m_lastStatus = QMediaRecorder::UnloadedStatus;
    }

    if (lastStatus != m_lastStatus)
        Q_EMIT statusChanged(m_lastStatus);
}

void AVFMediaRecorderControlIOS::stopWriter()
{
    if (m_lastStatus == QMediaRecorder::RecordingStatus) {
        m_state = QMediaRecorder::StoppedState;
        m_lastStatus = QMediaRecorder::FinalizingStatus;

        Q_EMIT stateChanged(m_state);
        Q_EMIT statusChanged(m_lastStatus);

        dispatch_async(m_writerQueue, ^{
            [m_writer stop];
        });
    }
}

#include "moc_avfmediarecordercontrol_ios.cpp"
