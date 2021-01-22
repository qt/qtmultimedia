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

#include "mfactivate.h"

#include <mfapi.h>

MFAbstractActivate::MFAbstractActivate()
    : m_attributes(0)
    , m_cRef(1)
{
    MFCreateAttributes(&m_attributes, 0);
}

MFAbstractActivate::~MFAbstractActivate()
{
    if (m_attributes)
        m_attributes->Release();
}


HRESULT MFAbstractActivate::QueryInterface(REFIID riid, LPVOID *ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (riid == IID_IMFActivate) {
        *ppvObject = static_cast<IMFActivate*>(this);
    } else if (riid == IID_IMFAttributes) {
        *ppvObject = static_cast<IMFAttributes*>(this);
    } else if (riid == IID_IUnknown) {
        *ppvObject = static_cast<IUnknown*>(static_cast<IMFActivate*>(this));
    } else {
        *ppvObject =  NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG MFAbstractActivate::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

ULONG MFAbstractActivate::Release(void)
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
        delete this;
    return cRef;
}
