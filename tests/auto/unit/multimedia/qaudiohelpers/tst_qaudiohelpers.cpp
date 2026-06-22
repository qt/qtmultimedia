// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qbytearray.h>
#include <QtTest/qtest.h>

#include <QtMultimedia/private/qaudiohelpers_p.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimediaTestLib/private/audiogenerationutils_p.h>
#include <QtMultimedia/private/qaudio_alignment_support_p.h>
#include <QtMultimedia/private/qaudio_qspan_support_p.h>

#include <random>

namespace {
template<typename T>
QSpan<const T> asSpan(const QByteArray &ba)
{
    return { reinterpret_cast<const T *>(ba.constData()),
             ba.size() / qsizetype(sizeof(T)) };
}
} // unnamed namespace

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

    void resampleAudioCatmullRom_identity();
    void resampleAudioCatmullRom_mono_downsample();
    void resampleAudioCatmullRom_stereo_upsample();
    void resampleAudioCatmullRom_empty();
    void resampleAudioCatmullRom_fractional_down();
    void resampleAudioCatmullRom_fractional_up();
    void resampleAudioCatmullRom_fractional_periodic_matching();

    void catmullRomInterpolator_mono_downsample();
    void catmullRomInterpolator_stereo_upsample();
    void catmullRomInterpolator_fractional();
    void catmullRomInterpolator_empty();
    void catmullRomInterpolator_reset();
    void catmullRomInterpolator_chunking_independence();

    void upmixMonoToStereo_data();
    void upmixMonoToStereo();

    void upmixMonoToStereo_span_data();
    void upmixMonoToStereo_span();

    void downmixStereoToMono_data();
    void downmixStereoToMono();

    void downmixStereoToMono_span_data();
    void downmixStereoToMono_span();
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

// ─── resampleAudioCatmullRom ──────────────────────────────────────────

void tst_QAudioHelpers::resampleAudioCatmullRom_identity()
{
    // Equal rates: shortcut returns data unchanged
    std::vector<float> samples = { 0.1f, 0.2f, 0.3f, 0.4f };
    QSpan<const float> input(samples.data(), samples.size());

    QByteArray result = QAudioHelperInternal::resampleAudioCatmullRom(input, 1, 44100, 44100);

    QCOMPARE_EQ(result.size(), int(samples.size() * sizeof(float)));
    QSpan fdata(asSpan<float>(result));
    for (size_t i = 0; i < samples.size(); ++i)
        QCOMPARE_FLOAT_NEAR(fdata[i], samples[i], 1e-6f);
}

void tst_QAudioHelpers::resampleAudioCatmullRom_mono_downsample()
{
    // Downsample mono: 44100 Hz → 22050 Hz (ratio 2:1)
    // Create simple ramp: 0, 1, 2, 3, 4, 5, 6, 7 → output should be roughly 0, 2, 4, 6
    std::vector<float> samples = { 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f };
    QSpan<const float> input(samples.data(), samples.size());

    QByteArray result = QAudioHelperInternal::resampleAudioCatmullRom(input, 1, 44100, 22050);

    const qsizetype outFrames = result.size() / qsizetype(sizeof(float));
    QSpan fdata(asSpan<float>(result));

    // Should have ~4 output frames (8 input frames / 2 ratio)
    QVERIFY(outFrames >= 3 && outFrames <= 5);

    // Output samples should roughly follow the even indices of input
    // (with Catmull-Rom smoothing, not exact)
    if (outFrames >= 2) {
        QCOMPARE_FLOAT_NEAR(fdata[0], 0.f, 0.5f);  // near input[0]
        QCOMPARE_FLOAT_NEAR(fdata[1], 2.f, 0.5f);  // near input[2]
    }
}

