// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgst_bus_p.h"

QT_BEGIN_NAMESPACE

QGstBus::QGstBus(QGstBusHandle bus)
    : QGstBusHandle{
          std::move(bus),
      }
{
    if (!get())
        return;

    GPollFD pollFd{};
    gst_bus_get_pollfd(get(), &pollFd);
    Q_ASSERT(pollFd.fd);

#ifndef Q_OS_WIN
    m_socketNotifier.setSocket(pollFd.fd);

    QObject::connect(&m_socketNotifier, &QSocketNotifier::activated, &m_socketNotifier,
                     [this](QSocketDescriptor, QSocketNotifier::Type) {
        this->processAllPendingMessages();
    });

    m_socketNotifier.setEnabled(true);
#else
    m_socketNotifier.setHandle(reinterpret_cast<Qt::HANDLE>(pollFd.fd));

    QObject::connect(&m_socketNotifier, &QWinEventNotifier::activated, &m_socketNotifier,
                     [this](QWinEventNotifier::HANDLE) {
        this->processAllPendingMessages();
    });
    m_socketNotifier.setEnabled(true);
#endif

    gst_bus_set_sync_handler(get(), (GstBusSyncHandler)syncGstBusFilter, this, nullptr);
}

QGstBus::~QGstBus()
{
    close();
}

void QGstBus::close()
{
    if (!get())
        return;

    gst_bus_set_sync_handler(get(), nullptr, nullptr, nullptr);
    QGstBusHandle::close();
}

void QGstBus::installMessageFilter(QGstreamerSyncMessageFilter *filter)
{
    Q_ASSERT(filter);
    QMutexLocker lock(&filterMutex);
    if (!syncFilters.contains(filter))
        syncFilters.append(filter);
}

void QGstBus::removeMessageFilter(QGstreamerSyncMessageFilter *filter)
{
    Q_ASSERT(filter);
    QMutexLocker lock(&filterMutex);
    syncFilters.removeAll(filter);
}

void QGstBus::installMessageFilter(QGstreamerBusMessageFilter *filter)
{
    Q_ASSERT(filter);
    if (!busFilters.contains(filter))
        busFilters.append(filter);
}

void QGstBus::removeMessageFilter(QGstreamerBusMessageFilter *filter)
{
    Q_ASSERT(filter);
    busFilters.removeAll(filter);
}

bool QGstBus::processNextPendingMessage(GstMessageType type,
                                    std::optional<std::chrono::nanoseconds> timeout)
{
    if (!get())
        return false;

    GstClockTime gstTimeout = [&]() -> GstClockTime {
        if (!timeout)
            return GST_CLOCK_TIME_NONE; // block forever
        return timeout->count();
    }();

    QGstreamerMessage message{
        gst_bus_timed_pop_filtered(get(), gstTimeout, type),
        QGstreamerMessage::HasRef,
    };
    if (!message)
        return false;

    for (QGstreamerBusMessageFilter *filter : std::as_const(busFilters)) {
        if (filter->processBusMessage(message))
            break;
    }

    return true;
}

bool QGstBus::currentThreadIsNotifierThread() const
{
    return m_socketNotifier.thread()->isCurrentThread();
}

void QGstBus::processAllPendingMessages()
{
    for (;;) {
        bool messageHandled = processNextPendingMessage(GST_MESSAGE_ANY, std::chrono::nanoseconds{ 0 });

        if (!messageHandled)
            return;
    }
}

GstBusSyncReply QGstBus::syncGstBusFilter(GstBus *bus, GstMessage *message, QGstBus *self)
{
    if (!message)
        return GST_BUS_PASS;

    QMutexLocker lock(&self->filterMutex);
    Q_ASSERT(bus == self->get());

    for (QGstreamerSyncMessageFilter *filter : std::as_const(self->syncFilters)) {
        if (filter->processSyncMessage(QGstreamerMessage{ message, QGstreamerMessage::NeedsRef })) {
            gst_message_unref(message);
            return GST_BUS_DROP;
        }
    }

    return GST_BUS_PASS;
}

QT_END_NAMESPACE
