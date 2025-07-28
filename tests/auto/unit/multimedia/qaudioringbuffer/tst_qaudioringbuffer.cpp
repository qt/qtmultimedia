// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtMultimedia/private/qaudioringbuffer_p.h>

#include <chrono>
#include <random>
#include <thread>

// NOLINTBEGIN(readability-convert-member-functions-to-static)

class tst_QAudioRingBuffer : public QObject
{
    Q_OBJECT

private slots:
    void capacityAPIs();
    void reset();
    void moveOnlyType();
    void moveOnlyType_produceOne();
    void destroyRemainingElements_inDestructor();
    void destroy_inConsume();
    void destroy_inConsumeStressTest();

    void stressTest();
    void stressTest_data();
};

struct IotaValidator
{
    bool consumeAndValidate(int i)
    {
        bool success = i == state;
        state += 1;

        if (!success)
            qDebug() << i << state;
        return success;
    }

    int state{};
};

void tst_QAudioRingBuffer::capacityAPIs()
{
    QtPrivate::QAudioRingBuffer<int> ringbuffer{ 64 };

    QCOMPARE(ringbuffer.size(), 64);
    QCOMPARE(ringbuffer.free(), 64);
    QCOMPARE(ringbuffer.used(), 0);

    ringbuffer.write(std::initializer_list<int>{ 1 });

    QCOMPARE(ringbuffer.size(), 64);
    QCOMPARE(ringbuffer.free(), 63);
    QCOMPARE(ringbuffer.used(), 1);

    ringbuffer.consumeAll([](auto) {
    });

    QCOMPARE(ringbuffer.size(), 64);
    QCOMPARE(ringbuffer.free(), 64);
    QCOMPARE(ringbuffer.used(), 0);
}

void tst_QAudioRingBuffer::reset()
{
    QtPrivate::QAudioRingBuffer<int> ringbuffer{ 64 };
    ringbuffer.write(std::initializer_list<int>{ 1 });

    ringbuffer.reset();

    QCOMPARE(ringbuffer.free(), 64);
    QCOMPARE(ringbuffer.used(), 0);

    int elementsConsumed = ringbuffer.consumeAll([](auto) {
    });
    QCOMPARE(elementsConsumed, 0);
}

void tst_QAudioRingBuffer::moveOnlyType()
{
    QtPrivate::QAudioRingBuffer<std::unique_ptr<int>> ringbuffer{ 64 };
    auto element = std::make_unique<int>(1);
    QVERIFY(ringbuffer.write(std::move(element)));

    int elementsConsumed = ringbuffer.consumeAll([](auto span) {
        std::unique_ptr<int> element = std::move(span.front());
        QCOMPARE(*element, 1);
    });
    QCOMPARE(elementsConsumed, 1);
}

void tst_QAudioRingBuffer::moveOnlyType_produceOne()
{
    QtPrivate::QAudioRingBuffer<std::unique_ptr<int>> dut{ 64 };
    QVERIFY(dut.produceOne([] {
        return std::make_unique<int>(2);
    }));

    int elementsConsumed = dut.consumeAll([](auto span) {
        std::unique_ptr<int> element = std::move(span.front());
        QCOMPARE(*element, 2);
    });
    QCOMPARE(elementsConsumed, 1);
}

void tst_QAudioRingBuffer::destroyRemainingElements_inDestructor()
{
    const auto element = std::make_shared<int>(1);

    {
        QtPrivate::QAudioRingBuffer<std::shared_ptr<int>> dut{ 64 };
        dut.write(element);
    }
    QCOMPARE(element.use_count(), 1);
}

void tst_QAudioRingBuffer::destroy_inConsume()
{
    const auto element = std::make_shared<int>(1);

    QtPrivate::QAudioRingBuffer<std::shared_ptr<int>> dut{ 64 };
    dut.write(element);
    dut.consumeAll([](auto) {
    });

    QCOMPARE(element.use_count(), 1);
}

