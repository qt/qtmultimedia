/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qwindowsmediaencoder_p.h"

#include "qwindowsmediadevicesession_p.h"
#include "qwindowsmediacapture_p.h"
#include <QtCore/QUrl>
#include <QtCore/QMimeType>
#include <Mferror.h>

QT_BEGIN_NAMESPACE

QWindowsMediaEncoder::QWindowsMediaEncoder(QMediaEncoder *parent)
    : QObject(parent),
    QPlatformMediaEncoder(parent)
{
    // Force an early initialization of the mime database
    // to avoid a delay when recording for the first time.
    m_settings.mimeType();
}

QUrl QWindowsMediaEncoder::outputLocation() const
{
    return m_outputLocation;
}

bool QWindowsMediaEncoder::setOutputLocation(const QUrl &location)
{
    m_outputLocation = location;
    return location.scheme() == QLatin1String("file") || location.scheme().isEmpty();
}

QMediaEncoder::RecorderState QWindowsMediaEncoder::state() const
{
    return m_state;
}

QMediaEncoder::Status QWindowsMediaEncoder::status() const
{
    return m_lastStatus;
}

qint64 QWindowsMediaEncoder::duration() const
{
    return m_duration;
}

void QWindowsMediaEncoder::applySettings()
{
    if (!m_mediaDeviceSession)
        return;

    const auto flag = m_mediaDeviceSession->activeCamera().isNull()
            ? QMediaFormat::NoFlags
            : QMediaFormat::RequiresVideo;

    m_settings.resolveFormat(flag);
    m_mediaDeviceSession->setVideoSettings(m_settings);
}

void QWindowsMediaEncoder::setState(QMediaEncoder::RecorderState state)
{
    if (!m_captureService || !m_mediaDeviceSession) {
        qWarning() << Q_FUNC_INFO << "Encoder is not set to a capture session";
        return;
    }

    if (state == m_state)
        return;

    switch (state) {
    case QMediaEncoder::RecordingState:
    {
        m_mediaDeviceSession->setActive(true);

        if (!m_mediaDeviceSession->isActive() && !m_mediaDeviceSession->isActivating()) {
            error(QMediaEncoder::ResourceError, tr("Failed to start recording"));
            return;
        }

        if (m_state == QMediaEncoder::PausedState) {
            if (m_mediaDeviceSession->resumeRecording()) {
                m_state = QMediaEncoder::RecordingState;
                stateChanged(m_state);
            } else {
                error(QMediaEncoder::FormatError, tr("Failed to resume recording"));
            }
        } else {

            applySettings();

            const bool audioOnly = m_settings.videoCodec() == QMediaFormat::VideoCodec::Unspecified;

            const QString path = (m_outputLocation.scheme() == QLatin1String("file") ?
                                      m_outputLocation.path() : m_outputLocation.toString());

            QString fileName = m_storageLocation.generateFileName(path, audioOnly
                                                                  ? QWindowsStorageLocation::Audio
                                                                  : QWindowsStorageLocation::Video,
                                                                  QLatin1String("clip_"),
                                                                  m_settings.mimeType().preferredSuffix());

            if (m_mediaDeviceSession->startRecording(fileName, audioOnly)) {

                m_state = QMediaEncoder::RecordingState;
                m_lastStatus = QMediaEncoder::StartingStatus;

                actualLocationChanged(QUrl::fromLocalFile(fileName));
                stateChanged(m_state);
                statusChanged(m_lastStatus);

            } else {
                error(QMediaEncoder::FormatError, tr("Failed to start recording"));
            }
        }
    } break;
    case QMediaEncoder::PausedState:
    {
        if (m_state == QMediaEncoder::RecordingState) {
            if (m_mediaDeviceSession->pauseRecording()) {
                m_state = QMediaEncoder::PausedState;
                stateChanged(m_state);
            } else {
                error(QMediaEncoder::FormatError, tr("Failed to pause recording"));
            }
        }
    } break;
    case QMediaEncoder::StoppedState:
    {
        m_mediaDeviceSession->stopRecording();
        m_lastStatus = QMediaEncoder::FinalizingStatus;
        statusChanged(m_lastStatus);
        // state will change in onRecordingStopped()
    } break;
    }
}

void QWindowsMediaEncoder::setEncoderSettings(const QMediaEncoderSettings &settings)
{
    m_settings = settings;
}

void QWindowsMediaEncoder::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QWindowsMediaCaptureService *captureSession = static_cast<QWindowsMediaCaptureService *>(session);
    if (m_captureService == captureSession)
        return;

    if (m_captureService)
        setState(QMediaEncoder::StoppedState);

    m_captureService = captureSession;
    if (!m_captureService) {
        m_mediaDeviceSession = nullptr;
        return;
    }

    m_mediaDeviceSession = m_captureService->session();
    Q_ASSERT(m_mediaDeviceSession);

    connect(m_mediaDeviceSession, &QWindowsMediaDeviceSession::recordingStarted, this, &QWindowsMediaEncoder::onRecordingStarted);
    connect(m_mediaDeviceSession, &QWindowsMediaDeviceSession::recordingStopped, this, &QWindowsMediaEncoder::onRecordingStopped);
    connect(m_mediaDeviceSession, &QWindowsMediaDeviceSession::streamingError, this, &QWindowsMediaEncoder::onStreamingError);
    connect(m_mediaDeviceSession, &QWindowsMediaDeviceSession::durationChanged, this, &QWindowsMediaEncoder::onDurationChanged);
    connect(m_captureService, &QWindowsMediaCaptureService::cameraChanged, this, &QWindowsMediaEncoder::onCameraChanged);
    onCameraChanged();
}

void QWindowsMediaEncoder::onDurationChanged(qint64 duration)
{
    m_duration = duration;
    durationChanged(m_duration);
}

void QWindowsMediaEncoder::onStreamingError(int errorCode)
{
    if (errorCode == MF_E_VIDEO_RECORDING_DEVICE_INVALIDATED)
        error(QMediaEncoder::ResourceError, tr("Camera is no longer present"));
    else
        error(QMediaEncoder::ResourceError, tr("Streaming error"));

    if (m_state != QMediaEncoder::StoppedState) {
        m_mediaDeviceSession->stopRecording();
        m_lastStatus = QMediaEncoder::FinalizingStatus;
        statusChanged(m_lastStatus);
    }
}

void QWindowsMediaEncoder::onCameraChanged()
{
}

void QWindowsMediaEncoder::onRecordingStarted()
{
    m_lastStatus = QMediaEncoder::RecordingStatus;
    statusChanged(m_lastStatus);
}

void QWindowsMediaEncoder::onRecordingStopped()
{
    auto lastState = m_state;
    auto lastStatus = m_lastStatus;
    m_state = QMediaEncoder::StoppedState;
    m_lastStatus = QMediaEncoder::StoppedStatus;
    if (m_state != lastState)
        stateChanged(m_state);
    if (m_lastStatus != lastStatus)
        statusChanged(m_lastStatus);
}

QT_END_NAMESPACE
