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

#ifndef QGSTREAMERBUSHELPER_P_H
#define QGSTREAMERBUSHELPER_P_H

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

#include <private/qgsttools_global_p.h>
#include <QObject>

#include "qgstreamermessage_p.h"

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class QGstreamerSyncMessageFilter {
public:
    //returns true if message was processed and should be dropped, false otherwise
    virtual bool processSyncMessage(const QGstreamerMessage &message) = 0;
};
#define QGstreamerSyncMessageFilter_iid "org.qt-project.qt.gstreamersyncmessagefilter/5.0"
Q_DECLARE_INTERFACE(QGstreamerSyncMessageFilter, QGstreamerSyncMessageFilter_iid)


class QGstreamerBusMessageFilter {
public:
    //returns true if message was processed and should be dropped, false otherwise
    virtual bool processBusMessage(const QGstreamerMessage &message) = 0;
};
#define QGstreamerBusMessageFilter_iid "org.qt-project.qt.gstreamerbusmessagefilter/5.0"
Q_DECLARE_INTERFACE(QGstreamerBusMessageFilter, QGstreamerBusMessageFilter_iid)


class QGstreamerBusHelperPrivate;

class Q_GSTTOOLS_EXPORT QGstreamerBusHelper : public QObject
{
    Q_OBJECT
    friend class QGstreamerBusHelperPrivate;

public:
    QGstreamerBusHelper(GstBus* bus, QObject* parent = 0);
    ~QGstreamerBusHelper();

    void installMessageFilter(QObject *filter);
    void removeMessageFilter(QObject *filter);

signals:
    void message(QGstreamerMessage const& message);

private:
    QGstreamerBusHelperPrivate *d = nullptr;
};

QT_END_NAMESPACE

#endif
