// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGST_DEBUG_P_H
#define QGST_DEBUG_P_H

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

#include "qgst_p.h"
#include <qdebug.h>
#include <gst/play/gstplay.h>

QT_BEGIN_NAMESPACE

class QGstreamerMessage;

QDebug operator<<(QDebug, const QGstCaps &);
QDebug operator<<(QDebug, const QGstStructureView &);
QDebug operator<<(QDebug, const QUniqueGstStructureHandle &);
QDebug operator<<(QDebug, const QGstElement &);
QDebug operator<<(QDebug, const QGstPad &);
QDebug operator<<(QDebug, const QGString &);
QDebug operator<<(QDebug, const QGValue &);
QDebug operator<<(QDebug, const QGstreamerMessage &);
QDebug operator<<(QDebug, const QUniqueGErrorHandle &);
QDebug operator<<(QDebug, const QUniqueGStringHandle &);
QDebug operator<<(QDebug, const QGstStreamCollectionHandle &);
QDebug operator<<(QDebug, const QGstStreamHandle &);
QDebug operator<<(QDebug, const QGstTagListHandle &);

QDebug operator<<(QDebug, const GstCaps *);
QDebug operator<<(QDebug, const GstVideoInfo *);
QDebug operator<<(QDebug, const GstStructure *);
QDebug operator<<(QDebug, const GstObject *);
QDebug operator<<(QDebug, const GstElement *);
QDebug operator<<(QDebug, const GstPad *);
QDebug operator<<(QDebug, const GstDevice *);
QDebug operator<<(QDebug, const GstMessage *);
QDebug operator<<(QDebug, const GstTagList *);
QDebug operator<<(QDebug, const GstQuery *);
QDebug operator<<(QDebug, const GstEvent *);
QDebug operator<<(QDebug, const GstPadTemplate *);
QDebug operator<<(QDebug, const GstStreamCollection *);
QDebug operator<<(QDebug, const GstStream *);

QDebug operator<<(QDebug, GstState);
QDebug operator<<(QDebug, GstStateChange);
QDebug operator<<(QDebug, GstStateChangeReturn);
QDebug operator<<(QDebug, GstMessageType);
QDebug operator<<(QDebug, GstPadDirection);
QDebug operator<<(QDebug, GstStreamStatusType);
QDebug operator<<(QDebug, GstStreamType);

QDebug operator<<(QDebug, const GValue *);
QDebug operator<<(QDebug, const GError *);

struct QCompactGstMessageAdaptor
{
    explicit QCompactGstMessageAdaptor(const QGstreamerMessage &m);
    explicit QCompactGstMessageAdaptor(GstMessage *m);
    GstMessage *msg;
};
QDebug operator<<(QDebug, const QCompactGstMessageAdaptor &);

QDebug operator<<(QDebug dbg, GstPlayState type);
QDebug operator<<(QDebug dbg, GstPlayMessage type);

struct QGstPlayMessageAdaptor
{
    explicit QGstPlayMessageAdaptor(const QGstreamerMessage &m);
    explicit QGstPlayMessageAdaptor(GstMessage *m);
    GstMessage *msg;
};
QDebug operator<<(QDebug, const QGstPlayMessageAdaptor &);

QT_END_NAMESPACE

#endif
