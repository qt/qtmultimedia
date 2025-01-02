// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qdebug.h>

#include "qmediatimerange.h"

QT_BEGIN_NAMESPACE

/*!
    \class QMediaTimeRange::Interval
    \brief The QMediaTimeRange::Interval class represents a time interval with
    integer precision.
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_core

    An interval is specified by an inclusive start() and end() time.  These
    must be set in the constructor, as this is an immutable class.  The
    specific units of time represented by the class have not been defined - it
    is suitable for any times which can be represented by a signed 64 bit
    integer.

    The isNormal() method determines if a time interval is normal (a normal
    time interval has start() <= end()). A normal interval can be received
    from an abnormal interval by calling the normalized() method.

    The contains() method determines if a specified time lies within the time
    interval.

    The translated() method returns a time interval which has been translated
    forwards or backwards through time by a specified offset.

    \sa QMediaTimeRange
*/

/*!
    \fn QMediaTimeRange::Interval::Interval(qint64 start, qint64 end)

    Constructs an interval with the specified \a start and \a end times.
*/

/*!
    \fn QMediaTimeRange::Interval::start() const

    Returns the start time of the interval.

    \sa end()
*/

/*!
    \fn QMediaTimeRange::Interval::end() const

    Returns the end time of the interval.

    \sa start()
*/

/*!
    \fn QMediaTimeRange::Interval::contains(qint64 time) const

    Returns true if the time interval contains the specified \a time.
    That is, start() <= time <= end().
*/

/*!
    \fn QMediaTimeRange::Interval::isNormal() const

    Returns true if this time interval is normal.
    A normal time interval has start() <= end().

    \sa normalized()
*/

/*!
    \fn QMediaTimeRange::Interval::normalized() const

    Returns a normalized version of this interval.

    If the start() time of the interval is greater than the end() time,
    then the returned interval has the start and end times swapped.
*/

/*!
    \fn QMediaTimeRange::Interval::translated(qint64 offset) const

    Returns a copy of this time interval, translated by a value of \a offset.
    An interval can be moved forward through time with a positive offset, or backward
    through time with a negative offset.
*/

/*!
    \fn bool QMediaTimeRange::Interval::operator==(QMediaTimeRange::Interval lhs, QMediaTimeRange::Interval rhs)

    Returns true if \a lhs is exactly equal to \a rhs.
*/

/*!
    \fn bool QMediaTimeRange::Interval::operator!=(QMediaTimeRange::Interval lhs, QMediaTimeRange::Interval rhs)

    Returns true if \a lhs is not exactly equal to \a rhs.
*/

class QMediaTimeRangePrivate : public QSharedData
{
public:
    QMediaTimeRangePrivate() = default;
    QMediaTimeRangePrivate(const QMediaTimeRange::Interval &interval);

    QList<QMediaTimeRange::Interval> intervals;

    void addInterval(const QMediaTimeRange::Interval &interval);
    void removeInterval(const QMediaTimeRange::Interval &interval);
};

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QMediaTimeRangePrivate);

QMediaTimeRangePrivate::QMediaTimeRangePrivate(const QMediaTimeRange::Interval &interval)
{
    if (interval.isNormal())
        intervals << interval;
}

void QMediaTimeRangePrivate::addInterval(const QMediaTimeRange::Interval &interval)
{
    // Handle normalized intervals only
    if (!interval.isNormal())
        return;

    // Find a place to insert the interval
    int i;
    for (i = 0; i < intervals.size(); i++) {
        // Insert before this element
        if(interval.s < intervals[i].s) {
            intervals.insert(i, interval);
            break;
        }
    }

    // Interval needs to be added to the end of the list
    if (i == intervals.size())
        intervals.append(interval);

    // Do we need to correct the element before us?
    if (i > 0 && intervals[i - 1].e >= interval.s - 1)
        i--;

    // Merge trailing ranges
    while (i < intervals.size() - 1
          && intervals[i].e >= intervals[i + 1].s - 1) {
        intervals[i].e = qMax(intervals[i].e, intervals[i + 1].e);
        intervals.removeAt(i + 1);
    }
}

void QMediaTimeRangePrivate::removeInterval(const QMediaTimeRange::Interval &interval)
{
    // Handle normalized intervals only
    if (!interval.isNormal())
        return;

    for (int i = 0; i < intervals.size(); i++) {
        const QMediaTimeRange::Interval r = intervals.at(i);

        if (r.e < interval.s) {
            // Before the removal interval
            continue;
        } else if (interval.e < r.s) {
            // After the removal interval - stop here
            break;
        } else if (r.s < interval.s && interval.e < r.e) {
            // Split case - a single range has a chunk removed
            intervals[i].e = interval.s -1;
            addInterval(QMediaTimeRange::Interval(interval.e + 1, r.e));
            break;
        } else if (r.s < interval.s) {
            // Trimming Tail Case
            intervals[i].e = interval.s - 1;
        } else if (interval.e < r.e) {
            // Trimming Head Case - we can stop after this
            intervals[i].s = interval.e + 1;
            break;
        } else {
            // Complete coverage case
            intervals.removeAt(i);
            --i;
        }
    }
}

