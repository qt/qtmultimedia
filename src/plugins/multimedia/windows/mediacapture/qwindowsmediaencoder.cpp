// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsmediaencoder_p.h"

#include "qwindowsmediadevicesession_p.h"
#include "qwindowsmediacapture_p.h"
#include "mfmetadata_p.h"
#include <QtCore/QUrl>
#include <mferror.h>
#include <shobjidl.h>
#include <private/qmediastoragelocation_p.h>
#include <private/qmediarecorder_p.h>

QT_BEGIN_NAMESPACE

QWindowsMediaEncoder::QWindowsMediaEncoder(QMediaRecorder *parent)
    : QObject(parent),
      QPlatformMediaRecorder(parent)
{
}

bool QWindowsMediaEncoder::isLocationWritable(const QUrl &location) const
{
    return location.scheme() == QLatin1String("file") || location.scheme().isEmpty();
}

QMediaRecorder::RecorderState QWindowsMediaEncoder::state() const
{
    return m_state;
}

qint64 QWindowsMediaEncoder::duration() const
{
    return m_duration;
}

void QWindowsMediaEncoder::record(QMediaEncoderSettings &settings)
{
    if (!m_captureService || !m_mediaDeviceSession) {
        qWarning() << Q_FUNC_INFO << "Encoder is not set to a capture session";
        return;
    }
    if (m_state != QMediaRecorder::StoppedState)
        return;

    m_sessionWasActive = m_mediaDeviceSession->isActive() || m_mediaDeviceSession->isActivating();

    if (!m_sessionWasActive) {

        m_mediaDeviceSession->setActive(true);

        if (!m_mediaDeviceSession->isActivating()) {
            updateError(QMediaRecorder::ResourceError,
                        QMediaRecorderPrivate::msgFailedStartRecording());
            return;
        }
    }

    const auto audioOnly = settings.videoCodec() == QMediaFormat::VideoCodec::Unspecified;
    m_fileName = QMediaStorageLocation::generateFileName(outputLocation().toLocalFile(),
                                                         audioOnly ? QStandardPaths::MusicLocation
                                                                   : QStandardPaths::MoviesLocation,
                                                         settings.preferredSuffix());

    QMediaRecorder::Error ec = m_mediaDeviceSession->startRecording(settings, m_fileName, audioOnly);
    if (ec == QMediaRecorder::NoError) {
        m_state = QMediaRecorder::RecordingState;

        actualLocationChanged(QUrl::fromLocalFile(m_fileName));
        stateChanged(m_state);

    } else {
        updateError(ec, QMediaRecorderPrivate::msgFailedStartRecording());
    }
}

void QWindowsMediaEncoder::pause()
{
    if (!m_mediaDeviceSession || m_state != QMediaRecorder::RecordingState)
        return;

    if (m_mediaDeviceSession->pauseRecording()) {
        m_state = QMediaRecorder::PausedState;
        stateChanged(m_state);
    } else {
        updateError(QMediaRecorder::FormatError, tr("Failed to pause recording"));
    }
}

void QWindowsMediaEncoder::resume()
{
    if (!m_mediaDeviceSession || m_state != QMediaRecorder::PausedState)
        return;

    if (m_mediaDeviceSession->resumeRecording()) {
        m_state = QMediaRecorder::RecordingState;
        stateChanged(m_state);
    } else {
        updateError(QMediaRecorder::FormatError, tr("Failed to resume recording"));
    }
}

void QWindowsMediaEncoder::stop()
{
    if (m_mediaDeviceSession && m_state != QMediaRecorder::StoppedState) {
        m_mediaDeviceSession->stopRecording();
        if (!m_sessionWasActive)
            m_mediaDeviceSession->setActive(false);
    }
}



void QWindowsMediaEncoder::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QWindowsMediaCaptureService *captureSession = static_cast<QWindowsMediaCaptureService *>(session);
    if (m_captureService == captureSession)
        return;

    if (m_captureService)
        stop();

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
    connect(m_mediaDeviceSession, &QWindowsMediaDeviceSession::recordingError, this, &QWindowsMediaEncoder::onRecordingError);
    connect(m_mediaDeviceSession, &QWindowsMediaDeviceSession::durationChanged, this, &QWindowsMediaEncoder::onDurationChanged);
    connect(m_captureService, &QWindowsMediaCaptureService::cameraChanged, this, &QWindowsMediaEncoder::onCameraChanged);
    onCameraChanged();
}

void QWindowsMediaEncoder::setMetaData(const QMediaMetaData &metaData)
{
    m_metaData = metaData;
}

QMediaMetaData QWindowsMediaEncoder::metaData() const
{
    return m_metaData;
}

void QWindowsMediaEncoder::saveMetadata()
{
    if (!m_metaData.isEmpty()) {

        const QString nativeFileName = QDir::toNativeSeparators(m_fileName);

        IPropertyStore *store = nullptr;

        if (SUCCEEDED(SHGetPropertyStoreFromParsingName(reinterpret_cast<LPCWSTR>(nativeFileName.utf16()),
                                                        nullptr, GPS_READWRITE, IID_PPV_ARGS(&store)))) {

            MFMetaData::toNative(m_metaData, store);

            store->Commit();
            store->Release();
        }
    }
}

void QWindowsMediaEncoder::onDurationChanged(qint64 duration)
{
    m_duration = duration;
    durationChanged(m_duration);
}

void QWindowsMediaEncoder::onStreamingError(int errorCode)
{
    if (errorCode == MF_E_VIDEO_RECORDING_DEVICE_INVALIDATED)
        updateError(QMediaRecorder::ResourceError, tr("Camera is no longer present"));
    else if (errorCode == MF_E_AUDIO_RECORDING_DEVICE_INVALIDATED)
        updateError(QMediaRecorder::ResourceError, tr("Audio input is no longer present"));
    else
        updateError(QMediaRecorder::ResourceError, tr("Streaming error"));

    if (m_state != QMediaRecorder::StoppedState) {
        m_mediaDeviceSession->stopRecording();
        if (!m_sessionWasActive)
            m_mediaDeviceSession->setActive(false);
    }
}

void QWindowsMediaEncoder::onRecordingError(int errorCode)
{
    Q_UNUSED(errorCode);
    updateError(QMediaRecorder::ResourceError, tr("Recording error"));

    auto lastState = m_state;
    m_state = QMediaRecorder::StoppedState;
    if (m_state != lastState)
        stateChanged(m_state);
}

void QWindowsMediaEncoder::onCameraChanged()
{
}

void QWindowsMediaEncoder::onRecordingStarted()
{
}

void QWindowsMediaEncoder::onRecordingStopped()
{
    saveMetadata();

    auto lastState = m_state;
    m_state = QMediaRecorder::StoppedState;
    if (m_state != lastState)
        stateChanged(m_state);
}

QT_END_NAMESPACE

#include "moc_qwindowsmediaencoder_p.cpp"
