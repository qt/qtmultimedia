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

    void runAudioCallback_sink_withVolume();
    void runAudioCallback_sink_withVolume_data();

    void runAudioCallback_sink_fmtConvert_QAudioFormat();

    void runAudioCallback_sink_fmtConvert_NativeFmt();

    void runAudioCallback_source_const_withVolume();
    void runAudioCallback_source_const_withVolume_data();

    void runAudioCallback_source_mutable_withVolume();

    void runAudioCallback_source_fmtConvert_QAudioFormat();
    void runAudioCallback_source_fmtConvert_QAudioFormat_data();

    void runAudioCallback_source_fmtConvert_NativeFmt();
    void runAudioCallback_source_fmtConvert_NativeFmt_data();

    void qMultiplySamples();
    void qMultiplySamples_data();

    void bestSampleFormat();

    void sanitizeVolume();
    void sanitizeVolume_data();

    void fillSilence_nativeFmt();
    void fillSilence_nativeFmt_data();

    void fillSilence_audioFormat();
    void fillSilence_audioFormat_data();
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


inline QAudioFormat testFmt(QAudioFormat::SampleFormat sf, int channels = 1)
{
    QAudioFormat fmt;
    fmt.setSampleFormat(sf);
    fmt.setSampleRate(44100);
    fmt.setChannelCount(channels);
    return fmt;
}

// Returns a QByteArray of n IEEE-754 float samples, each equal to val.
inline QByteArray floatBuf(int n, float val)
{
    QByteArray buf(n * int(sizeof(float)), Qt::Initialization::Uninitialized);
    auto *p = reinterpret_cast<float *>(buf.data());
    for (int i = 0; i < n; ++i)
        p[i] = val;
    return buf;
}

// Returns a QByteArray of n int16 samples encoding normalised val in [-1, 1].
inline QByteArray int16Buf(int n, float val)
{
    QByteArray buf(n * int(sizeof(int16_t)), Qt::Initialization::Uninitialized);
    auto *p = reinterpret_cast<int16_t *>(buf.data());
    const auto encoded = static_cast<int16_t>(val * float((1 << 15) - 1));
    for (int i = 0; i < n; ++i)
        p[i] = encoded;
    return buf;
}

// Reads the i-th float sample from a raw byte buffer.
inline float readFloat(const QByteArray &buf, int i = 0)
{
    float f;
    std::copy_n(buf.constData() + i * int(sizeof(float)), sizeof(float),
                reinterpret_cast<char *>(&f));
    return f;
}

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

        QTest::newRow(makeRowName("basic, 1.0").constData()) << fmt << 1.0f << 0.5f << 0.5f;
        QTest::newRow(makeRowName("basic, 0.5").constData()) << fmt << 0.5f << 0.5f << 0.25f;
        QTest::newRow(makeRowName("basic, -0.5").constData()) << fmt << -0.5f << 0.5f << -0.25f;
        QTest::newRow(makeRowName("basic, -1.0").constData()) << fmt << -1.f << 0.5f << -0.5f;

        if (fmt == SampleFormat::Float)
            QTest::newRow(
                    makeRowName("volume is not clamped for floating point samples").constData())
                    << fmt << 0.5f << 2.0f << 1.f;
        else
            QTest::newRow(
                    makeRowName("volume is clamped to 1.0 when using integer samples").constData())
                    << fmt << 0.5f << 2.0f << 0.5f;

        QTest::newRow(makeRowName("volume 1 (noop)").constData()) << fmt << 0.5f << 1.0f << 0.5f;
        QTest::newRow(makeRowName("volume 0 (results in silence)").constData())
                << fmt << 0.5f << 0.0f << 0.0f;
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

void tst_QAudioHelpers::runAudioCallback_sink_withVolume()
{
    using namespace QtMultimediaPrivate;
    QFETCH(float, fillValue);
    QFETCH(float, volume);
    QFETCH(float, expected);

    const auto fmt = testFmt(QAudioFormat::Float);
    QByteArray hostBuf = floatBuf(1, 0.0f);

    AudioSinkCallback cb = std::function<void(QSpan<float>)>([fillValue](QSpan<float> buf) {
        for (float &s : buf)
            s = fillValue;
    });

    runAudioCallback(cb, as_writable_bytes(QSpan{ hostBuf }), fmt, volume);

    QCOMPARE_FLOAT_NEAR(readFloat(hostBuf), expected, 1e-4f);
}

