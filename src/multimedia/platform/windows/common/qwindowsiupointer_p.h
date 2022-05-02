/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
    explicit QWindowsIUPointer(T *ptr = nullptr) : m_ptr(ptr) {}
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
