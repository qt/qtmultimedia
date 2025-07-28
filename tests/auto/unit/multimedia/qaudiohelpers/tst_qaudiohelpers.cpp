// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qbytearray.h>
#include <QtTest/qtest.h>

#include <QtMultimedia/private/qaudiohelpers_p.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qaudio_alignment_support_p.h>
#include <QtMultimedia/private/qaudio_qspan_support_p.h>

// NOLINTBEGIN(readability-convert-member-functions-to-static)

class tst_QAudioHelpers : public QObject
{
    Q_OBJECT

private slots:
    void applyVolume();
    void applyVolume_data();

    void alignmentSupport();

    void span_drop();
    void span_take();

    void wordConverter();
    void wordConverter_data();

    void wordConverter_checkLoopUnroll();

    void findBestNativeSampleFormat();

    void validateAudioCallbacks();
};

namespace WordConverter {
QByteArray toBytes(float value, QAudioFormat::SampleFormat sampleFormat)
{
    switch (sampleFormat) {
    case QAudioFormat::SampleFormat::Float: {
        return QByteArray(reinterpret_cast<char *>(&value), sizeof(float));
    }
    case QAudioFormat::SampleFormat::Int16: {
        static constexpr int16_t range = (int64_t(1) << 15) - 1;
        int16_t intVal = value * float(range);
        return QByteArray(reinterpret_cast<char *>(&intVal), sizeof(int16_t));
    }
    case QAudioFormat::SampleFormat::Int32: {
        static constexpr uint32_t range = (int64_t(1) << 31) - 1;
        int32_t intVal = value * double(range);
        return QByteArray(reinterpret_cast<char *>(&intVal), sizeof(int32_t));
    }
    case QAudioFormat::SampleFormat::UInt8: {
        static constexpr uint8_t bias = (1 << 7);
        static constexpr uint8_t range = (1 << 7) - 1;

        uint8_t intVal = value * range + bias;
        return QByteArray(reinterpret_cast<char *>(&intVal), sizeof(uint8_t));
    }
    default: {
        Q_UNREACHABLE_RETURN({});
    }
    }
}

float fromBytes(QByteArrayView value, QAudioFormat::SampleFormat sampleFormat)
{
    switch (sampleFormat) {
    case QAudioFormat::SampleFormat::Float: {
        QTEST_ASSERT(value.size() == sizeof(float));
        float f;
        std::copy_n(value.data(), sizeof(float), reinterpret_cast<char *>(&f));
        return f;
    }
    case QAudioFormat::SampleFormat::Int16: {
        QTEST_ASSERT(value.size() == sizeof(int16_t));
        int16_t intVal;
        std::copy_n(value.data(), sizeof(int16_t), reinterpret_cast<char *>(&intVal));
        return float(intVal) / float((1 << 15) - 1);
    }
    case QAudioFormat::SampleFormat::Int32: {
        QTEST_ASSERT(value.size() == sizeof(int32_t));
        int32_t intVal;
        std::copy_n(value.data(), sizeof(int32_t), reinterpret_cast<char *>(&intVal));
        return float(double(intVal) / double((int64_t(1) << 31) - 1));
    }
    case QAudioFormat::SampleFormat::UInt8: {
        QTEST_ASSERT(value.size() == sizeof(uint8_t));
        uint8_t intVal;
        std::copy_n(value.data(), sizeof(uint8_t), reinterpret_cast<char *>(&intVal));
        return float(intVal) / float((1 << 7) - 1) - 1.f;
    }
    default: {
        Q_UNREACHABLE_RETURN({});
    }
    }
}
}; // namespace WordConverter

// FIXME: it seems that qtestlib is missing floating point comparison helpers???
// Compare QTBUG-104000
#define QCOMPARE_FLOAT_NEAR(computed, baseline, epsilon) \
    QVERIFY2(std::abs(computed - baseline) < epsilon, \
             QStringLiteral("QCOMPARE_FLOAT_NEAR(%1, %2, %3) failed") \
                 .arg(computed) \
                 .arg(baseline) \
                 .arg(epsilon) \
                 .toUtf8() \
                 .data())

void tst_QAudioHelpers::applyVolume()
{
    QFETCH(QAudioFormat::SampleFormat, sampleFormat);
    QFETCH(float, value);
    QFETCH(float, factor);
    QFETCH(float, expectedResult);

    QByteArray data = WordConverter::toBytes(value, sampleFormat);
    QByteArray destination{ data,  };
    destination.detach();

    QAudioFormat fmt;
    fmt.setSampleFormat(sampleFormat);

    QAudioHelperInternal::applyVolume(factor, fmt, as_bytes(QSpan{ data }),
                                      as_writable_bytes(QSpan{ destination }));

    float epsilon = (sampleFormat != QAudioFormat::SampleFormat::UInt8) ? 0.001f : 0.05f;
    QCOMPARE_FLOAT_NEAR(WordConverter::fromBytes(destination, sampleFormat),
                        expectedResult,
                        epsilon);
}

