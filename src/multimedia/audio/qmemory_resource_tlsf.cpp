// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmemory_resource_tlsf_p.h"

#include <QtCore/qassert.h>

#include <memory_resource>

#include <tlsf.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

QTlsfMemoryResource::QTlsfMemoryResource(std::size_t preallocatedBytes, memory_resource *upstream):
    m_allocationSize {
        preallocatedBytes + 8192, // over-allocate by 8k for tlsf header
    },
    m_upstream{
        upstream,
    }
{
    m_buffer = reinterpret_cast<std::byte *>(operator new(m_allocationSize, poolAlignment));

    m_tlsf = QtPrivate::tlsf_create_with_pool(m_buffer, m_allocationSize);
    Q_ASSERT(m_tlsf);
}

QTlsfMemoryResource::~QTlsfMemoryResource()
{
    QtPrivate::tlsf_destroy(m_tlsf);
    ::operator delete(m_buffer, poolAlignment);
}

void *QTlsfMemoryResource::do_allocate(size_t bytes, size_t alignment)
{
    void *ret = QtPrivate::tlsf_memalign(m_tlsf, alignment, bytes);
    if (Q_UNLIKELY(ret == nullptr))
        ret = m_upstream->allocate(bytes, alignment);

    return ret;
}

void QTlsfMemoryResource::do_deallocate(void *p, size_t bytes, size_t alignment)
{
    if (Q_LIKELY(p >= m_buffer && p < m_buffer + m_allocationSize))
        QtPrivate::tlsf_free(m_tlsf, p);
    else
        m_upstream->deallocate(p, bytes, alignment);
}

bool QTlsfMemoryResource::do_is_equal(const memory_resource &other) const noexcept
{
    return this == &other;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
