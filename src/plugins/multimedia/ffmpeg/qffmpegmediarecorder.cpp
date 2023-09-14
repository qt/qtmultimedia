// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediarecorder_p.h"
#include "qaudiodevice.h"
#include <private/qmediastoragelocation_p.h>
#include <private/qplatformcamera_p.h>
#include <private/qplatformsurfacecapture_p.h>
#include "qaudiosource.h"
#include "qffmpegaudioinput_p.h"
#include "qaudiobuffer.h"
#include "qffmpegencoder_p.h"
#include "qffmpegmediacapturesession_p.h"

#include <qdebug.h>
#include <qloggingcategory.h>

static Q_LOGGING_CATEGORY(qLcMediaEncoder, "qt.multimedia.ffmpeg.encoder");

QT_BEGIN_NAMESPACE

QFFmpegMediaRecorder::QFFmpegMediaRecorder(QMediaRecorder *parent) : QPlatformMediaRecorder(parent)
{
}

QFFmpegMediaRecorder::~QFFmpegMediaRecorder() = default;

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

    auto videoSources = m_session->activeVideoSources();
    const auto hasVideo = !videoSources.empty();
    const auto hasAudio = m_session->audioInput() != nullptr;

    if (!hasVideo && !hasAudio) {
        error(QMediaRecorder::ResourceError, QMediaRecorder::tr("No video or audio input"));
        return;
    }

    const auto audioOnly = settings.videoCodec() == QMediaFormat::VideoCodec::Unspecified;

    auto primaryLocation = audioOnly ? QStandardPaths::MusicLocation : QStandardPaths::MoviesLocation;
    auto suffix = settings.mimeType().preferredSuffix();
    QString location = QMediaStorageLocation::generateFileName(outputLocation().toString(QUrl::PreferLocalFile), primaryLocation, suffix);
    qCDebug(qLcMediaEncoder) << "recording new video to" << location;
    qCDebug(qLcMediaEncoder) << "requested format:" << settings.fileFormat() << settings.audioCodec();

    Q_ASSERT(!location.isEmpty());

    m_encoder.reset(new Encoder(settings, location));
    m_encoder->setMetaData(m_metaData);
    connect(m_encoder.get(), &QFFmpeg::Encoder::durationChanged, this,
            &QFFmpegMediaRecorder::newDuration);
    connect(m_encoder.get(), &QFFmpeg::Encoder::finalizationDone, this,
            &QFFmpegMediaRecorder::finalizationDone);
    connect(m_encoder.get(), &QFFmpeg::Encoder::error, this,
            &QFFmpegMediaRecorder::handleSessionError);

    auto *audioInput = m_session->audioInput();
    if (audioInput) {
        if (audioInput->device.isNull())
            qWarning() << "Audio input device is null; cannot encode audio";
        else
            m_encoder->addAudioInput(static_cast<QFFmpegAudioInput *>(audioInput));
    }

    for (auto source : videoSources)
        m_encoder->addVideoSource(source);

    durationChanged(0);
    stateChanged(QMediaRecorder::RecordingState);
    actualLocationChanged(QUrl::fromLocalFile(location));

    m_encoder->start();
}

void QFFmpegMediaRecorder::pause()
{
    if (!m_session || state() != QMediaRecorder::RecordingState)
        return;

    Q_ASSERT(m_encoder);
    m_encoder->setPaused(true);

    stateChanged(QMediaRecorder::PausedState);
}

void QFFmpegMediaRecorder::resume()
{
    if (!m_session || state() != QMediaRecorder::PausedState)
        return;

    Q_ASSERT(m_encoder);
    m_encoder->setPaused(false);

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

    m_encoder.reset();
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

void QFFmpegMediaRecorder::setCaptureSession(QFFmpegMediaCaptureSession *session)
{
    auto *captureSession = session;
    if (m_session == captureSession)
        return;

    if (m_session)
        stop();

    m_session = captureSession;
    if (!m_session)
        return;
}

void QFFmpegMediaRecorder::EncoderDeleter::operator()(Encoder *encoder) const
{
    // ### all of the below should be done asynchronous. finalize() should do it's work in a thread
    // to avoid blocking the UI in case of slow codecs
    encoder->finalize();
}

QT_END_NAMESPACE

#include "moc_qffmpegmediarecorder_p.cpp"
