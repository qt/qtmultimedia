// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qmockintegration.h>

#include <QtTest/qtest.h>
#include <QtMultimedia/private/qsamplecache_p.h>
#include <QtCore/qendian.h>
#include <QtCore/qfile.h>
#include <QtCore/qfuturewatcher.h>

#include <cstdint>

Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN

class tst_QSampleCache : public QObject
{
    Q_OBJECT
public:

public slots:

private slots:
    void initTestCase();
    void cleanup();

    void testCachedSample_data() { generateTestData(); }
    void testCachedSample();

    void testNotCachedSample_data() { generateTestData(); }
    void testNotCachedSample();

    void testInvalidFile_data() { generateTestData(); }
    void testInvalidFile();

    void testIncompatibleFile_data() { generateTestData(); }
    void testIncompatibleFile();

    void testDRwavHeapBufferOverflow_data() { generateTestData(); }
    void testDRwavHeapBufferOverflow();

    void testDRwavIntegerUnderflow_data() { generateTestData(); }
    void testDRwavIntegerUnderflow();

    void testLoadSampleFromSpan_valid();
    void testLoadSampleFromSpan_invalid();
    void testLoadSampleFromSpan_declaredFrameCountMismatch_data();
    void testLoadSampleFromSpan_declaredFrameCountMismatch();
    void testLoadSampleFromSpan_multiChannel();
    void testLoadSampleViaDecoderBuffer_declaredFrameCountMismatch_data()
    {
        testLoadSampleFromSpan_declaredFrameCountMismatch_data();
    }
    void testLoadSampleViaDecoderBuffer_declaredFrameCountMismatch();
    void testLoadSampleViaDecoder_valid();
    void testLoadSampleViaDecoder_fallbackToDrWav();
    void testLoadSampleViaDecoderBuffer_valid();
    void testLoadSampleViaDecoderBuffer_fallbackToDrWav();
    void testFallbackToDecoder_nonWav();
    void testForcedDecoderPath();
    void testFallbackToDecoder_notSupported();
    void testMP3FallbackViaDecoder();
    void testMP3FallbackViaDecoderBuffer();
    void testLoadSampleAsyncViaDecoder_valid();
    void testLoadSampleAsyncViaDecoder_fallbackToDrWav();
    void testForcedDecoderPathAsync();

private:
    template <typename T>
    static void appendLittleEndian(QByteArray &out, T value)
    {
        const T le = qToLittleEndian(value);
        out.append(reinterpret_cast<const char *>(&le), sizeof(le));
    }

    static QByteArray makeRf64Wav(uint64_t declaredFrameCount, uint16_t channelCount,
                                  qsizetype dataBytes)
    {
        const uint16_t bytesPerFrame = channelCount * sizeof(int16_t);

        QByteArray wav;
        wav.append("RF64");
        appendLittleEndian<uint32_t>(wav, 0xFFFFFFFF);
        wav.append("WAVE");

        wav.append("ds64");
        appendLittleEndian<uint32_t>(wav, 28);
        appendLittleEndian<uint64_t>(wav, 0); // RIFF size, ignored
        appendLittleEndian<uint64_t>(wav, uint64_t(dataBytes));
        appendLittleEndian<uint64_t>(wav, declaredFrameCount);
        appendLittleEndian<uint32_t>(wav, 0); // table length

        wav.append("fmt ");
        appendLittleEndian<uint32_t>(wav, 16);
        appendLittleEndian<uint16_t>(wav, 1); // PCM
        appendLittleEndian<uint16_t>(wav, channelCount);
        appendLittleEndian<uint32_t>(wav, 8000);
        appendLittleEndian<uint32_t>(wav, 8000 * bytesPerFrame);
        appendLittleEndian<uint16_t>(wav, bytesPerFrame);
        appendLittleEndian<uint16_t>(wav, 16);

        wav.append("data");
        appendLittleEndian<uint32_t>(wav, 0xFFFFFFFF);
        wav.append(QByteArray(dataBytes, '\x7f'));
        return wav;
    }