void tst_QAudioHelpers::applyVolume_data()
{
    using SampleFormat = QAudioFormat::SampleFormat;

    QTest::addColumn<SampleFormat>("sampleFormat");
    QTest::addColumn<float>("value");
    QTest::addColumn<float>("factor");
    QTest::addColumn<float>("expectedResult");

    auto makeEntriesFor = [](const char *label, SampleFormat fmt) {
        auto makeRowName = [&](const char *testcase) {
            QString rowName = QLatin1String(label) + QByteArrayLiteral("_") + testcase;
            return rowName.toUtf8();
        };

        QTest::newRow(makeRowName("basic, 1.0")) << fmt << 1.0f << 0.5f << 0.5f;
        QTest::newRow(makeRowName("basic, 0.5")) << fmt << 0.5f << 0.5f << 0.25f;
        QTest::newRow(makeRowName("basic, -0.5")) << fmt << -0.5f << 0.5f << -0.25f;
        QTest::newRow(makeRowName("basic, -1.0")) << fmt << -1.f << 0.5f << -0.5f;

        if (fmt == SampleFormat::Float)
            QTest::newRow(makeRowName("volume is not clamped for floating point samples"))
                << fmt << 0.5f << 2.0f << 1.f;
        else
            QTest::newRow(makeRowName("volume is clamped to 1.0 when using integer samples"))
                << fmt << 0.5f << 2.0f << 0.5f;

        QTest::newRow(makeRowName("volume 1 (noop)")) << fmt << 0.5f << 1.0f << 0.5f;
        QTest::newRow(makeRowName("volume 0 (results in silence)")) << fmt << 0.5f << 0.0f << 0.0f;
    };

    makeEntriesFor("int16", SampleFormat::Int16);
    makeEntriesFor("float", SampleFormat::Float);
    makeEntriesFor("int32", SampleFormat::Int32);
    makeEntriesFor("uint8", SampleFormat::UInt8);
}

void tst_QAudioHelpers::alignmentSupport()
{
    using namespace QtMultimediaPrivate;
    static_assert(isPowerOfTwo(4));
    static_assert(!isPowerOfTwo(5));

    static_assert(alignUp(4, 8) == 8);
    static_assert(alignUp(12, 8) == 16);

    static_assert(alignDown(4, 8) == 0);
    static_assert(alignDown(12, 8) == 8);

    static_assert(!isAligned(4, 8));
    static_assert(isAligned(16, 8));

    auto intPtr = std::make_unique<int>();
    QVERIFY(isAligned(intPtr.get(), 4));

    auto charPtr = reinterpret_cast<char *>(intPtr.get());
    QVERIFY(!isAligned(charPtr + 1, 4));
    QCOMPARE_EQ(alignDown(charPtr + 1, 4), charPtr);
}

void tst_QAudioHelpers::span_drop()
{
    using namespace QtMultimediaPrivate;

    std::array<int, 3> x;
    QSpan<int> dut{ x };

    QVERIFY(drop(dut, 3).empty());
    QCOMPARE(drop(dut, 2).size(), 1);

    QSpan<int> emptySpan = {};
    QVERIFY(drop(emptySpan, 3).empty());
}

void tst_QAudioHelpers::span_take()
{
    using namespace QtMultimediaPrivate;

    std::array<int, 3> x;
    QSpan<int> dut{ x };

    QCOMPARE(take(dut, 4).size(), 3);
    QCOMPARE(take(dut, 3).size(), 3);
    QCOMPARE(take(dut, 2).size(), 2);
    QVERIFY(take(dut, 0).empty());

    QSpan<int> emptySpan = {};
    QVERIFY(take(emptySpan, 3).empty());
}

using NativeSampleFormat = QAudioHelperInternal::NativeSampleFormat;

void tst_QAudioHelpers::wordConverter()
{
    QFETCH(QByteArray, argument);
    QFETCH(QByteArray, expected);
    QFETCH(NativeSampleFormat, sourceFormat);
    QFETCH(NativeSampleFormat, destinationFormat);

    QByteArray destination(expected.size(), Qt::Initialization::Uninitialized);
    QAudioHelperInternal::convertSampleFormat(as_bytes(QSpan{argument}),
                                              sourceFormat,
                                              as_writable_bytes(QSpan{destination}),
                                              destinationFormat);

    QCOMPARE_EQ(destination, expected);
}

