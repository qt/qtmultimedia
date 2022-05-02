/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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

#include <private/qtmultimediaglobal_p.h>
#include <private/qgst_p.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QGstreamerMessage
{
public:
    QGstreamerMessage() = default;
    QGstreamerMessage(QGstreamerMessage const& m);
    explicit QGstreamerMessage(GstMessage* message);
    explicit QGstreamerMessage(const QGstStructure &structure);

    ~QGstreamerMessage();

    bool isNull() const { return !m_message; }
    GstMessageType type() const { return GST_MESSAGE_TYPE(m_message); }
    QGstObject source() const { return QGstObject(GST_MESSAGE_SRC(m_message), QGstObject::NeedsRef); }
    QGstStructure structure() const { return QGstStructure(gst_message_get_structure(m_message)); }

    GstMessage* rawMessage() const;

    QGstreamerMessage& operator=(QGstreamerMessage const& rhs);

private:
    GstMessage* m_message = nullptr;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QGstreamerMessage);

#endif
