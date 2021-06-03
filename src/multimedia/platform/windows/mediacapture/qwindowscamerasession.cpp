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

#include "qwindowscamerasession_p.h"

#include "qwindowscamerareader_p.h"
#include "qwindowsmultimediautils_p.h"
#include <qvideosink.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QWindowsCameraSession::QWindowsCameraSession(QObject *parent)
    : QObject(parent)
{
    m_cameraReader = new QWindowsCameraReader(this);
    connect(m_cameraReader, SIGNAL(streamingStarted()), this, SLOT(handleStreamingStarted()));
    connect(m_cameraReader, SIGNAL(streamingStopped()), this, SLOT(handleStreamingStopped()));
    connect(m_cameraReader, SIGNAL(streamingError(int)), this, SLOT(handleStreamingError(int)));
    connect(m_cameraReader, SIGNAL(newVideoFrame(QVideoFrame)), this, SLOT(handleNewVideoFrame(QVideoFrame)));
    connect(m_cameraReader, SIGNAL(recordingStarted()), this, SIGNAL(recordingStarted()));
    connect(m_cameraReader, SIGNAL(recordingStopped()), this, SIGNAL(recordingStopped()));
    connect(m_cameraReader, SIGNAL(durationChanged(qint64)), this, SIGNAL(durationChanged(qint64)));
}

QWindowsCameraSession::~QWindowsCameraSession()
{
    delete m_cameraReader;
}

bool QWindowsCameraSession::isActive() const
{
    return m_active;
}

void QWindowsCameraSession::setActive(bool active)
{
    if (m_active == active)
        return;

    if (active) {
        auto camId = QString::fromUtf8(m_activeCameraInfo.id());
        auto micId = QString::fromUtf8(m_audioInput.id());
        if (!camId.isEmpty() || !micId.isEmpty())
            m_cameraReader->activate(camId, micId);
        else
            qWarning() << Q_FUNC_INFO << "Camera ID and Microphone ID both undefined.";
    } else {
        m_cameraReader->deactivate();
        m_active = false;
        emit activeChanged(m_active);
        emit readyForCaptureChanged(m_active);
    }
}

void QWindowsCameraSession::setActiveCamera(const QCameraInfo &info)
{
    m_activeCameraInfo = info;
}

bool QWindowsCameraSession::isReadyForCapture() const
{
    return m_active;
}

void QWindowsCameraSession::setVideoSink(QVideoSink *surface)
{
    m_surface = surface;
}

void QWindowsCameraSession::handleStreamingStarted()
{
    m_active = true;
    emit activeChanged(m_active);
    emit readyForCaptureChanged(m_active);
}

void QWindowsCameraSession::handleStreamingStopped()
{
    m_active = false;
    emit activeChanged(m_active);
    emit readyForCaptureChanged(m_active);
}

void QWindowsCameraSession::handleStreamingError(int errorCode)
{
    if (m_surface)
        emit m_surface->newVideoFrame(QVideoFrame());
    emit streamingError(errorCode);
}

void QWindowsCameraSession::handleNewVideoFrame(const QVideoFrame &frame)
{
    if (m_surface)
        emit m_surface->newVideoFrame(frame);
    emit newVideoFrame(frame);
}

QMediaEncoderSettings QWindowsCameraSession::videoSettings() const
{
    return m_mediaEncoderSettings;
}

void QWindowsCameraSession::setVideoSettings(const QMediaEncoderSettings &settings)
{
    m_mediaEncoderSettings = settings;
}

bool QWindowsCameraSession::isMuted() const
{
    return m_cameraReader->isMuted();
}

void QWindowsCameraSession::setMuted(bool muted)
{
    m_cameraReader->setMuted(muted);
}

qreal QWindowsCameraSession::volume() const
{
    return m_cameraReader->volume();
}

void QWindowsCameraSession::setVolume(qreal volume)
{
    m_cameraReader->setVolume(volume);
}

QAudioDeviceInfo QWindowsCameraSession::audioInput() const
{
    return m_audioInput;
}

bool QWindowsCameraSession::setAudioInput(const QAudioDeviceInfo &info)
{
    m_audioInput = info;
    return true;
}

