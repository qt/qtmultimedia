// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QAUDIORINGBUFFER_P_H
#define QAUDIORINGBUFFER_P_H

#include <QtCore/qspan.h>
#include <QtCore/qtclasshelpermacros.h>
#include <QtMultimedia/private/qaudio_qspan_support_p.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <limits>
#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

// Single-producer, single-consumer wait-free queue
template <typename T>
class QAudioRingBuffer
{
    static constexpr bool isTriviallyDestructible = std::is_trivially_destructible_v<T>;

public:
    using ValueType = T;
    using Region = QSpan<T>;
    using ConstRegion = QSpan<const T>;

    explicit QAudioRingBuffer(int bufferSize) : m_bufferSize(bufferSize)
    {
        if (bufferSize)
            m_buffer = reinterpret_cast<T *>(
                    ::operator new(sizeof(T) * bufferSize, std::align_val_t(alignof(T))));
    }

    Q_DISABLE_COPY_MOVE(QAudioRingBuffer)

    ~QAudioRingBuffer()
    {
        if constexpr (!isTriviallyDestructible) {
            consumeAll([](auto /* elements*/) {
            });
        }

        ::operator delete(reinterpret_cast<void *>(m_buffer), std::align_val_t(alignof(T)));
    }

    int write(ConstRegion region)
    {
        using namespace QtMultimediaPrivate; // drop

        int elementsWritten = 0;
        while (!region.isEmpty()) {
            Region writeRegion = acquireWriteRegion(region.size());
            if (writeRegion.isEmpty())
                break;

            int toWrite = qMin(writeRegion.size(), region.size());
            std::uninitialized_copy_n(region.data(), toWrite, writeRegion.data());
            region = drop(region, toWrite);
            releaseWriteRegion(toWrite);
            elementsWritten += toWrite;
        }
        return elementsWritten;
    }

    bool write(T element)
    {
        Region writeRegion = acquireWriteRegion(1);
        if (writeRegion.isEmpty())
            return false;
        T *writeElement = writeRegion.data();
        new (writeElement) T{ std::move(element) };
        releaseWriteRegion(1);
        return true;
    }

    template <typename Functor>
    int consume(int elements, Functor &&consumer)
    {
        int elementsConsumed = 0;

        while (elements > elementsConsumed) {
            Region readRegion = acquireReadRegion(elements - elementsConsumed);
            if (readRegion.isEmpty())
                break;

            consumer(readRegion);
            if constexpr (!isTriviallyDestructible)
                std::destroy(readRegion.begin(), readRegion.end());

            elementsConsumed += readRegion.size();
            releaseReadRegionImpl(readRegion.size());
        }

        return elementsConsumed;
    }

    template <typename Functor>
    int consumeAll(Functor &&consumer)
    {
        return consume(std::numeric_limits<int>::max(), std::forward<Functor>(consumer));
    }

    // CAVEAT: beware of the thread safety
    int used() const { return m_bufferUsed.load(std::memory_order_relaxed); }
    int free() const { return m_bufferSize - m_bufferUsed.load(std::memory_order_relaxed); }

    int size() const { return m_bufferSize; };

    void reset()
#ifdef __cpp_concepts
            requires isTriviallyDestructible
#endif
    {
        m_readPos = 0;
        m_writePos = 0;
        m_bufferUsed.store(0, std::memory_order_relaxed);
    }

    Region acquireWriteRegion(int size)
    {
        const int free = m_bufferSize - m_bufferUsed.load(std::memory_order_acquire);

        Region output;
        if (free > 0) {
            const int writeSize = qMin(size, qMin(m_bufferSize - m_writePos, free));
            output = writeSize > 0 ? Region(m_buffer + m_writePos, writeSize) : Region();
        } else {
            output = Region();
        }
        return output;
    }

    void releaseWriteRegion(int elementsRead)
    {
        m_writePos = (m_writePos + elementsRead) % m_bufferSize;
        m_bufferUsed.fetch_add(elementsRead, std::memory_order_release);
    }

    Region acquireReadRegion(int size)
    {
        const int used = m_bufferUsed.load(std::memory_order_acquire);

        if (used > 0) {
            const int readSize = qMin(size, qMin(m_bufferSize - m_readPos, used));
            return readSize > 0 ? Region(m_buffer + m_readPos, readSize) : Region();
        }

        return Region();
    }

    void releaseReadRegion(int elementsWritten)
#ifdef __cpp_concepts
            requires isTriviallyDestructible
#endif
    {
        releaseReadRegionImpl(elementsWritten);
    }

private:
    // WARNING: we need to ensure that all elements are destroyed
    void releaseReadRegionImpl(int elementsWritten)
    {
        m_readPos = (m_readPos + elementsWritten) % m_bufferSize;
        m_bufferUsed.fetch_sub(elementsWritten, std::memory_order_release);
    }

    const int m_bufferSize;
    int m_readPos{};
    int m_writePos{};
    T *m_buffer{};
    std::atomic_int m_bufferUsed{};
};

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QAUDIORINGBUFFER_P_H
