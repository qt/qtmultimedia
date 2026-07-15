// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <private/qmultimedia_ranges_p.h>

#include <map>
#include <string>
#include <vector>

namespace ranges = QtMultimediaPrivate::ranges;
namespace views = QtMultimediaPrivate::views;

class tst_QMultimediaRanges : public QObject
{
    Q_OBJECT

private slots:
    // ranges::all_of
    void allOf_emptyRange_returnsTrue();
    void allOf_allMatch_returnsTrue();
    void allOf_oneDoesNotMatch_returnsFalse();

    // ranges::any_of
    void anyOf_emptyRange_returnsFalse();
    void anyOf_oneMatches_returnsTrue();
    void anyOf_noneMatch_returnsFalse();

    // ranges::copy
    void copy_copiesIntoPreallocated();
    void copy_copiesViaBackInserter();

    // ranges::none_of
    void noneOf_emptyRange_returnsTrue();
    void noneOf_noneMatch_returnsTrue();
    void noneOf_oneMatches_returnsFalse();

    // ranges::for_each
    void forEach_callsFuncForEachElement();
    void forEach_emptyRange_doesNotCallFunc();

    // ranges::equal
    void equal_identicalRanges_returnsTrue();
    void equal_differentValues_returnsFalse();
    void equal_differentSizes_returnsFalse();
    void equal_withoutPredicate_usesEqualityOperator();

    // ranges::fill
    void fill_setsAllElements();

    // ranges::find
    void find_found_returnsIteratorToElement();
    void find_notFound_returnsEnd();
    void find_emptyRange_returnsEnd();

    // ranges::find_if
    void findIf_found_returnsFirstMatch();
    void findIf_notFound_returnsEnd();

    // ranges::lower_bound / upper_bound / equal_range
    void lowerBound_returnsFirstNotLess();
    void upperBound_returnsFirstGreater();
    void equalRange_returnsMatchingSubrange();
    void equalRange_noMatch_returnsEmptySubrange();

    // ranges::sort / stable_sort
    void sort_sortsInAscendingOrder();
    void sort_withComparator_sortsDescending();
    void stableSort_preservesRelativeOrderOfEqualElements();

    // ranges::max / min / max_element / min_element
    void max_returnsLargestElement();
    void max_withComparator();
    void maxElement_returnsIteratorToLargest();
    void min_returnsSmallestElement();
    void min_withComparator();
    void minElement_returnsIteratorToSmallest();

    // ranges::transform (output iterator form)
    void transform_outputIterator_transformsElements();
    void transform_outputIterator_pointerToMemberData();
    void transform_outputIterator_pointerToMemberFunction();

    // ranges::contains
    void contains_elementPresent_returnsTrue();
    void contains_elementAbsent_returnsFalse();
    void contains_emptyRange_returnsFalse();

    // ranges::to
    void to_pipeIntoStdVector();
    void to_pipeIntoQList();
    void to_directCall();

    // views::filter
    void filter_keepsMatchingElements();
    void filter_allMatch_keepsAll();
    void filter_noMatches_producesEmptyRange();
    void filter_emptyInput_producesEmptyRange();
    void filter_pipeAdaptor();

    // views::keys / values
    void keys_extractsKeysFromMap();
    void values_extractsValuesFromPairs();

    // views::transform
    void viewsTransform_transformsEachElement();
    void viewsTransform_changesType();
    void viewsTransform_emptyInput_producesEmptyOutput();
    void viewsTransform_pipeAdaptor();
    void viewsTransform_pointerToMemberData();
    void viewsTransform_pipeAdaptor_pointerToMemberData();

    // Compositions
    void composition_filterThenTransformThenTo();
    void composition_transformThenTo();
    void composition_keysWithTransform();
};

// --- ranges::all_of ---

void tst_QMultimediaRanges::allOf_emptyRange_returnsTrue()
{
    std::vector<int> v;
    QVERIFY(ranges::all_of(v, [](int) {
        return false;
    }));
}

void tst_QMultimediaRanges::allOf_allMatch_returnsTrue()
{
    std::vector<int> v = { 2, 4, 6, 8 };
    QVERIFY(ranges::all_of(v, [](int x) {
        return x % 2 == 0;
    }));
}

void tst_QMultimediaRanges::allOf_oneDoesNotMatch_returnsFalse()
{
    std::vector<int> v = { 2, 4, 5, 8 };
    QVERIFY(!ranges::all_of(v, [](int x) {
        return x % 2 == 0;
    }));
}

// --- ranges::any_of ---

