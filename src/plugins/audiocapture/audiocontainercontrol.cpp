/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "audiocontainercontrol.h"
#include "audiocapturesession.h"

QT_BEGIN_NAMESPACE

AudioContainerControl::AudioContainerControl(QObject *parent)
    :QMediaContainerControl(parent)
{
    m_session = qobject_cast<AudioCaptureSession*>(parent);
}

AudioContainerControl::~AudioContainerControl()
{
}

QStringList AudioContainerControl::supportedContainers() const
{
    return QStringList() << QStringLiteral("audio/x-wav")
                         << QStringLiteral("audio/x-raw");
}

QString AudioContainerControl::containerFormat() const
{
    return m_session->containerFormat();
}

void AudioContainerControl::setContainerFormat(const QString &formatMimeType)
{
    if (formatMimeType.isEmpty() || supportedContainers().contains(formatMimeType))
        m_session->setContainerFormat(formatMimeType);
}

QString AudioContainerControl::containerDescription(const QString &formatMimeType) const
{
    if (QString::compare(formatMimeType, QLatin1String("audio/x-raw")) == 0)
        return tr("RAW (headerless) file format");
    if (QString::compare(formatMimeType, QLatin1String("audio/x-wav")) == 0)
        return tr("WAV file format");

    return QString();
}

QT_END_NAMESPACE