bool QWindowsCameraSession::startRecording(const QString &fileName)
{
    GUID container = QWindowsMultimediaUtils::containerForVideoFileFormat(m_mediaEncoderSettings.mediaFormat().fileFormat());
    GUID videoFormat = QWindowsMultimediaUtils::videoFormatForCodec(m_mediaEncoderSettings.videoCodec());
    GUID audioFormat = QWindowsMultimediaUtils::audioFormatForCodec(m_mediaEncoderSettings.audioCodec());

    QSize res = m_mediaEncoderSettings.videoResolution();
    UINT32 width, height;
    if (res.width() > 0 && res.height() > 0) {
        width = UINT32(res.width());
        height = UINT32(res.height());
    } else {
        width = m_cameraReader->frameWidth();
        height = m_cameraReader->frameHeight();
    }

    qreal fps = m_mediaEncoderSettings.videoFrameRate();
    qreal frameRate = (fps > 0) ? fps : m_cameraReader->frameRate();

    auto quality = m_mediaEncoderSettings.quality();
    int vbrate = m_mediaEncoderSettings.videoBitRate();
    int abrate = m_mediaEncoderSettings.audioBitRate();

    UINT32 videoBitRate = (vbrate > 0) ? UINT32(vbrate)
                                       : estimateVideoBitRate(videoFormat, width, height, frameRate, quality);

    UINT32 audioBitRate = (abrate > 0) ? UINT32(abrate)
                                       : estimateAudioBitRate(audioFormat, quality);

    return m_cameraReader->startRecording(fileName, container, videoFormat,
                                          videoBitRate, width, height, frameRate,
                                          audioFormat, audioBitRate);
}

void QWindowsCameraSession::stopRecording()
{
    m_cameraReader->stopRecording();
}

bool QWindowsCameraSession::pauseRecording()
{
    return m_cameraReader->pauseRecording();
}

bool QWindowsCameraSession::resumeRecording()
{
    return m_cameraReader->resumeRecording();
}

// empirical estimate of the required video bitrate (for H.264)
quint32 QWindowsCameraSession::estimateVideoBitRate(const GUID &videoFormat, quint32 width, quint32 height,
                                                   qreal frameRate, QMediaEncoderSettings::Quality quality)
{
    Q_UNUSED(videoFormat);

    qreal bitsPerPixel;
    switch (quality) {
    case QMediaEncoderSettings::Quality::VeryLowQuality:
        bitsPerPixel = 0.08;
        break;
    case QMediaEncoderSettings::Quality::LowQuality:
        bitsPerPixel = 0.2;
        break;
    case QMediaEncoderSettings::Quality::NormalQuality:
        bitsPerPixel = 0.3;
        break;
    case QMediaEncoderSettings::Quality::HighQuality:
        bitsPerPixel = 0.5;
        break;
    case QMediaEncoderSettings::Quality::VeryHighQuality:
        bitsPerPixel = 0.8;
        break;
    default:
        bitsPerPixel = 0.3;
    }

    // Required bitrate is not linear on the number of pixels; small resolutions
    // require more BPP, thus the minimum values, to try to compensate it.
    quint32 pixelsPerSec = quint32(qMax(width, 320u) * qMax(height, 240u) * qMax(frameRate, 6.0));
    return pixelsPerSec * bitsPerPixel;
}

quint32 QWindowsCameraSession::estimateAudioBitRate(const GUID &audioFormat, QMediaEncoderSettings::Quality quality)
{
    if (audioFormat == MFAudioFormat_AAC) {
        // Bitrates supported by the AAC encoder are 96K, 128K, 160K, 192K.
        switch (quality) {
        case QMediaEncoderSettings::Quality::VeryLowQuality:
            return 96000;
        case QMediaEncoderSettings::Quality::LowQuality:
            return 96000;
        case QMediaEncoderSettings::Quality::NormalQuality:
            return 128000;
        case QMediaEncoderSettings::Quality::HighQuality:
            return 160000;
        case QMediaEncoderSettings::Quality::VeryHighQuality:
            return 192000;
        }
        return 128000;
    } else if (audioFormat == MFAudioFormat_MP3) {
        // Bitrates supported by the MP3 encoder are
        // 32K, 40K, 48K, 56K, 64K, 80K, 96K, 112K, 128K, 160K, 192K, 224K, 256K, 320K.
        switch (quality) {
        case QMediaEncoderSettings::Quality::VeryLowQuality:
            return 48000;
        case QMediaEncoderSettings::Quality::LowQuality:
            return 96000;
        case QMediaEncoderSettings::Quality::NormalQuality:
            return 128000;
        case QMediaEncoderSettings::Quality::HighQuality:
            return 224000;
        case QMediaEncoderSettings::Quality::VeryHighQuality:
            return 320000;
        }
        return 128000;
    }
    return 0;  // Use default for format
}

QT_END_NAMESPACE