void tst_QAudioHelpers::resampleAudioCatmullRom_stereo_upsample()
{
    // Upsample stereo: 22050 Hz → 44100 Hz (ratio 1:2)
    // Input: 2 frames × 2 channels = 4 floats: [L0, R0, L1, R1]
    std::vector<float> samples = { 0.f, 1.f, 4.f, 5.f };  // 2 stereo frames
    QSpan<const float> input(samples.data(), samples.size());

    QByteArray result =
            QAudioHelperInternal::resampleAudioCatmullRom(input, 2, 22050, 44100);

    const qsizetype outFrames = result.size() / (2 * qsizetype(sizeof(float)));
    QSpan fdata(asSpan<float>(result));

    // Should have ~4 output frames (2 input frames × 2 ratio)
    QVERIFY(outFrames >= 3 && outFrames <= 5);

    // First frame should match input[0..1]
    if (outFrames >= 1) {
        QCOMPARE_FLOAT_NEAR(fdata[0], 0.f, 0.1f);  // L channel
        QCOMPARE_FLOAT_NEAR(fdata[1], 1.f, 0.1f);  // R channel
    }
}

void tst_QAudioHelpers::resampleAudioCatmullRom_empty()
{
    // Empty input
    QSpan<const float> emptyInput;
    QByteArray result = QAudioHelperInternal::resampleAudioCatmullRom(emptyInput, 1, 44100, 22050);
    QVERIFY(result.isEmpty());

    // Single frame mono: produces minimal/zero output due to algorithm
    std::vector<float> single = { 0.5f };
    QSpan<const float> singleInput(single.data(), single.size());
    result = QAudioHelperInternal::resampleAudioCatmullRom(singleInput, 1, 44100, 22050);
    // Result may be empty or have <1 frame (algorithm produces minimal output at ratio 2:1)
    QVERIFY(result.size() < int(2 * sizeof(float)));
}

void tst_QAudioHelpers::resampleAudioCatmullRom_fractional_down()
{
    // Fractional downsample: 48000 Hz → 44100 Hz (ratio ~0.9188)
    // Sine wave: freq = 1 Hz in source time. Period = 48000 samples @ 48kHz = 1 sec.
    // After resample to 44100 Hz, period = 44100 samples.
    // Zero crossings at: 0, 24000, 48000 (sample indices in 48kHz)
    // Should map to: 0, 22050, 44100 (sample indices in 44100 Hz)
    const int inRate = 48000;
    const int outRate = 44100;
    const int inFrames = 96000;  // 2 seconds @ 48kHz

    std::vector<float> samples(inFrames);
    {
        SineWaveSignal gen(100.0, inRate);
        std::copy_n(gen.begin(), inFrames, samples.begin());
    }

    QSpan<const float> input(samples.data(), samples.size());
    QByteArray result =
            QAudioHelperInternal::resampleAudioCatmullRom(input, 1, inRate, outRate);

    const qsizetype outFrames = result.size() / qsizetype(sizeof(float));
    QSpan fdata(asSpan<float>(result));

    // Output should have ~88200 frames (96000 * 44100 / 48000)
    QVERIFY(outFrames >= 87900 && outFrames <= 88500);

    // Zero crossings: sine crosses zero at phase 0, π, 2π
    // In source: indices 0, 24000, 48000, 72000, 96000
    // In output: indices 0, 22050, 44100, 66150, 88200
    auto expectZeroCrossing = [fdata, outFrames](qsizetype idx, float tolerance = 0.05f) {
        if (idx >= 0 && idx < outFrames) {
            const float val = std::abs(fdata[idx]);
            QVERIFY2(val < tolerance,
                     QStringLiteral("Zero crossing at idx %1: expected ~0, got %2")
                             .arg(idx)
                             .arg(val)
                             .toUtf8()
                             .data());
        }
    };

    expectZeroCrossing(0);          // Start
    expectZeroCrossing(22050);      // π / 2π crossings
    expectZeroCrossing(44100);      // Next full period
}