void tst_QMultimediaRanges::anyOf_emptyRange_returnsFalse()
{
    std::vector<int> v;
    QVERIFY(!ranges::any_of(v, [](int) {
        return true;
    }));
}

void tst_QMultimediaRanges::anyOf_oneMatches_returnsTrue()
{
    std::vector<int> v = { 1, 3, 4, 7 };
    QVERIFY(ranges::any_of(v, [](int x) {
        return x % 2 == 0;
    }));
}

void tst_QMultimediaRanges::anyOf_noneMatch_returnsFalse()
{
    std::vector<int> v = { 1, 3, 5, 7 };
    QVERIFY(!ranges::any_of(v, [](int x) {
        return x % 2 == 0;
    }));
}

// --- ranges::none_of ---

void tst_QMultimediaRanges::noneOf_emptyRange_returnsTrue()
{
    std::vector<int> v;
    QVERIFY(ranges::none_of(v, [](int) {
        return true;
    }));
}

void tst_QMultimediaRanges::noneOf_noneMatch_returnsTrue()
{
    std::vector<int> v = { 1, 3, 5, 7 };
    QVERIFY(ranges::none_of(v, [](int x) {
        return x % 2 == 0;
    }));
}

void tst_QMultimediaRanges::noneOf_oneMatches_returnsFalse()
{
    std::vector<int> v = { 1, 3, 4, 7 };
    QVERIFY(!ranges::none_of(v, [](int x) {
        return x % 2 == 0;
    }));
}

// --- ranges::for_each ---

void tst_QMultimediaRanges::forEach_callsFuncForEachElement()
{
    std::vector<int> v = { 1, 2, 3, 4 };
    std::vector<int> seen;
    ranges::for_each(v, [&](int x) {
        seen.push_back(x);
    });
    QCOMPARE(seen, v);
}

void tst_QMultimediaRanges::forEach_emptyRange_doesNotCallFunc()
{
    std::vector<int> v;
    int callCount = 0;
    ranges::for_each(v, [&](int) {
        ++callCount;
    });
    QCOMPARE(callCount, 0);
}

// --- ranges::copy ---

void tst_QMultimediaRanges::copy_copiesIntoPreallocated()
{
    std::vector<int> src = { 1, 2, 3, 4, 5 };
    std::vector<int> dst(5, 0);
    ranges::copy(src, dst.begin());
    QCOMPARE(dst, src);
}

void tst_QMultimediaRanges::copy_copiesViaBackInserter()
{
    std::vector<int> src = { 10, 20, 30 };
    std::vector<int> dst;
    ranges::copy(src, std::back_inserter(dst));
    QCOMPARE(dst, src);
}

// --- ranges::equal ---

void tst_QMultimediaRanges::equal_identicalRanges_returnsTrue()
{
    std::vector<int> a = { 1, 2, 3 };
    std::vector<int> b = { 1, 2, 3 };
    QVERIFY(ranges::equal(a, b, [](int x, int y) {
        return x == y;
    }));
}

void tst_QMultimediaRanges::equal_differentValues_returnsFalse()
{
    std::vector<int> a = { 1, 2, 3 };
    std::vector<int> b = { 1, 2, 4 };
    QVERIFY(!ranges::equal(a, b, [](int x, int y) {
        return x == y;
    }));
}

void tst_QMultimediaRanges::equal_differentSizes_returnsFalse()
{
    std::vector<int> a = { 1, 2, 3 };
    std::vector<int> b = { 1, 2 };
    QVERIFY(!ranges::equal(a, b, [](int x, int y) {
        return x == y;
    }));
}

void tst_QMultimediaRanges::equal_withoutPredicate_usesEqualityOperator()
{
    std::vector<int> a = { 1, 2, 3 };
    std::vector<int> b = { 1, 2, 3 };
    std::vector<int> c = { 1, 2, 4 };
    QVERIFY(ranges::equal(a, b));
    QVERIFY(!ranges::equal(a, c));
}

// --- ranges::fill ---

void tst_QMultimediaRanges::fill_setsAllElements()
{
    std::vector<int> v(5, 0);
    ranges::fill(v, 42);
    QVERIFY(ranges::all_of(v, [](int x) {
        return x == 42;
    }));
}

// --- ranges::find ---

void tst_QMultimediaRanges::find_found_returnsIteratorToElement()
{
    std::vector<int> v = { 10, 20, 30, 40 };
    auto it = ranges::find(v, 30);
    QVERIFY(it != v.end());
    QCOMPARE(*it, 30);
}

void tst_QMultimediaRanges::find_notFound_returnsEnd()
{
    std::vector<int> v = { 10, 20, 30 };
    QCOMPARE(ranges::find(v, 99), v.end());
}

