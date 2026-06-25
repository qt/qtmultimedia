// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEG_RANGES_P_H
#define QFFMPEG_RANGES_P_H

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

#include <QtCore/qassert.h>
#include <QtCore/qtconfigmacros.h>
#include <QtMultimedia/private/qiteratorfacade_p.h>

#ifdef __cpp_lib_ranges
#  include <ranges>
#endif

QT_BEGIN_NAMESPACE

namespace QFFmpeg
{

template <typename T, T (*IterateFn)(void **)>
class FFmpegOpaqueIteratorRange
{
    struct begin_tag
    {
    };

public:
    class iterator : public QtMultimediaPrivate::IteratorFacade<iterator, T>
    {
        void *m_state = nullptr;
        T m_current = nullptr;

    public:
        explicit iterator(begin_tag) { increment(); }
        iterator() = default;

        void increment() { m_current = IterateFn(&m_state); }
        const T &dereference() const { return m_current; }
        bool equals(const iterator &o) const { return m_current == o.m_current; }
    };

    constexpr iterator begin() const { return iterator(begin_tag{}); }
    constexpr iterator end() const { return iterator(); }
};

template <typename T, T (*IterateFn)(T)>
class FFmpegValueIteratorRange
{
    struct begin_tag
    {
    };

public:
    class iterator : public QtMultimediaPrivate::IteratorFacade<iterator, T>
    {
        T m_current = {};

    public:
        explicit iterator(begin_tag) { increment(); }
        iterator() = default;

        void increment() { m_current = IterateFn(m_current); }
        const T &dereference() const { return m_current; }
        bool equals(const iterator &o) const { return m_current == o.m_current; }
    };

    constexpr iterator begin() const { return iterator(begin_tag{}); }
    constexpr iterator end() const { return iterator(); }
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#ifdef __cpp_lib_ranges
namespace std::ranges {
template <typename T, T (*IterateFn)(void **)>
inline constexpr bool enable_view<QT_PREPEND_NAMESPACE(QFFmpeg::FFmpegOpaqueIteratorRange)
                                    <T, IterateFn>> = true;

template <typename T, T (*IterateFn)(T)>
inline constexpr bool enable_view<QT_PREPEND_NAMESPACE(QFFmpeg::FFmpegValueIteratorRange)
                                    <T, IterateFn>> = true;
}
#endif

#endif
