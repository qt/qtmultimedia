// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

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

    ringbuffer.write({ 1 });

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
    ringbuffer.write({ 1 });

    ringbuffer.reset();

    QCOMPARE(ringbuffer.free(), 64);
    QCOMPARE(ringbuffer.used(), 0);

    int elementsConsumed = ringbuffer.consumeAll([](auto) {
    });
    QCOMPARE(elementsConsumed, 0);
}

void tst_QAudioRingBuffer::stressTest()
{
    using namespace std::chrono_literals;

    QFETCH(bool, rateLimitProducer);
    QFETCH(bool, rateLimitConsumer);

    QtPrivate::QAudioRingBuffer<int> ringbuffer{ 64 };

    static constexpr int elementsToPush = 1'000'000;

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

    QTest::newRow("no rate limit") << false << false;
    QTest::newRow("rate limit producer") << true << false;
    QTest::newRow("rate limit consumer") << false << true;
}

QTEST_APPLESS_MAIN(tst_QAudioRingBuffer);

#include "tst_qaudioringbuffer.moc"