void tst_QAudioHelpers::resampleAudioCatmullRom_fractional_up()
{
    // Fractional upsample: 44100 Hz → 48000 Hz (ratio ~1.0884)
    // Sine wave: freq = 1 Hz. Period = 44100 samples @ 44.1kHz = 1 sec.
    // Zero crossings @ 44.1kHz: 0, 22050, 44100
    // Zero crossings @ 48kHz: 0, 24000, 48000
    const int inRate = 44100;
    const int outRate = 48000;
    const int inFrames = 88200;  // 2 seconds @ 44.1kHz

    std::vector<float> samples(inFrames);
    {
        SineWaveSignal gen(1.0, inRate);
        std::copy_n(gen.begin(), inFrames, samples.begin());
    }

    QSpan<const float> input(samples.data(), samples.size());
    QByteArray result =
            QAudioHelperInternal::resampleAudioCatmullRom(input, 1, inRate, outRate);

    const qsizetype outFrames = result.size() / qsizetype(sizeof(float));
    QSpan fdata(asSpan<float>(result));

    // Output should have ~96000 frames (88200 * 48000 / 44100)
    QVERIFY(outFrames >= 95700 && outFrames <= 96300);

    // Zero crossings in output @ 48kHz: 0, 24000, 48000, 72000, 96000
    auto expectZeroCrossing = [fdata, outFrames](qsizetype idx, float tolerance = 0.05f) {
        if (idx >= 0 && idx < outFrames) {
            const float val = std::abs(fdata[idx]);
            QVERIFY2(val < tolerance,
                     QStringLiteral("Zero crossing at idx %1: expected ~0, got %2")
                             .arg(idx)
                             .arg(val)
                             .toUtf8()
                             .data());
        }
    };

    expectZeroCrossing(0);          // Start
    expectZeroCrossing(24000);      // π crossing
    expectZeroCrossing(48000);      // 2π crossing
}

// Spot-test: verify that periodic samples map correctly across rates.
// In 48kHz signal, samples at indices 0, 480, 960, ... (every 480 samples) represent
// the same phase in a 1 Hz sine. After resampling 48kHz→44.1kHz, these should map to
// indices 0, 441, 882, ... and have matching values.
void tst_QAudioHelpers::resampleAudioCatmullRom_fractional_periodic_matching()
{
    const int inRate = 48000;
    const int outRate = 44100;
    const int inFrames = 48000;  // 1 second @ 48kHz

    std::vector<float> samples(inFrames);
    {
        SineWaveSignal gen(100.0, inRate);
        std::copy_n(gen.begin(), inFrames, samples.begin());
    }

    QSpan<const float> input(samples.data(), samples.size());
    QByteArray result =
            QAudioHelperInternal::resampleAudioCatmullRom(input, 1, inRate, outRate);

    const qsizetype outFrames = result.size() / qsizetype(sizeof(float));
    QSpan fdata(asSpan<float>(result));

    // 100 Hz @ 44.1kHz has period = 441 samples
    // Spot-test: samples at multiples of 441 in output should be close to phase-matched input
    // E.g., output[0], output[441], output[882] should all be ~sin(0) = 0
    auto spotTestPhase = [fdata, outFrames, &samples](
                                 qsizetype outIdx, float phaseExpected, float tolerance = 0.1f) {
        if (outIdx >= 0 && outIdx < outFrames) {
            // Map output sample index back to input time
            const double outSampleTime = double(outIdx) / 44100.0;
            const double inSampleIdx = outSampleTime * 48000.0;
            const qsizetype inIdx = qsizetype(std::round(inSampleIdx));

            if (inIdx >= 0 && inIdx < qsizetype(samples.size())) {
                const float inVal = samples[inIdx];
                const float outVal = fdata[outIdx];
                // Both should be close to expected phase
                QCOMPARE_FLOAT_NEAR(inVal, phaseExpected, tolerance);
                QCOMPARE_FLOAT_NEAR(outVal, phaseExpected, tolerance);
            }
        }
    };

    // Test a few phase-matched points
    // 100 Hz @ 48kHz: period 480. idx 0 = phase 0 (sin=0), idx 120 = phase π/2 (sin=1), idx 240 = phase π (sin=0), idx 360 = phase 3π/2 (sin=-1)
    // After resample to 44.1kHz: idx 0 still ~phase 0, idx 110 ~phase π/2, idx 220 ~phase π
    spotTestPhase(0, 0.0f);        // phase 0 → sin=0
    spotTestPhase(110, 1.0f, 0.15f);  // phase π/2 → sin=1 (peak)
    spotTestPhase(220, 0.0f);      // phase π → sin=0
}

