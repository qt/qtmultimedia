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
#include <QtMultimedia/private/qaudio_qspan_support_p.h>

#include <algorithm>
#include <atomic>
#include <limits>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

// Single-producer, single-consumer wait-free queue
template <typename T>
class QAudioRingBuffer
{
public:
    using ValueType = T;
    using Region = QSpan<T>;
    using ConstRegion = QSpan<const T>;

    explicit QAudioRingBuffer(int bufferSize) : m_bufferSize(bufferSize)
    {
        m_buffer.reset(new T[bufferSize]); // no value-initialization for trivial types
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
            std::copy_n(region.data(), toWrite, writeRegion.data());
            region = drop(region, toWrite);
            releaseWriteRegion(toWrite);
            elementsWritten += toWrite;
        }
        return elementsWritten;
    }

    template <typename Functor>
    int consume(int elements, Functor &&consumer)
    {
        int elementsConsumed = 0;

        while (elements > elementsConsumed) {
            ConstRegion readRegion = acquireReadRegion(elements - elementsConsumed);
            if (readRegion.isEmpty())
                break;

            consumer(readRegion);
            elementsConsumed += readRegion.size();
            releaseReadRegion(readRegion.size());
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
            output = writeSize > 0 ? Region(m_buffer.get() + m_writePos, writeSize) : Region();
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

    ConstRegion acquireReadRegion(int size)
    {
        const int used = m_bufferUsed.load(std::memory_order_acquire);

        if (used > 0) {
            const int readSize = qMin(size, qMin(m_bufferSize - m_readPos, used));
            return readSize > 0 ? Region(m_buffer.get() + m_readPos, readSize) : Region();
        }

        return Region();
    }

    void releaseReadRegion(int elementsWritten)
    {
        m_readPos = (m_readPos + elementsWritten) % m_bufferSize;
        m_bufferUsed.fetch_sub(elementsWritten, std::memory_order_release);
    }


private:
    const int m_bufferSize;
    int m_readPos{};
    int m_writePos{};
    std::unique_ptr<T[]> m_buffer;
    std::atomic_int m_bufferUsed{};
};

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QAUDIORINGBUFFER_P_H
