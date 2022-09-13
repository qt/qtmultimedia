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
#include <utility>
#include <optional>

QT_BEGIN_NAMESPACE

template<typename Value>
class QMaybe
{
public:
    QMaybe(const Value &v) : m_value(v) { }
    QMaybe(Value &&v) : m_value(std::move(v)) { }
    QMaybe(QString error) : m_error(std::move(error)) { }
    constexpr explicit operator bool() const { return bool(m_value); }
    constexpr Value &value() { return *m_value; }
    constexpr const Value &value() const { return *m_value; }
    constexpr const QString &error() const { return m_error; }
private:
    std::optional<Value> m_value;
    const QString m_error;
};

template<typename Value>
class QMaybe<Value *>
{
public:
    QMaybe(Value *v) : m_value(v) { }
    QMaybe(QString error) : m_error(std::move(error)) { }
    constexpr explicit operator bool() const { return bool(m_value); }
    constexpr Value *value() { return m_value; }
    constexpr const Value *value() const { return m_value; }
    constexpr const QString &error() const { return m_error; }
private:
    Value *m_value = nullptr;
    const QString m_error;
};

struct Fraction {
    int numerator;
    int denominator;
};

Q_MULTIMEDIA_EXPORT Fraction qRealToFraction(qreal value);

QT_END_NAMESPACE

#endif // QMULTIMEDIAUTILS_P_H

