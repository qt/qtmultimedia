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

#include "qgstreamerstreamscontrol.h"
#include <private/qgstreamerplayersession_p.h>

QGstreamerStreamsControl::QGstreamerStreamsControl(QGstreamerPlayerSession *session, QObject *parent)
    :QMediaStreamsControl(parent), m_session(session)
{
    connect(m_session, SIGNAL(streamsChanged()), SIGNAL(streamsChanged()));
}

QGstreamerStreamsControl::~QGstreamerStreamsControl()
{
}

int QGstreamerStreamsControl::streamCount()
{
    return m_session->streamCount();
}

QMediaStreamsControl::StreamType QGstreamerStreamsControl::streamType(int streamNumber)
{
    return m_session->streamType(streamNumber);
}

QVariant QGstreamerStreamsControl::metaData(int streamNumber, const QString &key)
{
    return m_session->streamProperties(streamNumber).value(key);
}

bool QGstreamerStreamsControl::isActive(int streamNumber)
{
    return streamNumber != -1 && streamNumber == m_session->activeStream(streamType(streamNumber));
}

void QGstreamerStreamsControl::setActive(int streamNumber, bool state)
{
    QMediaStreamsControl::StreamType type = m_session->streamType(streamNumber);
    if (type == QMediaStreamsControl::UnknownStream)
        return;

    if (state)
        m_session->setActiveStream(type, streamNumber);
    else {
        //only one active stream of certain type is supported
        if (m_session->activeStream(type) == streamNumber)
            m_session->setActiveStream(type, -1);
    }
}

