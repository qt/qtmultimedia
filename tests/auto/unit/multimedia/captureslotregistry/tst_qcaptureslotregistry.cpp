// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/private/qcaptureslotregistry_p.h>
#include <QtMultimedia/private/qmaterializingvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#include <QtCore/qmutex.h>
#include <QtCore/qsemaphore.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <vector>

// NOLINTBEGIN(readability-convert-member-functions-to-static)

using namespace std::chrono_literals;

using QtMultimediaPrivate::CaptureSlotRegistry;
using QtMultimediaPrivate::MaterializedVideoBuffer;
using QtMultimediaPrivate::MaterializingVideoBuffer;

namespace {

// A producer whose slots are plain ints, recording every slot handed back to it.
class MockProducer : public CaptureSlotRegistry<MockProducer, int>
{
public:
    MockProducer(size_t slotCount, size_t minFreeSlots)
        : CaptureSlotRegistry(slotCount, minFreeSlots)
    {
    }

    static std::shared_ptr<MockProducer> create(size_t slotCount = 8, size_t minFreeSlots = 2)
    {
        return std::make_shared<MockProducer>(slotCount, minFreeSlots);
    }

    void recycleSlot(int slot)
    {
        if (beforeRecycle)
            beforeRecycle(slot);

        std::lock_guard lock(m_mutex);
        m_recycled.push_back(slot);
    }

    std::vector<int> recycled() const
    {
        std::lock_guard lock(m_mutex);
        return m_recycled;
    }

    int recycleCount(int slot) const
    {
        std::lock_guard lock(m_mutex);
        return int(std::count(m_recycled.begin(), m_recycled.end(), slot));
    }

    // Called from inside recycleSlot(), before the slot is recorded.
    std::function<void(int)> beforeRecycle;

private:
    mutable QMutex m_mutex;
    std::vector<int> m_recycled;
};

struct MoveOnlySlot
{
    int value = 0;

    MoveOnlySlot() = default;
    explicit MoveOnlySlot(int v) : value(v) {}
    MoveOnlySlot(MoveOnlySlot &&) noexcept = default;
    MoveOnlySlot &operator=(MoveOnlySlot &&) noexcept = default;
    MoveOnlySlot(const MoveOnlySlot &) = delete;
    MoveOnlySlot &operator=(const MoveOnlySlot &) = delete;
};

class MoveOnlyProducer : public CaptureSlotRegistry<MoveOnlyProducer, MoveOnlySlot>
{
public:
    MoveOnlyProducer(size_t slotCount, size_t minFreeSlots)
        : CaptureSlotRegistry(slotCount, minFreeSlots)
    {
    }

    void recycleSlot(MoveOnlySlot) {}
};

// Producer-owned memory, modelled on mmap: mapReadOnly() hands out an independent mapping that
// stays readable until its unmap(), even after the producer has dropped the underlying pages.
class MockSlotMemory
{
public:
    MockSlotMemory(QByteArray content, std::vector<int> planeSizes, int bytesPerLine)
        : m_pages(std::make_shared<QByteArray>(std::move(content))),
          m_planeSizes(std::move(planeSizes)),
          m_bytesPerLine(bytesPerLine)
    {
    }

    std::optional<QAbstractVideoBuffer::MapData> mapReadOnly()
    {
        std::lock_guard lock(m_mutex);
        ++m_mapCount;

        auto mapping = std::make_shared<QByteArray>(*m_pages);
        QAbstractVideoBuffer::MapData mapData;
        mapData.planeCount = int(m_planeSizes.size());
        qsizetype offset = 0;
        for (int i = 0; i < mapData.planeCount; ++i) {
            mapData.data[i] = reinterpret_cast<uchar *>(mapping->data()) + offset;
            mapData.dataSize[i] = m_planeSizes[size_t(i)];
            mapData.bytesPerLine[i] = m_bytesPerLine;
            offset += m_planeSizes[size_t(i)];
        }

        m_mappings.push_back({ mapData.data[0], std::move(mapping) });
        return mapData;
    }

    void unmap(QAbstractVideoBuffer::MapData &mapData)
    {
        std::lock_guard lock(m_mutex);
        const auto it = std::find_if(m_mappings.begin(), m_mappings.end(), [&](const Mapping &m) {
            return m.base == mapData.data[0];
        });
        QVERIFY2(it != m_mappings.end(), "unmap() of a mapping that was never handed out");
        m_mappings.erase(it);
    }

    // The producer frees the slot: pages handed out earlier stay readable, new maps see poison.
    void invalidate()
    {
        std::lock_guard lock(m_mutex);
        m_pages = std::make_shared<QByteArray>(m_pages->size(), 'X');
    }

    int mapCount() const
    {
        std::lock_guard lock(m_mutex);
        return m_mapCount;
    }

