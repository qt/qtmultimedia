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

#include <QtCore/qstring.h>

#include "audiocaptureserviceplugin.h"
#include "audiocaptureservice.h"

#include "qmediaserviceproviderplugin.h"

QT_BEGIN_NAMESPACE

QMediaService* AudioCaptureServicePlugin::create(QString const& key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_AUDIOSOURCE))
        return new AudioCaptureService;

    return 0;
}

void AudioCaptureServicePlugin::release(QMediaService *service)
{
    delete service;
}

QT_END_NAMESPACE