    void generateTestData()
    {
        QTest::addColumn<QSampleCache::SampleSourceType>("sampleSourceType");
#ifdef QT_FEATURE_network
        QTest::newRow("NetworkManager") << QSampleCache::SampleSourceType::NetworkManager;
#endif
        QTest::newRow("File") << QSampleCache::SampleSourceType::File;
    }

    SharedSamplePtr requestSample(QSampleCache &cache, const QUrl &url,
                                  std::optional<QSampleCache::SampleSourceType> sourceType = std::nullopt)
    {
        auto future = cache.requestSampleFuture(url, std::nullopt, sourceType);
        QFutureWatcher<SharedSamplePtr> watcher;
        watcher.setFuture(future);

        QEventLoop loop;
        connect(&watcher, &QFutureWatcher<SharedSamplePtr>::finished, &loop, [&] {
            loop.exit(0);
        });
        loop.exec(QEventLoop::EventLoopExec);
        return future.result();
    }
};

void tst_QSampleCache::initTestCase()
{
    // Ensure mock flags start clean
    QMockIntegration::instance()->setFlags({});
}

void tst_QSampleCache::cleanup()
{
    // Reset mock flags after each test
    QMockIntegration::instance()->setFlags({});
}

void tst_QSampleCache::testCachedSample()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;

    SharedSamplePtr sample =
            requestSample(cache, QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")), sampleSourceType);
    QVERIFY(sample);

    SharedSamplePtr sampleCached =
            requestSample(cache, QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")), sampleSourceType);
    QCOMPARE(sample, sampleCached); // sample is cached
    QVERIFY(cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));
}

void tst_QSampleCache::testNotCachedSample()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;

    SharedSamplePtr sample =
            requestSample(cache, QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")), sampleSourceType);
    QVERIFY(sample);
    sample = {};

    QVERIFY(!cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));
}

void tst_QSampleCache::testInvalidFile()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;

    SharedSamplePtr sample = requestSample(cache, QUrl::fromLocalFile("invalid"), sampleSourceType);
    QVERIFY(!sample);
    sample = {};

    QVERIFY(!cache.isCached(QUrl::fromLocalFile("invalid")));
}

void tst_QSampleCache::testIncompatibleFile()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;

    const QUrl corruptedWavUrl = QUrl::fromLocalFile(QFINDTESTDATA("testdata/corrupted.wav"));
    SharedSamplePtr sample = requestSample(cache, corruptedWavUrl, sampleSourceType);
    // sampleSourceType is set → fallback disabled, drwav fails → null
    QVERIFY(!sample);
}

void tst_QSampleCache::testDRwavHeapBufferOverflow()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;

    const QUrl corruptedWavUrl =
            QUrl::fromLocalFile(QFINDTESTDATA("testdata/drwav_heap-buffer-overflow.wav"));
    SharedSamplePtr sample = requestSample(cache, corruptedWavUrl, sampleSourceType);
    QVERIFY(sample); // we can still read it
}

void tst_QSampleCache::testDRwavIntegerUnderflow()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;

    const QUrl corruptedWavUrl =
            QUrl::fromLocalFile(QFINDTESTDATA("testdata/drwav_integer-underflow.wav"));
    SharedSamplePtr sample = requestSample(cache, corruptedWavUrl, sampleSourceType);
    QVERIFY(!sample); // bad file
}

void tst_QSampleCache::testLoadSampleFromSpan_valid()
{
    QFile file(QFINDTESTDATA("testdata/test.wav"));
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray data = file.readAll();
    QVERIFY(!data.isEmpty());

    auto result = QSampleCache::loadSample(data);
    QVERIFY(result.has_value());
    QCOMPARE(result->second.sampleFormat(), QAudioFormat::Float);
    QVERIFY(result->second.sampleRate() > 0);
    QVERIFY(!result->first.isEmpty());
}

void tst_QSampleCache::testLoadSampleFromSpan_invalid()
{
    QByteArray garbage(100, '\xFF');
    auto result = QSampleCache::loadSample(garbage);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error(), QSampleLoadError::FormatError);
}

