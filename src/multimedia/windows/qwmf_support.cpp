// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qwmf_support_p.h"

#include <mferror.h>

QT_BEGIN_NAMESPACE

namespace QWMF {

HRESULT QByteArrayMFMediaBuffer::CreateInstance(QByteArray data, IMFMediaBuffer **ppBuffer,
                                                bool isReadOnly)
{
    if (!ppBuffer)
        return E_POINTER;

    DWORD size = data.size();

    QByteArrayMFMediaBuffer *pBuffer =
            new (std::nothrow) QByteArrayMFMediaBuffer(std::move(data), isReadOnly);
    if (!pBuffer)
        return E_OUTOFMEMORY;

    pBuffer->SetCurrentLength(size);

    HRESULT hr =
            pBuffer->QueryInterface(__uuidof(IMFMediaBuffer), reinterpret_cast<void **>(ppBuffer));

    pBuffer->Release();
    return hr;
}

HRESULT QByteArrayMFMediaBuffer::CreateInstance(qsizetype capacity, IMFMediaBuffer **ppBuffer)
{
    if (!ppBuffer)
        return E_POINTER;

    QByteArray buffer{ capacity, Qt::Initialization::Uninitialized };
    QByteArrayMFMediaBuffer *pBuffer =
            new (std::nothrow) QByteArrayMFMediaBuffer(std::move(buffer), /*isReadOnly=*/false);
    if (!pBuffer)
        return E_OUTOFMEMORY;

    HRESULT hr =
            pBuffer->QueryInterface(__uuidof(IMFMediaBuffer), reinterpret_cast<void **>(ppBuffer));

    pBuffer->Release();
    return hr;
}

HRESULT QByteArrayMFMediaBuffer::Lock(BYTE **ppbBuffer, DWORD *pcbMaxLength,
                                      DWORD *pcbCurrentLength)
{
    if (!ppbBuffer)
        return E_POINTER;

    if (m_isLocked.test_and_set(std::memory_order_acquire))
        return MF_E_INVALIDREQUEST; // Buffer is already locked.

    if (m_isReadOnly)
        // we assume that the IMFTransform is not working in-place, so we can avoid `detach` here
        *ppbBuffer = const_cast<BYTE *>(reinterpret_cast<const BYTE *>(m_byteArray.constData()));
    else
        *ppbBuffer = reinterpret_cast<BYTE *>(m_byteArray.data());

    if (pcbMaxLength)
        *pcbMaxLength = GetMaxLengthInternal();

    if (pcbCurrentLength)
        *pcbCurrentLength = m_currentLength;

    return S_OK;
}

HRESULT QByteArrayMFMediaBuffer::Unlock()
{
    m_isLocked.clear(std::memory_order_release);
    return S_OK;
}

HRESULT QByteArrayMFMediaBuffer::GetCurrentLength(DWORD *pcbCurrentLength)
{
    if (!pcbCurrentLength)
        return E_POINTER;

    *pcbCurrentLength = m_currentLength;
    return S_OK;
}

HRESULT QByteArrayMFMediaBuffer::SetCurrentLength(DWORD cbCurrentLength)
{
    if (cbCurrentLength > GetMaxLengthInternal())
        return E_INVALIDARG;

    m_currentLength = cbCurrentLength;
    return S_OK;
}

HRESULT QByteArrayMFMediaBuffer::GetMaxLength(DWORD *pcbMaxLength)
{
    if (!pcbMaxLength)
        return E_POINTER;

    *pcbMaxLength = GetMaxLengthInternal();
    return S_OK;
}

QByteArrayMFMediaBuffer::QByteArrayMFMediaBuffer(QByteArray &&data, bool isReadOnly)
    : m_byteArray(std::move(data)), m_isReadOnly(isReadOnly)
{
}

DWORD QByteArrayMFMediaBuffer::GetMaxLengthInternal() const
{
    return static_cast<DWORD>(m_byteArray.size());
}

QByteArray QByteArrayMFMediaBuffer::takeByteArray()
{
    return std::move(m_byteArray);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace QWMF

QT_END_NAMESPACE
