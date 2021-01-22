/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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


#include "camerabininfocontrol.h"

#include <private/qgstutils_p.h>

QT_BEGIN_NAMESPACE

CameraBinInfoControl::CameraBinInfoControl(GstElementFactory *sourceFactory, QObject *parent)
    : QCameraInfoControl(parent)
    , m_sourceFactory(sourceFactory)
{
    gst_object_ref(GST_OBJECT(m_sourceFactory));
}

CameraBinInfoControl::~CameraBinInfoControl()
{
    gst_object_unref(GST_OBJECT(m_sourceFactory));
}

QCamera::Position CameraBinInfoControl::cameraPosition(const QString &device) const
{
    return QGstUtils::cameraPosition(device, m_sourceFactory);
}

int CameraBinInfoControl::cameraOrientation(const QString &device) const
{
    return QGstUtils::cameraOrientation(device, m_sourceFactory);
}

QT_END_NAMESPACE
