/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#include "bbimageencodercontrol.h"

#include "bbcamerasession.h"

QT_BEGIN_NAMESPACE

BbImageEncoderControl::BbImageEncoderControl(BbCameraSession *session, QObject *parent)
    : QImageEncoderControl(parent)
    , m_session(session)
{
}

QStringList BbImageEncoderControl::supportedImageCodecs() const
{
    return QStringList() << QLatin1String("jpeg");
}

QString BbImageEncoderControl::imageCodecDescription(const QString &codecName) const
{
    if (codecName == QLatin1String("jpeg"))
        return tr("JPEG image");

    return QString();
}

QList<QSize> BbImageEncoderControl::supportedResolutions(const QImageEncoderSettings &settings, bool *continuous) const
{
    return m_session->supportedResolutions(settings, continuous);
}

QImageEncoderSettings BbImageEncoderControl::imageSettings() const
{
    return m_session->imageSettings();
}

void BbImageEncoderControl::setImageSettings(const QImageEncoderSettings &settings)
{
    m_session->setImageSettings(settings);
}

QT_END_NAMESPACE
