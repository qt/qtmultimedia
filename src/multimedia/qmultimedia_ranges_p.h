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

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate::ranges {

#ifdef __cpp_lib_ranges
using std::ranges::copy;
using std::ranges::equal;
using std::ranges::fill;

#else

// Caveat: best effort, not a 1-to-1 mapping to c++20 style ranges

constexpr auto copy = [](const auto &in, const auto &out) {
    return std::copy(in.begin(), in.end(), out);
};

constexpr auto equal = [](const auto &lhs, const auto &rhs, auto &&predicate) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), predicate);
};

constexpr auto fill = [](auto &range, auto &&value) {
    return std::fill(std::begin(range), std::end(range), value);
};

#endif

#if __cpp_lib_ranges_contains >= 202207L
using std::ranges::contains;
#else

constexpr auto contains = [](const auto &range, const auto &value) {
    return std::find(std::begin(range), std::end(range), value) != std::end(range);
};

#endif

} // namespace QtMultimediaPrivate::ranges

QT_END_NAMESPACE

#endif // QMULTIMEDIA_RANGES_P_H

