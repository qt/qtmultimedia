// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "mfactivate_p.h"

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