void tst_QMultimediaRanges::find_emptyRange_returnsEnd()
{
    std::vector<int> v;
    QCOMPARE(ranges::find(v, 1), v.end());
}

// --- ranges::find_if ---

void tst_QMultimediaRanges::findIf_found_returnsFirstMatch()
{
    std::vector<int> v = { 1, 3, 4, 6, 7 };
    auto it = ranges::find_if(v, [](int x) {
        return x % 2 == 0;
    });
    QVERIFY(it != v.end());
    QCOMPARE(*it, 4); // first even, not 6
}

void tst_QMultimediaRanges::findIf_notFound_returnsEnd()
{
    std::vector<int> v = { 1, 3, 5, 7 };
    QCOMPARE(ranges::find_if(v,
                             [](int x) {
        return x % 2 == 0;
    }),
             v.end());
}

// --- ranges::lower_bound / upper_bound / equal_range ---

void tst_QMultimediaRanges::lowerBound_returnsFirstNotLess()
{
    std::vector<int> v = { 1, 2, 4, 4, 7 };
    auto it = ranges::lower_bound(v, 4);
    QVERIFY(it != v.end());
    QCOMPARE(std::distance(v.begin(), it), 2); // index of first 4
}

void tst_QMultimediaRanges::upperBound_returnsFirstGreater()
{
    std::vector<int> v = { 1, 2, 4, 4, 7 };
    auto it = ranges::upper_bound(v, 4);
    QVERIFY(it != v.end());
    QCOMPARE(*it, 7);
    QCOMPARE(std::distance(v.begin(), it), 4); // past the last 4
}

void tst_QMultimediaRanges::equalRange_returnsMatchingSubrange()
{
    std::vector<int> v = { 1, 2, 4, 4, 7 };
    auto sub = ranges::equal_range(v, 4);
    std::vector<int> result(sub.begin(), sub.end());
    QCOMPARE(result, (std::vector<int>{ 4, 4 }));
}

void tst_QMultimediaRanges::equalRange_noMatch_returnsEmptySubrange()
{
    std::vector<int> v = { 1, 2, 4, 7 };
    auto sub = ranges::equal_range(v, 5);
    QVERIFY(sub.begin() == sub.end());
}

// --- ranges::sort / stable_sort ---

void tst_QMultimediaRanges::sort_sortsInAscendingOrder()
{
    std::vector<int> v = { 5, 3, 1, 4, 2 };
    ranges::sort(v);
    QCOMPARE(v, (std::vector<int>{ 1, 2, 3, 4, 5 }));
}

void tst_QMultimediaRanges::sort_withComparator_sortsDescending()
{
    std::vector<int> v = { 5, 3, 1, 4, 2 };
    ranges::sort(v, std::greater<int>{});
    QCOMPARE(v, (std::vector<int>{ 5, 4, 3, 2, 1 }));
}

void tst_QMultimediaRanges::stableSort_preservesRelativeOrderOfEqualElements()
{
    using P = std::pair<int, int>;
    // Sort by first element only; second encodes original position.
    std::vector<P> v = { { 2, 0 }, { 1, 1 }, { 2, 2 }, { 1, 3 } };
    ranges::stable_sort(v, [](const P &a, const P &b) {
        return a.first < b.first;
    });
    QCOMPARE(v[0], P(1, 1));
    QCOMPARE(v[1], P(1, 3));
    QCOMPARE(v[2], P(2, 0));
    QCOMPARE(v[3], P(2, 2));
}

// --- ranges::max / min ---

void tst_QMultimediaRanges::max_returnsLargestElement()
{
    std::vector<int> v = { 3, 1, 9, 4, 5 };
    QCOMPARE(ranges::max(v), 9);
}

void tst_QMultimediaRanges::max_withComparator()
{
    std::vector<int> v = { -5, 3, -1, 4 };
    // largest by absolute value
    QCOMPARE(ranges::max(v,
                         [](int a, int b) {
        return std::abs(a) < std::abs(b);
    }),
             -5);
}

void tst_QMultimediaRanges::maxElement_returnsIteratorToLargest()
{
    std::vector<int> v = { 3, 1, 9, 4 };
    auto it = ranges::max_element(v);
    QVERIFY(it != v.end());
    QCOMPARE(*it, 9);
    QCOMPARE(std::distance(v.begin(), it), 2);
}

void tst_QMultimediaRanges::min_returnsSmallestElement()
{
    std::vector<int> v = { 3, 1, 4, 1, 5 };
    QCOMPARE(ranges::min(v), 1);
}

