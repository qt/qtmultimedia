// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGST_BUS_P_H
#define QGST_BUS_P_H

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

#include <QtCore/qsocketnotifier.h>
#include <QtCore/qwineventnotifier.h>
#include <QtCore/qmutex.h>

#include "qgst_p.h"
#include "qgstreamermessage_p.h"

QT_BEGIN_NAMESPACE

class QGstreamerSyncMessageFilter
{
public:
    // returns true if message was processed and should be dropped, false otherwise
    virtual bool processSyncMessage(const QGstreamerMessage &message) = 0;
};

class QGstreamerBusMessageFilter
{
public:
    // returns true if message was processed and should be dropped, false otherwise
    virtual bool processBusMessage(const QGstreamerMessage &message) = 0;
};

class QGstBus : public QGstBusHandle
{
public:
    explicit QGstBus(QGstBusHandle);
    QGstBus(GstBus *, QGstBusHandle::RefMode);

    ~QGstBus();
    QGstBus(const QGstBus &) = delete;
    QGstBus(QGstBus &&) = delete;
    QGstBus &operator=(const QGstBus &) = delete;
    QGstBus &operator=(QGstBus &&) = delete;

    void installMessageFilter(QGstreamerSyncMessageFilter *);
    void removeMessageFilter(QGstreamerSyncMessageFilter *);
    void installMessageFilter(QGstreamerBusMessageFilter *);
    void removeMessageFilter(QGstreamerBusMessageFilter *);

    bool processNextPendingMessage(GstMessageType type = GST_MESSAGE_ANY,
                                   std::optional<std::chrono::nanoseconds> timeout = {});

private:
    void processAllPendingMessages();

    static GstBusSyncReply syncGstBusFilter(GstBus *, GstMessage *, QGstBus *);

    QGstBusHandle m_bus;

#ifndef Q_OS_WIN
    QSocketNotifier m_socketNotifier{ QSocketNotifier::Read };
#else
    QWinEventNotifier m_socketNotifier{};
#endif
    QMutex filterMutex;
    QList<QGstreamerSyncMessageFilter *> syncFilters;
    QList<QGstreamerBusMessageFilter *> busFilters;
};

QT_END_NAMESPACE

#endif // QGST_BUS_P_H
