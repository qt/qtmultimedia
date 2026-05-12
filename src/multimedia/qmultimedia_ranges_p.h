// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMULTIMEDIA_RANGES_P_H
#define QMULTIMEDIA_RANGES_P_H

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

#ifdef __cpp_lib_ranges
#  include <ranges> // IWYU pragma: export
#endif

#include <algorithm>
#include <functional>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate::ranges {

#ifdef __cpp_lib_ranges
using std::ranges::all_of;
using std::ranges::any_of;
using std::ranges::copy;
using std::ranges::equal;
using std::ranges::fill;
using std::ranges::find;
using std::ranges::find_if;
using std::ranges::max;
using std::ranges::max_element;
using std::ranges::min;
using std::ranges::min_element;
using std::ranges::sort;
using std::ranges::stable_sort;

#else

// Caveat: best effort, not a 1-to-1 mapping to c++20 style ranges

inline constexpr auto all_of = [](const auto &range, auto predicate) {
    return std::all_of(std::begin(range), std::end(range), std::move(predicate));
};

inline constexpr auto any_of = [](const auto &range, auto predicate) {
    return std::any_of(std::begin(range), std::end(range), std::move(predicate));
};

inline constexpr auto copy = [](const auto &in, const auto &out) {
    return std::copy(in.begin(), in.end(), out);
};

inline constexpr auto equal = [](const auto &lhs, const auto &rhs, auto predicate) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::move(predicate));
};

inline constexpr auto fill = [](auto &range, auto value) {
    return std::fill(std::begin(range), std::end(range), std::move(value));
};

inline constexpr auto find = [](const auto &range, const auto &value) {
    return std::find(std::begin(range), std::end(range), value);
};

inline constexpr auto find_if = [](const auto &range, auto predicate) {
    return std::find_if(std::begin(range), std::end(range), std::move(predicate));
};

namespace impl {

struct max_fn
{
    template <typename Range, typename Comp = std::less<>>
    auto operator()(const Range &range, Comp comp) const
    {
        auto it = std::max_element(std::begin(range), std::end(range), std::move(comp));
        return *it;
    }

    template <typename Range>
    auto operator()(const Range &range) const
    {
        auto it = std::max_element(std::begin(range), std::end(range));
        return *it;
    }
};

struct max_element_fn
{
    template <typename Range, typename Comp = std::less<>>
    auto operator()(const Range &range, Comp comp) const
    {
        return std::max_element(std::begin(range), std::end(range), std::move(comp));
    }

    template <typename Range>
    auto operator()(const Range &range) const
    {
        return std::max_element(std::begin(range), std::end(range));
    }
};

struct min_fn
{
    template <typename Range, typename Comp = std::less<>>
    auto operator()(const Range &range, Comp comp) const
    {
        auto it = std::min_element(std::begin(range), std::end(range), std::move(comp));
        return *it;
    }

    template <typename Range>
    auto operator()(const Range &range) const
    {
        auto it = std::min_element(std::begin(range), std::end(range));
        return *it;
    }
};

struct min_element_fn
{
    template <typename Range, typename Comp = std::less<>>
    auto operator()(const Range &range, Comp comp) const
    {
        return std::min_element(std::begin(range), std::end(range), std::move(comp));
    }

    template <typename Range>
    auto operator()(const Range &range) const
    {
        return std::min_element(std::begin(range), std::end(range));
    }
};

struct sort_fn
{
    template <typename Range, typename Comp = std::less<>>
    void operator()(Range &range, Comp comp) const
    {
        std::sort(std::begin(range), std::end(range), std::move(comp));
    }

    template <typename Range>
    void operator()(Range &range) const
    {
        std::sort(std::begin(range), std::end(range));
    }
};

struct stable_sort_fn
{
    template <typename Range, typename Comp = std::less<>>
    void operator()(Range &range, Comp comp) const
    {
        std::stable_sort(std::begin(range), std::end(range), std::move(comp));
    }

    template <typename Range>
    void operator()(Range &range) const
    {
        std::stable_sort(std::begin(range), std::end(range));
    }
};

} // namespace impl

inline constexpr auto sort = impl::sort_fn{};
inline constexpr auto stable_sort = impl::stable_sort_fn{};
inline constexpr auto max = impl::max_fn{};
inline constexpr auto max_element = impl::max_element_fn{};
inline constexpr auto min = impl::min_fn{};
inline constexpr auto min_element = impl::min_element_fn{};

#endif

#if __cpp_lib_ranges_contains >= 202207L
using std::ranges::contains;
#else

inline constexpr auto contains = [](const auto &range, const auto &value) {
    return std::find(std::begin(range), std::end(range), value) != std::end(range);
};

#endif

} // namespace QtMultimediaPrivate::ranges

QT_END_NAMESPACE

#endif // QMULTIMEDIA_RANGES_P_H