void tst_QSampleCache::testLoadSampleFromSpan_declaredFrameCountMismatch_data()
{
    QTest::addColumn<uint64_t>("declaredFrameCount");

    // 4 * (2^62 + 256) wraps to 1024 in 64-bit arithmetic
    QTest::newRow("wrapping") << ((uint64_t(1) << 62) + 256);
    QTest::newRow("huge") << (uint64_t(1) << 40);
    QTest::newRow("more-than-data") << uint64_t(1024 * 1024);
}

void tst_QSampleCache::testLoadSampleFromSpan_declaredFrameCountMismatch()
{
    QFETCH(const uint64_t, declaredFrameCount);

    const QByteArray wav = makeRf64Wav(declaredFrameCount, 1, 1024 * 1024);
    auto result = QSampleCache::loadSample(wav);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error(), QSampleLoadError::FormatError);
}

void tst_QSampleCache::testLoadSampleFromSpan_multiChannel()
{
    constexpr uint16_t channelCount = 2;
    constexpr uint64_t frameCount = 3'000'000; // spans multiple decode chunks

    const QByteArray wav =
            makeRf64Wav(frameCount, channelCount, qsizetype(frameCount * channelCount * 2));
    auto result = QSampleCache::loadSample(wav);
    QVERIFY(result.has_value());
    QCOMPARE_EQ(result->second.channelCount(), int(channelCount));
    QCOMPARE_EQ(result->first.size(), qsizetype(frameCount * channelCount * sizeof(float)));
}

void tst_QSampleCache::testLoadSampleViaDecoderBuffer_declaredFrameCountMismatch()
{
    QFETCH(const uint64_t, declaredFrameCount);

    QMockIntegration::instance()->setFlags(QMockIntegration::NoAudioDecoderInterface);

    const QByteArray wav = makeRf64Wav(declaredFrameCount, 1, 1024 * 1024);
    auto result = QSampleCache::loadSampleViaDecoder(wav);
    QVERIFY(!result.has_value());
}

void tst_QSampleCache::testLoadSampleViaDecoder_valid()
{
    const QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"));
    auto result = QSampleCache::loadSampleViaDecoder(url);
    QVERIFY(result.has_value());
    QCOMPARE(result->second.sampleFormat(), QAudioFormat::Float);
    QVERIFY(!result->first.isEmpty());
    QVERIFY(result->second.sampleRate() > 0);
    QVERIFY(result->second.channelCount() > 0);
}

void tst_QSampleCache::testLoadSampleViaDecoder_fallbackToDrWav()
{
    QMockIntegration::instance()->setFlags(QMockIntegration::NoAudioDecoderInterface);

    const QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("testdata/nokia-tune.mp3"));
    auto result = QSampleCache::loadSampleViaDecoder(url);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error(), QSampleLoadError::DecoderError);
}

void tst_QSampleCache::testLoadSampleViaDecoderBuffer_valid()
{
    QFile file(QFINDTESTDATA("testdata/test.wav"));
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray data = file.readAll();
    QVERIFY(!data.isEmpty());

    auto result = QSampleCache::loadSampleViaDecoder(data);
    QVERIFY(result.has_value());
    QCOMPARE(result->second.sampleFormat(), QAudioFormat::Float);
    QVERIFY(!result->first.isEmpty());
    QVERIFY(result->second.sampleRate() > 0);
    QVERIFY(result->second.channelCount() > 0);
}

void tst_QSampleCache::testLoadSampleViaDecoderBuffer_fallbackToDrWav()
{
    QMockIntegration::instance()->setFlags(QMockIntegration::NoAudioDecoderInterface);

    QFile file(QFINDTESTDATA("testdata/nokia-tune.mp3"));
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray data = file.readAll();

    auto result = QSampleCache::loadSampleViaDecoder(data);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error(), QSampleLoadError::DecoderError);
}

void tst_QSampleCache::testFallbackToDecoder_nonWav()
{
    // No sampleSourceType → fallback enabled
    QSampleCache cache;

    const QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("testdata/corrupted.wav"));
    SharedSamplePtr sample = requestSample(cache, url);
    // drwav fails, mock decoder succeeds → sample ready
    QVERIFY(sample);
    QCOMPARE(sample->state(), QSample::Ready);
}