    int activeMappingCount() const
    {
        std::lock_guard lock(m_mutex);
        return int(m_mappings.size());
    }

private:
    struct Mapping
    {
        const uchar *base;
        std::shared_ptr<QByteArray> pages;
    };

    mutable QMutex m_mutex;
    std::shared_ptr<QByteArray> m_pages;
    const std::vector<int> m_planeSizes;
    const int m_bytesPerLine;
    int m_mapCount = 0;
    std::vector<Mapping> m_mappings;
};

struct MockSource
{
    std::shared_ptr<MockSlotMemory> memory;

    bool holdsMemory() const { return memory != nullptr; }

    std::optional<QAbstractVideoBuffer::MapData> mapReadOnly() const
    {
        return memory->mapReadOnly();
    }
    void unmap(QAbstractVideoBuffer::MapData &mapData) const { memory->unmap(mapData); }
};

using MockLease = MockProducer::Lease;
using MockVideoBuffer = MaterializingVideoBuffer<MockSource, MockProducer>;

// A base buffer with its own constructor argument, standing in for QDmaBufVideoBuffer.
class TaggedBase : public QHwVideoBuffer
{
public:
    explicit TaggedBase(int tag) : m_tag(tag) { }

    bool isDmaBuf() const override { return true; }
    int tag() const { return m_tag; }

private:
    const int m_tag;
};

using TaggedVideoBuffer = MaterializingVideoBuffer<MockSource, MockProducer, TaggedBase>;

// Producer-owned memory with aliasing: mapReadOnly() hands out pointers into the live slot buffer,
// not a copy. unmap() poisons the region (accessible but contains invalid data). overwrite()
// simulates the driver refilling after recycle; if active > 0, it updates the live buffer.
class AliasingSlotMemory
{
public:
    AliasingSlotMemory(QByteArray content, std::vector<int> planeSizes, int bytesPerLine)
        : m_content(std::move(content)),
          m_planeSizes(std::move(planeSizes)),
          m_bytesPerLine(bytesPerLine),
          m_region(m_content)
    {
    }

    std::optional<QAbstractVideoBuffer::MapData> mapReadOnly()
    {
        std::lock_guard lock(m_mutex);
        if (m_activeMappings == 0)
            m_region = m_content;
        ++m_activeMappings;

        QAbstractVideoBuffer::MapData mapData;
        mapData.planeCount = int(m_planeSizes.size());
        qsizetype offset = 0;
        for (int i = 0; i < mapData.planeCount; ++i) {
            mapData.data[i] = reinterpret_cast<uchar *>(m_region.data()) + offset;
            mapData.dataSize[i] = m_planeSizes[size_t(i)];
            mapData.bytesPerLine[i] = m_bytesPerLine;
            offset += m_planeSizes[size_t(i)];
        }

        return mapData;
    }

    void unmap(QAbstractVideoBuffer::MapData &)
    {
        std::lock_guard lock(m_mutex);
        if (m_activeMappings > 0) {
            --m_activeMappings;
            if (m_activeMappings == 0)
                // Poison the region so any stale reads are detectable
                for (auto &byte : m_region)
                    byte = 'X';
        }
    }

    // Simulate the driver refilling the slot after recycle: updates m_content,
    // and if a mapping is active, also updates the live buffer.
    void overwrite(const QByteArray &newContent)
    {
        std::lock_guard lock(m_mutex);
        m_content = newContent;
        if (m_activeMappings > 0)
            m_region = newContent;
    }

    int activeMappingCount() const
    {
        std::lock_guard lock(m_mutex);
        return m_activeMappings;
    }

private:
    mutable QMutex m_mutex;
    QByteArray m_content;
    const std::vector<int> m_planeSizes;
    const int m_bytesPerLine;
    QByteArray m_region; // always same size as m_content, but content changes
    int m_activeMappings = 0;
};

struct AliasingSource
{
    std::shared_ptr<AliasingSlotMemory> memory;

    bool holdsMemory() const { return memory != nullptr; }

