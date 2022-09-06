// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TRACE_H
#define TRACE_H

#include <QDebug>

#define ENABLE_TRACE
//#define VERBOSE_TRACE

namespace Trace {

class NullDebug
{
public:
    template<typename T>
    NullDebug &operator<<(const T &)
    {
        return *this;
    }
};

inline NullDebug nullDebug()
{
    return NullDebug();
}

template<typename T>
struct PtrWrapper
{
    PtrWrapper(const T *ptr) : m_ptr(ptr) { }
    const T *const m_ptr;
};

} // namespace Trace

template<typename T>
inline QDebug &operator<<(QDebug &debug, const Trace::PtrWrapper<T> &wrapper)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '[' << static_cast<const void *>(wrapper.m_ptr) << ']';
    return debug;
}

template<typename T>
inline const void *qtVoidPtr(const T *ptr)
{
    return static_cast<const void *>(ptr);
}

#define qtThisPtr() qtVoidPtr(this)

#ifdef ENABLE_TRACE
inline QDebug qtTrace()
{
    return qDebug() << "[qmlvideo]";
}
#    ifdef VERBOSE_TRACE
inline QDebug qtVerboseTrace()
{
    return qtTrace();
}
#    else
inline Trace::NullDebug qtVerboseTrace()
{
    return Trace::nullDebug();
}
#    endif
#else
inline Trace::NullDebug qtTrace()
{
    return Trace::nullDebug();
}
inline Trace::NullDebug qtVerboseTrace()
{
    return Trace::nullDebug();
}
#endif

#endif // TRACE_H
