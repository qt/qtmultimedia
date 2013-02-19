/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "audioencodercontrol.h"
#include "audiocapturesession.h"

#include <qaudioformat.h>

#include <QtCore/qdebug.h>

AudioEncoderControl::AudioEncoderControl(QObject *parent)
    :QAudioEncoderSettingsControl(parent)
{
    m_session = qobject_cast<AudioCaptureSession*>(parent);

    QT_PREPEND_NAMESPACE(QAudioFormat) fmt;
    fmt.setSampleSize(8);
    fmt.setChannelCount(1);
    fmt.setSampleRate(8000);
    fmt.setSampleType(QT_PREPEND_NAMESPACE(QAudioFormat)::SignedInt);
    fmt.setCodec("audio/pcm");
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    m_session->setFormat(fmt);

    m_settings.setEncodingMode(QMultimedia::ConstantQualityEncoding);
    m_settings.setCodec("audio/pcm");
    m_settings.setBitRate(8000);
    m_settings.setChannelCount(1);
    m_settings.setSampleRate(8000);
    m_settings.setQuality(QMultimedia::LowQuality);
}

AudioEncoderControl::~AudioEncoderControl()
{
}

QStringList AudioEncoderControl::supportedAudioCodecs() const
{
    QStringList list;
    if (m_session->supportedContainers().size() > 0)
        list.append("audio/pcm");

    return list;
}

QString AudioEncoderControl::codecDescription(const QString &codecName) const
{
    if (codecName.contains(QLatin1String("audio/pcm")))
        return tr("PCM audio data");

    return QString();
}

QList<int> AudioEncoderControl::supportedSampleRates(const QAudioEncoderSettings &, bool *continuous) const
{
    if (continuous)
        *continuous = false;

    return m_session->deviceInfo()->supportedSampleRates();
}

QAudioEncoderSettings AudioEncoderControl::audioSettings() const
{
    return m_settings;
}

void AudioEncoderControl::setAudioSettings(const QAudioEncoderSettings &settings)
{
    QAudioFormat fmt = m_session->format();

    if (settings.encodingMode() == QMultimedia::ConstantQualityEncoding) {
        if (settings.quality() == QMultimedia::LowQuality) {
            fmt.setSampleSize(8);
            fmt.setChannelCount(1);
            fmt.setSampleRate(8000);
            fmt.setSampleType(QAudioFormat::UnSignedInt);

        } else if (settings.quality() == QMultimedia::NormalQuality) {
            fmt.setSampleSize(16);
            fmt.setChannelCount(1);
            fmt.setSampleRate(22050);
            fmt.setSampleType(QAudioFormat::SignedInt);

        } else {
            fmt.setSampleSize(16);
            fmt.setChannelCount(1);
            fmt.setSampleRate(44100);
            fmt.setSampleType(QAudioFormat::SignedInt);
        }

    } else {
        fmt.setChannelCount(settings.channelCount());
        fmt.setSampleRate(settings.sampleRate());
        if (settings.sampleRate() == 8000 && settings.bitRate() == 8000) {
            fmt.setSampleType(QAudioFormat::UnSignedInt);
            fmt.setSampleSize(8);
        } else {
            fmt.setSampleSize(16);
            fmt.setSampleType(QAudioFormat::SignedInt);
        }
    }
    fmt.setCodec("audio/pcm");

    m_session->setFormat(fmt);
    m_settings = settings;
}