// ─── CatmullRomInterpolator ─────────────────────────────────────────────

static QByteArray processIncremental(QSpan<const float> input, int nChannels,
                                     int inputRate, int outputRate, qsizetype chunkSize)
{
    using namespace QAudioHelperInternal;
    using ResampleResult = CatmullRomInterpolator::ResampleResult;
    using namespace QtMultimediaPrivate;

    CatmullRomInterpolator interp(nChannels, inputRate, outputRate);

    const qsizetype totalInputFrames = input.size() / nChannels;
    const qsizetype maxOutFrames =
            qsizetype(double(totalInputFrames) * double(outputRate) / double(inputRate) + 4);
    QByteArray outBuf{
        maxOutFrames * nChannels * qsizetype(sizeof(float)),
        Qt::Initialization::Uninitialized,
    };
    QSpan fullOut(reinterpret_cast<float *>(outBuf.data()), maxOutFrames * nChannels);
    qsizetype totalWritten = 0;

    qsizetype inPos = 0;
    while (inPos < totalInputFrames) {
        const qsizetype frames = qMin(chunkSize, totalInputFrames - inPos);
        auto chunk = take(drop(input, inPos * nChannels), frames * nChannels);

        auto outSpan = drop(fullOut, totalWritten * nChannels);
        ResampleResult result = interp.process(chunk, outSpan);

        totalWritten += result.output.size() / nChannels;
        const qsizetype consumed = frames - result.remainingInput.size() / nChannels;
        inPos += consumed;
    }

    // Flush: empty input signals end-of-stream
    ResampleResult result = interp.process({}, drop(fullOut, totalWritten * nChannels));
    totalWritten += result.output.size() / nChannels;

    outBuf.resize(totalWritten * nChannels * qsizetype(sizeof(float)));
    return outBuf;
}

void tst_QAudioHelpers::catmullRomInterpolator_mono_downsample()
{
    const int nChannels = 1;
    const int inRate = 44100;
    const int outRate = 22050;

    std::vector<float> samples(256);
    {
        SineWaveSignal gen(100.0, inRate);
        std::copy_n(gen.begin(), 256, samples.begin());
    }

    QSpan<const float> input(samples.data(), samples.size());

    QByteArray ref = QAudioHelperInternal::resampleAudioCatmullRom(input, nChannels, inRate, outRate);
    QByteArray result = processIncremental(input, nChannels, inRate, outRate, /*chunkSize=*/16);

    const qsizetype refFrames = ref.size() / (nChannels * qsizetype(sizeof(float)));
    const qsizetype resFrames = result.size() / (nChannels * qsizetype(sizeof(float)));

    QVERIFY(resFrames >= refFrames - 1 && resFrames <= refFrames + 1);

    QSpan refData(asSpan<float>(ref));
    QSpan resData(asSpan<float>(result));
    const qsizetype cmpFrames = qMin(refFrames, resFrames);
    for (qsizetype i = 0; i < cmpFrames * nChannels; ++i)
        QCOMPARE_FLOAT_NEAR(resData[i], refData[i], 1e-5f);
}

