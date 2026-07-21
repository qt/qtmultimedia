// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QITERATORFACADE_P_H
#define QITERATORFACADE_P_H

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

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qxptype_traits.h>

#include <iterator>
#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

namespace impl {

#if defined(__cpp_concepts)

template <typename T>
concept has_increment = requires(T & t)
{
    t.increment();
};

template <typename T>
concept has_decrement = requires(T & t)
{
    t.decrement();
};

template <typename T>
concept has_equals = requires(const T &t1, const T &t2)
{
    t1.equals(t2);
};

#else

template <typename T>
using increment_expr = decltype(std::declval<T &>().increment());

template <typename T>
using decrement_expr = decltype(std::declval<T &>().decrement());

template <typename T>
using equals_expr = decltype(std::declval<const T &>().equals(std::declval<const T &>()));

template <typename T>
constexpr bool has_increment = qxp::is_detected<increment_expr, T>::value;

template <typename T>
constexpr bool has_decrement = qxp::is_detected<decrement_expr, T>::value;

template <typename T>
constexpr bool has_equals = qxp::is_detected<equals_expr, T>::value;

#endif

} // namespace impl

// CRTP helper that turns a minimal iterator implementation into a full one.
//
// Derived must always implement:
//   <value> dereference() const          - access the current element
//
// and, for single-pass/forward/bidirectional iteration:
//   void increment()                     - move one step forward
//   bool equals(const Derived &o) const  - compare iterator positions
//   void decrement()                     - move one step backward (optional; enables operator--)
//
// For random access iteration, Derived may instead (or additionally) implement:
//   void advance_by(difference_type n)          - move n steps forward (n may be negative)
//   difference_type distance_to(const Derived &o) const
//                                                - signed number of steps from *this to o
template <typename Derived, typename ValueType, typename IteratorCategory = std::input_iterator_tag,
          typename Reference = ValueType &, typename DifferenceType = std::ptrdiff_t>
class IteratorFacade
{
public:
    using iterator_category = IteratorCategory;
    using value_type = ValueType;
    using reference = Reference;
    using difference_type = DifferenceType;
    using pointer = std::conditional_t<std::is_reference_v<Reference>,
                                        std::remove_reference_t<Reference> *, void>;

    constexpr decltype(auto) operator*() const { return derived().dereference(); }

    constexpr Derived &operator++()
    {
        stepForward();
        return derived();
    }

    constexpr Derived operator++(int)
    {
        Derived result = derived();
        stepForward();
        return result;
    }

    constexpr Derived &operator--()
    {
        stepBackward();
        return derived();
    }

    constexpr Derived operator--(int)
    {
        Derived result = derived();
        stepBackward();
        return result;
    }

    template <typename N>
#if defined(__cpp_concepts)
    requires requires(Derived &d, N n)
    {
        d.advance_by(n);
    }
#endif
    constexpr Derived &operator+=(N n)
    {
        derived().advance_by(n);
        return derived();
    }

    template <typename N>
#if defined(__cpp_concepts)
    requires requires(Derived &d, N n)
    {
        d.advance_by(n);
    }
#endif
    constexpr Derived &operator-=(N n)
    {
        derived().advance_by(-n);
        return derived();
    }

    template <typename N>
#if defined(__cpp_concepts)
    requires requires(Derived &d, N n)
    {
        d.advance_by(n);
    }
#endif
    friend constexpr Derived operator+(Derived it, N n)
    {
        it.advance_by(n);
        return it;
    }

    template <typename N>
#if defined(__cpp_concepts)
    requires requires(Derived &d, N n)
    {
        d.advance_by(n);
    }
#endif
    friend constexpr Derived operator+(N n, Derived it)
    {
        it.advance_by(n);
        return it;
    }

    template <typename N>
#if defined(__cpp_concepts)
    requires requires(Derived &d, N n)
    {
        d.advance_by(n);
    }
#endif
    friend constexpr Derived operator-(Derived it, N n)
    {
        it.advance_by(-n);
        return it;
    }

    template <typename N>
#if defined(__cpp_concepts)
    requires requires(Derived &d, N n)
    {
        d.advance_by(n);
    }
#endif
    friend constexpr Derived operator-(N n, Derived it)
    {
        it.advance_by(-n);
        return it;
    }

    template <typename N>
#if defined(__cpp_concepts)
    requires requires(Derived &d, N n)
    {
        d.advance_by(n);
    }
#endif
    constexpr decltype(auto) operator[](N n) const
    {
        Derived tmp = derived();
        tmp.advance_by(n);
        return tmp.dereference();
    }

    friend constexpr auto operator-(const Derived &lhs, const Derived &rhs)
#if defined(__cpp_concepts)
            requires requires(const Derived &d)
    {
        d.distance_to(d);
    }
#endif
    {
        return rhs.distance_to(lhs);
    }

    friend constexpr bool operator<(const Derived &lhs, const Derived &rhs)
#if defined(__cpp_concepts)
            requires requires(const Derived &d)
    {
        d.distance_to(d);
    }
#endif
    {
        return lhs.distance_to(rhs) > 0;
    }

    friend constexpr bool operator<=(const Derived &lhs, const Derived &rhs)
#if defined(__cpp_concepts)
            requires requires(const Derived &d)
    {
        d.distance_to(d);
    }
#endif
    {
        return lhs.distance_to(rhs) >= 0;
    }

    friend constexpr bool operator>(const Derived &lhs, const Derived &rhs)
#if defined(__cpp_concepts)
            requires requires(const Derived &d)
    {
        d.distance_to(d);
    }
#endif
    {
        return lhs.distance_to(rhs) < 0;
    }

    friend constexpr bool operator>=(const Derived &lhs, const Derived &rhs)
#if defined(__cpp_concepts)
            requires requires(const Derived &d)
    {
        d.distance_to(d);
    }
#endif
    {
        return lhs.distance_to(rhs) <= 0;
    }

    friend constexpr bool operator==(const Derived &lhs, const Derived &rhs)
    {
        return lhs.equalsImpl(rhs);
    }

    friend constexpr bool operator!=(const Derived &lhs, const Derived &rhs)
    {
        return !lhs.equalsImpl(rhs);
    }

private:
    constexpr Derived &derived() { return static_cast<Derived &>(*this); }
    constexpr const Derived &derived() const { return static_cast<const Derived &>(*this); }

    constexpr void stepForward() { stepForwardDispatch<impl::has_increment<Derived>>(); }

    template <bool HasIncrement>
    constexpr void stepForwardDispatch()
    {
        if constexpr (HasIncrement)
            derived().increment();
        else
            derived().advance_by(1);
    }

    constexpr void stepBackward() { stepBackwardDispatch<impl::has_decrement<Derived>>(); }

    template <bool HasDecrement>
    constexpr void stepBackwardDispatch()
    {
        if constexpr (HasDecrement)
            derived().decrement();
        else
            derived().advance_by(-1);
    }

    constexpr bool equalsImpl(const Derived &other) const
    {
        return equalsDispatch<impl::has_equals<Derived>>(other);
    }

    template <bool HasEquals>
    constexpr bool equalsDispatch(const Derived &other) const
    {
        if constexpr (HasEquals)
            return derived().equals(other);
        else
            return derived().distance_to(other) == 0;
    }
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QITERATORFACADE_P_H