void tst_QAudioHelpers::runAudioCallback_sink_withVolume_data()
{
    QTest::addColumn<float>("fillValue");
    QTest::addColumn<float>("volume");
    QTest::addColumn<float>("expected");

    QTest::newRow("0.5 * 0.5 = 0.25")   << 0.5f  << 0.5f << 0.25f;
    QTest::newRow("0.5 * 1.0 = 0.5")    << 0.5f  << 1.0f << 0.5f;
    QTest::newRow("0.5 * 0.0 = 0.0")    << 0.5f  << 0.0f << 0.0f;
    QTest::newRow("-0.5 * 0.5 = -0.25") << -0.5f << 0.5f << -0.25f;
}

void tst_QAudioHelpers::runAudioCallback_sink_fmtConvert_QAudioFormat()
{
    using namespace QtMultimediaPrivate;

    // Application: float; host: int16 — 1 frame, 1 channel.
    const auto appFmt = testFmt(QAudioFormat::Float);
    const auto hostFmt = testFmt(QAudioFormat::Int16);

    QByteArray hostBuf(int(sizeof(int16_t)), Qt::Initialization::Uninitialized);
    AudioSinkCallback cb = std::function<void(QSpan<float>)>([](QSpan<float> buf) {
        for (float &s : buf)
            s = 0.5f;
    });

    runAudioCallback(cb, as_writable_bytes(QSpan{ hostBuf }), appFmt, 1.0f, hostFmt);

    int16_t raw{};
    std::copy_n(hostBuf.constData(), sizeof(raw), reinterpret_cast<char *>(&raw));
    QCOMPARE_FLOAT_NEAR(float(raw) / float((1 << 15) - 1), 0.5f, 1e-3f);
}

void tst_QAudioHelpers::runAudioCallback_sink_fmtConvert_NativeFmt()
{
    using namespace QtMultimediaPrivate;

    // Same conversion as overload 3 but driven by NativeSampleFormat directly.
    const auto appFmt = testFmt(QAudioFormat::Float);

    QByteArray hostBuf(int(sizeof(int16_t)), Qt::Initialization::Uninitialized);
    AudioSinkCallback cb = std::function<void(QSpan<float>)>([](QSpan<float> buf) {
        for (float &s : buf)
            s = 0.5f;
    });

    runAudioCallback(cb, as_writable_bytes(QSpan{ hostBuf }), appFmt, 1.0f,
                     NativeSampleFormat::int16_t);

    int16_t raw{};
    std::copy_n(hostBuf.constData(), sizeof(raw), reinterpret_cast<char *>(&raw));
    QCOMPARE_FLOAT_NEAR(float(raw) / float((1 << 15) - 1), 0.5f, 1e-3f);
}

void tst_QAudioHelpers::runAudioCallback_source_const_withVolume()
{
    using namespace QtMultimediaPrivate;
    QFETCH(float, volume);
    QFETCH(float, expectedCaptured);

    const auto fmt = testFmt(QAudioFormat::Float);
    QByteArray hostBuf = floatBuf(1, 0.5f);

    float captured = 0.0f;
    bool invoked = false;
    AudioSourceCallback cb =
            std::function<void(QSpan<const float>)>([&](QSpan<const float> buf) {
                invoked = true;
                if (!buf.empty())
                    captured = buf[0];
            });

    runAudioCallback(cb, as_bytes(QSpan{ hostBuf }), fmt, volume);

    QVERIFY(invoked);
    QCOMPARE_FLOAT_NEAR(captured, expectedCaptured, 1e-4f);
    // Host buffer must be unmodified — it was passed as const bytes.
    QCOMPARE_FLOAT_NEAR(readFloat(hostBuf), 0.5f, 1e-6f);
}

