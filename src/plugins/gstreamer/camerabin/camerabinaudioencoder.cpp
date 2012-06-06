/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "camerabinaudioencoder.h"
#include "camerabincontainer.h"
#include "qgstcodecsinfo.h"

#include <QtCore/qdebug.h>

CameraBinAudioEncoder::CameraBinAudioEncoder(QObject *parent)
    :QAudioEncoderControl(parent),
     m_codecs(QGstCodecsInfo::AudioEncoder)
{
}

CameraBinAudioEncoder::~CameraBinAudioEncoder()
{
}

QStringList CameraBinAudioEncoder::supportedAudioCodecs() const
{
    return m_codecs.supportedCodecs();
}

QString CameraBinAudioEncoder::codecDescription(const QString &codecName) const
{
    return m_codecs.codecDescription(codecName);
}

QList<int> CameraBinAudioEncoder::supportedSampleRates(const QAudioEncoderSettings &, bool *) const
{
    //TODO check element caps to find actual values

    return QList<int>();
}

QAudioEncoderSettings CameraBinAudioEncoder::audioSettings() const
{
    return m_audioSettings;
}

void CameraBinAudioEncoder::setAudioSettings(const QAudioEncoderSettings &settings)
{
    m_userSettings = settings;
    m_audioSettings = settings;
    emit settingsChanged();
}

void CameraBinAudioEncoder::setActualAudioSettings(const QAudioEncoderSettings &settings)
{
    m_audioSettings = settings;
}

void CameraBinAudioEncoder::resetActualSettings()
{
    m_audioSettings = m_userSettings;
}

GstEncodingProfile *CameraBinAudioEncoder::createProfile()
{
    QString codec = m_audioSettings.codec();
    GstCaps *caps;

    if (codec.isEmpty())
        caps = gst_caps_new_any();
    else
        caps = gst_caps_from_string(codec.toLatin1());

    return (GstEncodingProfile *)gst_encoding_audio_profile_new(
                                        caps,
                                        NULL, //preset
                                        NULL, //restriction
                                        0); //presence
}
