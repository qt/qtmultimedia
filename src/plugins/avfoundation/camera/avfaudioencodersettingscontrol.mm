/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "avfaudioencodersettingscontrol.h"

#include "avfcameraservice.h"
#include "avfcamerasession.h"

#include <AVFoundation/AVFoundation.h>
#include <CoreAudio/CoreAudioTypes.h>

QT_BEGIN_NAMESPACE

struct AudioCodecInfo
{
    QString description;
    int id;

    AudioCodecInfo() : id(0) { }
    AudioCodecInfo(const QString &desc, int i)
        : description(desc), id(i)
    { }
};

typedef QMap<QString, AudioCodecInfo> SupportedAudioCodecs;
Q_GLOBAL_STATIC_WITH_ARGS(QString , defaultCodec, (QLatin1String("aac")))
Q_GLOBAL_STATIC(SupportedAudioCodecs, supportedCodecs)

AVFAudioEncoderSettingsControl::AVFAudioEncoderSettingsControl(AVFCameraService *service)
    : QAudioEncoderSettingsControl()
    , m_service(service)
{
    if (supportedCodecs->isEmpty()) {
        supportedCodecs->insert(QStringLiteral("lpcm"),
                                AudioCodecInfo(QStringLiteral("Linear PCM"),
                                               kAudioFormatLinearPCM));
        supportedCodecs->insert(QStringLiteral("ulaw"),
                                AudioCodecInfo(QStringLiteral("PCM Mu-Law 2:1"),
                                               kAudioFormatULaw));
        supportedCodecs->insert(QStringLiteral("alaw"),
                                AudioCodecInfo(QStringLiteral("PCM A-Law 2:1"),
                                               kAudioFormatALaw));
        supportedCodecs->insert(QStringLiteral("ima4"),
                                AudioCodecInfo(QStringLiteral("IMA 4:1 ADPCM"),
                                               kAudioFormatAppleIMA4));
        supportedCodecs->insert(QStringLiteral("alac"),
                                AudioCodecInfo(QStringLiteral("Apple Lossless Audio Codec"),
                                               kAudioFormatAppleLossless));
        supportedCodecs->insert(QStringLiteral("aac"),
                                AudioCodecInfo(QStringLiteral("MPEG-4 Low Complexity AAC"),
                                               kAudioFormatMPEG4AAC));
        supportedCodecs->insert(QStringLiteral("aach"),
                                AudioCodecInfo(QStringLiteral("MPEG-4 High Efficiency AAC"),
                                               kAudioFormatMPEG4AAC_HE));
        supportedCodecs->insert(QStringLiteral("aacl"),
                                AudioCodecInfo(QStringLiteral("MPEG-4 AAC Low Delay"),
                                               kAudioFormatMPEG4AAC_LD));
        supportedCodecs->insert(QStringLiteral("aace"),
                                AudioCodecInfo(QStringLiteral("MPEG-4 AAC Enhanced Low Delay"),
                                               kAudioFormatMPEG4AAC_ELD));
        supportedCodecs->insert(QStringLiteral("aacf"),
                                AudioCodecInfo(QStringLiteral("MPEG-4 AAC Enhanced Low Delay with SBR"),
                                               kAudioFormatMPEG4AAC_ELD_SBR));
        supportedCodecs->insert(QStringLiteral("aacp"),
                                AudioCodecInfo(QStringLiteral("MPEG-4 HE AAC V2"),
                                               kAudioFormatMPEG4AAC_HE_V2));
        supportedCodecs->insert(QStringLiteral("ilbc"),
                                AudioCodecInfo(QStringLiteral("iLBC"),
                                               kAudioFormatiLBC));
    }
}

QStringList AVFAudioEncoderSettingsControl::supportedAudioCodecs() const
{
    return supportedCodecs->keys();
}

QString AVFAudioEncoderSettingsControl::codecDescription(const QString &codecName) const
{
    return supportedCodecs->value(codecName).description;
}

