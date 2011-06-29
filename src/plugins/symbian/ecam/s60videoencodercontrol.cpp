/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "s60videoencodercontrol.h"
#include "s60videocapturesession.h"

S60VideoEncoderControl::S60VideoEncoderControl(QObject *parent) :
    QVideoEncoderControl(parent)
{
}

S60VideoEncoderControl::S60VideoEncoderControl(S60VideoCaptureSession *session, QObject *parent) :
    QVideoEncoderControl(parent)
{
    m_session = session;
}

S60VideoEncoderControl::~S60VideoEncoderControl()
{
}

QStringList S60VideoEncoderControl::supportedVideoCodecs() const
{
    return m_session->supportedVideoCaptureCodecs();
}

QString S60VideoEncoderControl::videoCodecDescription(const QString &codecName) const
{
    return m_session->videoCaptureCodecDescription(codecName);
}

QList<qreal> S60VideoEncoderControl::supportedFrameRates(const QVideoEncoderSettings &settings, bool *continuous) const
{
    if (!settings.isNull())
        return m_session->supportedVideoFrameRates(settings, continuous);
    return m_session->supportedVideoFrameRates(continuous);
}

QList<QSize> S60VideoEncoderControl::supportedResolutions(const QVideoEncoderSettings &settings, bool *continuous) const
{
    if (!settings.isNull())
        return m_session->supportedVideoResolutions(settings, continuous);
    return m_session->supportedVideoResolutions(continuous);
}

QStringList S60VideoEncoderControl::supportedEncodingOptions(const QString &codec) const
{
    // Possible settings: EncodingMode, Codec, Resolution, FrameRate, BitRate, Quality
    // Possible (codec specific) options: PixelAspectRatio, Gain, MaxClipSizeInBytes

    // Following options are valid for all codecs
    Q_UNUSED(codec);

    QStringList options;
    options.append("pixelAspectRatio");
    options.append("gain");
    options.append("maxClipSizeInBytes");

    return options;
}

QVariant S60VideoEncoderControl::encodingOption(const QString &codec, const QString &name) const
{
    Q_UNUSED(codec);

    // Possible settings: EncodingMode, Codec, Resolution, FrameRate, BitRate, Quality
    // Possible (codec specific) options: PixelAspectRatio, Gain, MaxClipSizeInBytes

    QVariant returnValue;

    if (qstrcmp(name.toLocal8Bit().constData(), "pixelAspectRatio") == 0)
        returnValue.setValue(m_session->pixelAspectRatio());
    else if (qstrcmp(name.toLocal8Bit().constData(), "gain") == 0)
        returnValue.setValue((int)m_session->gain());
    else if (qstrcmp(name.toLocal8Bit().constData(), "maxClipSizeInBytes") == 0)
        returnValue.setValue(m_session->maxClipSizeInBytes());

    return returnValue;
}

void S60VideoEncoderControl::setEncodingOption(
        const QString &codec, const QString &name, const QVariant &value)
{
    // Set the codec first if not already set
    m_session->setVideoCaptureCodec(codec);

    if (qstrcmp(name.toLocal8Bit().constData(), "pixelAspectRatio") == 0)
        m_session->setPixelAspectRatio(value.toSize());
    else if (qstrcmp(name.toLocal8Bit().constData(), "gain") == 0)
        m_session->setGain(value.toInt());
    else if (qstrcmp(name.toLocal8Bit().constData(), "maxClipSizeInBytes") == 0)
        m_session->setMaxClipSizeInBytes(value.toInt());
    else
        m_session->setError(KErrNotSupported, tr("Requested encoding option is not supported"));
}

QVideoEncoderSettings S60VideoEncoderControl::videoSettings() const
{
    QVideoEncoderSettings settings;
    m_session->videoEncoderSettings(settings);

    return settings;
}