void tst_QAudioHelpers::wordConverter_data()
{
    QTest::addColumn<QByteArray>("argument");
    QTest::addColumn<NativeSampleFormat>("sourceFormat");
    QTest::addColumn<QByteArray>("expected");
    QTest::addColumn<NativeSampleFormat>("destinationFormat");

    // uint8 source
    QTest::newRow("uint8 (0) to int32")
            << QByteArray("\x80", 1) << NativeSampleFormat::uint8_t
            << QByteArray("\x00\x00\x00\x00", 4) << NativeSampleFormat::int32_t;

    QTest::newRow("uint8 (-1.f) to int32")
            << QByteArray("\x00", 1) << NativeSampleFormat::uint8_t
            << QByteArray("\x00\x00\x00\x80", 4) << NativeSampleFormat::int32_t;

    QTest::newRow("uint8 (1.f) to int32")
            << QByteArray("\xff", 1) << NativeSampleFormat::uint8_t
            << QByteArray("\x00\x00\x00\x7f", 4) << NativeSampleFormat::int32_t;

    QTest::newRow("uint8 (0) to int16") << QByteArray("\x80", 1) << NativeSampleFormat::uint8_t
                                        << QByteArray("\x00\x00", 2) << NativeSampleFormat::int16_t;

    QTest::newRow("uint8 (-1.f) to int16")
            << QByteArray("\x00", 1) << NativeSampleFormat::uint8_t << QByteArray("\x00\x80", 2)
            << NativeSampleFormat::int16_t;

    QTest::newRow("uint8 (1.f) to int16")
            << QByteArray("\xff", 1) << NativeSampleFormat::uint8_t << QByteArray("\x00\x7f", 2)
            << NativeSampleFormat::int16_t;

    QTest::newRow("uint8 (0) to float")
            << QByteArray("\x80", 1) << NativeSampleFormat::uint8_t
            << QByteArray("\x00\x00\x00\x00", 4) << NativeSampleFormat::float32_t;

    QTest::newRow("uint8 (-1.f) to float")
            << QByteArray("\x00", 1) << NativeSampleFormat::uint8_t
            << QByteArray("\x00\x00\x80\xbf", 4) << NativeSampleFormat::float32_t;

    QTest::newRow("uint8 (1.f) to float")
            << QByteArray("\xff", 1) << NativeSampleFormat::uint8_t
            << QByteArray("\x00\x00\x7e\x3f", 4) << NativeSampleFormat::float32_t;

    QTest::newRow("uint8 (0) to int24_t_3b")
            << QByteArray("\x80", 1) << NativeSampleFormat::uint8_t << QByteArray("\x00\x00\x00", 3)
            << NativeSampleFormat::int24_t_3b;

    QTest::newRow("uint8 (-1.f) to int24_t_3b")
            << QByteArray("\x00", 1) << NativeSampleFormat::uint8_t << QByteArray("\x00\x00\x80", 3)
            << NativeSampleFormat::int24_t_3b;

    QTest::newRow("uint8 (1.f) to int24_t_3b")
            << QByteArray("\xff", 1) << NativeSampleFormat::uint8_t << QByteArray("\x00\x00\x7f", 3)
            << NativeSampleFormat::int24_t_3b;

    QTest::newRow("uint8 (0) to int24_t_4b_low")
            << QByteArray("\x80", 1) << NativeSampleFormat::uint8_t
            << QByteArray("\x00\x00\x00\x00", 4) << NativeSampleFormat::int24_t_4b_low;

    QTest::newRow("uint8 (-1.f) to int24_t_4b_low")
            << QByteArray("\x00", 1) << NativeSampleFormat::uint8_t
            << QByteArray("\x00\x00\x80\x00", 4) << NativeSampleFormat::int24_t_4b_low;

    QTest::newRow("uint8 (1.f) to int24_t_4b_low")
            << QByteArray("\xff", 1) << NativeSampleFormat::uint8_t
            << QByteArray("\x00\x00\x7f\x00", 4) << NativeSampleFormat::int24_t_4b_low;

    // float source
    QTest::newRow("float (0.f) to int16")
            << QByteArray("\x00\x00\x00\x00", 4) << NativeSampleFormat::float32_t
            << QByteArray("\x00\x00", 2) << NativeSampleFormat::int16_t;

    QTest::newRow("float (1.f) to int16")
            << QByteArray("\x00\x00\x7e\x3f", 4) << NativeSampleFormat::float32_t
            << QByteArray("\xff\x7e", 2) << NativeSampleFormat::int16_t;

    QTest::newRow("float (-1.f) to int16")
            << QByteArray("\x00\x00\x80\xbf", 4) << NativeSampleFormat::float32_t
            << QByteArray("\x00\x80", 2) << NativeSampleFormat::int16_t;
}