void tst_QAudioRingBuffer::destroy_inConsumeStressTest()
{
    int destructorCount{};
    int writeCount{};
    int consumeCount{};

    auto makeDestructionCounter = [&] {
        return QScopeGuard([&]() {
            ++destructorCount;
        });
    };
    using DestructionCounter = decltype(makeDestructionCounter());

    auto makeSharedDestructionCounter = [&] {
        return std::make_shared<DestructionCounter>(makeDestructionCounter());
    };

    QtPrivate::QAudioRingBuffer<std::shared_ptr<DestructionCounter>> dut{ 3 };

    enum class mode
    {
        push_one,
        push_multiple,
        consume_one,
        consume_two,
        consume_all,
    };

    constexpr int iterations = 100'000;
    std::mt19937 rng;
    std::discrete_distribution<unsigned> dist{
        5, // push_one
        5, // push_multiple
        3, // consume_one
        1, // consume_two
        1, // consume_all
    };

    for (int i = 0; i != iterations; ++i) {
        auto m = mode(dist(rng));

        switch (m) {
        case mode::push_one: {
            auto element = makeSharedDestructionCounter();
            bool written = dut.write(element);
            if (written)
                writeCount += 1;
            else
                element->dismiss(); // don't count dtor on if write fails
            continue;
        }
        case mode::push_multiple: {
            auto elements = std::array{
                makeSharedDestructionCounter(),
                makeSharedDestructionCounter(),
            };

            int elementsWritten = dut.write(elements);
            if (elementsWritten)
                writeCount += elementsWritten;

            if (elementsWritten != 2) {
                // don't count dtor on if write fails
                int elmentsToDiscard = 2 - elementsWritten;
                auto it = elements.rbegin();
                while (elmentsToDiscard > 0) {
                    --elmentsToDiscard;
                    (*it++)->dismiss();
                };
            }

            continue;
        }
        case mode::consume_one:
            dut.consume(1, [&](auto) {
                consumeCount += 1;
            });
            continue;
        case mode::consume_two:
            consumeCount += dut.consumeSome([&](auto region) {
                if (region.size() == 2) {
                    std::uniform_int_distribution coinFlip(1, 2);
                    int elementsToConsume = coinFlip(rng);
                    return QtMultimediaPrivate::take(region, elementsToConsume);
                } else {
                    return region;
                }
            }, 2);
            continue;
        case mode::consume_all:
            dut.consumeAll([&](auto region) {
                consumeCount += region.size();
            });
            QCOMPARE_EQ(destructorCount, writeCount);
            QCOMPARE_EQ(destructorCount, consumeCount);
            continue;
        }
    }

    dut.consumeAll([&](auto region) {
        consumeCount += region.size();
    });
    QCOMPARE_EQ(destructorCount, writeCount);
    QCOMPARE_EQ(destructorCount, consumeCount);
}

void tst_QAudioRingBuffer::stressTest()
{
    using namespace std::chrono_literals;

    QFETCH(bool, rateLimitProducer);
    QFETCH(bool, rateLimitConsumer);
    QFETCH(int, ringbufferSize);

    QtPrivate::QAudioRingBuffer<int> ringbuffer{ ringbufferSize };

    static constexpr int elementsToPush = 200'000;

    std::thread producer([&] {
        std::mt19937 rng;

        int remain = elementsToPush;
        std::vector<int> writeBuffer;

        int index = 0;

        while (remain) {
            int maxElementsInChunk = std::min(128, remain);
            std::uniform_int_distribution<int> sizeDist(1, maxElementsInChunk);
            int elementsInChunk = sizeDist(rng);
            remain -= elementsInChunk;

            std::generate_n(std::back_inserter(writeBuffer), elementsInChunk, [&] {
                return index++;
            });

            QSpan writeRegion = writeBuffer;
            while (!writeRegion.isEmpty()) {
                int written = ringbuffer.write(writeRegion);
                writeRegion = writeRegion.subspan(written);
            }
            writeBuffer.clear();

            if (rateLimitProducer)
                std::this_thread::sleep_for(1ms);
        }
    });

    // consumer:
    IotaValidator validator;
    int elementsConsumed = 0;
    std::mt19937 rng;
    std::uniform_int_distribution<int> readSizeDist(1, ringbuffer.size() + 2);
    while (validator.state != elementsToPush) {
        int readSize = readSizeDist(rng);

        elementsConsumed += ringbuffer.consume(readSize, [&](QSpan<const int> readRegion) {
            for (int i : readRegion)
                QVERIFY(validator.consumeAndValidate(i));
        });
        if (rateLimitConsumer)
            std::this_thread::sleep_for(1ms);
    }

    producer.join();

    QCOMPARE(elementsConsumed, elementsToPush);
}

void tst_QAudioRingBuffer::stressTest_data()
{
    QTest::addColumn<bool>("rateLimitProducer");
    QTest::addColumn<bool>("rateLimitConsumer");
    QTest::addColumn<int>("ringbufferSize");

    QTest::newRow("no rate limit") << false << false << 64;
    QTest::newRow("no rate limit, size 63") << false << false << 63;
    QTest::newRow("no rate limit, size 67") << false << false << 67;

    QTest::newRow("rate limit producer") << true << false << 64;
    QTest::newRow("rate limit consumer") << false << true << 64;
}

QTEST_APPLESS_MAIN(tst_QAudioRingBuffer);

#include "tst_qaudioringbuffer.moc"