/*!
    \class QMediaTimeRange
    \brief The QMediaTimeRange class represents a set of zero or more disjoint
    time intervals.
    \ingroup multimedia
    \inmodule QtMultimedia

    \reentrant

    The earliestTime(), latestTime(), intervals() and isEmpty()
    methods are used to get information about the current time range.

    The addInterval(), removeInterval() and clear() methods are used to modify
    the current time range.

    When adding or removing intervals from the time range, existing intervals
    within the range may be expanded, trimmed, deleted, merged or split to ensure
    that all intervals within the time range remain distinct and disjoint. As a
    consequence, all intervals added or removed from a time range must be
    \l{QMediaTimeRange::Interval::isNormal()}{normal}.

    \sa QMediaTimeRange::Interval
*/

/*!
    Constructs an empty time range.
*/
QMediaTimeRange::QMediaTimeRange()
    : d(new QMediaTimeRangePrivate)
{

}

/*!
    Constructs a time range that contains an initial interval from
    \a start to \a end inclusive.

    If the interval is not \l{QMediaTimeRange::Interval::isNormal()}{normal},
    the resulting time range will be empty.

    \sa addInterval()
*/
QMediaTimeRange::QMediaTimeRange(qint64 start, qint64 end)
    : QMediaTimeRange(Interval(start, end))
{
}

/*!
    Constructs a time range that contains an initial interval, \a interval.

    If \a interval is not \l{QMediaTimeRange::Interval::isNormal()}{normal},
    the resulting time range will be empty.

    \sa addInterval()
*/
QMediaTimeRange::QMediaTimeRange(const QMediaTimeRange::Interval &interval)
    : d(new QMediaTimeRangePrivate(interval))
{
}

/*!
    Constructs a time range by copying another time \a range.
*/
QMediaTimeRange::QMediaTimeRange(const QMediaTimeRange &range) noexcept = default;

/*! \fn QMediaTimeRange::QMediaTimeRange(QMediaTimeRange &&other)

    Constructs a time range by moving from \a other.
*/

/*!
    \fn void QMediaTimeRange::swap(QMediaTimeRange &other) noexcept

    Swaps the current instance with the \a other.
*/

/*!
    \fn QMediaTimeRange::~QMediaTimeRange()

    Destructor.
*/
QMediaTimeRange::~QMediaTimeRange() = default;

/*!
    Takes a copy of the \a other time range and returns itself.
*/
QMediaTimeRange &QMediaTimeRange::operator=(const QMediaTimeRange &other) noexcept = default;

/*!
    \fn QMediaTimeRange &QMediaTimeRange::operator=(QMediaTimeRange &&other)

    Moves \a other into this time range.
*/

/*!
    Sets the time range to a single continuous interval, \a interval.
*/
QMediaTimeRange &QMediaTimeRange::operator=(const QMediaTimeRange::Interval &interval)
{
    d = new QMediaTimeRangePrivate(interval);
    return *this;
}

/*!
    Returns the earliest time within the time range.

    For empty time ranges, this value is equal to zero.

    \sa latestTime()
*/
qint64 QMediaTimeRange::earliestTime() const
{
    if (!d->intervals.isEmpty())
        return d->intervals[0].start();

    return 0;
}

/*!
    Returns the latest time within the time range.

    For empty time ranges, this value is equal to zero.

    \sa earliestTime()
*/
qint64 QMediaTimeRange::latestTime() const
{
    if (!d->intervals.isEmpty())
        return d->intervals[d->intervals.size() - 1].end();

    return 0;
}

/*!
    \overload

    Adds the interval specified by \a start and \a end
    to the time range.

    \sa addInterval()
*/
void QMediaTimeRange::addInterval(qint64 start, qint64 end)
{
    detach();
    d->addInterval(Interval(start, end));
}

/*!
    Adds the specified \a interval to the time range.

    Adding intervals which are not \l{QMediaTimeRange::Interval::isNormal()}{normal}
    is invalid, and will be ignored.

    If the specified interval is adjacent to, or overlaps existing
    intervals within the time range, these intervals will be merged.

    This operation takes linear time.

    \sa removeInterval()
*/
void QMediaTimeRange::addInterval(const QMediaTimeRange::Interval &interval)
{
    detach();
    d->addInterval(interval);
}

/*!
    Adds each of the intervals in \a range to this time range.

    Equivalent to calling addInterval() for each interval in \a range.
*/
void QMediaTimeRange::addTimeRange(const QMediaTimeRange &range)
{
    detach();
    const auto intervals = range.intervals();
    for (const Interval &i : intervals) {
        d->addInterval(i);
    }
}

