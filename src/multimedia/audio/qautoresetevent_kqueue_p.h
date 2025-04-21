// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QAUTORESETEVENT_KQUEUE_P_H
#define QAUTORESETEVENT_KQUEUE_P_H

#include <QtCore/qsocketnotifier.h>
#include <QtMultimedia/qtmultimediaexports.h>

#include <sys/event.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

class Q_MULTIMEDIA_EXPORT QAutoResetEventKQueue final : public QObject
{
    Q_OBJECT

public:
    explicit QAutoResetEventKQueue(QObject *parent = nullptr);
    ~QAutoResetEventKQueue();
    Q_DISABLE_COPY_MOVE(QAutoResetEventKQueue)

    bool isValid() const;
    void set();

    template <typename... Args>
    QMetaObject::Connection callOnActivated(Args &&...args)
    {
        return connect(this, &QAutoResetEventKQueue::activated, std::forward<Args>(args)...);
    }

    template <typename Functor>
    QMetaObject::Connection callOnActivated(Functor &&functor)
    {
        return connect(this, &QAutoResetEventKQueue::activated, this,
                       std::forward<Functor>(functor));
    }

Q_SIGNALS:
    void activated();

private:
    QSocketNotifier m_notifier;
    int m_kqueue = -1;
    struct kevent m_event;
};

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QAUTORESETEVENT_KQUEUE_P_H
