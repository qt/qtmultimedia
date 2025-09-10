// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qautoresetevent_linux_p.h"

#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/qdebug.h>
#include <QtMultimedia/private/qaudio_rtsan_support_p.h>

#include <sys/eventfd.h>
#include <cstdint>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

QAutoResetEventEventFD::QAutoResetEventEventFD(QObject *parent)
        : QObject{
              parent,
          },
          m_notifier{
              QSocketNotifier::Type::Read,
          }
{
    m_fd = eventfd(/*initval=*/0, EFD_NONBLOCK);
    if (m_fd == -1) {
        qCritical() << "eventfd failed:" << qt_error_string(errno);
        return;
    }

    connect(&m_notifier, &QSocketNotifier::activated, this, [this] {
        uint64_t payload;

        qt_safe_read(m_fd, &payload, sizeof(payload));

        emit activated();
    });
    m_notifier.setSocket(m_fd);
    m_notifier.setEnabled(true);
}

QAutoResetEventEventFD::~QAutoResetEventEventFD()
{
    if (m_fd != -1)
        qt_safe_close(m_fd);
}

void QAutoResetEventEventFD::set()
{
    Q_ASSERT(isValid());

    constexpr uint64_t increment{ 1 };

    ScopedRTSanDisabler disabler; // opened via EFD_NONBLOCK
    qint64 bytesWritten = qt_safe_write(m_fd, &increment, sizeof(increment));
    if (bytesWritten == -1)
        qCritical("QAutoResetEvent::set failed");
}

} // namespace QtPrivate

QT_END_NAMESPACE

#include "moc_qautoresetevent_linux_p.cpp"
