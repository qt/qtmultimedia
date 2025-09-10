// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsmediadevicesession_p.h"

#include "qwindowsmediadevicereader_p.h"
#include "private/qwindowsmultimediautils_p.h"
#include "private/qplatformvideosink_p.h"
#include <qvideosink.h>
#include <QtCore/qdebug.h>
#include <qaudioinput.h>
#include <qaudiooutput.h>

#include <cguid.h>

QT_BEGIN_NAMESPACE

QWindowsMediaDeviceSession::QWindowsMediaDeviceSession(QObject *parent)
    : QObject(parent)
{
    m_mediaDeviceReader = new QWindowsMediaDeviceReader(this);
    connect(m_mediaDeviceReader, &QWindowsMediaDeviceReader::streamingStarted,
            this, &QWindowsMediaDeviceSession::handleStreamingStarted);
    connect(m_mediaDeviceReader, &QWindowsMediaDeviceReader::streamingStopped,
            this, &QWindowsMediaDeviceSession::handleStreamingStopped);
    connect(m_mediaDeviceReader, &QWindowsMediaDeviceReader::streamingError,
            this, &QWindowsMediaDeviceSession::handleStreamingError);
    connect(m_mediaDeviceReader, &QWindowsMediaDeviceReader::videoFrameChanged,
            this, &QWindowsMediaDeviceSession::handleVideoFrameChanged);
    connect(m_mediaDeviceReader, &QWindowsMediaDeviceReader::recordingStarted,
            this, &QWindowsMediaDeviceSession::recordingStarted);
    connect(m_mediaDeviceReader, &QWindowsMediaDeviceReader::recordingStopped,
            this, &QWindowsMediaDeviceSession::recordingStopped);
    connect(m_mediaDeviceReader, &QWindowsMediaDeviceReader::recordingError,
            this, &QWindowsMediaDeviceSession::recordingError);
    connect(m_mediaDeviceReader, &QWindowsMediaDeviceReader::durationChanged,
            this, &QWindowsMediaDeviceSession::durationChanged);
}

QWindowsMediaDeviceSession::~QWindowsMediaDeviceSession()
{
    delete m_mediaDeviceReader;
}

bool QWindowsMediaDeviceSession::isActive() const
{
    return m_active;
}

bool QWindowsMediaDeviceSession::isActivating() const
{
    return m_activating;
}

void QWindowsMediaDeviceSession::setActive(bool active)
{
    if ((active && (m_active || m_activating)) || (!active && !m_active && !m_activating))
        return;

    if (active) {
        auto camId = QString::fromUtf8(m_activeCameraDevice.id());
        auto micId = m_audioInput ? QString::fromUtf8(m_audioInput->device().id()) : QString();
        if (!camId.isEmpty() || !micId.isEmpty()) {
            if (m_mediaDeviceReader->activate(camId, m_cameraFormat, micId)) {
                m_activating = true;
            } else {
                emit streamingError(MF_E_NOT_AVAILABLE);
            }
        } else {
            qWarning() << Q_FUNC_INFO << "Camera ID and Microphone ID both undefined.";
        }
    } else {
        m_mediaDeviceReader->deactivate();
        m_active = false;
        m_activating = false;
        emit activeChanged(m_active);
        emit readyForCaptureChanged(m_active);
    }
}

void QWindowsMediaDeviceSession::reactivate()
{
    if (m_active || m_activating) {
        pauseRecording();
        setActive(false);
        setActive(true);
        resumeRecording();
    }
}

void QWindowsMediaDeviceSession::setActiveCamera(const QCameraDevice &camera)
{
    m_activeCameraDevice = camera;
    reactivate();
}

QCameraDevice QWindowsMediaDeviceSession::activeCamera() const
{
    return m_activeCameraDevice;
}

void QWindowsMediaDeviceSession::setCameraFormat(const QCameraFormat &cameraFormat)
{
    m_cameraFormat = cameraFormat;
}

void QWindowsMediaDeviceSession::setVideoSink(QVideoSink *surface)
{
    m_surface = surface;
}

void QWindowsMediaDeviceSession::handleStreamingStarted()
{
    if (m_activating) {
        m_active = true;
        m_activating = false;
        emit activeChanged(m_active);
        emit readyForCaptureChanged(m_active);
    }
}

void QWindowsMediaDeviceSession::handleStreamingStopped()
{
    m_active = false;
    emit activeChanged(m_active);
    emit readyForCaptureChanged(m_active);
}