void tst_QAudioHelpers::catmullRomInterpolator_stereo_upsample()
{
    const int nChannels = 2;
    const int inRate = 22050;
    const int outRate = 44100;

    std::vector<float> samples(128);
    {
        SineWaveGenerator genL(50.0, inRate);
        SineWaveGenerator genR(75.0, inRate);
        for (int i = 0; i < 64; ++i) {
            samples[i * 2 + 0] = genL();
            samples[i * 2 + 1] = genR();
        }
    }

    QSpan<const float> input(samples.data(), samples.size());

    QByteArray ref = QAudioHelperInternal::resampleAudioCatmullRom(input, nChannels, inRate, outRate);
    QByteArray result = processIncremental(input, nChannels, inRate, outRate, /*chunkSize=*/8);

    const qsizetype refFrames = ref.size() / (nChannels * qsizetype(sizeof(float)));
    const qsizetype resFrames = result.size() / (nChannels * qsizetype(sizeof(float)));

    QVERIFY(resFrames >= refFrames - 1 && resFrames <= refFrames + 1);

    QSpan refData(asSpan<float>(ref));
    QSpan resData(asSpan<float>(result));
    const qsizetype cmpFrames = qMin(refFrames, resFrames);
    for (qsizetype i = 0; i < cmpFrames * nChannels; ++i)
        QCOMPARE_FLOAT_NEAR(resData[i], refData[i], 1e-5f);
}

void tst_QAudioHelpers::catmullRomInterpolator_fractional()
{
    const int nChannels = 1;
    const int inRate = 48000;
    const int outRate = 44100;

    const int inFrames = 48000; // 1 second
    std::vector<float> samples(inFrames);
    {
        SineWaveSignal gen(100.0, inRate);
        std::copy_n(gen.begin(), inFrames, samples.begin());
    }

    QSpan<const float> input(samples.data(), samples.size());

    QByteArray ref = QAudioHelperInternal::resampleAudioCatmullRom(input, nChannels, inRate, outRate);
    QByteArray result = processIncremental(input, nChannels, inRate, outRate, /*chunkSize=*/256);

    const qsizetype refFrames = ref.size() / (nChannels * qsizetype(sizeof(float)));
    const qsizetype resFrames = result.size() / (nChannels * qsizetype(sizeof(float)));

    QVERIFY(resFrames >= refFrames - 1 && resFrames <= refFrames + 1);

    QSpan refData(asSpan<float>(ref));
    QSpan resData(asSpan<float>(result));
    const qsizetype cmpFrames = qMin(refFrames, resFrames);
    for (qsizetype i = 0; i < cmpFrames * nChannels; ++i)
        QCOMPARE_FLOAT_NEAR(resData[i], refData[i], 1e-5f);
}

void tst_QAudioHelpers::catmullRomInterpolator_empty()
{
    QAudioHelperInternal::CatmullRomInterpolator interp(1, 44100, 22050);

    float dummy;
    QSpan<float> outSpan(&dummy, 0);

    auto result = interp.process({}, outSpan);
    QCOMPARE_EQ(result.output.size(), qsizetype(0));
    QCOMPARE_EQ(result.remainingInput.size(), qsizetype(0));
}

void tst_QAudioHelpers::catmullRomInterpolator_reset()
{
    const int nChannels = 1;
    const int inRate = 44100;
    const int outRate = 22050;

    std::vector<float> samples(64);
    for (int i = 0; i < 64; ++i)
        samples[i] = float(i) / 64.f;

    QSpan<const float> input(samples.data(), samples.size());

    // Process once
    QByteArray ref = processIncremental(input, nChannels, inRate, outRate, /*chunkSize=*/16);

    // Reset and process again — should produce identical output
    QByteArray result = processIncremental(input, nChannels, inRate, outRate, /*chunkSize=*/16);

    QCOMPARE_EQ(ref.size(), result.size());
    QSpan a(asSpan<float>(ref));
    QSpan b(asSpan<float>(result));

    for (int i = 0; i < a.size(); ++i)
        QCOMPARE_FLOAT_NEAR(a[i], b[i], 1e-6f);
}

