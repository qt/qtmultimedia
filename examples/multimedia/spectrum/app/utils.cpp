/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QAudioFormat>
#include "utils.h"

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

const qint16  PCMS16MaxValue     =  32767;
const quint16 PCMS16MaxAmplitude =  32768; // because minimum is -32768

qreal pcmToReal(qint16 pcm)
{
    return qreal(pcm) / PCMS16MaxAmplitude;
}

qint16 realToPcm(qreal real)
{
    return real * PCMS16MaxValue;
}