void tst_QAudioHelpers::runAudioCallback_source_const_withVolume_data()
{
    QTest::addColumn<float>("volume");
    QTest::addColumn<float>("expectedCaptured");

    QTest::newRow("vol=1.0 (direct path)")    << 1.0f << 0.5f;
    QTest::newRow("vol=0.5 (temp buffer)")    << 0.5f << 0.25f;
    QTest::newRow("vol=0.0 (silence)")        << 0.0f << 0.0f;
}

void tst_QAudioHelpers::runAudioCallback_source_mutable_withVolume()
{
    using namespace QtMultimediaPrivate;

    const auto fmt = testFmt(QAudioFormat::Float);
    QByteArray hostBuf = floatBuf(1, 0.5f);

    float captured = 0.0f;
    bool invoked = false;
    AudioSourceCallback cb =
            std::function<void(QSpan<const float>)>([&](QSpan<const float> buf) {
                invoked = true;
                if (!buf.empty())
                    captured = buf[0];
            });

    runAudioCallback(cb, as_writable_bytes(QSpan{ hostBuf }), fmt, 0.5f);

    QVERIFY(invoked);
    QCOMPARE_FLOAT_NEAR(captured, 0.25f, 1e-4f);
    // Host buffer is modified in-place by applyVolume before the callback.
    QCOMPARE_FLOAT_NEAR(readFloat(hostBuf), 0.25f, 1e-4f);
}

void tst_QAudioHelpers::runAudioCallback_source_fmtConvert_QAudioFormat()
{
    using namespace QtMultimediaPrivate;
    QFETCH(bool, mutableHostBuffer);

    // Host: int16 encoding of 0.5f;  application: float.
    const auto appFmt = testFmt(QAudioFormat::Float);
    const auto hostFmt = testFmt(QAudioFormat::Int16);
    QByteArray hostBuf = int16Buf(1, 0.5f);

    float captured = 0.0f;
    bool invoked = false;
    AudioSourceCallback cb =
            std::function<void(QSpan<const float>)>([&](QSpan<const float> buf) {
                invoked = true;
                if (!buf.empty())
                    captured = buf[0];
            });

    if (mutableHostBuffer)
        runAudioCallback(cb, as_writable_bytes(QSpan{ hostBuf }), appFmt, 1.0f, hostFmt);
    else
        runAudioCallback(cb, as_bytes(QSpan{ hostBuf }), appFmt, 1.0f, hostFmt);

    QVERIFY(invoked);
    QCOMPARE_FLOAT_NEAR(captured, 0.5f, 1e-3f);
}

void tst_QAudioHelpers::runAudioCallback_source_fmtConvert_QAudioFormat_data()
{
    QTest::addColumn<bool>("mutableHostBuffer");
    QTest::newRow("mutable (QSpan<std::byte>)")        << true;
    QTest::newRow("immutable (QSpan<const std::byte>)") << false;
}

void tst_QAudioHelpers::runAudioCallback_source_fmtConvert_NativeFmt()
{
    using namespace QtMultimediaPrivate;
    QFETCH(bool, mutableHostBuffer);

    const auto appFmt = testFmt(QAudioFormat::Float);
    QByteArray hostBuf = int16Buf(1, 0.5f);

    float captured = 0.0f;
    bool invoked = false;
    AudioSourceCallback cb =
            std::function<void(QSpan<const float>)>([&](QSpan<const float> buf) {
                invoked = true;
                if (!buf.empty())
                    captured = buf[0];
            });

    if (mutableHostBuffer)
        runAudioCallback(cb, as_writable_bytes(QSpan{ hostBuf }), appFmt, 1.0f,
                         NativeSampleFormat::int16_t);
    else
        runAudioCallback(cb, as_bytes(QSpan{ hostBuf }), appFmt, 1.0f,
                         NativeSampleFormat::int16_t);

    QVERIFY(invoked);
    QCOMPARE_FLOAT_NEAR(captured, 0.5f, 1e-3f);
}