void tst_QAudioHelpers::wordConverter_checkLoopUnroll()
{
    auto floatData = QByteArray("\x00\x00\x00\x00" // 0
                                "\x00\x00\x80\xbf" // -1
                                "\x00\x00\x7e\x3f" // 1
                                "\x00\x00\x00\x00" // 0
                                "\x00\x00\x80\xbf" // -1
                                "\x00\x00\x7e\x3f" // 1
                                ,
                                sizeof(float) * 6);

    auto int16Result = QByteArray("\x00\x00" // 0
                                  "\x00\x80" // -1
                                  "\xff\x7e" // 1
                                  "\x00\x00" // 0
                                  "\x00\x80" // -1
                                  "\xff\x7e" // 1
                                  ,
                                  sizeof(int16_t) * 6);

    QByteArray destination(int16Result.size(), Qt::Initialization::Uninitialized);

    QAudioHelperInternal::convertSampleFormat(
            as_bytes(QSpan{ floatData }), NativeSampleFormat::float32_t,
            as_writable_bytes(QSpan{ destination }), NativeSampleFormat::int16_t);

    QCOMPARE_EQ(destination, int16Result);
}

void tst_QAudioHelpers::findBestNativeSampleFormat()
{
    using namespace QAudioHelperInternal;

    auto makeFormat = [](QAudioFormat::SampleFormat sampleFormat) {
        QAudioFormat fmt;
        fmt.setSampleRate(44100);
        fmt.setSampleFormat(sampleFormat);
        fmt.setChannelCount(2);
        return fmt;
    };

    QAudioFormat fmtInt16 = makeFormat(QAudioFormat::SampleFormat::Int16);
    QAudioFormat fmtFloat = makeFormat(QAudioFormat::SampleFormat::Float);

    static const QList<NativeSampleFormat> allFormats{
        NativeSampleFormat::uint8_t,        NativeSampleFormat::int16_t,
        NativeSampleFormat::int32_t,        NativeSampleFormat::int24_t_3b,
        NativeSampleFormat::int24_t_4b_low, NativeSampleFormat::float32_t,
    };
    static const QList<NativeSampleFormat> all24_32_intFormats{
        NativeSampleFormat::int32_t,
        NativeSampleFormat::int24_t_3b,
        NativeSampleFormat::int24_t_4b_low,
    };
    static const QList<NativeSampleFormat> telephoneFormats{
        NativeSampleFormat::uint8_t,
    };

    QCOMPARE_EQ(bestNativeSampleFormat(fmtInt16, allFormats), NativeSampleFormat::int16_t);
    QCOMPARE_EQ(bestNativeSampleFormat(fmtInt16, all24_32_intFormats),
                NativeSampleFormat::int24_t_3b);
    QCOMPARE_EQ(bestNativeSampleFormat(fmtInt16, telephoneFormats), NativeSampleFormat::uint8_t);

    QCOMPARE_EQ(bestNativeSampleFormat(fmtFloat, allFormats), NativeSampleFormat::float32_t);
    QCOMPARE_EQ(bestNativeSampleFormat(fmtFloat, all24_32_intFormats),
                NativeSampleFormat::int24_t_3b);
    QCOMPARE_EQ(bestNativeSampleFormat(fmtFloat, telephoneFormats), NativeSampleFormat::uint8_t);
}

static constexpr QAudioFormat fmtFloat = [] {
    QAudioFormat fmt;
    fmt.setSampleFormat(QAudioFormat::Float);
    fmt.setSampleRate(44100);
    return fmt;
}();

void tst_QAudioHelpers::validateAudioCallbacks()
{
    using namespace QtMultimediaPrivate;

    std::function<void(QSpan<float>)> nullFunction;
    QVERIFY(!validateAudioCallback(nullFunction, fmtFloat));

    std::function<void(QSpan<float>)> trivialFunction = [](QSpan<float>) {
    };
    QVERIFY(validateAudioCallback(trivialFunction, fmtFloat));

    QVERIFY(validateAudioCallback([](QSpan<float>) {
    }, fmtFloat));
    QVERIFY(!validateAudioCallback([](QSpan<int16_t>) {
    }, fmtFloat));

    static_assert(getSampleFormat<std::function<void(QSpan<float>)>>()
                  == QAudioFormat::SampleFormat::Float);
    static_assert(getSampleFormat<std::function<void(QSpan<int16_t>)>>()
                  == QAudioFormat::SampleFormat::Int16);

    static_assert(getSampleFormat<float>() == QAudioFormat::SampleFormat::Float);
    static_assert(getSampleFormat<int16_t>() == QAudioFormat::SampleFormat::Int16);
}

QTEST_APPLESS_MAIN(tst_QAudioHelpers);

#include "tst_qaudiohelpers.moc"