void QWindowsMediaDeviceSession::handleStreamingError(int errorCode)
{
    if (m_surface)
        m_surface->platformVideoSink()->setVideoFrame(QVideoFrame());
    emit streamingError(errorCode);
}

void QWindowsMediaDeviceSession::handleVideoFrameChanged(const QVideoFrame &frame)
{
    if (m_surface)
        m_surface->platformVideoSink()->setVideoFrame(frame);
    emit videoFrameChanged(frame);
}

void QWindowsMediaDeviceSession::setAudioInputMuted(bool muted)
{
    m_mediaDeviceReader->setInputMuted(muted);
}

void QWindowsMediaDeviceSession::setAudioInputVolume(float volume)
{
    m_mediaDeviceReader->setInputVolume(volume);
}

void QWindowsMediaDeviceSession::audioInputDeviceChanged()
{
    reactivate();
}

void QWindowsMediaDeviceSession::setAudioOutputMuted(bool muted)
{
    m_mediaDeviceReader->setOutputMuted(muted);
}

void QWindowsMediaDeviceSession::setAudioOutputVolume(float volume)
{
    m_mediaDeviceReader->setOutputVolume(volume);
}

void QWindowsMediaDeviceSession::audioOutputDeviceChanged()
{
    if (m_active || m_activating)
        m_mediaDeviceReader->setAudioOutput(QString::fromUtf8(m_audioOutput->device().id()));
}

void QWindowsMediaDeviceSession::setAudioInput(QAudioInput *input)
{
    if (m_audioInput == input)
        return;
    if (m_audioInput)
        m_audioInput->disconnect(this);
    m_audioInput = input;

    audioInputDeviceChanged();

    if (!m_audioInput)
        return;
    connect(m_audioInput, &QAudioInput::mutedChanged, this, &QWindowsMediaDeviceSession::setAudioInputMuted);
    connect(m_audioInput, &QAudioInput::volumeChanged, this, &QWindowsMediaDeviceSession::setAudioInputVolume);
    connect(m_audioInput, &QAudioInput::deviceChanged, this, &QWindowsMediaDeviceSession::audioInputDeviceChanged);
}

void QWindowsMediaDeviceSession::setAudioOutput(QAudioOutput *output)
{
    if (m_audioOutput == output)
        return;
    if (m_audioOutput)
        m_audioOutput->disconnect(this);
    m_audioOutput = output;
    if (!m_audioOutput) {
        m_mediaDeviceReader->setAudioOutput({});
        return;
    }

    m_mediaDeviceReader->setAudioOutput(QString::fromUtf8(m_audioOutput->device().id()));

    connect(m_audioOutput, &QAudioOutput::mutedChanged, this, &QWindowsMediaDeviceSession::setAudioOutputMuted);
    connect(m_audioOutput, &QAudioOutput::volumeChanged, this, &QWindowsMediaDeviceSession::setAudioOutputVolume);
    connect(m_audioOutput, &QAudioOutput::deviceChanged, this, &QWindowsMediaDeviceSession::audioOutputDeviceChanged);
}

QMediaRecorder::Error QWindowsMediaDeviceSession::startRecording(QMediaEncoderSettings &settings, const QString &fileName, bool audioOnly)
{
    GUID container = audioOnly ? QWindowsMultimediaUtils::containerForAudioFileFormat(settings.mediaFormat().fileFormat())
                               : QWindowsMultimediaUtils::containerForVideoFileFormat(settings.mediaFormat().fileFormat());
    GUID videoFormat = QWindowsMultimediaUtils::videoFormatForCodec(settings.videoCodec());
    GUID audioFormat = QWindowsMultimediaUtils::audioFormatForCodec(settings.audioCodec());

    QSize res = settings.videoResolution();
    UINT32 width, height;
    if (res.width() > 0 && res.height() > 0) {
        width = UINT32(res.width());
        height = UINT32(res.height());
    } else {
        width = m_mediaDeviceReader->frameWidth();
        height = m_mediaDeviceReader->frameHeight();
        settings.setVideoResolution(QSize(int(width), int(height)));
    }

    qreal frameRate = settings.videoFrameRate();
    if (frameRate <= 0) {
        frameRate = m_mediaDeviceReader->frameRate();
        settings.setVideoFrameRate(frameRate);
    }

    auto quality = settings.quality();

    UINT32 videoBitRate = 0;
    if (settings.videoBitRate() > 0) {
        videoBitRate = UINT32(settings.videoBitRate());
    } else {
        videoBitRate = estimateVideoBitRate(videoFormat, width, height, frameRate, quality);
        settings.setVideoBitRate(int(videoBitRate));
    }

    UINT32 audioBitRate = 0;
    if (settings.audioBitRate() > 0) {
        audioBitRate = UINT32(settings.audioBitRate());
    } else {
        audioBitRate = estimateAudioBitRate(audioFormat, quality);
        settings.setAudioBitRate(int(audioBitRate));
    }

    return m_mediaDeviceReader->startRecording(fileName, container, audioOnly ? GUID_NULL : videoFormat,
                                               videoBitRate, width, height, frameRate,
                                               audioFormat, audioBitRate);
}

