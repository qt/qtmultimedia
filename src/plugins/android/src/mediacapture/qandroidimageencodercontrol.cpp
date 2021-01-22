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

#include "qandroidimageencodercontrol.h"

#include "qandroidcamerasession.h"
#include "androidcamera.h"

QT_BEGIN_NAMESPACE

QAndroidImageEncoderControl::QAndroidImageEncoderControl(QAndroidCameraSession *session)
    : QImageEncoderControl()
    , m_session(session)
{
    connect(m_session, SIGNAL(opened()),
            this, SLOT(onCameraOpened()));
}

QStringList QAndroidImageEncoderControl::supportedImageCodecs() const
{
    return QStringList() << QLatin1String("jpeg");
}

QString QAndroidImageEncoderControl::imageCodecDescription(const QString &codecName) const
{
    if (codecName == QLatin1String("jpeg"))
        return tr("JPEG image");

    return QString();
}

QList<QSize> QAndroidImageEncoderControl::supportedResolutions(const QImageEncoderSettings &settings, bool *continuous) const
{
    Q_UNUSED(settings);

    if (continuous)
        *continuous = false;

    return m_supportedResolutions;
}

QImageEncoderSettings QAndroidImageEncoderControl::imageSettings() const
{
    return m_session->imageSettings();
}

void QAndroidImageEncoderControl::setImageSettings(const QImageEncoderSettings &settings)
{
    m_session->setImageSettings(settings);
}

void QAndroidImageEncoderControl::onCameraOpened()
{
    m_supportedResolutions = m_session->camera()->getSupportedPictureSizes();
}

QT_END_NAMESPACE