void tst_QAudioHelpers::runAudioCallback_source_fmtConvert_NativeFmt_data()
{
    QTest::addColumn<bool>("mutableHostBuffer");
    QTest::newRow("mutable (QSpan<std::byte>)")        << true;
    QTest::newRow("immutable (QSpan<const std::byte>)") << false;
}

// ─── qMultiplySamples ─────────────────────────────────────────────────────

void tst_QAudioHelpers::qMultiplySamples()
{
    QFETCH(QAudioFormat::SampleFormat, sampleFormat);
    QFETCH(float, srcVal);
    QFETCH(float, factor);
    QFETCH(float, expectedVal);

    QByteArray src = WordConverter::toBytes(srcVal, sampleFormat);
    QByteArray dst(src.size(), Qt::Initialization::Uninitialized);

    QAudioFormat fmt;
    fmt.setSampleFormat(sampleFormat);

    QAudioHelperInternal::qMultiplySamples(factor, fmt, src.constData(), dst.data(),
                                           src.size());

    float eps = (sampleFormat != QAudioFormat::SampleFormat::UInt8) ? 0.001f : 0.05f;
    QCOMPARE_FLOAT_NEAR(WordConverter::fromBytes(dst, sampleFormat), expectedVal, eps);
}

void tst_QAudioHelpers::qMultiplySamples_data()
{
    using SF = QAudioFormat::SampleFormat;

    QTest::addColumn<SF>("sampleFormat");
    QTest::addColumn<float>("srcVal");
    QTest::addColumn<float>("factor");
    QTest::addColumn<float>("expectedVal");

    auto rows = [](const char *label, SF fmt) {
        auto mkName = [&](const char *desc) {
            return QString(QLatin1String(label) + "_" + desc).toUtf8();
        };
        QTest::newRow(mkName("0.5 * 0.5").constData())      << fmt << 0.5f  << 0.5f << 0.25f;
        QTest::newRow(mkName("0.5 * 1.0").constData())      << fmt << 0.5f  << 1.0f << 0.5f;
        QTest::newRow(mkName("0.5 * 0.0").constData())      << fmt << 0.5f  << 0.0f << 0.0f;
        QTest::newRow(mkName("-0.5 * 0.5").constData())     << fmt << -0.5f << 0.5f << -0.25f;
    };

    rows("float", SF::Float);
    rows("int16", SF::Int16);
    rows("int32", SF::Int32);
    rows("uint8", SF::UInt8);
}

// ─── bestSampleFormat ─────────────────────────────────────────────────────

void tst_QAudioHelpers::bestSampleFormat()
{
    // Verify round-trip: toNativeSampleFormat → bestSampleFormat → toNativeSampleFormat
    const auto formats = {
        QAudioFormat::SampleFormat::UInt8,
        QAudioFormat::SampleFormat::Int16,
        QAudioFormat::SampleFormat::Int32,
        QAudioFormat::SampleFormat::Float,
    };

    for (auto sfmt : formats) {
        auto nfmt = QAudioHelperInternal::toNativeSampleFormat(sfmt);
        auto roundtrip = QAudioHelperInternal::bestSampleFormat(nfmt);
        QCOMPARE_EQ(roundtrip, sfmt);
    }
}

// ─── sanitizeVolume ──────────────────────────────────────────────────────

void tst_QAudioHelpers::sanitizeVolume()
{
    QFETCH(float, input);
    QFETCH(float, last);
    QFETCH(std::optional<float>, expectedResult);

    auto result = QAudioHelperInternal::sanitizeVolume(input, last);
    QCOMPARE_EQ(result, expectedResult);
}

void tst_QAudioHelpers::sanitizeVolume_data()
{
    QTest::addColumn<float>("input");
    QTest::addColumn<float>("last");
    QTest::addColumn<std::optional<float>>("expectedResult");

    QTest::newRow("no change 0.5") << 0.5f  << 0.5f << std::optional<float>{std::nullopt};
    QTest::newRow("no change 1.0") << 1.0f  << 1.0f << std::optional<float>{std::nullopt};
    QTest::newRow("no change 0.0") << 0.0f  << 0.0f << std::optional<float>{std::nullopt};

    QTest::newRow("change 0.3")    << 0.3f  << 0.5f << std::optional<float>{ 0.3f };
    QTest::newRow("clamp 1.5")     << 1.5f  << 0.0f << std::optional<float>{ 1.0f };
    QTest::newRow("clamp -0.5")    << -0.5f << 1.0f << std::optional<float>{ 0.0f };
}

