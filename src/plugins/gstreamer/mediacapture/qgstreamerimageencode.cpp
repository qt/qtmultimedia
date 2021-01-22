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

#include "qgstreamerimageencode.h"
#include "qgstreamercapturesession.h"

#include <QtCore/qdebug.h>

#include <math.h>

QGstreamerImageEncode::QGstreamerImageEncode(QGstreamerCaptureSession *session)
    :QImageEncoderControl(session), m_session(session)
{
}

QGstreamerImageEncode::~QGstreamerImageEncode()
{
}

QList<QSize> QGstreamerImageEncode::supportedResolutions(const QImageEncoderSettings &, bool *continuous) const
{
    if (continuous)
        *continuous = m_session->videoInput() != 0;

    return m_session->videoInput() ? m_session->videoInput()->supportedResolutions() : QList<QSize>();
}

QStringList QGstreamerImageEncode::supportedImageCodecs() const
{
    return QStringList() << "jpeg";
}

QString QGstreamerImageEncode::imageCodecDescription(const QString &codecName) const
{
    if (codecName == "jpeg")
        return tr("JPEG image encoder");

    return QString();
}

QImageEncoderSettings QGstreamerImageEncode::imageSettings() const
{
    return m_settings;
}

void QGstreamerImageEncode::setImageSettings(const QImageEncoderSettings &settings)
{
    if (m_settings != settings) {
        m_settings = settings;
        emit settingsChanged();
    }
}
