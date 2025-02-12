// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTAGGEDTIME_P_H
#define QTAGGEDTIME_P_H

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

#include "qcompare.h"

QT_BEGIN_NAMESPACE

template <typename T, typename Tag>
class QTaggedTime
{
public:
    explicit QTaggedTime(T value) : m_value(value) { }

    T get() const { return m_value; }

    QTaggedTime(const QTaggedTime &value) noexcept = default;

    friend bool comparesEqual(const QTaggedTime &lhs, const QTaggedTime &rhs) noexcept
    {
        return lhs.m_value == rhs.m_value;
    }

    friend Qt::strong_ordering compareThreeWay(const QTaggedTime &lhs,
                                               const QTaggedTime &rhs) noexcept
    {
        return qCompareThreeWay(lhs.m_value, rhs.m_value);
    }

    Q_DECLARE_STRONGLY_ORDERED(QTaggedTime);

    QTaggedTime operator+(const QTaggedTime &other) const noexcept
    {
        return QTaggedTime(m_value + other.m_value);
    }

    QTaggedTime operator-(const QTaggedTime &other) const noexcept
    {
        return QTaggedTime(m_value - other.m_value);
    }

    QTaggedTime &operator+=(const QTaggedTime &other) const noexcept
    {
        m_value += other.m_value;
        return *this;
    }

    QTaggedTime operator-=(const QTaggedTime &other) const noexcept
    {
        m_value -= other.m_value;
        return *this;
    }

private:
    T m_value;
};

QT_END_NAMESPACE

#endif // QTAGGEDTIME_P_H
