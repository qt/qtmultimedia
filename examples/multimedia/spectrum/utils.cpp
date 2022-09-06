// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "utils.h"
#include <QAudioFormat>

qreal nyquistFrequency(const QAudioFormat &format)
{
    return format.sampleRate() / 2;
}

QString formatToString(const QAudioFormat &format)
{
    QString result;

    if (QAudioFormat() != format) {

        QString formatType;
        switch (format.sampleFormat()) {
        case QAudioFormat::UInt8:
            formatType = "Unsigned8";
            break;
        case QAudioFormat::Int16:
            formatType = "Signed16";
            break;
        case QAudioFormat::Int32:
            formatType = "Signed32";
            break;
        case QAudioFormat::Float:
            formatType = "Float";
            break;
        default:
            formatType = "Unknown";
            break;
        }

        QString formatChannels = QString("%1 channels").arg(format.channelCount());
        switch (format.channelCount()) {
        case 1:
            formatChannels = "mono";
            break;
        case 2:
            formatChannels = "stereo";
            break;
        }

        result = QString("%1 Hz %2 bit %3 %4")
                         .arg(format.sampleRate())
                         .arg(format.bytesPerSample() * 8)
                         .arg(formatType)
                         .arg(formatChannels);
    }

    return result;
}

const qint16 PCMS16MaxValue = 32767;
const quint16 PCMS16MaxAmplitude = 32768; // because minimum is -32768

qreal pcmToReal(qint16 pcm)
{
    return qreal(pcm) / PCMS16MaxAmplitude;
}

qint16 realToPcm(qreal real)
{
    return real * PCMS16MaxValue;
}
