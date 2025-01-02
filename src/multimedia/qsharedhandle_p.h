// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSHAREDHANDLE_P_H
#define QSHAREDHANDLE_P_H

#include <QtCore/private/quniquehandle_p.h>
#include <QtCore/qtconfigmacros.h>

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

QT_BEGIN_NAMESPACE

template <typename HandleTraits>
struct QSharedHandle : private QUniqueHandle<HandleTraits>
{
    using BaseClass = QUniqueHandle<HandleTraits>;

    enum RefMode
    {
        HasRef,
        NeedsRef
    };

    QSharedHandle() = default;

    explicit QSharedHandle(typename HandleTraits::Type object, RefMode mode)
        : BaseClass{ mode == NeedsRef ? HandleTraits::ref(object) : object }
    {
    }

    QSharedHandle(const QSharedHandle &o)
        : BaseClass{
              HandleTraits::ref(o.get()),
          }
    {
    }

    QSharedHandle(QSharedHandle &&) noexcept = default;

    QSharedHandle &operator=(const QSharedHandle &o) // NOLINT: bugprone-unhandled-self-assign
    {
        if (BaseClass::get() != o.get())
            reset(HandleTraits::ref(o.get()));
        return *this;
    };

    QSharedHandle &operator=(QSharedHandle &&o) noexcept
    {
        if (get() != o.get())
            reset(o.release());
        else
            o.close();

        return *this;
    }

    [[nodiscard]] friend bool operator==(const QSharedHandle &lhs,
                                         const QSharedHandle &rhs) noexcept
    {
        return lhs.get() == rhs.get();
    }

    [[nodiscard]] friend bool operator!=(const QSharedHandle &lhs,
                                         const QSharedHandle &rhs) noexcept
    {
        return lhs.get() != rhs.get();
    }

    [[nodiscard]] friend bool operator<(const QSharedHandle &lhs, const QSharedHandle &rhs) noexcept
    {
        return lhs.get() < rhs.get();
    }

    [[nodiscard]] friend bool operator<=(const QSharedHandle &lhs,
                                         const QSharedHandle &rhs) noexcept
    {
        return lhs.get() <= rhs.get();
    }

    [[nodiscard]] friend bool operator>(const QSharedHandle &lhs, const QSharedHandle &rhs) noexcept
    {
        return lhs.get() > rhs.get();
    }

    [[nodiscard]] friend bool operator>=(const QSharedHandle &lhs,
                                         const QSharedHandle &rhs) noexcept
    {
        return lhs.get() >= rhs.get();
    }

    using BaseClass::get;
    using BaseClass::isValid;
    using BaseClass::operator bool;
    using BaseClass::release;
    using BaseClass::reset;
    using BaseClass::operator&;
    using BaseClass::close;
};

QT_END_NAMESPACE

#endif // QSHAREDHANDLE_P_H
