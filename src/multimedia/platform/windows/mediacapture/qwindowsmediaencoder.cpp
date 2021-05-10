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

#include "qwindowscamerasession_p.h"
#include "qwindowsmediacapture_p.h"
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

QWindowsMediaEncoder::QWindowsMediaEncoder(QMediaEncoder *parent)
    : QPlatformMediaEncoder(parent)
{
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

QMediaEncoder::State QWindowsMediaEncoder::state() const
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
    if (m_cameraSession)
        m_cameraSession->setVideoSettings(m_settings);
}

void QWindowsMediaEncoder::setState(QMediaEncoder::State state)
{
    if (!m_captureService || !m_cameraSession) {
        qWarning() << Q_FUNC_INFO << "Encoder is not set to a capture session";
        return;
    }

    if (state == m_state)
        return;

    switch (state) {
    case QMediaEncoder::RecordingState:
    {
        if (!m_cameraSession->isActive()) {
            emit error(QMediaEncoder::ResourceError, tr("Failed to start recording"));
            return;
        }

        if (m_state == QMediaEncoder::PausedState) {
            if (m_cameraSession->resumeRecording()) {
                m_state = QMediaEncoder::RecordingState;
                emit stateChanged(m_state);
            } else {
                emit error(QMediaEncoder::FormatError, tr("Failed to resume recording"));
            }
        } else {

            const QString path = (m_outputLocation.scheme() == QLatin1String("file") ?
                                      m_outputLocation.path() : m_outputLocation.toString());

            auto encoderSettings = m_settings;
            encoderSettings.resolveFormat();

            QString fileName = m_storageLocation.generateFileName(path, QWindowsStorageLocation::Video,
                               QLatin1String("clip_"), encoderSettings.mimeType().preferredSuffix());

            applySettings();

            if (m_cameraSession->startRecording(fileName)) {

                m_state = QMediaEncoder::RecordingState;
                m_lastStatus = QMediaEncoder::StartingStatus;

                emit actualLocationChanged(QUrl::fromLocalFile(fileName));
                emit stateChanged(m_state);
                emit statusChanged(m_lastStatus);

            } else {
                emit error(QMediaEncoder::FormatError, tr("Failed to start recording"));
            }
        }
    } break;
    case QMediaEncoder::PausedState:
    {
        if (m_state == QMediaEncoder::RecordingState) {
            if (m_cameraSession->pauseRecording()) {
                m_state = QMediaEncoder::PausedState;
                emit stateChanged(m_state);
            } else {
                emit error(QMediaEncoder::FormatError, tr("Failed to pause recording"));
            }
        }
    } break;
    case QMediaEncoder::StoppedState:
    {
        m_cameraSession->stopRecording();
        m_lastStatus = QMediaEncoder::FinalizingStatus;
        emit statusChanged(m_lastStatus);
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
        m_cameraSession = nullptr;
        return;
    }

    m_cameraSession = m_captureService->session();
    Q_ASSERT(m_cameraSession);

    connect(m_cameraSession, &QWindowsCameraSession::recordingStarted, this, &QWindowsMediaEncoder::onRecordingStarted);
    connect(m_cameraSession, &QWindowsCameraSession::recordingStopped, this, &QWindowsMediaEncoder::onRecordingStopped);
    connect(m_cameraSession, &QWindowsCameraSession::durationChanged, this, &QWindowsMediaEncoder::onDurationChanged);
    connect(m_captureService, &QWindowsMediaCaptureService::cameraChanged, this, &QWindowsMediaEncoder::onCameraChanged);
    onCameraChanged();
}

void QWindowsMediaEncoder::onDurationChanged(qint64 duration)
{
    m_duration = duration;
    emit durationChanged(m_duration);
}

void QWindowsMediaEncoder::onCameraChanged()
{
}

void QWindowsMediaEncoder::onRecordingStarted()
{
    m_lastStatus = QMediaEncoder::RecordingStatus;
    emit statusChanged(m_lastStatus);
}

void QWindowsMediaEncoder::onRecordingStopped()
{
    auto lastState = m_state;
    auto lastStatus = m_lastStatus;
    m_state = QMediaEncoder::StoppedState;
    m_lastStatus = QMediaEncoder::StoppedStatus;
    if (m_state != lastState)
        emit stateChanged(m_state);
    if (m_lastStatus != lastStatus)
        emit statusChanged(m_lastStatus);
}

QT_END_NAMESPACE
