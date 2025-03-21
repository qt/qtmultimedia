// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMAYBE_P_H
#define QMAYBE_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <qstring.h>
#include <optional>
#include <utility>
#include <memory>

QT_BEGIN_NAMESPACE

template <class Error = QString>
struct QUnexpected
{
    constexpr QUnexpected(const QUnexpected &) = default;
    constexpr QUnexpected(QUnexpected &&) = default;

    template <class Err = Error,
              class Enabler = std::enable_if_t<!std::is_same_v<std::decay_t<Err>, QUnexpected>>>
    constexpr explicit QUnexpected(Err &&e) : e{ std::forward<Err>(e) }
    {
    }

    Error e;

    constexpr const Error &error() const & noexcept { return e; }
    constexpr Error &error() & noexcept { return e; }
    constexpr const Error &&error() const && noexcept { return std::move(e); };
    constexpr Error &&error() && noexcept { return std::move(e); }
};

template <class E>
QUnexpected(E) -> QUnexpected<E>;

struct QUnexpect
{
};

inline constexpr QUnexpect unexpect{};

template <typename Value, typename Error = QString>
class QMaybe
{
public:
    QMaybe(const Value &v)
    {
        if constexpr (std::is_pointer_v<Value>) {
            if (!v)
                return; // nullptr is treated as nullopt (for raw pointer types only)
        }
        m_value = v;
    }

    QMaybe(Value &&v)
    {
        if constexpr (std::is_pointer_v<Value>) {
            if (!v)
                return; // nullptr is treated as nullopt (for raw pointer types only)
        }
        m_value = std::move(v);
    }

    QMaybe(const QMaybe &other) = default;

    QMaybe &operator=(const QMaybe &other) = default;

    template <class... Args>
    QMaybe(QUnexpect, Args &&...args) : m_error{ std::forward<Args>(args)... }
    {
        static_assert(std::is_constructible_v<Error, Args &&...>,
                      "Invalid arguments for creating an error type");
    }

    template <class G>
    constexpr QMaybe(const QUnexpected<G> &e) : m_error{ e.error() }
    {
    }

    template <class G>
    constexpr QMaybe(QUnexpected<G> &&e) : m_error{ e.error() }
    {
    }

    // NOTE: Returns false if holding a nullptr value, even if no error is set.
    // This is different from std::expected, where a nullptr is a valid value, not
    // an error.
    constexpr explicit operator bool() const noexcept { return m_value.has_value(); }

    constexpr Value &value()
    {
        Q_ASSERT(m_value.has_value());
        return *m_value;
    }

    constexpr const Value &value() const
    {
        Q_ASSERT(m_value.has_value());
        return *m_value;
    }

    constexpr Value *operator->() noexcept { return std::addressof(value()); }
    constexpr const Value *operator->() const noexcept { return std::addressof(value()); }

    constexpr Value &operator*() &noexcept { return value(); }
    constexpr const Value &operator*() const &noexcept { return value(); }

    constexpr const Error &error() const { return m_error; }

private:
    QMaybe(std::nullptr_t) { }

    std::optional<Value> m_value;
    Error m_error{};
};

QT_END_NAMESPACE

#endif // QMAYBE_P_H
