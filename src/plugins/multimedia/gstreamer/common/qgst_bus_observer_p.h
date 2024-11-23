// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGST_BUS_OBSERVER_P_H
#define QGST_BUS_OBSERVER_P_H

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

class QGstreamerBusMessageFilter
{
public:
    // returns true if message was processed and should be dropped, false otherwise
    virtual bool processBusMessage(const QGstreamerMessage &message) = 0;
};

class QGstBusObserver : private QGstBusHandle
{
public:
    using QGstBusHandle::get;
    using QGstBusHandle::HasRef;
    using QGstBusHandle::RefMode;

    explicit QGstBusObserver(QGstBusHandle);

    ~QGstBusObserver();
    QGstBusObserver(const QGstBusObserver &) = delete;
    QGstBusObserver(QGstBusObserver &&) = delete;
    QGstBusObserver &operator=(const QGstBusObserver &) = delete;
    QGstBusObserver &operator=(QGstBusObserver &&) = delete;

    void close();

    void installMessageFilter(QGstreamerBusMessageFilter *);
    void removeMessageFilter(QGstreamerBusMessageFilter *);

    bool processNextPendingMessage(GstMessageType type = GST_MESSAGE_ANY,
                                   std::optional<std::chrono::nanoseconds> timeout = {});

    bool currentThreadIsNotifierThread() const;

private:
    void processAllPendingMessages();

#ifndef Q_OS_WIN
    QSocketNotifier m_socketNotifier{ QSocketNotifier::Read };
#else
    QWinEventNotifier m_socketNotifier{};
#endif
    QList<QGstreamerBusMessageFilter *> busFilters;
};

QT_END_NAMESPACE

#endif // QGST_BUS_OBSERVER_P_H
