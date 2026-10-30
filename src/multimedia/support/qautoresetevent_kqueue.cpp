// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qautoresetevent_kqueue_p.h"

#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

QAutoResetEventKQueue::QAutoResetEventKQueue(QObject *parent)
    : QObject{
          parent,
      },
      m_kqueue{
          kqueue(),
      },
      m_notifier{
          QSocketNotifier::Type::Read,
      }
{
    if (!isValid()) {
        qCritical() << "kqueue failed:" << qt_error_string(errno);
        return;
    }

    // Register a custom EVFILT_USER event with the EV_CLEAR flag.
    // The EV_CLEAR flag causes the event to auto-reset once it is delivered.
    EV_SET(&m_event, 1, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
    if (kevent(m_kqueue.get(), &m_event, 1, nullptr, 0, nullptr) == -1) {
        qCritical() << "Failed to register EVFILT_USER event:" << qt_error_string(errno);
        m_kqueue.reset();
        return;
    }

    connect(&m_notifier, &QSocketNotifier::activated, this, [this] {
        struct kevent ev;
        int nev = kevent(m_kqueue.get(), nullptr, 0, &ev, 1, nullptr);
        if (nev > 0)
            Q_EMIT activated();
    });
    m_notifier.setSocket(m_kqueue.get());
    m_notifier.setEnabled(true);
}

void QAutoResetEventKQueue::set()
{
    Q_ASSERT(isValid());

    EV_SET(&m_event, 1, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);

    if (kevent(m_kqueue.get(), &m_event, 1, nullptr, 0, nullptr) == -1)
        qWarning() << "Failed to signal event";
}

} // namespace QtPrivate

QT_END_NAMESPACE

#include "moc_qautoresetevent_kqueue_p.cpp"
