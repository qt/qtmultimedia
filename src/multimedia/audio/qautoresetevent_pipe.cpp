// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qautoresetevent_pipe_p.h"

#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/qdebug.h>

#include <unistd.h>
#include <cstdint>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

QAutoResetEventPipe::QAutoResetEventPipe(QObject *parent)
        : QObject{
              parent,
          },
          m_notifier{
              QSocketNotifier::Type::Read,
          }
{
    int pipedes[2];
    int status = pipe(pipedes);
    if (status == -1) {
        qCritical() << "pipe failed:" << qt_error_string(errno);
        return;
    }

    m_fdConsumer = pipedes[0];
    m_fdProducer = pipedes[1];

    connect(&m_notifier, &QSocketNotifier::activated, this, [this] {
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_GCC("-Wmaybe-uninitialized")
        std::array<char, 1024> buffer;

        qint64 bytesRead = qt_safe_read(m_fdConsumer, buffer.data(), buffer.size());
        Q_ASSERT(bytesRead > 0);

        m_consumePending.clear();
        emit activated();
        QT_WARNING_POP
    });
    m_notifier.setSocket(m_fdConsumer);
    m_notifier.setEnabled(true);
}

QAutoResetEventPipe::~QAutoResetEventPipe()
{
    if (m_fdConsumer != -1) {
        qt_safe_close(m_fdConsumer);
        qt_safe_close(m_fdProducer);
    }
}

void QAutoResetEventPipe::set()
{
    Q_ASSERT(isValid());

    bool consumePending = m_consumePending.test_and_set();
    if (consumePending)
        return;

    constexpr uint8_t dummy{ 1 };

    qint64 bytesWritten = qt_safe_write(m_fdProducer, &dummy, sizeof(dummy));
    if (bytesWritten == -1)
        qCritical("QAutoResetEvent::set failed");
}

} // namespace QtPrivate

QT_END_NAMESPACE

#include "moc_qautoresetevent_pipe_p.cpp"
