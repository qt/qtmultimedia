// Copyright (C) 2024 The Qt Company Ltd.
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

#ifndef QAUTORESETEVENT_PIPE_P_H
#define QAUTORESETEVENT_PIPE_P_H

#include <QtCore/qsocketnotifier.h>
#include <QtMultimedia/qtmultimediaexports.h>
#include <atomic>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

class Q_MULTIMEDIA_EXPORT QAutoResetEventPipe final : public QObject
{
    Q_OBJECT

public:
    explicit QAutoResetEventPipe(QObject *parent = nullptr);
    ~QAutoResetEventPipe();
    Q_DISABLE_COPY_MOVE(QAutoResetEventPipe)

    bool isValid() const { return m_fdProducer != -1; }

    void set();

    template <typename... Args>
    QMetaObject::Connection callOnActivated(Args &&...args)
    {
        return connect(this, &QAutoResetEventPipe::activated, std::forward<Args>(args)...);
    }

    template <typename Functor>
    QMetaObject::Connection callOnActivated(Functor &&functor)
    {
        return connect(this, &QAutoResetEventPipe::activated, this, std::forward<Functor>(functor));
    }

Q_SIGNALS:
    void activated();

private:
    QSocketNotifier m_notifier;
    int m_fdProducer = -1;
    int m_fdConsumer = -1;
    std::atomic_flag m_consumePending{};
};

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QAUTORESETEVENT_PIPE_P_H