    std::optional<QAbstractVideoBuffer::MapData> mapReadOnly() const
    {
        return memory->mapReadOnly();
    }
    void unmap(QAbstractVideoBuffer::MapData &mapData) const { memory->unmap(mapData); }
};

using AliasingVideoBuffer = MaterializingVideoBuffer<AliasingSource, MockProducer>;

std::shared_ptr<AliasingSlotMemory> makeAliasingMemory(QByteArray content, int bytesPerLine = 16)
{
    const int size = int(content.size());
    return std::make_shared<AliasingSlotMemory>(std::move(content), std::vector<int>{ size },
                                                bytesPerLine);
}

// A source whose mapping always fails, modelling mmap/Lock failures.
struct NullSource
{
    std::optional<QAbstractVideoBuffer::MapData> mapReadOnly() const { return std::nullopt; }
    void unmap(QAbstractVideoBuffer::MapData &) const { }
};

using NullVideoBuffer = MaterializingVideoBuffer<NullSource, MockProducer>;

// Each buffer owns its state exclusively, so none of them may be copied or moved.
template <typename T>
constexpr bool isCopyOrMovable = std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>
        || std::is_copy_assignable_v<T> || std::is_move_assignable_v<T>;
static_assert(!isCopyOrMovable<QtMultimediaPrivate::MaterializableSource<MockSource>>);
static_assert(!isCopyOrMovable<MockVideoBuffer>);
static_assert(!isCopyOrMovable<MaterializedVideoBuffer<MockSource>>);

QByteArray testPattern(qsizetype size, char first = 0)
{
    QByteArray data(size, Qt::Uninitialized);
    for (qsizetype i = 0; i < size; ++i)
        data[i] = char(uchar((int(first) + int(i % 251)) & 0xff));
    return data;
}

std::shared_ptr<MockSlotMemory> makeMemory(QByteArray content, int bytesPerLine = 16)
{
    const int size = int(content.size());
    return std::make_shared<MockSlotMemory>(std::move(content), std::vector<int>{ size },
                                            bytesPerLine);
}

std::unique_ptr<MockVideoBuffer> makeBuffer(MockProducer &producer, int slot,
                                            const std::shared_ptr<MockSlotMemory> &memory)
{
    return MockVideoBuffer::tryCreate(producer, slot, MockSource{ memory });
}

} // namespace

class tst_QCaptureSlotRegistry : public QObject
{
    Q_OBJECT

private slots:
    void tryReserveSlot_refusesAtThreshold_data();
    void tryReserveSlot_refusesAtThreshold();
    void tryReserveSlot_keepsSlotOnRefusal();
    void slotHandle_recyclesSlotOnce_whenDestroyed();
    void reservation_recyclesSlot_whenNeverConsumed();
    void stop_dropsSlotsReleasedAfterwards();
    void slotHandle_keepsProducerAlive_whenItOutlivesTheOwner();
    void recycleSlot_runsWithoutTheRegistryLock();
    void stop_waitsForInFlightRecycle();
    void map_returnsSourceMapping_whenReadOnly();
    void map_materializes_whenWriteRequested();
    void materialize_keepsHandedOutMapping_whenSourceIsMapped();
    void materialize_survivesSourceInvalidation();
    void materialize_releasesSource_onceNothingIsMapped();
    void materialize_preservesPlaneLayout_forMultiPlaneSources();
    void videoFrame_recyclesSlot_whenDroppedOnAnotherThread();
    void stress_recyclesEverySlotExactlyOnce();
    void tryCreate_returnsNullptr_whenPoolUnderPressure();
    void tryCreate_propagatesFormat();
    void tryCreate_forwardsBaseArgs_andHoldsSlot();
    void materializedBuffer_copiesEagerly_withoutHoldingSlot();
    void map_returnsEmpty_whenSourceMappingFails();

    void map_afterMaterialize_servesCopy_notSourceMapping();
    void unmap_poisonsSourceMapping_andFrameBitsAreNull();
    void stress_recyclesEverySlotExactlyOnce_aliasing();
};

void tst_QCaptureSlotRegistry::tryReserveSlot_refusesAtThreshold_data()
{
    QTest::addColumn<int>("slotCount");
    QTest::addColumn<int>("minFreeSlots");
    QTest::addColumn<int>("expectedReservations");

    QTest::newRow("libcamera 8/2") << 8 << 2 << 6;
    QTest::newRow("4/1") << 4 << 1 << 3;
    QTest::newRow("2/2") << 2 << 2 << 0;
    QTest::newRow("no reserve") << 3 << 0 << 3;
}

void tst_QCaptureSlotRegistry::tryReserveSlot_refusesAtThreshold()
{
    QFETCH(const int, slotCount);
    QFETCH(const int, minFreeSlots);
    QFETCH(const int, expectedReservations);

    const auto producer = MockProducer::create(size_t(slotCount), size_t(minFreeSlots));

    std::vector<MockProducer::Lease> reservations;
    for (int i = 0; i < slotCount + 1; ++i) {
        std::optional reservation = producer->tryReserveSlot(i);
        if (!reservation)
            break;
        reservations.push_back(std::move(*reservation));
    }

    QCOMPARE(int(reservations.size()), expectedReservations);
    QCOMPARE(int(producer->outstandingSlotCount()), expectedReservations);

    // One more must be refused, whatever the threshold was.
    QVERIFY(!producer->tryReserveSlot(slotCount).has_value());
}

