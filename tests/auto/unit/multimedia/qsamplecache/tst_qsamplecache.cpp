// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimedia/private/qsamplecache_p.h>

#include <QtTest/qtest.h>
#include <QtCore/qfile.h>
#include <QtCore/qfuturewatcher.h>

#include <qmockintegration.h>

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
    void testLoadSampleViaDecoder_valid();
    void testLoadSampleViaDecoder_notSupported();
    void testLoadSampleViaDecoderBuffer_valid();
    void testLoadSampleViaDecoderBuffer_notSupported();
    void testFallbackToDecoder_nonWav();
    void testForcedDecoderPath();
    void testFallbackToDecoder_notSupported();
    void testMP3FallbackViaDecoder();
    void testMP3FallbackViaDecoderBuffer();
    void testLoadSampleAsyncViaDecoder_valid();
    void testLoadSampleAsyncViaDecoder_notSupported();
    void testForcedDecoderPathAsync();

private:
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

void tst_QSampleCache::testLoadSampleViaDecoder_notSupported()
{
    QMockIntegration::instance()->setFlags(QMockIntegration::NoAudioDecoderInterface);

    QTest::ignoreMessage(QtWarningMsg, "Failed to initialize QAudioDecoder \"No audio decoder\"");
    const QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"));
    auto result = QSampleCache::loadSampleViaDecoder(url);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error(), QSampleLoadError::NotSupported);
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

void tst_QSampleCache::testLoadSampleViaDecoderBuffer_notSupported()
{
    QMockIntegration::instance()->setFlags(QMockIntegration::NoAudioDecoderInterface);

    QFile file(QFINDTESTDATA("testdata/test.wav"));
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray data = file.readAll();

    QTest::ignoreMessage(QtWarningMsg, "Failed to initialize QAudioDecoder \"No audio decoder\"");
    auto result = QSampleCache::loadSampleViaDecoder(data);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error(), QSampleLoadError::NotSupported);
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

    // No sampleSourceType → fallback enabled, but decoder not supported
    QSampleCache cache;

    const QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("testdata/corrupted.wav"));
    QTest::ignoreMessage(QtWarningMsg, "Failed to initialize QAudioDecoder \"No audio decoder\"");
    QTest::ignoreMessage(QtWarningMsg, "Failed to initialize QAudioDecoder \"No audio decoder\"");
    SharedSamplePtr sample = requestSample(cache, url);
    // drwav fails, decoder not supported → null
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

void tst_QSampleCache::testLoadSampleAsyncViaDecoder_notSupported()
{
    QMockIntegration::instance()->setFlags(QMockIntegration::NoAudioDecoderInterface);

    QFile file(QFINDTESTDATA("testdata/test.wav"));
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray data = file.readAll();

    QTest::ignoreMessage(QtWarningMsg, "Failed to initialize QAudioDecoder \"No audio decoder\"");
    auto future = QSampleCache::loadSampleAsyncViaDecoder(data);
    QTRY_VERIFY(future.isFinished());

    auto result = future.result();
    QVERIFY(!result.has_value());
    QCOMPARE(result.error(), QSampleLoadError::NotSupported);
    QVERIFY(QMockIntegration::instance()->lastAudioDecoder() == nullptr);
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
