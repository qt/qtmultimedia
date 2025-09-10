// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERMESSAGE_P_H
#define QGSTREAMERMESSAGE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/private/qsharedhandle_p.h>
#include "qgst_handle_types_p.h"
#include "qgst_p.h"

QT_BEGIN_NAMESPACE

class QGstreamerMessage : public QGstMessageHandle
{
    using BaseClass = QGstMessageHandle;

public:
    using BaseClass::BaseClass;
    QGstreamerMessage(const QGstreamerMessage &) = default;
    QGstreamerMessage(QGstreamerMessage &&) noexcept = default;
    QGstreamerMessage &operator=(const QGstreamerMessage &) = default;
    QGstreamerMessage &operator=(QGstreamerMessage &&) noexcept = default;

    GstMessageType type() const { return GST_MESSAGE_TYPE(get()); }
    QGstObject source() const { return QGstObject(GST_MESSAGE_SRC(get()), QGstObject::NeedsRef); }
    QGstStructureView structure() const { return QGstStructureView(gst_message_get_structure(get())); }

    GstMessage *message() const { return get(); }
};

QT_END_NAMESPACE

#endif
