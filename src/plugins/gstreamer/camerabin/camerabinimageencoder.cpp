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

#include "camerabinimageencoder.h"
#include "camerabinsession.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

CameraBinImageEncoder::CameraBinImageEncoder(CameraBinSession *session)
    :QImageEncoderControl(session), m_session(session)
{
}

CameraBinImageEncoder::~CameraBinImageEncoder()
{
}

QList<QSize> CameraBinImageEncoder::supportedResolutions(const QImageEncoderSettings &, bool *continuous) const
{
    if (continuous)
        *continuous = false;

    return m_session->supportedResolutions(qMakePair<int,int>(0,0), continuous, QCamera::CaptureStillImage);
}

QStringList CameraBinImageEncoder::supportedImageCodecs() const
{
    return QStringList() << "jpeg";
}

QString CameraBinImageEncoder::imageCodecDescription(const QString &codecName) const
{
    if (codecName == "jpeg")
        return tr("JPEG image");

    return QString();
}

QImageEncoderSettings CameraBinImageEncoder::imageSettings() const
{
    return m_settings;
}

void CameraBinImageEncoder::setImageSettings(const QImageEncoderSettings &settings)
{
    m_settings = settings;
    emit settingsChanged();
}

QT_END_NAMESPACE
