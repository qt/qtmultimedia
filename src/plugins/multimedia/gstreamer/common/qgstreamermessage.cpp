// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include <gst/gst.h>

#include "qgstreamermessage_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QGstreamerMessage
    \internal
*/

QGstreamerMessage::QGstreamerMessage(const QGstStructure &structure)
{
    gst_structure_get(structure.structure, "message", GST_TYPE_MESSAGE, &m_object, nullptr);
}

GstMessage* QGstreamerMessage::rawMessage() const
{
    return m_object;
}

QT_END_NAMESPACE
