// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgst_bus_observer_p.h"

QT_BEGIN_NAMESPACE

QGstBusObserver::QGstBusObserver(QGstBusHandle bus)
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

}

QGstBusObserver::~QGstBusObserver()
{
    close();
}

void QGstBusObserver::close()
{
    if (!get())
        return;

    QGstBusHandle::reset();
}

void QGstBusObserver::installMessageFilter(QGstreamerBusMessageFilter *filter)
{
    Q_ASSERT(filter);
    if (!busFilters.contains(filter))
        busFilters.append(filter);
}

void QGstBusObserver::removeMessageFilter(QGstreamerBusMessageFilter *filter)
{
    Q_ASSERT(filter);
    busFilters.removeAll(filter);
}

bool QGstBusObserver::processNextPendingMessage(GstMessageType type,
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

bool QGstBusObserver::currentThreadIsNotifierThread() const
{
    return m_socketNotifier.thread()->isCurrentThread();
}

void QGstBusObserver::processAllPendingMessages()
{
    for (;;) {
        bool messageHandled = processNextPendingMessage(GST_MESSAGE_ANY, std::chrono::nanoseconds{ 0 });

        if (!messageHandled)
            return;
    }
}

QT_END_NAMESPACE
