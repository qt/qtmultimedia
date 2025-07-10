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

HRESULT QPmrMediaBuffer::CreateInstance(QSpan<const std::byte> data,
                                        std::pmr::memory_resource *resource,
                                        IMFMediaBuffer **ppBuffer)
{
    if (!ppBuffer || !resource)
        return E_POINTER;

    *ppBuffer = nullptr;

    auto *buffer = resource->allocate(sizeof(QPmrMediaBuffer), alignof(QPmrMediaBuffer));
    if (!buffer)
        return E_OUTOFMEMORY;

    QPmrMediaBuffer *pBuffer = new (buffer) QPmrMediaBuffer(data, resource);

    HRESULT hr =
            pBuffer->QueryInterface(__uuidof(IMFMediaBuffer), reinterpret_cast<void **>(ppBuffer));

    pBuffer->Release();

    return hr;
}

HRESULT QPmrMediaBuffer::CreateInstance(qsizetype capacity, std::pmr::memory_resource *resource,
                                        IMFMediaBuffer **ppBuffer)
{
    if (!ppBuffer || !resource)
        return E_POINTER;

    *ppBuffer = nullptr;
    void *buffer = resource->allocate(sizeof(QPmrMediaBuffer), alignof(QPmrMediaBuffer));
    QPmrMediaBuffer *pBuffer = new (buffer) QPmrMediaBuffer(capacity, resource);
    if (!pBuffer)
        return E_OUTOFMEMORY;

    HRESULT hr =
            pBuffer->QueryInterface(__uuidof(IMFMediaBuffer), reinterpret_cast<void **>(ppBuffer));
    pBuffer->Release();
    return hr;
}

ULONG QPmrMediaBuffer::AddRef()
{
    return m_referenceCount.fetch_add(1, std::memory_order_relaxed) + 1;
}

ULONG QPmrMediaBuffer::Release()
{
    const LONG referenceCount = m_referenceCount.fetch_sub(1, std::memory_order_release) - 1;
    if (referenceCount == 0) {
        // This acquire fence synchronizes with the release operation in other threads.
        // It ensures that all memory writes made to this object by other threads
        // are visible to this thread before we proceed to delete it.
        std::atomic_thread_fence(std::memory_order_acquire);

        std::pmr::memory_resource *resource = m_memoryResource;

        this->~QPmrMediaBuffer();
        resource->deallocate(this, sizeof(QPmrMediaBuffer), alignof(QPmrMediaBuffer));
    }

    return referenceCount;
}

STDMETHODIMP QPmrMediaBuffer::Lock(BYTE **ppbBuffer, DWORD *pcbMaxLength, DWORD *pcbCurrentLength)
{
    if (!ppbBuffer)
        return E_POINTER;

    if (m_isLocked.test_and_set(std::memory_order_acquire))
        return MF_E_INVALIDREQUEST;

    *ppbBuffer = reinterpret_cast<BYTE *>(m_buffer);

    if (pcbMaxLength)
        *pcbMaxLength = m_maxLength;

    if (pcbCurrentLength)
        *pcbCurrentLength = m_currentLength;

    return S_OK;
}

STDMETHODIMP QPmrMediaBuffer::Unlock()
{
    m_isLocked.clear(std::memory_order_release);
    return S_OK;
}

STDMETHODIMP QPmrMediaBuffer::GetCurrentLength(DWORD *pcbCurrentLength)
{
    if (!pcbCurrentLength)
        return E_POINTER;

    *pcbCurrentLength = m_currentLength;
    return S_OK;
}

STDMETHODIMP QPmrMediaBuffer::SetCurrentLength(DWORD cbCurrentLength)
{
    if (cbCurrentLength > m_maxLength)
        return E_INVALIDARG;

    m_currentLength = cbCurrentLength;
    return S_OK;
}

STDMETHODIMP QPmrMediaBuffer::GetMaxLength(DWORD *pcbMaxLength)
{
    if (!pcbMaxLength)
        return E_POINTER;

    *pcbMaxLength = m_maxLength;
    return S_OK;
}

static constexpr auto mfBufferAlignment = 16;

QPmrMediaBuffer::QPmrMediaBuffer(QSpan<const std::byte> data, std::pmr::memory_resource *resource)
    : QPmrMediaBuffer(data.size(), resource)
{
    m_currentLength = data.size(), std::copy(data.begin(), data.end(), m_buffer);
}

QPmrMediaBuffer::QPmrMediaBuffer(qsizetype capacity, std::pmr::memory_resource *resource)
    : m_memoryResource(resource),
      m_maxLength(DWORD(capacity)),
      m_buffer(static_cast<std::byte *>(resource->allocate(capacity, mfBufferAlignment)))
{
}

QPmrMediaBuffer::~QPmrMediaBuffer()
{
    m_memoryResource->deallocate(m_buffer, m_maxLength, mfBufferAlignment);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace QWMF

QT_END_NAMESPACE
