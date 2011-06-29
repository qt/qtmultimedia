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

#include "s60audioencodercontrol.h"
#include "s60videocapturesession.h"

S60AudioEncoderControl::S60AudioEncoderControl(QObject *parent) :
    QAudioEncoderControl(parent)
{
}

S60AudioEncoderControl::S60AudioEncoderControl(S60VideoCaptureSession *session, QObject *parent) :
    QAudioEncoderControl(parent)
{
    m_session = session;
}

S60AudioEncoderControl::~S60AudioEncoderControl()
{
}

QStringList S60AudioEncoderControl::supportedAudioCodecs() const
{
    return m_session->supportedAudioCaptureCodecs();
}

QString S60AudioEncoderControl::codecDescription(const QString &codecName) const
{
    // According to ForumNokia MMF camcorder plugin supports AAC, AMR and QCELP
    // QCELP is speech codec and can be discarded
    if (qstrcmp(codecName.toLocal8Bit().constData(), "audio/aac") == 0)
        return QLatin1String("Advanced Audio Coding");
    else if (qstrcmp(codecName.toLocal8Bit().constData(), "audio/amr") == 0)
        return QLatin1String("Adaptive Multi-Rate Audio Codec");

    return QString();
}

QStringList S60AudioEncoderControl::supportedEncodingOptions(const QString &codec) const
{
    // Possible settings: EncodingMode, Codec, BitRate, ChannelCount, SampleRate, Quality
    // Possible (codec specific) Options: None
    Q_UNUSED(codec);
    return QStringList();
}

QVariant S60AudioEncoderControl::encodingOption(const QString &codec, const QString &name) const
{
    // Possible settings: EncodingMode, Codec, BitRate, ChannelCount, SampleRate, Quality
    // Possible (codec specific) Options: None
    Q_UNUSED(codec);
    Q_UNUSED(name);
    return QVariant();
}

void S60AudioEncoderControl::setEncodingOption(
    const QString &codec, const QString &name, const QVariant &value)
{
    m_session->setError(KErrNotSupported, tr("Audio encoding option is not supported"));

    // The audio settings can currently be set only using setAudioSettings() function
    Q_UNUSED(value)
    Q_UNUSED(codec)
    Q_UNUSED(name)
}

QList<int> S60AudioEncoderControl::supportedSampleRates(
    const QAudioEncoderSettings &settings, bool *continuous) const
{
    return m_session->supportedSampleRates(settings, continuous);
}

QAudioEncoderSettings S60AudioEncoderControl::audioSettings() const
{
    QAudioEncoderSettings settings;
    m_session->audioEncoderSettings(settings);

    return settings;
}

void S60AudioEncoderControl::setAudioSettings(const QAudioEncoderSettings &settings)
{
    // Notify that settings have been implicitly set and there's no need to
    // initialize them in case camera is changed
    m_session->notifySettingsSet();

    // Quality defines SampleRate/BitRate combination if either or both are missing
    if (settings.codec().isEmpty()) { // Empty settings
        m_session->setAudioCaptureQuality(settings.quality(), S60VideoCaptureSession::EOnlyAudioQuality);

    } else if (settings.bitRate() == -1 && settings.sampleRate() != -1) { // Only SampleRate set
        m_session->setAudioCaptureCodec(settings.codec());
        m_session->setAudioChannelCount(settings.channelCount());
        m_session->setAudioSampleRate(settings.sampleRate());
        m_session->setAudioEncodingMode(settings.encodingMode());
        m_session->setAudioCaptureQuality(settings.quality(), S60VideoCaptureSession::EAudioQualityAndSampleRate);

    } else if (settings.bitRate() != -1 && settings.sampleRate() == -1) { // Only BitRate set
        m_session->setAudioCaptureCodec(settings.codec());
        m_session->setAudioChannelCount(settings.channelCount());
        m_session->setAudioBitRate(settings.bitRate());
        m_session->setAudioEncodingMode(settings.encodingMode());
        m_session->setAudioCaptureQuality(settings.quality(), S60VideoCaptureSession::EAudioQualityAndBitRate);

    } else if (settings.bitRate() == -1 && settings.sampleRate() == -1) { // No BitRate or SampleRate set
        m_session->setAudioCaptureCodec(settings.codec());
        m_session->setAudioChannelCount(settings.channelCount());
        m_session->setAudioEncodingMode(settings.encodingMode());
        m_session->setAudioCaptureQuality(settings.quality(), S60VideoCaptureSession::EOnlyAudioQuality);

    } else { // Both SampleRate and BitRate set
        m_session->setAudioCaptureCodec(settings.codec());
        m_session->setAudioChannelCount(settings.channelCount());
        m_session->setAudioSampleRate(settings.sampleRate());
        m_session->setAudioBitRate(settings.bitRate());
        m_session->setAudioEncodingMode(settings.encodingMode());
        m_session->setAudioCaptureQuality(settings.quality(), S60VideoCaptureSession::ENoAudioQuality);
    }
}

// End of file