// ─── fillSilence (NativeSampleFormat variant) ──────────────────────────────

void tst_QAudioHelpers::fillSilence_nativeFmt()
{
    QFETCH(NativeSampleFormat, fmt);
    QFETCH(int, frameCount);

    const int bytes = frameCount * int(QAudioHelperInternal::bytesPerSample(fmt));
    QByteArray buf(bytes, Qt::Initialization::Uninitialized);

    // Fill with garbage first
    for (int i = 0; i < buf.size(); ++i)
        buf[i] = char(0xAA);

    QAudioHelperInternal::fillSilence(as_writable_bytes(QSpan{ buf }), fmt);

    // Verify silence: uint8=0x80, others=0x00
    const quint8 expected = (fmt == NativeSampleFormat::uint8_t) ? 0x80u : 0x00u;
    for (int i = 0; i < buf.size(); ++i)
        QCOMPARE_EQ(quint8(buf[i]), expected);
}

void tst_QAudioHelpers::fillSilence_nativeFmt_data()
{
    QTest::addColumn<NativeSampleFormat>("fmt");
    QTest::addColumn<int>("frameCount");

    QTest::newRow("uint8, 1")                << NativeSampleFormat::uint8_t      << 1;
    QTest::newRow("int16, 2")                << NativeSampleFormat::int16_t      << 2;
    QTest::newRow("int32, 3")                << NativeSampleFormat::int32_t      << 3;
    QTest::newRow("float, 4")                << NativeSampleFormat::float32_t    << 4;
    QTest::newRow("int24_3b, 10")            << NativeSampleFormat::int24_t_3b   << 10;
    QTest::newRow("int24_4b_low, 5")         << NativeSampleFormat::int24_t_4b_low << 5;
}

// ─── fillSilence (QAudioFormat variant) ────────────────────────────────────

void tst_QAudioHelpers::fillSilence_audioFormat()
{
    QFETCH(QAudioFormat::SampleFormat, sfmt);
    QFETCH(int, channels);
    QFETCH(int, frames);

    auto fmt = testFmt(sfmt, channels);
    int bytes = fmt.bytesForFrames(frames);
    QByteArray buf(bytes, Qt::Initialization::Uninitialized);

    for (int i = 0; i < buf.size(); ++i)
        buf[i] = char(0xBB);

    QAudioHelperInternal::fillSilence(as_writable_bytes(QSpan{ buf }), fmt);

    // Verify silence: uint8=0x80, others=0x00
    const quint8 expected = (sfmt == QAudioFormat::UInt8) ? 0x80u : 0x00u;
    for (int i = 0; i < buf.size(); ++i)
        QCOMPARE_EQ(quint8(buf[i]), expected);
}

void tst_QAudioHelpers::fillSilence_audioFormat_data()
{
    using SF = QAudioFormat::SampleFormat;

    QTest::addColumn<SF>("sfmt");
    QTest::addColumn<int>("channels");
    QTest::addColumn<int>("frames");

    QTest::newRow("float, 1ch, 1fr")    << SF::Float  << 1 << 1;
    QTest::newRow("float, 2ch, 2fr")    << SF::Float  << 2 << 2;
    QTest::newRow("int16, 1ch, 4fr")    << SF::Int16  << 1 << 4;
    QTest::newRow("int16, 2ch, 3fr")    << SF::Int16  << 2 << 3;
    QTest::newRow("int32, 2ch, 5fr")    << SF::Int32  << 2 << 5;
    QTest::newRow("uint8, 1ch, 8fr")    << SF::UInt8  << 1 << 8;
}

QTEST_APPLESS_MAIN(tst_QAudioHelpers);

#include "tst_qaudiohelpers.moc"
