/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    if (encoder) {
        encoder->finalize();
        delete encoder;
    }
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

qint64 QFFmpegMediaRecorder::duration() const
{
    return 0;
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

//    if (m_audioSource)
//        m_audioSource->suspend();
    // ####
    stateChanged(QMediaRecorder::PausedState);
}

void QFFmpegMediaRecorder::resume()
{
    if (!m_session || state() != QMediaRecorder::PausedState)
        return;

//    if (m_audioSource)
//        m_audioSource->resume();
    // ####
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
    if (encoder) {
        encoder->finalize();
        delete encoder;
        encoder = nullptr;
    }
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
