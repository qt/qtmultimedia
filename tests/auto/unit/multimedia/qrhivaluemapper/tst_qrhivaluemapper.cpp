// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <private/qrhivaluemapper_p.h>
#include <rhi/qrhi.h>

#include <memory>

QT_USE_NAMESPACE

namespace {

using Value = std::shared_ptr<int>;

Value makeValue(int value)
{
    return std::make_shared<int>(value);
}

struct ValueProvider
{
    operator Value()
    {
        ++invocationCount;
        return value;
    }

    Value value = makeValue(0);
    int invocationCount = 0;
};

std::unique_ptr<QRhi> createRhi()
{
    return std::unique_ptr<QRhi>{ QRhi::create(QRhi::Null, {}) };
}

} // namespace

class tst_QRhiValueMapper : public QObject
{
    Q_OBJECT
public slots:
    void init() { m_mapper = {}; }

private slots:
    void tryMap_mapsValueToRhi_whenRhiIsNotInTheMap()
    {
        // Arrange
        Value value1 = makeValue(1);
        ValueProvider value2Provider{ makeValue(2) };

        auto rhi1 = createRhi();
        auto rhi2 = createRhi();

        // Act
        auto mapResult1 = m_mapper.tryMap(*rhi1, value1);
        auto mapResult2 = m_mapper.tryMap(*rhi2, value2Provider);

        // Assert

        QVERIFY(mapResult1.second);
        QVERIFY(mapResult2.second);

        QVERIFY(mapResult1.first);
        QVERIFY(mapResult2.first);

        QCOMPARE(*mapResult1.first, value1);
        QCOMPARE(*mapResult2.first, value2Provider.value);

        QCOMPARE(value1.use_count(), 2);
        QCOMPARE(value2Provider.value.use_count(), 2);
        QCOMPARE(value2Provider.invocationCount, 1);
    }

    void tryMap_doesntMapValueToRhi_whenRhiIsInTheMap()
    {
        // Arrange
        auto rhi1 = createRhi();
        auto rhi2 = createRhi();

        Value value1 = makeValue(1);
        Value value2 = makeValue(2);

        m_mapper.tryMap(*rhi1, value1);
        m_mapper.tryMap(*rhi2, value2);

        ValueProvider valueProvider{ makeValue(10) };

        // Act
        auto mapResult1 = m_mapper.tryMap(*rhi1, valueProvider);
        auto mapResult2 = m_mapper.tryMap(*rhi2, valueProvider);

        // Assert
        QVERIFY(!mapResult1.second);
        QVERIFY(!mapResult2.second);

        QVERIFY(mapResult1.first);
        QVERIFY(mapResult2.first);

        QCOMPARE(*mapResult1.first, value1);
        QCOMPARE(*mapResult2.first, value2);

        QCOMPARE(valueProvider.invocationCount, 0);
    }

    void get_returnsValuePointer_whenRhiIsInTheMap()
    {
        // Arrange
        auto rhi = createRhi();

        Value value = makeValue(1);
        Value *addingResult = m_mapper.tryMap(*rhi, value).first;

        // Act
        Value *gettingResult = m_mapper.get(rhi.get());

        // Assert
        QVERIFY(gettingResult);
        QCOMPARE(gettingResult, addingResult);
        QCOMPARE(*addingResult, value);
    }

    void get_returnsNull_whenRhiIsNotInTheMap()
    {
        // Arrange
        auto rhi1 = createRhi();
        auto rhi2 = createRhi();
        m_mapper.tryMap(*rhi1, makeValue(1));

        // Act
        Value *gettingResult = m_mapper.get(rhi2.get());

        // Assert
        QCOMPARE(gettingResult, nullptr);
    }

    void clears_removesAllElements()
    {
        // Arrange
        auto rhi1 = createRhi();
        auto rhi2 = createRhi();

        Value value1 = makeValue(1);
        Value value2 = makeValue(2);

        m_mapper.tryMap(*rhi1, value1);
        m_mapper.tryMap(*rhi2, value2);

        // Act
        m_mapper.clear();

        // Assert
        QCOMPARE(m_mapper.get(rhi1.get()), nullptr);
        QCOMPARE(m_mapper.get(rhi2.get()), nullptr);

        QCOMPARE(value1.use_count(), 1);
        QCOMPARE(value2.use_count(), 1);
    }

    void valueIsDeleted_whenRhiIsDeleted()
    {
        // Arrange
        auto rhi1 = createRhi();
        auto rhi2 = createRhi();

        Value value1 = makeValue(1);
        Value value2 = makeValue(2);

        m_mapper.tryMap(*rhi1, value1);
        Value *addingResult = m_mapper.tryMap(*rhi2, value2).first;

        // Act
        rhi1.reset();

        // Assert
        QCOMPARE(m_mapper.get(rhi2.get()), addingResult);
        QCOMPARE(value1.use_count(), 1);
        QCOMPARE(value2.use_count(), 2);
    }

    void mappedValueIsRemoved_whenMatchingRhiIsCleaned()
    {
        // Arrange
        auto rhi1 = createRhi();
        auto rhi2 = createRhi();

        Value value1 = makeValue(1);
        Value value2 = makeValue(2);

        m_mapper.tryMap(*rhi1, value1);
        Value *addingResult = m_mapper.tryMap(*rhi2, value2).first;

        // Act
        QRhi *rhi1Ptr = rhi1.get();
        rhi1 = nullptr;

        // Assert
        QCOMPARE(m_mapper.get(rhi1Ptr), nullptr);
        QCOMPARE(m_mapper.get(rhi2.get()), addingResult);

        QCOMPARE(value1.use_count(), 1);
        QCOMPARE(value2.use_count(), 2);
    }

    void findRhi_findsRhiAccordingToPredicate()
    {
        // Arrange
        auto rhi1 = createRhi();
        auto rhi2 = createRhi();

        m_mapper.tryMap(*rhi1, makeValue(1));
        m_mapper.tryMap(*rhi2, makeValue(2));

        // Act
        auto foundRhi1 = m_mapper.findRhi([&rhi1](QRhi &rhi) {
            return rhi1.get() == &rhi;
        });

        auto foundRhi2 = m_mapper.findRhi([&rhi2](QRhi &rhi) {
            return rhi2.get() == &rhi;
        });

        auto notFoundRhi = m_mapper.findRhi([](QRhi &) {
            return false;
        });

        // Assert
        QCOMPARE(foundRhi1, rhi1.get());
        QCOMPARE(foundRhi2, rhi2.get());
        QCOMPARE(notFoundRhi, nullptr);
    }

private:
    QRhiValueMapper<Value> m_mapper;
};

QTEST_APPLESS_MAIN(tst_QRhiValueMapper)

#include "tst_qrhivaluemapper.moc"

//clang-format on
