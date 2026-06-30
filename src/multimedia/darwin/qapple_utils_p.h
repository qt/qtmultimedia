// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QAPPLE_UTILS_P_H
#define QAPPLE_UTILS_P_H

#include <QtCore/qdebug.h>
#include <QtCore/qendian.h>
#include <QtCore/qglobal.h>
#include <QtCore/private/qcore_mac_p.h>

#include <QtMultimedia/private/qiteratorfacade_p.h>

#ifdef Q_OS_MACOS
#  include <CoreAudioTypes/CoreAudioTypes.h>
#endif

#include <CoreFoundation/CFArray.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

struct QOSStatus
{
    OSStatus status;
    explicit QOSStatus(OSStatus s) : status(s) {}

    friend QDebug operator<<(QDebug dbg, const QOSStatus &qos)
    {
        QDebugStateSaver saver(dbg);
        dbg.noquote();

        if (qos.status == noErr) {
            dbg << "noErr";
            return dbg;
        }

        std::array<char, 4> buf;
        qToBigEndian(qos.status, buf.data());

        bool isPrintable = std::all_of(buf.begin(), buf.end(), [](unsigned char c) {
            return std::isprint(c);
        });

        if (isPrintable)
            return dbg << QLatin1String(buf.data(), buf.size());
        else
            return dbg << qos.status;
    }
};

template <typename T>
class CFArrayIterator
    : public IteratorFacade<CFArrayIterator<T>, T, std::random_access_iterator_tag, T>
{
public:
    static_assert(std::is_pointer_v<T>, "CFArrayIterator must be instantiated with a pointer type");

    using difference_type = std::ptrdiff_t;

    CFArrayIterator() = default;
    CFArrayIterator(QCFType<CFArrayRef> array, CFIndex index)
        : m_array(std::move(array)), m_index(index)
    {
    }

    T dereference() const { return static_cast<T>(CFArrayGetValueAtIndex(m_array, m_index)); }

    void advance_by(difference_type n) { m_index += n; }

    difference_type distance_to(const CFArrayIterator &other) const
    {
        return other.m_index - m_index;
    }

private:
    QCFType<CFArrayRef> m_array{};
    CFIndex m_index{};
};

template <typename T = CFTypeRef>
class CFArrayRange
{
public:
    static_assert(std::is_pointer_v<T>, "CFArrayRange must be instantiated with a pointer type");

    using iterator = CFArrayIterator<T>;

    CFArrayRange() = default;
    explicit CFArrayRange(QCFType<CFArrayRef> array) : m_array(std::move(array)) {}

    iterator begin() const { return iterator(m_array, 0); }
    iterator end() const { return iterator(m_array, m_array ? CFArrayGetCount(m_array) : 0); }

    bool empty() const { return m_array == nullptr || CFArrayGetCount(m_array) == 0; }

    std::size_t size() const
    {
        return m_array ? static_cast<std::size_t>(CFArrayGetCount(m_array)) : 0;
    }

private:
    QCFType<CFArrayRef> m_array;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QAPPLE_UTILS_P_H
