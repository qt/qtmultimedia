// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#ifndef QCOMTASKRESOURCE_P_H
#define QCOMTASKRESOURCE_P_H

#include <QtCore/qassert.h>

#include <objbase.h>
#include <utility>

template<typename T>
class QComTaskResource final
{
public:
    QComTaskResource() = default;
    explicit QComTaskResource(T *resource) : m_resource(resource) { }
    ~QComTaskResource() { reset(); }

    QComTaskResource(const QComTaskResource<T> &source) = delete;
    QComTaskResource &operator=(const QComTaskResource<T> &right) = delete;

    explicit operator bool() const { return m_resource != nullptr; }
    T *operator->() const { return m_resource; }

    T **address()
    {
        Q_ASSERT(m_resource == nullptr);
        return &m_resource;
    }
    T *get() const { return m_resource; }
    T *release() { return std::exchange(m_resource, nullptr); }
    void reset(T *resource = nullptr)
    {
        if (m_resource != resource) {
            if (m_resource)
                CoTaskMemFree(m_resource);
            m_resource = resource;
        }
    }

private:
    T *m_resource = nullptr;
};

#endif
