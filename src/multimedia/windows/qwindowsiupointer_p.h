// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSIUPOINTER_H
#define QWINDOWSIUPOINTER_H

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

template <class T>
class QWindowsIUPointer
{
public:
    explicit QWindowsIUPointer(T *ptr) : m_ptr(ptr) {}
    QWindowsIUPointer() : m_ptr(nullptr) {}
    QWindowsIUPointer(const QWindowsIUPointer<T> &uiPtr) : m_ptr(uiPtr.m_ptr) { if (m_ptr) m_ptr->AddRef(); }
    QWindowsIUPointer(QWindowsIUPointer<T> &&uiPtr) : m_ptr(uiPtr.m_ptr) { uiPtr.m_ptr = nullptr; }
    ~QWindowsIUPointer() { if (m_ptr) m_ptr->Release(); }

    QWindowsIUPointer& operator=(const QWindowsIUPointer<T> &rhs) {
        if (this != &rhs) {
            if (m_ptr)
                m_ptr->Release();
            m_ptr = rhs.m_ptr;
            m_ptr->AddRef();
        }
        return *this;
    }

    QWindowsIUPointer& operator=(QWindowsIUPointer<T> &&rhs) noexcept {
        if (m_ptr)
            m_ptr->Release();
        m_ptr = rhs.m_ptr;
        rhs.m_ptr = nullptr;
        return *this;
    }

    explicit operator bool() const { return m_ptr != nullptr; }
    T *operator->() const { return m_ptr; }

    T **address() { Q_ASSERT(m_ptr == nullptr); return &m_ptr; }
    void reset(T *ptr = nullptr) { if (m_ptr) m_ptr->Release(); m_ptr = ptr; }
    T *release() { T *ptr = m_ptr; m_ptr = nullptr; return ptr; }
    T *get() const { return m_ptr; }

private:
    T *m_ptr;
};

#endif
