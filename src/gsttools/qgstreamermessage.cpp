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

#include <gst/gst.h>

#include "qgstreamermessage_p.h"

QT_BEGIN_NAMESPACE

static int wuchi = qRegisterMetaType<QGstreamerMessage>();


/*!
    \class QGstreamerMessage
    \internal
*/

QGstreamerMessage::QGstreamerMessage(GstMessage* message):
    m_message(message)
{
    gst_message_ref(m_message);
}

QGstreamerMessage::QGstreamerMessage(QGstreamerMessage const& m):
    m_message(m.m_message)
{
    gst_message_ref(m_message);
}


QGstreamerMessage::~QGstreamerMessage()
{
    if (m_message != 0)
        gst_message_unref(m_message);
}

GstMessage* QGstreamerMessage::rawMessage() const
{
    return m_message;
}

QGstreamerMessage& QGstreamerMessage::operator=(QGstreamerMessage const& rhs)
{
    if (rhs.m_message != m_message) {
        if (rhs.m_message != 0)
            gst_message_ref(rhs.m_message);

        if (m_message != 0)
            gst_message_unref(m_message);

        m_message = rhs.m_message;
    }

    return *this;
}

QT_END_NAMESPACE