QList<int> AVFAudioEncoderSettingsControl::supportedSampleRates(const QAudioEncoderSettings &settings, bool *continuous) const
{
    Q_UNUSED(settings)

    if (continuous)
        *continuous = true;

    return QList<int>() << 8000 << 96000;
}

QAudioEncoderSettings AVFAudioEncoderSettingsControl::audioSettings() const
{
    return m_actualSettings;
}

void AVFAudioEncoderSettingsControl::setAudioSettings(const QAudioEncoderSettings &settings)
{
    if (m_requestedSettings == settings)
        return;

    m_requestedSettings = m_actualSettings = settings;
}

NSDictionary *AVFAudioEncoderSettingsControl::applySettings()
{
    if (m_service->session()->state() != QCamera::LoadedState &&
        m_service->session()->state() != QCamera::ActiveState) {
        return nil;
    }

    NSMutableDictionary *settings = [NSMutableDictionary dictionary];

    QString codec = m_requestedSettings.codec().isEmpty() ? *defaultCodec : m_requestedSettings.codec();
    if (!supportedCodecs->contains(codec)) {
        qWarning("Unsupported codec: '%s'", codec.toLocal8Bit().constData());
        codec = *defaultCodec;
    }
    [settings setObject:[NSNumber numberWithInt:supportedCodecs->value(codec).id] forKey:AVFormatIDKey];
    m_actualSettings.setCodec(codec);

#ifdef Q_OS_OSX
    if (m_requestedSettings.encodingMode() == QMultimedia::ConstantQualityEncoding) {
        int quality;
        switch (m_requestedSettings.quality()) {
        case QMultimedia::VeryLowQuality:
            quality = AVAudioQualityMin;
            break;
        case QMultimedia::LowQuality:
            quality = AVAudioQualityLow;
            break;
        case QMultimedia::HighQuality:
            quality = AVAudioQualityHigh;
            break;
        case QMultimedia::VeryHighQuality:
            quality = AVAudioQualityMax;
            break;
        case QMultimedia::NormalQuality:
        default:
            quality = AVAudioQualityMedium;
            break;
        }
        [settings setObject:[NSNumber numberWithInt:quality] forKey:AVEncoderAudioQualityKey];

    } else
#endif
    if (m_requestedSettings.bitRate() > 0){
        [settings setObject:[NSNumber numberWithInt:m_requestedSettings.bitRate()] forKey:AVEncoderBitRateKey];
    }

    int sampleRate = m_requestedSettings.sampleRate();
    int channelCount = m_requestedSettings.channelCount();

#ifdef Q_OS_IOS
    // Some keys are mandatory only on iOS
    if (codec == QLatin1String("lpcm")) {
        [settings setObject:[NSNumber numberWithInt:16] forKey:AVLinearPCMBitDepthKey];
        [settings setObject:[NSNumber numberWithInt:NO] forKey:AVLinearPCMIsBigEndianKey];
        [settings setObject:[NSNumber numberWithInt:NO] forKey:AVLinearPCMIsFloatKey];
        [settings setObject:[NSNumber numberWithInt:NO] forKey:AVLinearPCMIsNonInterleaved];
    }

    if (codec == QLatin1String("alac"))
        [settings setObject:[NSNumber numberWithInt:24] forKey:AVEncoderBitDepthHintKey];

    if (sampleRate <= 0)
        sampleRate = codec == QLatin1String("ilbc") ? 8000 : 44100;
    if (channelCount <= 0)
        channelCount = codec == QLatin1String("ilbc") ? 1 : 2;
#endif

    if (sampleRate > 0) {
        [settings setObject:[NSNumber numberWithInt:sampleRate] forKey:AVSampleRateKey];
        m_actualSettings.setSampleRate(sampleRate);
    }
    if (channelCount > 0) {
        [settings setObject:[NSNumber numberWithInt:channelCount] forKey:AVNumberOfChannelsKey];
        m_actualSettings.setChannelCount(channelCount);
    }

    return settings;
}

void AVFAudioEncoderSettingsControl::unapplySettings()
{
    m_actualSettings = m_requestedSettings;
}

QT_END_NAMESPACE
