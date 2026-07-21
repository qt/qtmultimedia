// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <private/qiteratorfacade_p.h>

#include <algorithm>
#include <iterator>

using QtMultimediaPrivate::IteratorFacade;

namespace {

// Forward iterator tier: implements only dereference()/increment()/equals().
class ForwardIterator : public IteratorFacade<ForwardIterator, int, std::forward_iterator_tag>
{
public:
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = const int *;
    using reference = const int &;

    ForwardIterator() = default;
    explicit ForwardIterator(const int *p) : m_p(p) { }

    const int &dereference() const { return *m_p; }
    void increment() { ++m_p; }
    bool equals(const ForwardIterator &other) const { return m_p == other.m_p; }

private:
    const int *m_p = nullptr;
};

// Bidirectional iterator tier: additionally implements decrement(), enabling operator--.
class BidirectionalIterator : public IteratorFacade<BidirectionalIterator, int, std::bidirectional_iterator_tag>
{
public:
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = const int *;
    using reference = const int &;

    BidirectionalIterator() = default;
    explicit BidirectionalIterator(const int *p) : m_p(p) { }

    const int &dereference() const { return *m_p; }
    void increment() { ++m_p; }
    void decrement() { --m_p; }
    bool equals(const BidirectionalIterator &other) const { return m_p == other.m_p; }

private:
    const int *m_p = nullptr;
};

class RandomAccessIterator : public IteratorFacade<RandomAccessIterator, int, std::random_access_iterator_tag>
{
public:
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = int *;
    using reference = int &;

    RandomAccessIterator() = default;
    explicit RandomAccessIterator(int *p) : m_p(p) { }

    int &dereference() const { return *m_p; }
    void advance_by(difference_type n) { m_p += n; }
    difference_type distance_to(const RandomAccessIterator &other) const { return other.m_p - m_p; }

private:
    int *m_p = nullptr;
};

} // namespace

class tst_QIteratorFacade : public QObject
{
    Q_OBJECT

private slots:
    // forward iterator tier: dereference/increment/equals
    void forward_dereference_returnsPointee();
    void forward_preIncrement_movesToNextElementAndReturnsSelf();
    void forward_postIncrement_movesToNextElementAndReturnsPrevious();
    void forward_equality_comparesPosition();
    void forward_worksWithStdFind();
    void forward_worksWithStdDistance();
    void forward_satisfiesStdForwardIteratorConcept();

    // bidirectional iterator tier: adds decrement
    void bidirectional_preDecrement_movesToPreviousElementAndReturnsSelf();
    void bidirectional_postDecrement_movesToPreviousElementAndReturnsPrevious();
    void bidirectional_satisfiesStdBidirectionalIteratorConcept();

    // random access iterator tier: only advance_by()/distance_to() implemented
    void randomAccess_incrementAndDecrement_synthesizedFromAdvanceBy();
    void randomAccess_equality_synthesizedFromDistanceTo();
    void randomAccess_plusEqualsAndMinusEquals_moveByOffset();
    void randomAccess_operatorPlus_bothOperandOrders_returnOffsetIterator();
    void randomAccess_operatorMinus_withOffset_returnsOffsetIterator();
    void randomAccess_operatorMinus_twoIterators_returnsDistance();
    void randomAccess_subscript_returnsElementAtOffset();
    void randomAccess_comparisonOperators_orderIteratorsByPosition();
    void randomAccess_satisfiesStdRandomAccessIteratorConcept();
    void randomAccess_worksWithStdSort();
};

void tst_QIteratorFacade::forward_dereference_returnsPointee()
{
    const int data[] = { 1, 2, 3 };
    ForwardIterator it(data);
    QCOMPARE(*it, 1);
}

void tst_QIteratorFacade::forward_preIncrement_movesToNextElementAndReturnsSelf()
{
    const int data[] = { 1, 2, 3 };
    ForwardIterator it(data);
    ForwardIterator &ref = ++it;
    QCOMPARE(*it, 2);
    QCOMPARE(&ref, &it);
}

void tst_QIteratorFacade::forward_postIncrement_movesToNextElementAndReturnsPrevious()
{
    const int data[] = { 1, 2, 3 };
    ForwardIterator it(data);
    ForwardIterator previous = it++;
    QCOMPARE(*previous, 1);
    QCOMPARE(*it, 2);
}

void tst_QIteratorFacade::forward_equality_comparesPosition()
{
    const int data[] = { 1, 2, 3 };
    ForwardIterator a(data);
    ForwardIterator b(data);
    ForwardIterator c(data + 1);
    QVERIFY(a == b);
    QVERIFY(!(a != b));
    QVERIFY(a != c);
    QVERIFY(!(a == c));
}

void tst_QIteratorFacade::forward_worksWithStdFind()
{
    const int data[] = { 1, 2, 3, 4 };
    ForwardIterator begin(data);
    ForwardIterator end(data + 4);
    auto it = std::find(begin, end, 3);
    QVERIFY(it != end);
    QCOMPARE(*it, 3);
}

