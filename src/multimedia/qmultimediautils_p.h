// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMULTIMEDIAUTILS_P_H
#define QMULTIMEDIAUTILS_P_H

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

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/private/qglobal_p.h>
#include <qstring.h>
#include <qsize.h>
#include <utility>
#include <optional>

QT_BEGIN_NAMESPACE

struct QUnexpect
{
};

static constexpr QUnexpect unexpect{};

template<typename Value, typename Error = QString>
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

    QMaybe(const Error& error) : m_error(error) { }

    template<class... Args>
    QMaybe(QUnexpect, Args &&...args) : m_error{ std::forward<Args>(args)... }
    {
        static_assert(std::is_constructible_v<Error, Args &&...>,
                      "Invalid arguments for creating an error type");
    }

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

    constexpr Value *operator->() noexcept { return &value(); }
    constexpr const Value *operator->() const noexcept { return &value(); }

    constexpr Value &operator*() & noexcept { return value(); }
    constexpr const Value &operator*() const & noexcept { return value(); }

    constexpr const Error &error() const { return m_error; }

private:
    std::optional<Value> m_value;
    const Error m_error;
};

struct Fraction {
    int numerator;
    int denominator;
};

Q_MULTIMEDIA_EXPORT Fraction qRealToFraction(qreal value);

Q_MULTIMEDIA_EXPORT QSize qCalculateFrameSize(QSize resolution, Fraction pixelAspectRatio);

QT_END_NAMESPACE

#endif // QMULTIMEDIAUTILS_P_H