void tst_QCaptureSlotRegistry::tryReserveSlot_keepsSlotOnRefusal()
{
    const auto producer = std::make_shared<MoveOnlyProducer>(size_t(1), size_t(0));

    std::optional lease = producer->tryReserveSlot(MoveOnlySlot{ 1 });
    QVERIFY(lease);

    MoveOnlySlot slot{ 42 };
    QCOMPARE(producer->tryReserveSlot(std::move(slot)).has_value(), false);
    QCOMPARE(slot.value, 42);

    lease.reset();
    std::optional retry = producer->tryReserveSlot(std::move(slot));
    QVERIFY(retry);
}

void tst_QCaptureSlotRegistry::slotHandle_recyclesSlotOnce_whenDestroyed()
{
    const auto producer = MockProducer::create();

    {
        auto handle = std::make_unique<MockLease>(*producer->tryReserveSlot(42));
        QCOMPARE(producer->outstandingSlotCount(), 1u);
        QVERIFY(producer->recycled().empty());
    }

    QCOMPARE(producer->outstandingSlotCount(), 0u);
    QCOMPARE(producer->recycled(), std::vector<int>{ 42 });
}

void tst_QCaptureSlotRegistry::reservation_recyclesSlot_whenNeverConsumed()
{
    const auto producer = MockProducer::create();

    {
        std::optional reservation = producer->tryReserveSlot(7);
        QVERIFY(reservation);
        QCOMPARE(producer->outstandingSlotCount(), 1u);
    }

    QCOMPARE(producer->outstandingSlotCount(), 0u);
    QCOMPARE(producer->recycled(), std::vector<int>{ 7 });
}

void tst_QCaptureSlotRegistry::stop_dropsSlotsReleasedAfterwards()
{
    const auto producer = MockProducer::create();
    auto lease = std::make_unique<MockLease>(*producer->tryReserveSlot(1));

    producer->stop();
    QVERIFY(!producer->tryReserveSlot(2).has_value());
    QCOMPARE(producer->outstandingSlotCount(), 1u);

    lease.reset();
    QVERIFY(producer->recycled().empty());
    QCOMPARE(producer->outstandingSlotCount(), 0u);
}

void tst_QCaptureSlotRegistry::slotHandle_keepsProducerAlive_whenItOutlivesTheOwner()
{
    std::weak_ptr<MockProducer> weakProducer;
    std::unique_ptr<MockLease> handle;

    {
        const auto producer = MockProducer::create();
        weakProducer = producer;
        handle = std::make_unique<MockLease>(*producer->tryReserveSlot(5));
    }

    QVERIFY(!weakProducer.expired());
    const auto producer = weakProducer.lock();
    handle.reset();
    QCOMPARE(producer->recycled(), std::vector<int>{ 5 });
}

void tst_QCaptureSlotRegistry::recycleSlot_runsWithoutTheRegistryLock()
{
    const auto producer = MockProducer::create();

    size_t outstandingSeenFromCallback = 12345;
    producer->beforeRecycle = [&](int) {
        // Deadlocks if the registry still holds its lock across the callback.
        outstandingSeenFromCallback = producer->outstandingSlotCount();
    };

    {
        MockLease handle(*producer->tryReserveSlot(9));
    }

    QCOMPARE(outstandingSeenFromCallback, 0u);
    QCOMPARE(producer->recycled(), std::vector<int>{ 9 });
}

void tst_QCaptureSlotRegistry::stop_waitsForInFlightRecycle()
{
    const auto producer = MockProducer::create();
    auto released = std::make_unique<MockLease>(*producer->tryReserveSlot(2));

    QSemaphore recycleEntered;
    QSemaphore letRecycleFinish;
    producer->beforeRecycle = [&](int) {
        recycleEntered.release();
        letRecycleFinish.acquire();
    };

    std::thread releaser([&] {
        released.reset();
    });
    QVERIFY(recycleEntered.tryAcquire(1, 5s));

    std::atomic_bool stopReturned{ false };
    std::thread stopper([&] {
        producer->stop();
        stopReturned = true;
    });

    // The recycle is still in flight, so teardown must not have completed.
    std::this_thread::sleep_for(50ms);
    QVERIFY(!stopReturned.load());

    letRecycleFinish.release();
    releaser.join();
    stopper.join();

    QVERIFY(stopReturned.load());
    QCOMPARE(producer->recycled(), std::vector<int>{ 2 });

    producer->beforeRecycle = nullptr;
}

void tst_QCaptureSlotRegistry::map_returnsSourceMapping_whenReadOnly()
{
    const auto producer = MockProducer::create();
    const QByteArray content = testPattern(96);
    const auto memory = makeMemory(content);

    auto buffer = makeBuffer(*producer, 1, memory);
    QVERIFY(buffer);

    const QAbstractVideoBuffer::MapData mapData = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(mapData.planeCount, 1);
    QCOMPARE(mapData.dataSize[0], int(content.size()));
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mapData.data[0]), content.size()), content);
    QCOMPARE(memory->mapCount(), 1);
    QCOMPARE(memory->activeMappingCount(), 1);
    QVERIFY(!buffer->state().isMaterialized());

    // Mapping again while mapped must not map the source a second time.
    QCOMPARE(buffer->map(QVideoFrame::ReadOnly).data[0], mapData.data[0]);
    QCOMPARE(memory->mapCount(), 1);

    buffer->unmap();
    QCOMPARE(memory->activeMappingCount(), 0);
}