/*!
    \overload

    Removes the interval specified by \a start and \a end
    from the time range.

    \sa removeInterval()
*/
void QMediaTimeRange::removeInterval(qint64 start, qint64 end)
{
    detach();
    d->removeInterval(Interval(start, end));
}

/*!
    \fn QMediaTimeRange::removeInterval(const QMediaTimeRange::Interval &interval)

    Removes the specified \a interval from the time range.

    Removing intervals which are not \l{QMediaTimeRange::Interval::isNormal()}{normal}
    is invalid, and will be ignored.

    Intervals within the time range will be trimmed, split or deleted
    such that no intervals within the time range include any part of the
    target interval.

    This operation takes linear time.

    \sa addInterval()
*/
void QMediaTimeRange::removeInterval(const QMediaTimeRange::Interval &interval)
{
    detach();
    d->removeInterval(interval);
}

/*!
    Removes each of the intervals in \a range from this time range.

    Equivalent to calling removeInterval() for each interval in \a range.
*/
void QMediaTimeRange::removeTimeRange(const QMediaTimeRange &range)
{
    detach();
    const auto intervals = range.intervals();
    for (const Interval &i : intervals) {
        d->removeInterval(i);
    }
}

/*!
    Adds each interval in \a other to the time range and returns the result.
*/
QMediaTimeRange& QMediaTimeRange::operator+=(const QMediaTimeRange &other)
{
    addTimeRange(other);
    return *this;
}

/*!
    Adds the specified \a interval to the time range and returns the result.
*/
QMediaTimeRange& QMediaTimeRange::operator+=(const QMediaTimeRange::Interval &interval)
{
    addInterval(interval);
    return *this;
}

/*!
    Removes each interval in \a other from the time range and returns the result.
*/
QMediaTimeRange& QMediaTimeRange::operator-=(const QMediaTimeRange &other)
{
    removeTimeRange(other);
    return *this;
}

/*!
    Removes the specified \a interval from the time range and returns the result.
*/
QMediaTimeRange& QMediaTimeRange::operator-=(const QMediaTimeRange::Interval &interval)
{
    removeInterval(interval);
    return *this;
}

/*!
    \fn QMediaTimeRange::clear()

    Removes all intervals from the time range.

    \sa removeInterval()
*/
void QMediaTimeRange::clear()
{
    detach();
    d->intervals.clear();
}

/*!
    \internal
*/
void QMediaTimeRange::detach()
{
    d.detach();
}

/*!
    \fn QMediaTimeRange::intervals() const

    Returns the list of intervals covered by this time range.
*/
QList<QMediaTimeRange::Interval> QMediaTimeRange::intervals() const
{
    return d->intervals;
}

/*!
    \fn QMediaTimeRange::isEmpty() const

    Returns true if there are no intervals within the time range.

    \sa intervals()
*/
bool QMediaTimeRange::isEmpty() const
{
    return d->intervals.isEmpty();
}

/*!
    \fn QMediaTimeRange::isContinuous() const

    Returns true if the time range consists of a continuous interval.
    That is, there is one or fewer disjoint intervals within the time range.
*/
bool QMediaTimeRange::isContinuous() const
{
    return (d->intervals.size() <= 1);
}

/*!
    \fn QMediaTimeRange::contains(qint64 time) const

    Returns true if the specified \a time lies within the time range.
*/
bool QMediaTimeRange::contains(qint64 time) const
{
    for (int i = 0; i < d->intervals.size(); i++) {
        if (d->intervals[i].contains(time))
            return true;

        if (time < d->intervals[i].start())
            break;
    }

    return false;
}

/*!
    \fn bool QMediaTimeRange::operator==(const QMediaTimeRange &lhs, const QMediaTimeRange &rhs)

    Returns true if all intervals in \a lhs are present in \a rhs.
*/

/*!
    \fn bool QMediaTimeRange::operator!=(const QMediaTimeRange &lhs, const QMediaTimeRange &rhs)

    Returns true if one or more intervals in \a lhs are not present in \a rhs.
*/

/*!
    \fn QMediaTimeRange operator+(const QMediaTimeRange &r1, const QMediaTimeRange &r2)
    \relates QMediaTimeRange

    Returns a time range containing the union between \a r1 and \a r2.
 */

/*!
    \fn QMediaTimeRange operator-(const QMediaTimeRange &r1, const QMediaTimeRange &r2)
    \relates QMediaTimeRange

    Returns a time range containing \a r2 subtracted from \a r1.
 */

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QMediaTimeRange::Interval &interval)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QMediaTimeRange::Interval( " << interval.start() << ", " << interval.end() << " )";
    return dbg;
}

QDebug operator<<(QDebug dbg, const QMediaTimeRange &range)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QMediaTimeRange( ";
    const auto intervals = range.intervals();
    for (const auto &interval : intervals)
        dbg << '(' <<  interval.start() << ", " << interval.end() << ") ";
    dbg.space();
    dbg << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE
