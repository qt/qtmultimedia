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

#include <QtMultimedia/private/qiteratorfacade_p.h>

#ifdef __cpp_lib_ranges
#  include <ranges> // IWYU pragma: export
#endif

#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate::ranges {

#ifdef __cpp_lib_ranges

using std::ranges::range_value_t;

using std::ranges::all_of;
using std::ranges::any_of;
using std::ranges::copy;
using std::ranges::equal;
using std::ranges::equal_range;
using std::ranges::fill;
using std::ranges::find;
using std::ranges::find_if;
using std::ranges::for_each;
using std::ranges::lower_bound;
using std::ranges::max;
using std::ranges::max_element;
using std::ranges::min;
using std::ranges::min_element;
using std::ranges::none_of;
using std::ranges::sort;
using std::ranges::stable_sort;
using std::ranges::transform;
using std::ranges::upper_bound;

#else

template <typename R>
using range_iterator_t = decltype(std::begin(std::declval<R &>()));

template <typename It>
using iter_value_t = typename std::iterator_traits<It>::value_type;

template <typename R>
using range_value_t = iter_value_t<range_iterator_t<R>>;

// Caveat: best effort, not a 1-to-1 mapping to c++20 style ranges

inline constexpr auto all_of = [](auto &&range, auto predicate) {
    return std::all_of(std::begin(range), std::end(range), std::move(predicate));
};

inline constexpr auto any_of = [](auto &&range, auto predicate) {
    return std::any_of(std::begin(range), std::end(range), std::move(predicate));
};

inline constexpr auto none_of = [](auto &&range, auto predicate) {
    return std::none_of(std::begin(range), std::end(range), std::move(predicate));
};

inline constexpr auto for_each = [](auto &&range, auto func) {
    return std::for_each(std::begin(range), std::end(range), std::move(func));
};

inline constexpr auto copy = [](auto &&in, auto out) {
    return std::copy(std::begin(in), std::end(in), std::move(out));
};

inline constexpr auto fill = [](auto &&range, const auto &value) {
    return std::fill(std::begin(range), std::end(range), value);
};

inline constexpr auto find = [](auto &&range, const auto &value) {
    return std::find(std::begin(range), std::end(range), value);
};

inline constexpr auto find_if = [](auto &&range, auto predicate) {
    return std::find_if(std::begin(range), std::end(range), std::move(predicate));
};

template <typename Iterator>
struct subrange
{
    Iterator m_begin;
    Iterator m_end;
    constexpr Iterator begin() const { return m_begin; }
    constexpr Iterator end() const { return m_end; }
};

namespace impl {

struct lower_bound_fn
{
    template <typename Range, typename T, typename Comp = std::less<>>
    auto operator()(Range &&range, const T &value, Comp comp = {}) const
    {
        return std::lower_bound(std::begin(range), std::end(range), value, std::move(comp));
    }
};

struct upper_bound_fn
{
    template <typename Range, typename T, typename Comp = std::less<>>
    auto operator()(Range &&range, const T &value, Comp comp = {}) const
    {
        return std::upper_bound(std::begin(range), std::end(range), value, std::move(comp));
    }
};

struct equal_range_fn
{
    template <typename Range, typename T, typename Comp = std::less<>>
    auto operator()(Range &&range, const T &value, Comp comp = {}) const
    {
        auto [b, e] = std::equal_range(std::begin(range), std::end(range), value, std::move(comp));
        return subrange<decltype(b)>{ b, e };
    }
};

struct max_fn
{
    template <typename Range, typename Comp = std::less<>>
    auto operator()(Range &&range, Comp comp) const
    {
        auto it = std::max_element(std::begin(range), std::end(range), std::move(comp));
        return *it;
    }

    template <typename Range>
    auto operator()(Range &&range) const
    {
        auto it = std::max_element(std::begin(range), std::end(range));
        return *it;
    }
};

struct max_element_fn
{
    template <typename Range, typename Comp = std::less<>>
    auto operator()(Range &&range, Comp comp) const
    {
        return std::max_element(std::begin(range), std::end(range), std::move(comp));
    }

    template <typename Range>
    auto operator()(Range &&range) const
    {
        return std::max_element(std::begin(range), std::end(range));
    }
};

struct min_fn
{
    template <typename Range, typename Comp = std::less<>>
    auto operator()(Range &&range, Comp comp) const
    {
        auto it = std::min_element(std::begin(range), std::end(range), std::move(comp));
        return *it;
    }

    template <typename Range>
    auto operator()(Range &&range) const
    {
        auto it = std::min_element(std::begin(range), std::end(range));
        return *it;
    }
};

struct min_element_fn
{
    template <typename Range, typename Comp = std::less<>>
    auto operator()(Range &&range, Comp comp) const
    {
        return std::min_element(std::begin(range), std::end(range), std::move(comp));
    }

    template <typename Range>
    auto operator()(Range &&range) const
    {
        return std::min_element(std::begin(range), std::end(range));
    }
};

struct sort_fn
{
    template <typename Range, typename Comp = std::less<>>
    void operator()(Range &&range, Comp comp) const
    {
        std::sort(std::begin(range), std::end(range), std::move(comp));
    }

    template <typename Range>
    void operator()(Range &&range) const
    {
        std::sort(std::begin(range), std::end(range));
    }
};

struct stable_sort_fn
{
    template <typename Range, typename Comp = std::less<>>
    void operator()(Range &&range, Comp comp) const
    {
        std::stable_sort(std::begin(range), std::end(range), std::move(comp));
    }

    template <typename Range>
    void operator()(Range &&range) const
    {
        std::stable_sort(std::begin(range), std::end(range));
    }
};

struct equal_fn
{
    template <typename Range1, typename Range2, typename Pred = std::equal_to<>>
    bool operator()(Range1 &&lhs, Range2 &&rhs, Pred pred = {}) const
    {
        return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs),
                          std::move(pred));
    }
};

} // namespace impl

inline constexpr auto equal = impl::equal_fn{};
inline constexpr auto lower_bound = impl::lower_bound_fn{};
inline constexpr auto upper_bound = impl::upper_bound_fn{};
inline constexpr auto equal_range = impl::equal_range_fn{};
inline constexpr auto sort = impl::sort_fn{};
inline constexpr auto stable_sort = impl::stable_sort_fn{};
inline constexpr auto max = impl::max_fn{};
inline constexpr auto max_element = impl::max_element_fn{};
inline constexpr auto min = impl::min_fn{};
inline constexpr auto min_element = impl::min_element_fn{};
inline constexpr auto transform = [](auto &&range, auto output, auto op) {
    return std::transform(std::begin(range), std::end(range), std::move(output),
                          [op = std::move(op)](const auto &x) {
        return std::invoke(op, x);
    });
};

#endif

#if __cpp_lib_ranges_contains >= 202207L
using std::ranges::contains;
#else

inline constexpr auto contains = [](auto &&range, const auto &value) {
    return std::find(std::begin(range), std::end(range), value) != std::end(range);
};

#endif

#if __cpp_lib_ranges_to_container >= 202202L
using std::ranges::to;
#else

namespace impl {

template <typename Container>
struct to_adaptor
{
};

template <typename Container, typename Range>
Container operator|(Range &&range, to_adaptor<Container>)
{
    return Container(std::begin(range), std::end(range));
}

template <template <class...> class Container>
struct to_adaptor_template_template
{
};

template <template <class...> class Container, class Range>
auto operator|(Range &&range, to_adaptor_template_template<Container>)
{
    return Container<ranges::range_value_t<Range>>(std::begin(range), std::end(range));
}

} // namespace impl

template <typename Container, typename Range>
Container to(Range &&range)
{
    return Container(std::begin(range), std::end(range));
}

template <typename Container>
impl::to_adaptor<Container> to()
{
    return {};
}

template <template <class...> class Container>
auto to()
{
    return impl::to_adaptor_template_template<Container>{};
}
#endif

} // namespace QtMultimediaPrivate::ranges

namespace QtMultimediaPrivate::views {

#ifdef __cpp_lib_ranges
using std::views::filter;
using std::views::keys;
using std::views::transform;
using std::views::values;
#else

namespace impl {

template <typename Range>
class KeysView
{
    using BaseIt = decltype(std::begin(std::declval<Range &>()));
    using KeyReference = decltype((*std::declval<BaseIt>()).first);
    using KeyValue = std::remove_cv_t<std::remove_reference_t<KeyReference>>;

    class iterator
        : public IteratorFacade<iterator, KeyValue, std::input_iterator_tag, KeyReference>
    {
        BaseIt m_it;

    public:
        constexpr explicit iterator(BaseIt it) : m_it(std::move(it)) { }

        using reference = KeyReference;
        constexpr reference dereference() const { return (*m_it).first; }
        constexpr void increment() { ++m_it; }
        constexpr bool equals(const iterator &o) const { return m_it == o.m_it; }
    };

    Range &m_range;

public:
    constexpr explicit KeysView(Range &range) : m_range(range) { }
    constexpr iterator begin() const { return iterator{ std::begin(m_range) }; }
    constexpr iterator end() const { return iterator{ std::end(m_range) }; }
};

template <typename Range>
class ValuesView
{
    using BaseIt = decltype(std::begin(std::declval<Range &>()));
    using ValueReference = decltype((*std::declval<BaseIt>()).second);
    using MappedValue = std::remove_cv_t<std::remove_reference_t<ValueReference>>;

    class iterator
        : public IteratorFacade<iterator, MappedValue, std::input_iterator_tag, ValueReference>
    {
        BaseIt m_it;

    public:
        constexpr explicit iterator(BaseIt it) : m_it(std::move(it)) { }

        using reference = ValueReference;
        constexpr reference dereference() const { return (*m_it).second; }
        constexpr void increment() { ++m_it; }
        constexpr bool equals(const iterator &o) const { return m_it == o.m_it; }
    };

    Range &m_range;

public:
    constexpr explicit ValuesView(Range &range) : m_range(range) { }
    constexpr iterator begin() const { return iterator{ std::begin(m_range) }; }
    constexpr iterator end() const { return iterator{ std::end(m_range) }; }
};

struct keys_tag { };
struct values_tag { };

template <typename Range>
constexpr KeysView<const Range> operator|(const Range &range, keys_tag) { return KeysView<const Range>{ range }; }

template <typename Range>
constexpr ValuesView<const Range> operator|(const Range &range, values_tag) { return ValuesView<const Range>{ range }; }

template <typename Container, typename Predicate>
class FilterView
{
    using BaseIt = decltype(std::begin(std::declval<Container &>()));
    using ElementReference = decltype(*std::declval<BaseIt>());
    using ElementValue = std::remove_cv_t<std::remove_reference_t<ElementReference>>;

    class iterator
        : public IteratorFacade<iterator, ElementValue, std::input_iterator_tag, ElementReference>
    {
        BaseIt m_it;
        BaseIt m_end;
        Predicate m_pred;

        constexpr void advance()
        {
            while (m_it != m_end && !m_pred(*m_it))
                ++m_it;
        }

    public:
        using reference = ElementReference;

        constexpr iterator(BaseIt it, BaseIt end, Predicate pred)
            : m_it(std::move(it)), m_end(std::move(end)), m_pred(std::move(pred))
        {
            advance();
        }

        constexpr reference dereference() const { return *m_it; }
        constexpr void increment()
        {
            ++m_it;
            advance();
        }
        constexpr bool equals(const iterator &o) const { return m_it == o.m_it; };
    };

    Container &m_container;
    Predicate m_pred;

public:
    constexpr FilterView(Container &container, Predicate pred)
        : m_container(container), m_pred(std::move(pred))
    {
    }

    constexpr iterator begin() const
    {
        return iterator{
            std::begin(m_container),
            std::end(m_container),
            m_pred,
        };
    }

    constexpr iterator end() const
    {
        return iterator{
            std::end(m_container),
            std::end(m_container),
            m_pred,
        };
    }
};

template <typename Predicate>
struct FilterAdaptor
{
    Predicate pred;
};

// operator| is in impl so ADL finds it via the FilterAdaptor type
// Takes const Container& to allow binding both lvalues and rvalues (chained FilterViews).
// The resulting FilterView stores a const ref, valid for the lifetime of the full expression.
template <typename Container, typename Predicate>
constexpr auto operator|(const Container &container, FilterAdaptor<Predicate> adaptor)
{
    return FilterView<const Container, Predicate>(container, std::move(adaptor.pred));
}

template <typename Range, typename Transform>
class TransformView
{
    using BaseIt = decltype(std::begin(std::declval<Range &>()));
    using TransformedValue = std::remove_cv_t<std::remove_reference_t<
            decltype(std::invoke(std::declval<Transform>(), *std::declval<BaseIt>()))>>;

    class iterator : public IteratorFacade<iterator, TransformedValue, std::input_iterator_tag,
                                           TransformedValue>
    {
        BaseIt m_it;
        Transform m_transform;

    public:
        using reference = TransformedValue;

        constexpr iterator(BaseIt it, Transform transform)
            : m_it(std::move(it)), m_transform(std::move(transform))
        {
        }

        constexpr reference dereference() const { return std::invoke(m_transform, *m_it); }
        constexpr void increment() { ++m_it; }
        constexpr void decrement() { --m_it; }
        constexpr bool equals(const iterator &o) const { return m_it == o.m_it; }
    };

    Range &m_range;
    Transform m_transform;

public:
    constexpr TransformView(Range &range, Transform transform)
        : m_range(range), m_transform(std::move(transform))
    {
    }

    constexpr iterator begin() const { return iterator{ std::begin(m_range), m_transform }; }
    constexpr iterator end() const { return iterator{ std::end(m_range), m_transform }; }
};

template <typename Transform>
struct TransformAdaptor
{
    Transform transform;
};

template <typename Container, typename Transform>
constexpr auto operator|(const Container &container, TransformAdaptor<Transform> adaptor)
{
    return TransformView<const Container, Transform>(container, std::move(adaptor.transform));
}

} // namespace impl

template <typename Container, typename Predicate>
constexpr auto filter(const Container &container, Predicate pred)
{
    return impl::FilterView<const Container, Predicate>(container, std::move(pred));
}

template <typename Predicate>
constexpr auto filter(Predicate pred)
{
    return impl::FilterAdaptor<Predicate>{ std::move(pred) };
}

template <typename Container, typename Transform>
constexpr auto transform(const Container &container, Transform t)
{
    return impl::TransformView<const Container, Transform>(container, std::move(t));
}

template <typename Transform>
constexpr auto transform(Transform t)
{
    return impl::TransformAdaptor<Transform>{ std::move(t) };
}

inline constexpr impl::keys_tag keys{};
inline constexpr impl::values_tag values{};

#endif

inline constexpr auto filter_nonnull = views::filter([](const auto &arg) {
    return bool(arg);
});

} // namespace QtMultimediaPrivate::views

QT_END_NAMESPACE

#endif // QMULTIMEDIA_RANGES_P_H