void tst_QCaptureSlotRegistry::map_materializes_whenWriteRequested()
{
    const auto producer = MockProducer::create();
    const QByteArray content = testPattern(64);
    const auto memory = makeMemory(content);

    auto buffer = makeBuffer(*producer, 1, memory);
    QVERIFY(buffer);

    QAbstractVideoBuffer::MapData mapData = buffer->map(QVideoFrame::ReadWrite);
    QVERIFY(buffer->state().isMaterialized());
    QCOMPARE(mapData.planeCount, 1);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mapData.data[0]), content.size()), content);

    // Writing hits the copy, never the producer's memory.
    mapData.data[0][0] = 'Z';
    buffer->unmap();
    QCOMPARE(memory->activeMappingCount(), 0);

    const QAbstractVideoBuffer::MapData reread = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(reread.data[0][0], uchar('Z'));
}

void tst_QCaptureSlotRegistry::materialize_keepsHandedOutMapping_whenSourceIsMapped()
{
    const auto producer = MockProducer::create();
    const QByteArray content = testPattern(80, 3);
    const auto memory = makeMemory(content);

    auto buffer = makeBuffer(*producer, 1, memory);
    QVERIFY(buffer);

    const QAbstractVideoBuffer::MapData mapData = buffer->map(QVideoFrame::ReadOnly);
    const uchar *handedOut = mapData.data[0];

    buffer->state().materialize();
    QVERIFY(buffer->state().isMaterialized());

    // Materializing copied out of the existing mapping instead of mapping again, and left it in
    // place so the pointer the consumer already holds still reads correctly.
    QCOMPARE(memory->mapCount(), 1);
    QCOMPARE(memory->activeMappingCount(), 1);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(handedOut), content.size()), content);

    memory->invalidate();
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(handedOut), content.size()), content);

    buffer->unmap();
    QCOMPARE(memory->activeMappingCount(), 0);

    const QAbstractVideoBuffer::MapData afterDetach = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(afterDetach.data[0]), content.size()),
             content);
    QCOMPARE(memory->mapCount(), 1);
}

void tst_QCaptureSlotRegistry::materialize_survivesSourceInvalidation()
{
    const auto producer = MockProducer::create();
    const QByteArray content = testPattern(48, 7);
    const auto memory = makeMemory(content);

    auto buffer = makeBuffer(*producer, 1, memory);
    QVERIFY(buffer);

    // Never mapped by a consumer: materializing has to map the source itself and release it again.
    buffer->state().materialize();
    QCOMPARE(memory->mapCount(), 1);
    QCOMPARE(memory->activeMappingCount(), 0);

    memory->invalidate();

    const QAbstractVideoBuffer::MapData mapData = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mapData.data[0]), content.size()), content);
    QCOMPARE(memory->mapCount(), 1);
}

void tst_QCaptureSlotRegistry::materialize_releasesSource_onceNothingIsMapped()
{
    const auto producer = MockProducer::create();
    std::weak_ptr<MockSlotMemory> weakMemory;

    auto buffer = [&] {
        const auto memory = makeMemory(testPattern(32));
        weakMemory = memory;
        return makeBuffer(*producer, 1, memory);
    }();
    QVERIFY(buffer);

    // Mapped: the source is still needed to undo the mapping.
    const QAbstractVideoBuffer::MapData mapData = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(mapData.planeCount, 1);
    buffer->state().materialize();
    QVERIFY(!weakMemory.expired());

    // Unmapped and materialized: nothing reads from the producer any more.
    buffer->unmap();
    QVERIFY(weakMemory.expired());

    QCOMPARE(buffer->map(QVideoFrame::ReadOnly).planeCount, 1);
}

void tst_QCaptureSlotRegistry::materialize_preservesPlaneLayout_forMultiPlaneSources()
{
    const auto producer = MockProducer::create();
    const QByteArray content = testPattern(64 + 16 + 16);
    const auto memory =
            std::make_shared<MockSlotMemory>(content, std::vector<int>{ 64, 16, 16 }, 8);

    auto buffer = makeBuffer(*producer, 1, memory);
    QVERIFY(buffer);

    const QAbstractVideoBuffer::MapData source = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(source.planeCount, 3);
    buffer->unmap();

    buffer->state().materialize();
    memory->invalidate();

    const QAbstractVideoBuffer::MapData mapData = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(mapData.planeCount, 3);
    qsizetype offset = 0;
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(mapData.dataSize[i], source.dataSize[i]);
        QCOMPARE(mapData.bytesPerLine[i], source.bytesPerLine[i]);
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(mapData.data[i]), mapData.dataSize[i]),
                 content.mid(offset, mapData.dataSize[i]));
        offset += mapData.dataSize[i];
    }
}

