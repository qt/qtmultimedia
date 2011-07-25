/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKAUDIOENCODERCONTROL_H
#define MOCKAUDIOENCODERCONTROL_H

#include "qaudioencodercontrol.h"

class MockAudioEncoderControl : public QAudioEncoderControl
{
    Q_OBJECT
public:
    MockAudioEncoderControl(QObject *parent):
        QAudioEncoderControl(parent)
    {
        m_codecs << "audio/pcm" << "audio/mpeg";
        m_descriptions << "Pulse Code Modulation" << "mp3 format";
        m_supportedEncodeOptions.insert("audio/pcm", QStringList());
        m_supportedEncodeOptions.insert("audio/mpeg", QStringList() << "quality" << "bitrate" << "mode" << "vbr");
        m_audioSettings.setCodec("audio/pcm");
        m_audioSettings.setBitRate(128*1024);
        m_audioSettings.setSampleRate(8000);
        m_freqs << 8000 << 11025 << 22050 << 44100;
    }

    ~MockAudioEncoderControl() {}

    QAudioEncoderSettings audioSettings() const
    {
        return m_audioSettings;
    }

    void setAudioSettings(const QAudioEncoderSettings &settings)
    {
        m_audioSettings = settings;
    }

    QList<int> supportedChannelCounts(const QAudioEncoderSettings & = QAudioEncoderSettings()) const
    {
        QList<int> list; list << 1 << 2; return list;
    }

    QList<int> supportedSampleRates(const QAudioEncoderSettings & = QAudioEncoderSettings(), bool *continuous = 0) const
    {
        if (continuous)
            *continuous = false;

        return m_freqs;
    }

    QStringList supportedAudioCodecs() const
    {
        return m_codecs;
    }

    QString codecDescription(const QString &codecName) const
    {
        return m_descriptions.value(m_codecs.indexOf(codecName));
    }

    QStringList supportedEncodingOptions(const QString &codec) const
    {
        return m_supportedEncodeOptions.value(codec);
    }

    QVariant encodingOption(const QString &codec, const QString &name) const
    {
        return m_encodeOptions[codec].value(name);
    }

    void setEncodingOption(const QString &codec, const QString &name, const QVariant &value)
    {
        m_encodeOptions[codec][name] = value;
    }

private:
    QAudioEncoderSettings m_audioSettings;

    QStringList  m_codecs;
    QStringList  m_descriptions;

    QList<int>   m_freqs;

    QMap<QString, QStringList> m_supportedEncodeOptions;
    QMap<QString, QMap<QString, QVariant> > m_encodeOptions;

};

#endif // MOCKAUDIOENCODERCONTROL_H