void tst_QAudioHelpers::catmullRomInterpolator_chunking_independence()
{
    const int nChannels = 1;
    const int inRate = 48000;
    const int outRate = 44100;

    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    auto nextFloat = [&] {
        return dist(rng);
    };

    const qsizetype totalFrames = 4096;
    std::vector<float> samples(totalFrames * nChannels);
    for (auto &s : samples)
        s = nextFloat();

    QSpan<const float> input(samples.data(), samples.size());

    constexpr std::array<qsizetype, 7> chunkSizes = { 3, 7, 16, 63, 64, 100, 512 };

    // Process with each chunk size and store results
    QByteArray results[chunkSizes.size()];
    for (size_t c = 0; c < chunkSizes.size(); ++c)
        results[c] = processIncremental(input, nChannels, inRate, outRate, chunkSizes[c]);

    // All results must have the same length
    for (size_t c = 1; c < chunkSizes.size(); ++c)
        QCOMPARE_EQ(results[0].size(), results[c].size());

    // All results must be bit-identical
    const qsizetype totalSamples = results[0].size() / qsizetype(sizeof(float));
    for (size_t c = 1; c < chunkSizes.size(); ++c) {
        QSpan a(asSpan<float>(results[0]));
        QSpan b(asSpan<float>(results[c]));

        for (qsizetype i = 0; i < totalSamples; ++i) {
            if (std::abs(a[i] - b[i]) >= 1e-6f) {
                qWarning("  first diff: chunkSize %lld vs %lld at sample %lld: %f vs %f",
                         chunkSizes[0], chunkSizes[c], i, a[i], b[i]);
                break;
            }
        }
        for (qsizetype i = 0; i < totalSamples; ++i)
            QCOMPARE_FLOAT_NEAR(a[i], b[i], 1e-6f);
    }
}

// ─── upmixMonoToStereo ──────────────────────────────────────────────────

void tst_QAudioHelpers::upmixMonoToStereo_data()
{
    using US = QAudioHelperInternal::UpmixScaling;

    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QAudioHelperInternal::UpmixScaling>("scaling");
    QTest::addColumn<QByteArray>("expected");

    auto makeBuf = [](std::initializer_list<float> vals) {
        return QByteArray{ as_bytes(QSpan{ vals }) };
    };

    QTest::newRow("empty") << QByteArray{} << US::Duplicate << QByteArray{};

    QTest::newRow("single,Dup")
            << makeBuf({0.5f}) << US::Duplicate << makeBuf({0.5f, 0.5f});
    QTest::newRow("single,EQ")
            << makeBuf({0.5f}) << US::EqualPower
            << makeBuf({0.35355339f, 0.35355339f});
    QTest::newRow("multi,Dup")
            << makeBuf({1.0f, -0.5f, 0.0f}) << US::Duplicate
            << makeBuf({1.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f});
    QTest::newRow("multi,EQ")
            << makeBuf({1.0f, -0.5f, 0.0f}) << US::EqualPower
            << makeBuf({0.70710678f, 0.70710678f, -0.35355339f, -0.35355339f, 0.0f, 0.0f});
}

void tst_QAudioHelpers::upmixMonoToStereo()
{
    QFETCH(QByteArray, input);
    QFETCH(QAudioHelperInternal::UpmixScaling, scaling);
    QFETCH(QByteArray, expected);

    QSpan<const float> inputSpan;
    if (!input.isEmpty())
        inputSpan = asSpan<float>(input);

    QByteArray result = QAudioHelperInternal::upmixMonoToStereo(inputSpan, scaling);
    QCOMPARE_EQ(result, expected);
}

// ─── downmixStereoToMono ────────────────────────────────────────────────

