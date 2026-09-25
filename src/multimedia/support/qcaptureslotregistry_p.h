// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCAPTURESLOTREGISTRY_P_H
#define QCAPTURESLOTREGISTRY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qtmultimediaglobal.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qvarlengtharray.h>

#include <array>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>

#ifdef __cpp_concepts
#  include <concepts>
#endif

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(qLcCaptureSlots, Q_MULTIMEDIA_EXPORT)
QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(qLcCaptureSlotsTrace, Q_MULTIMEDIA_EXPORT)

namespace QtMultimediaPrivate {

#ifdef __cpp_concepts
// clang-format off

template <typename Slot>
concept CaptureSlot = std::movable<Slot>;

template <typename Producer, typename Slot>
concept CaptureSlotProducer = requires(Producer &producer, Slot slot) {
    { producer.recycleSlot(std::move(slot)) } -> std::same_as<void>;
};

// clang-format on
#endif

#ifdef __cpp_concepts
template <typename Producer, CaptureSlot Slot, size_t Capacity = 32>
#else
template <typename Producer, typename Slot, size_t Capacity = 32>
#endif
class CaptureSlotRegistry : public std::enable_shared_from_this<Producer>
{
public:
    using SlotType = Slot;

    CaptureSlotRegistry(std::size_t slotCount, std::size_t minFreeSlots)
        : m_slotCount(slotCount), m_minFreeSlots(minFreeSlots)
    {
        Q_ASSERT(slotCount <= Capacity);
#ifdef __cpp_concepts
        static_assert(CaptureSlotProducer<Producer, Slot>);
#endif

        std::generate_n(std::back_inserter(m_freeSlotIndices), slotCount,
                        [n = slotCount]() mutable {
            return --n;
        });
    }

    class Lease
    {
    public:
        Lease(const Lease &) = delete;
        Lease(Lease &&other) noexcept = default;

        Lease &operator=(const Lease &) = delete;
        Lease &operator=(Lease &&other) noexcept
        {
            if (this != &other) {
                release();
                m_producer = std::exchange(other.m_producer, {});
                m_index = other.m_index;
            }
            return *this;
        }

        ~Lease() { release(); }

    private:
        friend class CaptureSlotRegistry;

        Lease(std::shared_ptr<Producer> producer, std::size_t index)
            : m_producer(std::move(producer)), m_index(index)
        {
        }

        void release()
        {
            if (m_producer)
                std::exchange(m_producer, {})->releaseEntry(m_index);
        }

        std::shared_ptr<Producer> m_producer;
        std::size_t m_index = 0;
    };

    // Takes the slot only when a lease is returned. On refusal the argument is left intact, so the
    // caller can still recycle it or copy the buffer it stands for.
    template <typename SlotArg>
    std::optional<Lease> tryReserveSlot(SlotArg &&slot)
    {
        std::lock_guard lock(m_mutex);
        if (m_stopped || m_outstandingCount + m_minFreeSlots >= m_slotCount) {
            qCDebug(qLcCaptureSlotsTrace) << "refusing slot: stopped:" << m_stopped
                                          << "outstanding:" << m_outstandingCount << "of"
                                          << m_slotCount << "keeping" << m_minFreeSlots << "free";
            return std::nullopt;
        }

        ++m_outstandingCount;
        const std::size_t index = allocateEntry(Slot(std::forward<SlotArg>(slot)));
        qCDebug(qLcCaptureSlotsTrace)
                << "reserved entry" << index << "outstanding:" << m_outstandingCount;
        return Lease{
            this->shared_from_this(),
            index,
        };
    }

    std::size_t outstandingSlotCount() const
    {
        std::lock_guard lock(m_mutex);
        return m_outstandingCount;
    }

    // Refuses new slots, and makes slots that are released from now on drop instead of recycling.
    // Waits for recycles in flight, so the producer may tear down once this returns.
    void stop()
    {
        qCDebug(qLcCaptureSlots) << "stopping with" << outstandingSlotCount()
                                 << "slots outstanding";

        std::unique_lock lock(m_mutex);
        m_stopped = true;
        while (m_releasesInFlight != 0)
            m_releaseFinished.wait(&m_mutex);
    }

protected:
    ~CaptureSlotRegistry() = default;

private:
    friend class Lease;

    struct Entry
    {
        constexpr bool isFree() const { return !slot.has_value(); }
        std::optional<Slot> slot;
    };

    Producer &producer() { return static_cast<Producer &>(*this); }

    std::size_t allocateEntry(Slot slot)
    {
        Q_ASSERT(!m_freeSlotIndices.empty());
        const std::size_t index = m_freeSlotIndices.back();
        m_freeSlotIndices.pop_back();

        Q_ASSERT(m_entries[index].isFree());
        qCDebug(qLcCaptureSlotsTrace) << "using entry" << index;
        m_entries[index] = Entry{ std::move(slot) };
        return index;
    }

    void releaseEntry(std::size_t index)
    {
        std::optional<Slot> slot;
        {
            std::lock_guard lock(m_mutex);
            Entry &entry = m_entries[index];
            Q_ASSERT(entry.slot && "a lease releases its entry once");

            slot = std::move(entry.slot);
            entry = Entry{};
            m_freeSlotIndices.push_back(index);
            --m_outstandingCount;

            if (m_stopped) {
                qCDebug(qLcCaptureSlotsTrace) << "entry" << index << "dropped, recycling stopped";
                return;
            }
            ++m_releasesInFlight;
        }

        qCDebug(qLcCaptureSlotsTrace) << "recycling entry" << index;
        producer().recycleSlot(std::move(*slot));

        std::lock_guard lock(m_mutex);
        if (--m_releasesInFlight == 0)
            m_releaseFinished.wakeAll();
    }

    const std::size_t m_slotCount;
    const std::size_t m_minFreeSlots;

    mutable QMutex m_mutex;
    QWaitCondition m_releaseFinished;
    std::array<Entry, Capacity> m_entries;
    QVarLengthArray<std::size_t, Capacity> m_freeSlotIndices; // std::inplace_vector
    std::size_t m_outstandingCount = 0;
    std::size_t m_releasesInFlight = 0;
    bool m_stopped = false;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QCAPTURESLOTREGISTRY_P_H