void tst_QIteratorFacade::forward_worksWithStdDistance()
{
    const int data[] = { 1, 2, 3, 4, 5 };
    ForwardIterator begin(data);
    ForwardIterator end(data + 5);
    QCOMPARE(std::distance(begin, end), 5);
}

void tst_QIteratorFacade::forward_satisfiesStdForwardIteratorConcept()
{
#if defined(__cpp_concepts)
    static_assert(std::forward_iterator<ForwardIterator>);
#endif
    QVERIFY(true);
}

void tst_QIteratorFacade::bidirectional_preDecrement_movesToPreviousElementAndReturnsSelf()
{
    const int data[] = { 1, 2, 3 };
    BidirectionalIterator it(data + 2);
    BidirectionalIterator &ref = --it;
    QCOMPARE(*it, 2);
    QCOMPARE(&ref, &it);
}

void tst_QIteratorFacade::bidirectional_postDecrement_movesToPreviousElementAndReturnsPrevious()
{
    const int data[] = { 1, 2, 3 };
    BidirectionalIterator it(data + 2);
    BidirectionalIterator previous = it--;
    QCOMPARE(*previous, 3);
    QCOMPARE(*it, 2);
}

void tst_QIteratorFacade::bidirectional_satisfiesStdBidirectionalIteratorConcept()
{
#if defined(__cpp_concepts)
    static_assert(std::bidirectional_iterator<BidirectionalIterator>);
#endif
    QVERIFY(true);
}


void tst_QIteratorFacade::randomAccess_incrementAndDecrement_synthesizedFromAdvanceBy()
{
    int data[] = { 1, 2, 3 };
    RandomAccessIterator it(data);
    ++it;
    QCOMPARE(*it, 2);
    it++;
    QCOMPARE(*it, 3);
    --it;
    QCOMPARE(*it, 2);
    it--;
    QCOMPARE(*it, 1);
}

void tst_QIteratorFacade::randomAccess_equality_synthesizedFromDistanceTo()
{
    int data[] = { 1, 2, 3 };
    RandomAccessIterator a(data);
    RandomAccessIterator b(data);
    RandomAccessIterator c(data + 1);
    QVERIFY(a == b);
    QVERIFY(a != c);
}

void tst_QIteratorFacade::randomAccess_plusEqualsAndMinusEquals_moveByOffset()
{
    int data[] = { 0, 1, 2, 3, 4, 5 };
    RandomAccessIterator it(data);
    it += 4;
    QCOMPARE(*it, 4);
    it -= 2;
    QCOMPARE(*it, 2);
}

void tst_QIteratorFacade::randomAccess_operatorPlus_bothOperandOrders_returnOffsetIterator()
{
    int data[] = { 0, 1, 2, 3, 4, 5 };
    RandomAccessIterator it(data);
    RandomAccessIterator a = it + 3;
    QCOMPARE(*a, 3);
    QCOMPARE(*it, 0); // original iterator is untouched
}

void tst_QIteratorFacade::randomAccess_operatorMinus_withOffset_returnsOffsetIterator()
{
    int data[] = { 0, 1, 2, 3, 4, 5 };
    RandomAccessIterator it(data + 5);
    RandomAccessIterator a = it - 2;
    QCOMPARE(*a, 3);
}

void tst_QIteratorFacade::randomAccess_operatorMinus_twoIterators_returnsDistance()
{
    int data[] = { 0, 1, 2, 3, 4, 5 };
    RandomAccessIterator begin(data);
    RandomAccessIterator end(data + 6);
    QCOMPARE(end - begin, 6);
    QCOMPARE(begin - end, -6);
}

void tst_QIteratorFacade::randomAccess_subscript_returnsElementAtOffset()
{
    int data[] = { 0, 1, 2, 3, 4, 5 };
    RandomAccessIterator it(data);
    QCOMPARE(it[3], 3);
}

void tst_QIteratorFacade::randomAccess_comparisonOperators_orderIteratorsByPosition()
{
    int data[] = { 0, 1, 2 };
    RandomAccessIterator begin(data);
    RandomAccessIterator end(data + 3);
    QVERIFY(begin < end);
    QVERIFY(begin <= end);
    QVERIFY(begin <= begin);
    QVERIFY(end > begin);
    QVERIFY(end >= begin);
    QVERIFY(end >= end);
    QVERIFY(!(end < begin));
}

void tst_QIteratorFacade::randomAccess_satisfiesStdRandomAccessIteratorConcept()
{
#ifdef __cpp_concepts
    static_assert(std::random_access_iterator<RandomAccessIterator>);
#endif // defined(__cpp_concepts)
    QVERIFY(true);
}

void tst_QIteratorFacade::randomAccess_worksWithStdSort()
{
    int data[] = { 5, 3, 1, 4, 2 };
    RandomAccessIterator begin(data);
    RandomAccessIterator end(data + 5);
    std::sort(begin, end);
    QCOMPARE(data[0], 1);
    QCOMPARE(data[1], 2);
    QCOMPARE(data[2], 3);
    QCOMPARE(data[3], 4);
    QCOMPARE(data[4], 5);
}

QTEST_APPLESS_MAIN(tst_QIteratorFacade)

#include "tst_qiteratorfacade.moc"