void QWindowsMediaDeviceSession::stopRecording()
{
    m_mediaDeviceReader->stopRecording();
}

bool QWindowsMediaDeviceSession::pauseRecording()
{
    return m_mediaDeviceReader->pauseRecording();
}

bool QWindowsMediaDeviceSession::resumeRecording()
{
    return m_mediaDeviceReader->resumeRecording();
}

// empirical estimate of the required video bitrate (for H.264)
quint32 QWindowsMediaDeviceSession::estimateVideoBitRate(const GUID &videoFormat, quint32 width, quint32 height,
                                                         qreal frameRate, QMediaRecorder::Quality quality)
{
    Q_UNUSED(videoFormat);

    qreal bitsPerPixel;
    switch (quality) {
    case QMediaRecorder::Quality::VeryLowQuality:
        bitsPerPixel = 0.08;
        break;
    case QMediaRecorder::Quality::LowQuality:
        bitsPerPixel = 0.2;
        break;
    case QMediaRecorder::Quality::NormalQuality:
        bitsPerPixel = 0.3;
        break;
    case QMediaRecorder::Quality::HighQuality:
        bitsPerPixel = 0.5;
        break;
    case QMediaRecorder::Quality::VeryHighQuality:
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

quint32 QWindowsMediaDeviceSession::estimateAudioBitRate(const GUID &audioFormat, QMediaRecorder::Quality quality)
{
    if (audioFormat == MFAudioFormat_AAC) {
        // Bitrates supported by the AAC encoder are 96K, 128K, 160K, 192K.
        switch (quality) {
        case QMediaRecorder::Quality::VeryLowQuality:
            return 96000;
        case QMediaRecorder::Quality::LowQuality:
            return 96000;
        case QMediaRecorder::Quality::NormalQuality:
            return 128000;
        case QMediaRecorder::Quality::HighQuality:
            return 160000;
        case QMediaRecorder::Quality::VeryHighQuality:
            return 192000;
        default:
            return 128000;
        }
    } else if (audioFormat == MFAudioFormat_MP3) {
        // Bitrates supported by the MP3 encoder are
        // 32K, 40K, 48K, 56K, 64K, 80K, 96K, 112K, 128K, 160K, 192K, 224K, 256K, 320K.
        switch (quality) {
        case QMediaRecorder::Quality::VeryLowQuality:
            return 48000;
        case QMediaRecorder::Quality::LowQuality:
            return 96000;
        case QMediaRecorder::Quality::NormalQuality:
            return 128000;
        case QMediaRecorder::Quality::HighQuality:
            return 224000;
        case QMediaRecorder::Quality::VeryHighQuality:
            return 320000;
        default:
            return 128000;
        }
    } else if (audioFormat == MFAudioFormat_WMAudioV8) {
        // Bitrates supported by the Windows Media Audio 8 encoder
        switch (quality) {
        case QMediaRecorder::Quality::VeryLowQuality:
            return 32000;
        case QMediaRecorder::Quality::LowQuality:
            return 96000;
        case QMediaRecorder::Quality::NormalQuality:
            return 192000;
        case QMediaRecorder::Quality::HighQuality:
            return 256016;
        case QMediaRecorder::Quality::VeryHighQuality:
            return 320032;
        default:
            return 192000;
        }
    } else if (audioFormat == MFAudioFormat_WMAudioV9) {
        // Bitrates supported by the Windows Media Audio 9 encoder
        switch (quality) {
        case QMediaRecorder::Quality::VeryLowQuality:
            return 32000;
        case QMediaRecorder::Quality::LowQuality:
            return 96000;
        case QMediaRecorder::Quality::NormalQuality:
            return 192000;
        case QMediaRecorder::Quality::HighQuality:
            return 256016;
        case QMediaRecorder::Quality::VeryHighQuality:
            return 384000;
        default:
            return 192000;
        }
    }
    return 0;  // Use default for format
}

QT_END_NAMESPACE

#include "moc_qwindowsmediadevicesession_p.cpp"
