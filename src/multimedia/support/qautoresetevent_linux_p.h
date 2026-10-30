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

#ifndef QAUTORESETEVENT_LINUX_P_H
#define QAUTORESETEVENT_LINUX_P_H

#include <QtMultimedia/qtmultimediaexports.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/private/quniquehandle_types_p.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

class Q_MULTIMEDIA_EXPORT QAutoResetEventEventFD final : public QObject
{
    Q_OBJECT

public:
    explicit QAutoResetEventEventFD(QObject *parent = nullptr);
    Q_DISABLE_COPY_MOVE(QAutoResetEventEventFD)

    bool isValid() const { return m_fd.isValid(); }
    void set();

    template <typename... Args>
    QMetaObject::Connection callOnActivated(Args &&...args)
    {
        return connect(this, &QAutoResetEventEventFD::activated, std::forward<Args>(args)...);
    }

    template <typename Functor>
    QMetaObject::Connection callOnActivated(Functor &&functor)
    {
        return connect(this, &QAutoResetEventEventFD::activated, this,
                       std::forward<Functor>(functor));
    }

Q_SIGNALS:
    void activated();

private:
    QUniqueFileDescriptorHandle m_fd;
    QSocketNotifier m_notifier;
};

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QAUTORESETEVENT_LINUX_P_H
