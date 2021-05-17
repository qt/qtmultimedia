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
#include "qwindowscameraexposure_p.h"
#include "qwindowscameraimageprocessing_p.h"
#include "qwindowsmultimediautils_p.h"
#include <qvideosink.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QWindowsCameraSession::QWindowsCameraSession(QObject *parent)
    : QObject(parent)
{
    m_cameraReader = new QWindowsCameraReader(this);
    m_cameraExposure = new QWindowsCameraExposure(this);
    m_cameraImageProcessing = new QWindowsCameraImageProcessing(this);
    connect(m_cameraReader, SIGNAL(streamingStarted()), this, SLOT(handleStreamingStarted()));
    connect(m_cameraReader, SIGNAL(streamingStopped()), this, SLOT(handleStreamingStopped()));
    connect(m_cameraReader, SIGNAL(recordingStarted()), this, SIGNAL(recordingStarted()));
    connect(m_cameraReader, SIGNAL(recordingStopped()), this, SIGNAL(recordingStopped()));
    connect(m_cameraReader, SIGNAL(durationChanged(qint64)), this, SIGNAL(durationChanged(qint64)));
}

QWindowsCameraSession::~QWindowsCameraSession()
{
    delete m_cameraImageProcessing;
    delete m_cameraExposure;
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
        m_cameraReader->activate(QString::fromUtf8(m_activeCameraInfo.id()));
    } else {
        m_cameraReader->deactivate();
        m_active = false;
        emit activeChanged(m_active);
    }
}

void QWindowsCameraSession::setActiveCamera(const QCameraInfo &info)
{
    m_activeCameraInfo = info;
}

QImageEncoderSettings QWindowsCameraSession::imageSettings() const
{
    return m_imageEncoderSettings;
}

void QWindowsCameraSession::setImageSettings(const QImageEncoderSettings &settings)
{
    m_imageEncoderSettings = settings;
}

bool QWindowsCameraSession::isReadyForCapture() const
{
    return m_active && m_readyForCapture;
}

void QWindowsCameraSession::setReadyForCapture(bool ready)
{
    if (m_readyForCapture == ready)
        return;

    // Still image capture not yet supported
    // m_readyForCapture = ready;
    // emit readyForCaptureChanged(ready);
}

int QWindowsCameraSession::capture(const QString &fileName)
{
    return 0;
}

void QWindowsCameraSession::setVideoSink(QVideoSink *surface)
{
    m_cameraReader->setSurface(surface);
}

QWindowsCameraExposure *QWindowsCameraSession::exposureControl()
{
    return m_cameraExposure;
}

QWindowsCameraImageProcessing *QWindowsCameraSession::imageProcessingControl()
{
    return m_cameraImageProcessing;
}

void QWindowsCameraSession::handleStreamingStarted()
{
    m_active = true;
    emit activeChanged(m_active);
    setReadyForCapture(true);
}

void QWindowsCameraSession::handleStreamingStopped()
{
    setReadyForCapture(false);
    m_active = false;
    emit activeChanged(m_active);
}

QMediaEncoderSettings QWindowsCameraSession::videoSettings() const
{
    return m_mediaEncoderSettings;
}

void QWindowsCameraSession::setVideoSettings(const QMediaEncoderSettings &settings)
{
    m_mediaEncoderSettings = settings;
}

bool QWindowsCameraSession::startRecording(const QString &fileName)
{
    GUID container = QWindowsMultimediaUtils::containerForVideoFileFormat(m_mediaEncoderSettings.format());
    GUID videoFormat = QWindowsMultimediaUtils::videoFormatForCodec(m_mediaEncoderSettings.videoCodec());

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

    UINT32 videoBitRate;
    if (vbrate > 0)
        videoBitRate = UINT32(vbrate);
    else
        videoBitRate = estimateVideoBitRate(videoFormat, width, height, frameRate, quality);

    return m_cameraReader->startRecording(fileName, container, videoFormat,
                                          videoBitRate, width, height, frameRate);
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

QT_END_NAMESPACE