void tst_QMultimediaRanges::min_withComparator()
{
    std::vector<int> v = { -5, 3, -1, 4 };
    // smallest by absolute value
    QCOMPARE(ranges::min(v,
                         [](int a, int b) {
        return std::abs(a) < std::abs(b);
    }),
             -1);
}

void tst_QMultimediaRanges::minElement_returnsIteratorToSmallest()
{
    std::vector<int> v = { 3, 1, 4, 2 };
    auto it = ranges::min_element(v);
    QVERIFY(it != v.end());
    QCOMPARE(*it, 1);
    QCOMPARE(std::distance(v.begin(), it), 1);
}

// --- ranges::transform (output-iterator form) ---

void tst_QMultimediaRanges::transform_outputIterator_transformsElements()
{
    std::vector<int> src = { 1, 2, 3, 4 };
    std::vector<int> dst(4);
    ranges::transform(src, dst.begin(), [](int x) {
        return x * 2;
    });
    QCOMPARE(dst, (std::vector<int>{ 2, 4, 6, 8 }));
}

void tst_QMultimediaRanges::transform_outputIterator_pointerToMemberData()
{
    struct Item { int value; };
    std::vector<Item> src = { { 10 }, { 20 }, { 30 } };
    std::vector<int> dst(3);
    ranges::transform(src, dst.begin(), &Item::value);
    QCOMPARE(dst, (std::vector<int>{ 10, 20, 30 }));
}

void tst_QMultimediaRanges::transform_outputIterator_pointerToMemberFunction()
{
    struct Item {
        int value;
        int doubled() const { return value * 2; }
    };
    std::vector<Item> src = { { 1 }, { 2 }, { 3 } };
    std::vector<int> dst(3);
    ranges::transform(src, dst.begin(), &Item::doubled);
    QCOMPARE(dst, (std::vector<int>{ 2, 4, 6 }));
}

// --- ranges::contains ---

void tst_QMultimediaRanges::contains_elementPresent_returnsTrue()
{
    std::vector<int> v = { 1, 2, 3, 4, 5 };
    QVERIFY(ranges::contains(v, 3));
}

void tst_QMultimediaRanges::contains_elementAbsent_returnsFalse()
{
    std::vector<int> v = { 1, 2, 3 };
    QVERIFY(!ranges::contains(v, 99));
}

void tst_QMultimediaRanges::contains_emptyRange_returnsFalse()
{
    std::vector<int> v;
    QVERIFY(!ranges::contains(v, 0));
}

// --- ranges::to ---

void tst_QMultimediaRanges::to_pipeIntoStdVector()
{
    std::vector<int> src = { 1, 2, 3 };
    auto result = src | ranges::to<std::vector<int>>();
    QCOMPARE(result, src);

    auto result_2 = src | ranges::to<std::vector>();
    QCOMPARE(result_2, src);
}

void tst_QMultimediaRanges::to_pipeIntoQList()
{
    std::vector<int> src = { 10, 20, 30 };
    auto result = src | ranges::to<QList<int>>();
    QCOMPARE(result, (QList<int>{ 10, 20, 30 }));
}

void tst_QMultimediaRanges::to_directCall()
{
    std::vector<int> src = { 5, 6, 7 };
    auto result = ranges::to<std::vector<int>>(src);
    QCOMPARE(result, src);
}

// --- views::filter ---

void tst_QMultimediaRanges::filter_keepsMatchingElements()
{
    std::vector<int> v = { 1, 2, 3, 4, 5, 6 };
    auto result = ranges::to<std::vector<int>>(views::filter(v, [](int x) {
        return x % 2 == 0;
    }));
    QCOMPARE(result, (std::vector<int>{ 2, 4, 6 }));
}

void tst_QMultimediaRanges::filter_allMatch_keepsAll()
{
    std::vector<int> v = { 2, 4, 6 };
    auto result = ranges::to<std::vector<int>>(views::filter(v, [](int x) {
        return x % 2 == 0;
    }));
    QCOMPARE(result, v);
}

void tst_QMultimediaRanges::filter_noMatches_producesEmptyRange()
{
    std::vector<int> v = { 1, 3, 5 };
    auto filtered = views::filter(v, [](int x) {
        return x % 2 == 0;
    });
    QVERIFY(filtered.begin() == filtered.end());
}

void tst_QMultimediaRanges::filter_emptyInput_producesEmptyRange()
{
    std::vector<int> v;
    auto filtered = views::filter(v, [](int) {
        return true;
    });
    QVERIFY(filtered.begin() == filtered.end());
}

