// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediarecorder_p.h"
#include "qaudiodevice.h"
#include <private/qmediastoragelocation_p.h>
#include <private/qplatformcamera_p.h>
#include "qaudiosource.h"
#include "qffmpegaudioinput_p.h"
#include "qaudiobuffer.h"
#include "qffmpegencoder_p.h"
#include "qffmpegmediaformatinfo_p.h"

#include <qdebug.h>
#include <qeventloop.h>
#include <qstandardpaths.h>
#include <qmimetype.h>
#include <qloggingcategory.h>

Q_LOGGING_CATEGORY(qLcMediaEncoder, "qt.multimedia.encoder")

QFFmpegMediaRecorder::QFFmpegMediaRecorder(QMediaRecorder *parent)
  : QPlatformMediaRecorder(parent)
{
}

QFFmpegMediaRecorder::~QFFmpegMediaRecorder()
{
    if (encoder)
        encoder->finalize();
}

bool QFFmpegMediaRecorder::isLocationWritable(const QUrl &) const
{
    return true;
}

void QFFmpegMediaRecorder::handleSessionError(QMediaRecorder::Error code, const QString &description)
{
    error(code, description);
    stop();
}

void QFFmpegMediaRecorder::record(QMediaEncoderSettings &settings)
{
    if (!m_session || state() != QMediaRecorder::StoppedState)
        return;

    const auto hasVideo = m_session->camera() && m_session->camera()->isActive();
    const auto hasAudio = m_session->audioInput() != nullptr;

    if (!hasVideo && !hasAudio) {
        error(QMediaRecorder::ResourceError, QMediaRecorder::tr("No camera or audio input"));
        return;
    }

    const auto audioOnly = settings.videoCodec() == QMediaFormat::VideoCodec::Unspecified;

    auto primaryLocation = audioOnly ? QStandardPaths::MusicLocation : QStandardPaths::MoviesLocation;
    auto container = settings.mimeType().preferredSuffix();
    auto location = QMediaStorageLocation::generateFileName(outputLocation().toLocalFile(), primaryLocation, container);

    QUrl actualSink = QUrl::fromLocalFile(QDir::currentPath()).resolved(location);
    qCDebug(qLcMediaEncoder) << "recording new video to" << actualSink;
    qDebug() << "requested format:" << settings.fileFormat() << settings.audioCodec();

    Q_ASSERT(!actualSink.isEmpty());

    encoder = new QFFmpeg::Encoder(settings, actualSink);
    encoder->setMetaData(m_metaData);
    connect(encoder, &QFFmpeg::Encoder::durationChanged, this, &QFFmpegMediaRecorder::newDuration);
    connect(encoder, &QFFmpeg::Encoder::finalizationDone, this, &QFFmpegMediaRecorder::finalizationDone);
    connect(encoder, &QFFmpeg::Encoder::error, this, &QFFmpegMediaRecorder::handleSessionError);

    auto *audioInput = m_session->audioInput();
    if (audioInput)
        encoder->addAudioInput(static_cast<QFFmpegAudioInput *>(audioInput));

    auto *camera = m_session->camera();
    if (camera)
        encoder->addVideoSource(camera);

    durationChanged(0);
    stateChanged(QMediaRecorder::RecordingState);
    actualLocationChanged(QUrl::fromLocalFile(location));

    encoder->start();
}

void QFFmpegMediaRecorder::pause()
{
    if (!m_session || state() != QMediaRecorder::RecordingState)
        return;

    Q_ASSERT(encoder);
    encoder->setPaused(true);

    stateChanged(QMediaRecorder::PausedState);
}

void QFFmpegMediaRecorder::resume()
{
    if (!m_session || state() != QMediaRecorder::PausedState)
        return;

    Q_ASSERT(encoder);
    encoder->setPaused(false);

    stateChanged(QMediaRecorder::RecordingState);
}

void QFFmpegMediaRecorder::stop()
{
    if (!m_session || state() == QMediaRecorder::StoppedState)
        return;
    auto * input = m_session ? m_session->audioInput() : nullptr;
    if (input)
        static_cast<QFFmpegAudioInput *>(input)->setRunning(false);
    qCDebug(qLcMediaEncoder) << "stop";
    // ### all of the below should be done asynchronous. finalize() should do it's work in a thread
    // to avoid blocking the UI in case of slow codecs
    if (encoder) {
        encoder->finalize();
        encoder = nullptr;
    }
}

void QFFmpegMediaRecorder::finalizationDone()
{
    stateChanged(QMediaRecorder::StoppedState);
}

void QFFmpegMediaRecorder::setMetaData(const QMediaMetaData &metaData)
{
    if (!m_session)
        return;
    m_metaData = metaData;
}

QMediaMetaData QFFmpegMediaRecorder::metaData() const
{
    return m_metaData;
}

void QFFmpegMediaRecorder::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    auto *captureSession = static_cast<QFFmpegMediaCaptureSession *>(session);
    if (m_session == captureSession)
        return;

    if (m_session)
        stop();

    m_session = captureSession;
    if (!m_session)
        return;
}
