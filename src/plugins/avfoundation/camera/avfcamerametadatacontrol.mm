/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfcamerametadatacontrol.h"
#include "avfcamerasession.h"
#include "avfcameraservice.h"

QT_USE_NAMESPACE

//metadata support is not implemented yet

AVFCameraMetaDataControl::AVFCameraMetaDataControl(AVFCameraService *service, QObject *parent)
    :QMetaDataWriterControl(parent)
{
    Q_UNUSED(service);
}

AVFCameraMetaDataControl::~AVFCameraMetaDataControl()
{
}

bool AVFCameraMetaDataControl::isMetaDataAvailable() const
{
    return !m_tags.isEmpty();
}

bool AVFCameraMetaDataControl::isWritable() const
{
    return false;
}

QVariant AVFCameraMetaDataControl::metaData(const QString &key) const
{
    return m_tags.value(key);
}

void AVFCameraMetaDataControl::setMetaData(const QString &key, const QVariant &value)
{
    m_tags.insert(key, value);
}

QStringList AVFCameraMetaDataControl::availableMetaData() const
{
    return m_tags.keys();
}

#include "moc_avfcamerametadatacontrol.cpp"