void S60VideoEncoderControl::setVideoSettings(const QVideoEncoderSettings &settings)
{
    // Notify that settings have been implicitly set and there's no need to
    // initialize them in case camera is changed
    m_session->notifySettingsSet();

    if (settings.codec().isEmpty()
        || (settings.resolution() == QSize(-1,-1) && settings.frameRate() == 0 && settings.bitRate() == -1)) {
        if (!settings.codec().isEmpty())
            m_session->setVideoCaptureCodec(settings.codec());
        m_session->setVideoEncodingMode(settings.encodingMode());
        m_session->setVideoCaptureQuality(settings.quality(), S60VideoCaptureSession::EOnlyVideoQuality);
    } else if (settings.resolution() != QSize(-1,-1) && settings.frameRate() == 0 && settings.bitRate() == -1) { // Only Resolution
        m_session->setVideoCaptureCodec(settings.codec());
        m_session->setVideoEncodingMode(settings.encodingMode());
        m_session->setVideoResolution(settings.resolution());
        m_session->setVideoCaptureQuality(settings.quality(), S60VideoCaptureSession::EVideoQualityAndResolution);

    } else if (settings.resolution() == QSize(-1,-1) && settings.frameRate() != 0 && settings.bitRate() == -1) { // Only Framerate
        m_session->setVideoCaptureCodec(settings.codec());
        m_session->setVideoEncodingMode(settings.encodingMode());
        m_session->setFrameRate(settings.frameRate());
        m_session->setVideoCaptureQuality(settings.quality(), S60VideoCaptureSession::EVideoQualityAndFrameRate);

    } else if (settings.resolution() == QSize(-1,-1) && settings.frameRate() == 0 && settings.bitRate() != -1) { // Only BitRate
        m_session->setVideoCaptureCodec(settings.codec());
        m_session->setVideoEncodingMode(settings.encodingMode());
        m_session->setBitrate(settings.bitRate());
        m_session->setVideoCaptureQuality(settings.quality(), S60VideoCaptureSession::EVideoQualityAndBitRate);

    } else if (settings.resolution() != QSize(-1,-1) && settings.frameRate() != 0 && settings.bitRate() == -1) { // Resolution and FrameRate
        m_session->setVideoCaptureCodec(settings.codec());
        m_session->setVideoEncodingMode(settings.encodingMode());
        m_session->setVideoResolution(settings.resolution());
        m_session->setFrameRate(settings.frameRate());
        m_session->setVideoCaptureQuality(settings.quality(), S60VideoCaptureSession::EVideoQualityAndResolutionAndFrameRate);

    } else if (settings.resolution() != QSize(-1,-1) && settings.frameRate() == 0 && settings.bitRate() != -1) { // Resolution and BitRate
        m_session->setVideoCaptureCodec(settings.codec());
        m_session->setVideoEncodingMode(settings.encodingMode());
        m_session->setVideoResolution(settings.resolution());
        m_session->setBitrate(settings.bitRate());
        m_session->setVideoCaptureQuality(settings.quality(), S60VideoCaptureSession::EVideoQualityAndResolutionAndBitRate);

    } else if (settings.resolution() == QSize(-1,-1) && settings.frameRate() != 0 && settings.bitRate() != -1) { // FrameRate and BitRate
        m_session->setVideoCaptureCodec(settings.codec());
        m_session->setVideoEncodingMode(settings.encodingMode());
        m_session->setFrameRate(settings.frameRate());
        m_session->setBitrate(settings.bitRate());
        m_session->setVideoCaptureQuality(settings.quality(), S60VideoCaptureSession::EVideoQualityAndFrameRateAndBitRate);

    } else { // All: Resolution, BitRate and FrameRate
        m_session->setVideoCaptureCodec(settings.codec());
        m_session->setVideoEncodingMode(settings.encodingMode());
        m_session->setVideoResolution(settings.resolution());
        m_session->setFrameRate(settings.frameRate());
        m_session->setBitrate(settings.bitRate());
        m_session->setVideoCaptureQuality(settings.quality(), S60VideoCaptureSession::ENoVideoQuality);
    }
}

// End of file