void tst_QSampleCache::testForcedDecoderPath()
{
    QSampleCache cache;

    const QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"));
    SharedSamplePtr sample = requestSample(cache, url, QSampleCache::SampleSourceType::AudioDecoder);
    // AudioDecoder skips drwav, uses mock decoder directly
    QVERIFY(sample);
    QCOMPARE(sample->state(), QSample::Ready);
    QCOMPARE(sample->format().sampleFormat(), QAudioFormat::Float);
}

void tst_QSampleCache::testFallbackToDecoder_notSupported()
{
    QMockIntegration::instance()->setFlags(QMockIntegration::NoAudioDecoderInterface);

    // No sampleSourceType → fallback enabled; drwav fallback via QAudioDecoder
    // still fails because the file itself is corrupted, not because a
    // platform decoder is unavailable.
    QSampleCache cache;

    const QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("testdata/corrupted.wav"));
    SharedSamplePtr sample = requestSample(cache, url);
    // drwav fails, decoder fallback also fails on the corrupted data → null
    QVERIFY(!sample);
}

void tst_QSampleCache::testMP3FallbackViaDecoder()
{
    // No sampleSourceType → fallback enabled
    QSampleCache cache;

    const QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("testdata/nokia-tune.mp3"));
    SharedSamplePtr sample = requestSample(cache, url);
    // drwav fails on MP3, mock decoder succeeds → sample ready
    QVERIFY(sample);
    QCOMPARE(sample->state(), QSample::Ready);
    QCOMPARE(sample->format().sampleFormat(), QAudioFormat::Float);
    // Verify decoder was actually invoked via the mock
    QVERIFY(QMockIntegration::instance()->lastAudioDecoder() != nullptr);
}

void tst_QSampleCache::testMP3FallbackViaDecoderBuffer()
{
    QFile file(QFINDTESTDATA("testdata/nokia-tune.mp3"));
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray data = file.readAll();
    QVERIFY(!data.isEmpty());

    auto result = QSampleCache::loadSampleViaDecoder(data);
    // drwav fails, mock decoder succeeds via QBuffer
    QVERIFY(result.has_value());
    QCOMPARE(result->second.sampleFormat(), QAudioFormat::Float);
    QVERIFY(!result->first.isEmpty());
}

void tst_QSampleCache::testLoadSampleAsyncViaDecoder_valid()
{
    QFile file(QFINDTESTDATA("testdata/test.wav"));
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray data = file.readAll();
    QVERIFY(!data.isEmpty());

    auto future = QSampleCache::loadSampleAsyncViaDecoder(data);
    QTRY_VERIFY(future.isFinished());

    auto result = future.result();
    QVERIFY(result.has_value());
    QCOMPARE(result->second.sampleFormat(), QAudioFormat::Float);
    QVERIFY(!result->first.isEmpty());
    QVERIFY(result->second.sampleRate() > 0);
    QVERIFY(result->second.channelCount() > 0);
    QVERIFY(QMockIntegration::instance()->lastAudioDecoder() != nullptr);
}

void tst_QSampleCache::testLoadSampleAsyncViaDecoder_fallbackToDrWav()
{
    QMockIntegration::instance()->setFlags(QMockIntegration::NoAudioDecoderInterface);

    QFile file(QFINDTESTDATA("testdata/nokia-tune.mp3"));
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray data = file.readAll();

    auto future = QSampleCache::loadSampleAsyncViaDecoder(data);
    QTRY_VERIFY(future.isFinished());

    auto result = future.result();
    QVERIFY(!result.has_value());
    QCOMPARE(result.error(), QSampleLoadError::DecoderError);
}

void tst_QSampleCache::testForcedDecoderPathAsync()
{
    QSampleCache cache;

    const QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"));
    SharedSamplePtr sample = requestSample(cache, url, QSampleCache::SampleSourceType::AudioDecoder);
    QVERIFY(sample);
    QCOMPARE(sample->state(), QSample::Ready);
    QCOMPARE(sample->format().sampleFormat(), QAudioFormat::Float);
}

QTEST_GUILESS_MAIN(tst_QSampleCache)

#include "tst_qsamplecache.moc"