void tst_QAudioHelpers::downmixStereoToMono_data()
{
    using DS = QAudioHelperInternal::DownmixScaling;

    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QAudioHelperInternal::DownmixScaling>("scaling");
    QTest::addColumn<QByteArray>("expected");

    auto makeBuf = [](std::initializer_list<float> vals) {
        return QByteArray{ as_bytes(QSpan{ vals }) };
    };

    QTest::newRow("empty") << QByteArray{} << DS::Average << QByteArray{};

    QTest::newRow("L=R=0.5,Avg")
            << makeBuf({0.5f, 0.5f}) << DS::Average << makeBuf({0.5f});
    QTest::newRow("L=1,R=-1,Avg")
            << makeBuf({1.0f, -1.0f}) << DS::Average << makeBuf({0.0f});
    QTest::newRow("L=R=0.5,KP")
            << makeBuf({0.5f, 0.5f}) << DS::KeepPower << makeBuf({0.70710678f});
    QTest::newRow("L=1,R=-1,KP")
            << makeBuf({1.0f, -1.0f}) << DS::KeepPower << makeBuf({0.0f});
    QTest::newRow("multi,Avg")
            << makeBuf({0.25f, 0.75f, -0.5f, 0.5f}) << DS::Average
            << makeBuf({0.5f, 0.0f});
    QTest::newRow("multi,KP")
            << makeBuf({0.25f, 0.75f, -0.5f, 0.5f}) << DS::KeepPower
            << makeBuf({0.70710678f, 0.0f});
}

void tst_QAudioHelpers::downmixStereoToMono()
{
    QFETCH(QByteArray, input);
    QFETCH(QAudioHelperInternal::DownmixScaling, scaling);
    QFETCH(QByteArray, expected);

    QSpan<const float> inputSpan;
    if (!input.isEmpty())
        inputSpan = asSpan<float>(input);

    QByteArray result = QAudioHelperInternal::downmixStereoToMono(inputSpan, scaling);
    QCOMPARE_EQ(result, expected);
}

// ─── upmixMonoToStereo (QSpan overload) ─────────────────────────────────

void tst_QAudioHelpers::upmixMonoToStereo_span_data()
{
    upmixMonoToStereo_data();
}

void tst_QAudioHelpers::upmixMonoToStereo_span()
{
    QFETCH(QByteArray, input);
    QFETCH(QAudioHelperInternal::UpmixScaling, scaling);
    QFETCH(QByteArray, expected);

    QSpan<const float> in;
    if (!input.isEmpty())
        in = asSpan<float>(input);

    if (in.empty()) {
        // QSpan overload with empty input: nothing to assert (no-op path)
        return;
    }

    QByteArray out(expected.size(), Qt::Initialization::Uninitialized);
    QAudioHelperInternal::upmixMonoToStereo(
            QSpan<float>(reinterpret_cast<float *>(out.data()),
                         out.size() / qsizetype(sizeof(float))),
            in, scaling);
    QCOMPARE_EQ(out, expected);
}

// ─── downmixStereoToMono (QSpan overload) ──────────────────────────────

void tst_QAudioHelpers::downmixStereoToMono_span_data()
{
    downmixStereoToMono_data();
}

void tst_QAudioHelpers::downmixStereoToMono_span()
{
    QFETCH(QByteArray, input);
    QFETCH(QAudioHelperInternal::DownmixScaling, scaling);
    QFETCH(QByteArray, expected);

    QSpan<const float> in;
    if (!input.isEmpty())
        in = asSpan<float>(input);

    if (in.empty())
        return;

    QByteArray out(expected.size(), Qt::Initialization::Uninitialized);
    QAudioHelperInternal::downmixStereoToMono(
            QSpan<float>(reinterpret_cast<float *>(out.data()),
                         out.size() / qsizetype(sizeof(float))),
            in, scaling);
    QCOMPARE_EQ(out, expected);
}

QTEST_APPLESS_MAIN(tst_QAudioHelpers);

#include "tst_qaudiohelpers.moc"
