// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qautoresetevent_win32_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

QAutoResetEventWin32::QAutoResetEventWin32(QObject *parent)
    : QObject{
          parent,
      },
      m_handle{
          ::CreateEventW(/*lpEventAttributes=*/0,
                         /*bManualReset=*/false,
                         /*bInitialState=*/false,
                         /*lpName=*/nullptr),
      },
      m_notifier{
          this,
      }
{
    if (!isValid()) {
        qCritical() << "CreateEventW failed:" << qt_error_string(GetLastError());
        return;
    }

    connect(&m_notifier, &QWinEventNotifier::activated, this, &QAutoResetEventWin32::activated);

    m_notifier.setHandle(m_handle.get());
    m_notifier.setEnabled(true);
}

void QAutoResetEventWin32::set()
{
    Q_ASSERT(isValid());

    bool status = ::SetEvent(m_handle.get());
    if (!status)
        qCritical("QAutoResetEvent::set failed");
}

} // namespace QtPrivate

QT_END_NAMESPACE

#include "moc_qautoresetevent_win32_p.cpp"