void tst_QMultimediaRanges::filter_pipeAdaptor()
{
    std::vector<int> v = { 1, 2, 3, 4, 5 };
    auto result = v | views::filter([](int x) {
        return x > 3;
    }) | ranges::to<std::vector<int>>();
    QCOMPARE(result, (std::vector<int>{ 4, 5 }));
}

// --- views::keys / values ---

void tst_QMultimediaRanges::keys_extractsKeysFromMap()
{
    std::map<int, std::string> m = { { 1, "a" }, { 2, "b" }, { 3, "c" } };
    auto result = m | views::keys | ranges::to<std::vector<int>>();
    QCOMPARE(result, (std::vector<int>{ 1, 2, 3 }));
}

void tst_QMultimediaRanges::values_extractsValuesFromPairs()
{
    std::vector<std::pair<int, int>> v = { { 1, 10 }, { 2, 20 }, { 3, 30 } };
    auto result = v | views::values | ranges::to<std::vector<int>>();
    QCOMPARE(result, (std::vector<int>{ 10, 20, 30 }));
}

// --- views::transform ---

void tst_QMultimediaRanges::viewsTransform_transformsEachElement()
{
    std::vector<int> v = { 1, 2, 3, 4 };
    auto result = views::transform(v, [](int x) {
        return x * x;
    }) | ranges::to<std::vector<int>>();
    QCOMPARE(result, (std::vector<int>{ 1, 4, 9, 16 }));
}

void tst_QMultimediaRanges::viewsTransform_changesType()
{
    std::vector<int> v = { 1, 22, 333 };
    auto result = v | views::transform([](int x) {
        return std::to_string(x);
    }) | ranges::to<std::vector<std::string>>();
    QCOMPARE(result, (std::vector<std::string>{ "1", "22", "333" }));
}

void tst_QMultimediaRanges::viewsTransform_emptyInput_producesEmptyOutput()
{
    std::vector<int> v;
    auto transformed = views::transform(v, [](int x) {
        return x;
    });
    QVERIFY(transformed.begin() == transformed.end());
}

void tst_QMultimediaRanges::viewsTransform_pipeAdaptor()
{
    std::vector<int> v = { 1, 2, 3 };
    auto result = v | views::transform([](int x) {
        return x + 10;
    }) | ranges::to<std::vector<int>>();
    QCOMPARE(result, (std::vector<int>{ 11, 12, 13 }));
}

void tst_QMultimediaRanges::viewsTransform_pointerToMemberData()
{
    struct Item { int value; };
    std::vector<Item> src = { { 10 }, { 20 }, { 30 } };
    auto result = views::transform(src, &Item::value) | ranges::to<std::vector<int>>();
    QCOMPARE(result, (std::vector<int>{ 10, 20, 30 }));
}

void tst_QMultimediaRanges::viewsTransform_pipeAdaptor_pointerToMemberData()
{
    struct Item { int value; };
    std::vector<Item> src = { { 5 }, { 6 }, { 7 } };
    auto result = src | views::transform(&Item::value) | ranges::to<std::vector<int>>();
    QCOMPARE(result, (std::vector<int>{ 5, 6, 7 }));
}

// --- Compositions ---

void tst_QMultimediaRanges::composition_filterThenTransformThenTo()
{
    // All in one expression so temporaries stay alive until materialization.
    std::vector<int> v = { 1, 2, 3, 4, 5, 6 };
    auto result = v | views::filter([](int x) {
        return x % 2 == 0;
    }) | views::transform([](int x) {
        return x * 10;
    }) | ranges::to<std::vector<int>>();
    QCOMPARE(result, (std::vector<int>{ 20, 40, 60 }));
}

void tst_QMultimediaRanges::composition_transformThenTo()
{
    std::vector<std::string> v = { "hello", "world", "qt" };
    auto result = v | views::transform([](const std::string &s) {
        return static_cast<int>(s.size());
    }) | ranges::to<std::vector<int>>();
    QCOMPARE(result, (std::vector<int>{ 5, 5, 2 }));
}

void tst_QMultimediaRanges::composition_keysWithTransform()
{
    std::map<int, int> m = { { 1, 100 }, { 2, 200 }, { 3, 300 } };
    // keys | transform to double each key
    auto result = m | views::keys | views::transform([](int k) {
        return k * 2;
    }) | ranges::to<std::vector<int>>();
    QCOMPARE(result, (std::vector<int>{ 2, 4, 6 }));
}

QTEST_APPLESS_MAIN(tst_QMultimediaRanges)

#include "tst_qmultimedia_ranges.moc"
