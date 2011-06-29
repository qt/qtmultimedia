/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qxaaudioencodercontrol.h"
#include "qxarecordsession.h"
#include "qxacommon.h"

QXAAudioEncoderControl::QXAAudioEncoderControl(QXARecordSession *session, QObject *parent)
:QAudioEncoderControl(parent), m_session(session)
{
}

QXAAudioEncoderControl::~QXAAudioEncoderControl()
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
}

QStringList QXAAudioEncoderControl::supportedAudioCodecs() const
{
    if (m_session)
        return m_session->supportedAudioCodecs();
    return QStringList();
}

QString QXAAudioEncoderControl::codecDescription(const QString &codecName) const
{
    if (m_session)
        return m_session->codecDescription(codecName);
    return QString();
}

QList<int> QXAAudioEncoderControl::supportedSampleRates(
            const QAudioEncoderSettings &settings,
            bool *continuous) const
{
    if (m_session)
        return m_session->supportedSampleRates(settings, continuous);
    return QList<int>();
}

QAudioEncoderSettings QXAAudioEncoderControl::audioSettings() const
{
    if (m_session)
        return m_session->audioSettings();
    return QAudioEncoderSettings();
}

void QXAAudioEncoderControl::setAudioSettings(const QAudioEncoderSettings &settings)
{
    if (m_session)
        m_session->setAudioSettings(settings);
}

QStringList QXAAudioEncoderControl::supportedEncodingOptions(const QString &codec) const
{
    if (m_session)
        return m_session->supportedEncodingOptions(codec);
    return QStringList();
}

QVariant QXAAudioEncoderControl::encodingOption(const QString &codec, const QString &name) const
{
    if (m_session)
        return m_session->encodingOption(codec, name);
    return QVariant();
}

void QXAAudioEncoderControl::setEncodingOption(
        const QString &codec, const QString &name, const QVariant &value)
{
    if (m_session)
        m_session->setEncodingOption(codec, name, value);
}
