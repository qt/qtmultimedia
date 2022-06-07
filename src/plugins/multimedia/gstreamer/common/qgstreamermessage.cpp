// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <gst/gst.h>

#include "qgstreamermessage_p.h"

QT_BEGIN_NAMESPACE

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

QGstreamerMessage::QGstreamerMessage(const QGstStructure &structure)
{
    gst_structure_get(structure.structure, "message", GST_TYPE_MESSAGE, &m_message, nullptr);
}

QGstreamerMessage::~QGstreamerMessage()
{
    if (m_message != nullptr)
        gst_message_unref(m_message);
}

GstMessage* QGstreamerMessage::rawMessage() const
{
    return m_message;
}

QGstreamerMessage& QGstreamerMessage::operator=(QGstreamerMessage const& rhs)
{
    if (rhs.m_message != m_message) {
        if (rhs.m_message != nullptr)
            gst_message_ref(rhs.m_message);

        if (m_message != nullptr)
            gst_message_unref(m_message);

        m_message = rhs.m_message;
    }

    return *this;
}

QT_END_NAMESPACE
