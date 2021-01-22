/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef DIRECTSHOWOBJECT_H
#define DIRECTSHOWOBJECT_H

#include "directshowglobal.h"

QT_BEGIN_NAMESPACE

#define COM_REF_MIXIN \
    volatile ULONG m_ref = 1;                              \
public:                                                    \
    STDMETHODIMP_(ULONG) AddRef() override                 \
    {                                                      \
        return InterlockedIncrement(&m_ref);               \
    }                                                      \
    STDMETHODIMP_(ULONG) Release() override                \
    {                                                      \
        const ULONG ref = InterlockedDecrement(&m_ref);    \
        if (ref == 0)                                      \
            delete this;                                   \
        return ref;                                        \
    }

QT_END_NAMESPACE

#endif // DIRECTSHOWOBJECT_H