void tst_QCaptureSlotRegistry::videoFrame_recyclesSlot_whenDroppedOnAnotherThread()
{
    const auto producer = MockProducer::create();
    const QByteArray content = testPattern(128);
    const auto memory = makeMemory(content);

    auto buffer = makeBuffer(*producer, 11, memory);
    QVERIFY(buffer);

    QVideoFrame frame = QVideoFramePrivate::createFrame(
            std::move(buffer), QVideoFrameFormat(QSize(16, 8), QVideoFrameFormat::Format_Y8));
    QVERIFY(frame.isValid());
    QCOMPARE(producer->outstandingSlotCount(), 1u);

    std::thread consumer([frame]() mutable {
        QVERIFY(frame.map(QVideoFrame::ReadOnly));
        frame.unmap();
        frame = {};
    });
    consumer.join();

    frame = {};
    QCOMPARE(producer->outstandingSlotCount(), 0u);
    QCOMPARE(producer->recycled(), std::vector<int>{ 11 });
}

void tst_QCaptureSlotRegistry::stress_recyclesEverySlotExactlyOnce()
{
    constexpr int slotCount = 8;
    constexpr int frameCount = 2000;

    const auto producer = MockProducer::create(slotCount, 2);
    const auto memory = makeMemory(testPattern(256));

    QMutex queueMutex;
    std::vector<QVideoFrame> queue;
    std::atomic_bool producing{ true };
    std::atomic_int wrapped{ 0 };
    std::atomic_int copied{ 0 };
    std::atomic_int stops{ 0 };
    std::array<std::atomic_int, slotCount> wrapsPerSlot{};

    std::thread consumer([&] {
        std::mt19937 rng(4242);
        for (;;) {
            std::vector<QVideoFrame> taken;
            {
                std::lock_guard lock(queueMutex);
                if (!queue.empty()) {
                    // Drop a random prefix, so frames die out of order and off the producer thread.
                    const size_t count =
                            std::uniform_int_distribution<size_t>(1, queue.size())(rng);
                    taken.assign(std::make_move_iterator(queue.begin()),
                                 std::make_move_iterator(queue.begin() + qsizetype(count)));
                    queue.erase(queue.begin(), queue.begin() + qsizetype(count));
                }
            }
            for (QVideoFrame &frame : taken) {
                if (frame.map(QVideoFrame::ReadOnly))
                    frame.unmap();
            }
            taken.clear();

            if (!producing.load()) {
                std::lock_guard lock(queueMutex);
                if (queue.empty())
                    return;
            }
            std::this_thread::yield();
        }
    });

    for (int i = 0; i < frameCount; ++i) {
        if (auto buffer = makeBuffer(*producer, i % slotCount, memory)) {
            ++wrapped;
            ++wrapsPerSlot[size_t(i % slotCount)];
            std::lock_guard lock(queueMutex);
            queue.push_back(QVideoFramePrivate::createFrame(
                    std::move(buffer),
                    QVideoFrameFormat(QSize(16, 16), QVideoFrameFormat::Format_Y8)));
        } else {
            ++copied; // back-pressure: the real backends copy eagerly here
        }
    }

    // Tear the pool down while the consumer is still dropping frames on its own thread.
    std::thread stopper([&] {
        producer->stop();
        ++stops;
    });

    producing = false;
    consumer.join();
    stopper.join();

    QVERIFY(wrapped.load() > 0);
    QVERIFY(copied.load() > 0); // the threshold must have bitten at least once
    QCOMPARE(stops.load(), 1);
    QCOMPARE(producer->outstandingSlotCount(), 0u);

    // A slot is handed back at most once per time it was handed out: more would be a double
    // release, fewer means the teardown reclaimed it instead.
    QVERIFY(int(producer->recycled().size()) <= wrapped.load());
    for (int slot = 0; slot < slotCount; ++slot)
        QVERIFY(producer->recycleCount(slot) <= wrapsPerSlot[size_t(slot)].load());
}

void tst_QCaptureSlotRegistry::tryCreate_returnsNullptr_whenPoolUnderPressure()
{
    const auto producer = MockProducer::create(8, 2);
    const auto memory = makeMemory(testPattern(64));

    // Fill every reservable slot: with 8 buffers and 2 kept free, 6 wrap.
    std::vector<std::unique_ptr<MockVideoBuffer>> buffers;
    for (int i = 0; i < 6; ++i) {
        auto buffer = makeBuffer(*producer, i, memory);
        QVERIFY(buffer);
        buffers.push_back(std::move(buffer));
    }
    QCOMPARE(producer->outstandingSlotCount(), 6u);

    // The pool is under pressure: the caller must copy eagerly instead. Refusing consumes
    // nothing and hands nothing back.
    QVERIFY(!makeBuffer(*producer, 6, memory));
    QCOMPARE(producer->outstandingSlotCount(), 6u);
    QVERIFY(producer->recycled().empty());

    buffers.pop_back();
    QVERIFY(makeBuffer(*producer, 6, memory));
}

void tst_QCaptureSlotRegistry::tryCreate_propagatesFormat()
{
    const auto producer = MockProducer::create();
    const auto memory = makeMemory(testPattern(64));

    const QVideoFrameFormat format(QSize(16, 8), QVideoFrameFormat::Format_Y8);
    auto buffer = MockVideoBuffer::tryCreate(*producer, 1, MockSource{ memory }, format);
    QVERIFY(buffer);
    // QHwVideoBuffer::format() is invalid by design; the format is carried by the frame.
    QVERIFY(!buffer->format().isValid());

    const QVideoFrame frame = QVideoFramePrivate::createFrame(std::move(buffer), format);
    QCOMPARE(frame.surfaceFormat().frameSize(), QSize(16, 8));
    QCOMPARE(frame.surfaceFormat().pixelFormat(), QVideoFrameFormat::Format_Y8);
}

void tst_QCaptureSlotRegistry::tryCreate_forwardsBaseArgs_andHoldsSlot()
{
    const auto producer = MockProducer::create();
    const auto memory = makeMemory(testPattern(64));

    auto buffer = TaggedVideoBuffer::tryCreate(*producer, 1, MockSource{ memory }, {}, 42);
    QVERIFY(buffer);
    QCOMPARE(buffer->tag(), 42);
    QVERIFY(buffer->isDmaBuf());
    QCOMPARE(producer->outstandingSlotCount(), std::size_t(1));
    QVERIFY(producer->recycled().empty());

    buffer.reset();
    QCOMPARE(producer->outstandingSlotCount(), std::size_t(0));
    QCOMPARE(producer->recycled(), std::vector<int>{ 1 });
}

void tst_QCaptureSlotRegistry::materializedBuffer_copiesEagerly_withoutHoldingSlot()
{
    const auto producer = MockProducer::create();
    const QByteArray content = testPattern(64);
    const auto memory = makeMemory(content);

    const QVideoFrameFormat format(QSize(16, 8), QVideoFrameFormat::Format_Y8);
    auto buffer =
            std::make_unique<MaterializedVideoBuffer<MockSource>>(MockSource{ memory }, format);
    QVERIFY(buffer->format() == format);

    // Holds no capture slot: the producer's pool is untouched ...
    QCOMPARE(producer->outstandingSlotCount(), 0u);

    // ... and the copy happened up front, so nothing still reads from the producer's memory.
    QCOMPARE(memory.use_count(), 1);

    const QAbstractVideoBuffer::MapData mapData = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(mapData.planeCount, 1);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mapData.data[0]), content.size()), content);
    buffer->unmap();

    memory->invalidate();
    const QAbstractVideoBuffer::MapData reread = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(reread.data[0]), content.size()), content);
    buffer->unmap();
}

void tst_QCaptureSlotRegistry::map_returnsEmpty_whenSourceMappingFails()
{
    const auto producer = MockProducer::create();

    // Reserving the slot does not touch the source, so this succeeds even though every mapping
    // will fail.
    auto buffer = NullVideoBuffer::tryCreate(*producer, 1, NullSource{});
    QVERIFY(buffer);
    QCOMPARE(producer->outstandingSlotCount(), 1u);

    QCOMPARE(buffer->map(QVideoFrame::ReadOnly).planeCount, 0);
    QVERIFY(!buffer->state().isMaterialized());

    // Materializing a failed mapping is safe: empty, but consistent. Unmapping without a
    // mapping is a no-op.
    buffer->state().materialize();
    QVERIFY(buffer->state().isMaterialized());
    QCOMPARE(buffer->map(QVideoFrame::ReadOnly).planeCount, 0);
    buffer->unmap();

    buffer.reset();
    QCOMPARE(producer->outstandingSlotCount(), 0u);
}

void tst_QCaptureSlotRegistry::map_afterMaterialize_servesCopy_notSourceMapping()
{
    const auto producer = MockProducer::create();
    const QByteArray content = testPattern(72);
    const auto memory = makeAliasingMemory(content);

    auto buffer = AliasingVideoBuffer::tryCreate(*producer, 5, AliasingSource{ memory });
    QVERIFY(buffer);

    buffer->map(QVideoFrame::ReadOnly);
    buffer->state().materialize();

    // A nested map after materializing must return the copy, not the source mapping.
    const auto mapDataAfter = buffer->map(QVideoFrame::ReadOnly);
    QVERIFY(buffer->state().isMaterialized());
    QCOMPARE(mapDataAfter.planeCount, 1);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mapDataAfter.data[0]), content.size()),
             content);

    buffer->unmap();
    QCOMPARE(memory->activeMappingCount(), 0);
}

void tst_QCaptureSlotRegistry::unmap_poisonsSourceMapping_andFrameBitsAreNull()
{
    const auto producer = MockProducer::create();
    const QByteArray content = testPattern(56);
    const auto memory = makeAliasingMemory(content);

    auto buffer = AliasingVideoBuffer::tryCreate(*producer, 6, AliasingSource{ memory });
    QVERIFY(buffer);

    QVideoFrame frame = QVideoFramePrivate::createFrame(
            std::move(buffer), QVideoFrameFormat(QSize(16, 8), QVideoFrameFormat::Format_Y8));
    QVERIFY(frame.isValid());

    frame.map(QVideoFrame::ReadOnly);
    const uchar *bits = frame.bits(0);
    QVERIFY(bits != nullptr);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(bits), content.size()), content);

    frame.unmap();

    // After unmap: bits() returns nullptr, and the source memory is poisoned.
    QCOMPARE(frame.bits(0), nullptr);
    QCOMPARE(memory->activeMappingCount(), 0);
}

void tst_QCaptureSlotRegistry::stress_recyclesEverySlotExactlyOnce_aliasing()
{
    constexpr int slotCount = 8;
    constexpr int frameCount = 2000;

    const auto producer = MockProducer::create(slotCount, 2);
    const auto memory = makeAliasingMemory(testPattern(256));

    QMutex queueMutex;
    std::vector<QVideoFrame> queue;
    std::atomic_bool producing{ true };
    std::atomic_int wrapped{ 0 };
    std::atomic_int copied{ 0 };
    std::atomic_int stops{ 0 };
    std::array<std::atomic_int, slotCount> wrapsPerSlot{};
    QMutex freeListMutex;
    std::vector<int> freeSlots;
    for (int i = 0; i < slotCount; ++i)
        freeSlots.push_back(i);

    std::thread consumer([&] {
        std::mt19937 rng(4242);
        for (;;) {
            std::vector<QVideoFrame> taken;
            {
                std::lock_guard lock(queueMutex);
                if (!queue.empty()) {
                    const size_t count =
                            std::uniform_int_distribution<size_t>(1, queue.size())(rng);
                    taken.assign(std::make_move_iterator(queue.begin()),
                                 std::make_move_iterator(queue.begin() + qsizetype(count)));
                    queue.erase(queue.begin(), queue.begin() + qsizetype(count));
                }
            }
            for (QVideoFrame &frame : taken) {
                if (frame.map(QVideoFrame::ReadOnly))
                    frame.unmap();
            }
            taken.clear();

            if (!producing.load()) {
                std::lock_guard lock(queueMutex);
                if (queue.empty())
                    return;
            }
            std::this_thread::yield();
        }
    });

    producer->beforeRecycle = [&](int slot) {
        // Record when recycled (with active mappings check).
        QCOMPARE(memory->activeMappingCount(), 0);
        // Push the slot back to the free list for reuse.
        std::lock_guard lock(freeListMutex);
        freeSlots.push_back(slot);
    };

    for (int i = 0; i < frameCount; ++i) {
        int slot = -1;
        {
            std::lock_guard lock(freeListMutex);
            if (!freeSlots.empty()) {
                slot = freeSlots.back();
                freeSlots.pop_back();
            }
        }
        if (slot < 0)
            continue; // No free slot, skip this frame.

        if (auto buffer =
                    AliasingVideoBuffer::tryCreate(*producer, slot, AliasingSource{ memory })) {
            ++wrapped;
            ++wrapsPerSlot[size_t(slot)];
            std::lock_guard lock(queueMutex);
            queue.push_back(QVideoFramePrivate::createFrame(
                    std::move(buffer),
                    QVideoFrameFormat(QSize(16, 16), QVideoFrameFormat::Format_Y8)));
        } else {
            // Back-pressure: could not get a slot.
            ++copied;
            std::lock_guard lock(freeListMutex);
            freeSlots.push_back(slot);
        }
    }

    std::thread stopper([&] {
        producer->stop();
        ++stops;
    });

    producing = false;
    consumer.join();
    stopper.join();

    QVERIFY(wrapped.load() > 0);
    QCOMPARE(stops.load(), 1);
    QCOMPARE(producer->outstandingSlotCount(), 0u);

    // A frame released after the stop began is dropped rather than recycled, so the stop racing the
    // consumer allows fewer recycles than wraps -- but never more.
    for (int slot = 0; slot < slotCount; ++slot)
        QVERIFY(producer->recycleCount(slot) <= wrapsPerSlot[size_t(slot)].load());
}

QTEST_MAIN(tst_QCaptureSlotRegistry)

#include "tst_qcaptureslotregistry.moc"
