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

#include "directshowcameraimageencodercontrol.h"
#include "dscamerasession.h"
#include <QImageWriter>

QT_BEGIN_NAMESPACE

DirectShowCameraImageEncoderControl::DirectShowCameraImageEncoderControl(DSCameraSession *session)
    : QImageEncoderControl(session)
    , m_session(session)
{
}

QList<QSize> DirectShowCameraImageEncoderControl::supportedResolutions(const QImageEncoderSettings &settings, bool *continuous) const
{
    QList<QSize> res;
    if (!settings.codec().isEmpty() && !supportedImageCodecs().contains(settings.codec(), Qt::CaseInsensitive))
        return res;

    QList<QSize> resolutions = m_session->supportedResolutions(continuous);
    QSize r = settings.resolution();
    if (!r.isValid())
        return resolutions;

    if (resolutions.contains(r))
        res << settings.resolution();

    return res;
}

QStringList DirectShowCameraImageEncoderControl::supportedImageCodecs() const
{
    QStringList supportedCodecs;
    for (const QByteArray &type: QImageWriter::supportedImageFormats()) {
        supportedCodecs << type;
    }

    return supportedCodecs;
}

QString DirectShowCameraImageEncoderControl::imageCodecDescription(const QString &codecName) const
{
    Q_UNUSED(codecName);
    return QString();
}

QImageEncoderSettings DirectShowCameraImageEncoderControl::imageSettings() const
{
    return m_session->imageEncoderSettings();
}

void DirectShowCameraImageEncoderControl::setImageSettings(const QImageEncoderSettings &settings)
{
    m_session->setImageEncoderSettings(settings);
}

QT_END_NAMESPACE
