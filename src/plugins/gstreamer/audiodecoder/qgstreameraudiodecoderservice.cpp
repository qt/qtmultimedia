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

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

#include "qgstreameraudiodecoderservice.h"
#include "qgstreameraudiodecodercontrol.h"
#include "qgstreameraudiodecodersession.h"

QT_BEGIN_NAMESPACE

QGstreamerAudioDecoderService::QGstreamerAudioDecoderService(QObject *parent)
    : QMediaService(parent)
{
    m_session = new QGstreamerAudioDecoderSession(this);
    m_control = new QGstreamerAudioDecoderControl(m_session, this);
}

QGstreamerAudioDecoderService::~QGstreamerAudioDecoderService()
{
}

QMediaControl *QGstreamerAudioDecoderService::requestControl(const char *name)
{
    if (qstrcmp(name, QAudioDecoderControl_iid) == 0)
        return m_control;

    return 0;
}

void QGstreamerAudioDecoderService::releaseControl(QMediaControl *control)
{
    Q_UNUSED(control);
}

QT_END_NAMESPACE
