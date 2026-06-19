// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qfile.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtMultimedia/qaudiodecoder.h>
#include <QtMultimedia/private/qsamplecache_p.h>

using namespace Qt::Literals;

class tst_QSampleCacheBackend : public QObject
{
    Q_OBJECT

public:
    tst_QSampleCacheBackend(QObject *parent = nullptr) : QObject(parent) { }

private slots:
    void initTestCase();

    void testValidWavLoads();
    void testMP3FallbackViaCache();
    void testMP3LoadSampleViaDecoder();
    void testMP3LoadSampleViaDecoderBuffer();
    void testMP3LoadSampleAsyncViaDecoder();
    void testMP3LoadSampleAsyncViaDecoderEarlyExit();
    void testMP3SpanFails();

private:
    QByteArray mp3Data;
};

void tst_QSampleCacheBackend::initTestCase()
{
    if constexpr (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Android)
        QSKIP("Android cannot read mp3 files");

    QFile file(QFINDTESTDATA("testdata/nokia-tune.mp3"));
    QVERIFY(file.open(QFile::ReadOnly));
    mp3Data = file.readAll();
    QVERIFY(!mp3Data.isEmpty());
}

void tst_QSampleCacheBackend::testValidWavLoads()
{
    QSampleCache cache;

    QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"));
    auto future = cache.requestSampleFuture(url);
    QTRY_VERIFY(future.isFinished());

    SharedSamplePtr sample = future.result();
    QVERIFY(sample);
    QCOMPARE(sample->state(), QSample::Ready);
    QCOMPARE(sample->format().sampleFormat(), QAudioFormat::Float);
}

void tst_QSampleCacheBackend::testMP3FallbackViaCache()
{
    QAudioDecoder probe;
    if (!probe.isSupported())
        QSKIP("Audio decoder not available on this platform");

    QSampleCache cache;

    QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("testdata/nokia-tune.mp3"));
    auto future = cache.requestSampleFuture(url);
    QTRY_VERIFY(future.isFinished());

    SharedSamplePtr sample = future.result();
    // drwav fails, real backend QAudioDecoder handles MP3
    QVERIFY(sample);
    QCOMPARE(sample->state(), QSample::Ready);
    QCOMPARE(sample->format().sampleFormat(), QAudioFormat::Float);
    QVERIFY(sample->format().sampleRate() > 0);
    QVERIFY(sample->format().channelCount() > 0);
    QVERIFY(!sample->data().isEmpty());
}

void tst_QSampleCacheBackend::testMP3LoadSampleViaDecoder()
{
    QAudioDecoder probe;
    if (!probe.isSupported())
        QSKIP("Audio decoder not available on this platform");

    QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("testdata/nokia-tune.mp3"));
    auto result = QSampleCache::loadSampleViaDecoder(url);
    QVERIFY(result.has_value());
    QCOMPARE(result->second.sampleFormat(), QAudioFormat::Float);
    QVERIFY(result->second.sampleRate() > 0);
    QVERIFY(!result->first.isEmpty());
}

void tst_QSampleCacheBackend::testMP3LoadSampleViaDecoderBuffer()
{
    QAudioDecoder probe;
    if (!probe.isSupported())
        QSKIP("Audio decoder not available on this platform");

    auto result = QSampleCache::loadSampleViaDecoder(mp3Data);
    if (!result.has_value())
        QSKIP("device-based decoding not supported by this backend");
    QCOMPARE(result->second.sampleFormat(), QAudioFormat::Float);
    QVERIFY(result->second.sampleRate() > 0);
    QVERIFY(!result->first.isEmpty());
}

void tst_QSampleCacheBackend::testMP3LoadSampleAsyncViaDecoder()
{
    QAudioDecoder probe;
    if (!probe.isSupported())
        QSKIP("Audio decoder not available on this platform");

    auto future = QSampleCache::loadSampleAsyncViaDecoder(mp3Data);
    QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 15s);

    auto result = future.result();
    if (!result.has_value())
        QSKIP("async device-based decoding not supported by this backend");
    QCOMPARE(result->second.sampleFormat(), QAudioFormat::Float);
    QVERIFY(result->second.sampleRate() > 0);
    QVERIFY(!result->first.isEmpty());
}

void tst_QSampleCacheBackend::testMP3LoadSampleAsyncViaDecoderEarlyExit()
{
    QAudioDecoder probe;
    if (!probe.isSupported())
        QSKIP("Audio decoder not available on this platform");

    // Start decoding but drop the future immediately — tests that decoder
    // cleanup (deleteLater, shared_ptr promise) doesn't leak or crash.
    std::ignore = QSampleCache::loadSampleAsyncViaDecoder(mp3Data);
}

void tst_QSampleCacheBackend::testMP3SpanFails()
{
    // drwav cannot parse MP3
    auto result = QSampleCache::loadSample(mp3Data);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error(), QSampleLoadError::FormatError);
}

QTEST_MAIN(tst_QSampleCacheBackend)

#include "tst_qsamplecachebackend.moc"
