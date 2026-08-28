// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qdebug.h>

#include <QtMultimedia/qaudiodecoder.h>

#include <QtMultimediaTestLib/private/mediabackendutils_p.h>
#include <QtMultimediaTestLib/private/qintegrationtestbase_p.h>
#include <QtMultimediaTestLib/private/mediafileselector_p.h>

#include <QtTest/qtest.h>

#include <cmath>
#include <optional>
#include <vector>

#include <private/mediafileselector_p.h>
#include <private/mediabackendutils_p.h>

namespace {

constexpr char TEST_FILE_NAME[] = "testdata/test.wav";
constexpr char TEST_UNSUPPORTED_FILE_NAME[] = "testdata/test-unsupported.avi";
constexpr char TEST_CORRUPTED_FILE_NAME[] = "testdata/test-corrupted.wav";
constexpr char TEST_INVALID_SOURCE[] = "invalid";
constexpr char TEST_NO_AUDIO_TRACK[] = "testdata/test-no-audio-track.mp4";
constexpr char TEST_24BIT_FILE_NAME[] = "testdata/test-24bit.wav";
constexpr char TEST_FLOAT32_FILE_NAME[] = "testdata/test-float32.wav";

constexpr int testFileSampleCount = 44094;
constexpr int testFileSampleRate = 44100;

constexpr qint64 testFileDurationUs = [] {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    auto duration = nanoseconds(1s) * testFileSampleCount / testFileSampleRate;
    return round<microseconds>(duration).count();
}();

using namespace std::chrono_literals;

std::vector<qint16> decodeToInt16(const QUrl &source,
                                  const std::optional<QAudioFormat> &desiredFormat = std::nullopt)
{
    QAudioDecoder d;
    d.setSource(source);
    if (desiredFormat && desiredFormat->isValid())
        d.setAudioFormat(*desiredFormat);

    std::vector<qint16> out;

    QObject::connect(&d, &QAudioDecoder::bufferReady, &d, [&] {
        for (;;) {
            auto buffer = d.read();
            if (!buffer.isValid())
                return;

            const QAudioFormat fmt = buffer.format();
            const char *data = buffer.constData<const char>();
            const qsizetype samples = buffer.sampleCount();
            const int bps = fmt.bytesPerSample();

            for (qsizetype i = 0; i < samples; ++i) {
                const void *samplePtr = data + i * bps;
                float normalized = fmt.normalizedSampleValue(samplePtr);
                long v = std::lround(normalized * 32767.0f);
                out.push_back(static_cast<qint16>(qBound(-32768l, v, 32767l)));
            }
        }
    });

    d.start();

    QSignalSpy finishedSpy(&d, &QAudioDecoder::finished);
    finishedSpy.wait(10s);

    return out;
}

void compareMonoReferenceToInterleaved(QSpan<const qint16> monoRef, QSpan<const qint16> other,
                                       int otherChannels, float otherNormalization = 1.0f)
{
    const qsizetype frames = monoRef.size();
    QCOMPARE(qsizetype(other.size()), frames * qsizetype(otherChannels));

    for (qsizetype f = 0; f < frames; ++f) {
        qint16 ref = monoRef[f];
        qint16 otherLeft = other[f * otherChannels];
        int otherNorm = int(std::lround(otherLeft * otherNormalization));
        otherNorm = int(qBound(-32768l, long(otherNorm), 32767l));
        int diff = std::abs(int(ref) - otherNorm);
        if (diff > 6) { // allow difference due to normalization and/or sample format conversion
            QString msg = QString::fromUtf8("Sample mismatch at frame %1: ref=%2 otherLeft=%3 norm=%4 diff=%5")
                                  .arg(qlonglong(f))
                                  .arg(ref)
                                  .arg(otherLeft)
                                  .arg(otherNorm)
                                  .arg(diff);
            QFAIL(qPrintable(msg));
        }
    }
}

} // namespace

/*
 This is the backend conformance test.

 Since it relies on platform media framework
 it may be less stable.
*/

class tst_QAudioDecoderBackend : public QIntegrationTestBase
{
    Q_OBJECT
public slots:
    void init();
    void cleanup();
    void initTestCase();

private slots:
    void testMediaFilesAreSupported();
    void directBruteForceReading();
    void indirectReadingByBufferReadySignal();
    void indirectReadingByBufferAvailableSignal();
    void stopOnBufferReady();
    void restartOnBufferReady();
    void restartOnFinish();
    void fileTest();
    void qrcFileTest();
    void unsupportedFileTest();
    void corruptedFileTest();
    void invalidSource();
    void invalidQrcSourceTest();
    void deviceTest();
    void play_emitsFormatError_whenMediaHasNoAudioTrack();
    void readIntoDifferentSampleFormat();
    void readIntoDifferentChannelCount();
    void read24bitFile();
    void readFloat32File();
    void readMp3File();
    void readOggFile();
    void readAacFile();
    void readFlacFile();

private:
    QUrl testFileUrl(const QString filePath);
    void checkNoMoreChanges(QAudioDecoder &decoder);
#ifdef Q_OS_ANDROID
    QTemporaryFile *temporaryFile = nullptr;
#endif

    MediaFileSelector m_mediaSelector;
    MaybeUrl m_wavFile = MaybeUrl{ q23::unexpect };
};

void tst_QAudioDecoderBackend::init()
{
}

void tst_QAudioDecoderBackend::initTestCase()
{
    if (!initIntegrationTestCase())
        return;

    QAudioDecoder d;
    if (!d.isSupported())
        QSKIP("Audio decoder service is not available");

    m_wavFile = m_mediaSelector.select(QFINDTESTDATA(TEST_FILE_NAME));
}

void tst_QAudioDecoderBackend::cleanup()
{
#ifdef Q_OS_ANDROID
    if (temporaryFile) {
        delete temporaryFile;
        temporaryFile = nullptr;
    }
#endif
}

QUrl tst_QAudioDecoderBackend::testFileUrl(const QString filePath)
{
    QUrl url;
#ifndef Q_OS_ANDROID
    QFileInfo fileInfo(QFINDTESTDATA(filePath));
    url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
#else
    QFile file(":/" + filePath);
    if (temporaryFile) {
        delete temporaryFile;
        temporaryFile = nullptr;
    }
    if (file.open(QIODevice::ReadOnly)) {
        temporaryFile = QTemporaryFile::createNativeFile(file);
        url = QUrl(temporaryFile->fileName());
    }
#endif
    return url;
}

void tst_QAudioDecoderBackend::checkNoMoreChanges(QAudioDecoder &decoder)
{
    QSignalSpy finishedSpy(&decoder, &QAudioDecoder::finished);
    QSignalSpy bufferReadySpy(&decoder, &QAudioDecoder::bufferReady);
    QSignalSpy bufferAvailableSpy(&decoder, &QAudioDecoder::bufferAvailableChanged);

    QTest::qWait(50); // wait a bit to check nothing happened after finish

    QCOMPARE(finishedSpy.size(), 0);
    QCOMPARE(bufferReadySpy.size(), 0);
    QCOMPARE(bufferAvailableSpy.size(), 0);
}

void tst_QAudioDecoderBackend::testMediaFilesAreSupported()
{
    QCOMPARE(m_mediaSelector.dumpErrors(), "");
}

void tst_QAudioDecoderBackend::directBruteForceReading()
{
    CHECK_SELECTED_URL(m_wavFile);

    QAudioDecoder decoder;
    if (decoder.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    int sampleCount = 0;
    QSignalSpy decodingSpy(&decoder, &QAudioDecoder::isDecodingChanged);

    decoder.setSource(*m_wavFile);
    QVERIFY(!decoder.isDecoding());
    QVERIFY(!decoder.bufferAvailable());

    decoder.start();
    QTRY_VERIFY(decodingSpy.size() >= 1); // Wait until decoding starts

    auto waitAndCheck = [](auto &&predicate) { QVERIFY(QTest::qWaitFor(predicate)); };

    auto waitForBufferAvailable = [&]() {
        waitAndCheck([&]() { return !decoder.isDecoding() || decoder.bufferAvailable(); });

        return decoder.bufferAvailable();
    };

    while (waitForBufferAvailable()) {
        auto buffer = decoder.read();
        QVERIFY(buffer.isValid());

        sampleCount += buffer.sampleCount();
    }

    checkNoMoreChanges(decoder);

    QCOMPARE(sampleCount, testFileSampleCount);
}

void tst_QAudioDecoderBackend::indirectReadingByBufferReadySignal()
{
    CHECK_SELECTED_URL(m_wavFile);

    QAudioDecoder decoder;
    if (decoder.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    int sampleCount = 0;

    connect(&decoder, &QAudioDecoder::bufferReady, this, [&]() {
        QVERIFY(decoder.bufferAvailable());

        auto buffer = decoder.read();
        QVERIFY(buffer.isValid());
        QVERIFY(!decoder.bufferAvailable());

        sampleCount += buffer.sampleCount();
    });

    QSignalSpy decodingSpy(&decoder, &QAudioDecoder::isDecodingChanged);
    QSignalSpy finishSpy(&decoder, &QAudioDecoder::finished);

    decoder.setSource(*m_wavFile);
    QVERIFY(!decoder.isDecoding());
    QVERIFY(!decoder.bufferAvailable());

    decoder.start();

    QTRY_VERIFY_WITH_TIMEOUT(decodingSpy.size() >= 1,
                             60s); // High timeout because decoding can take a long time

    QTRY_VERIFY_WITH_TIMEOUT(finishSpy.size() == 1,
                             60s); // High timeout because decoding can take a long time

    QVERIFY(!decoder.isDecoding());

    checkNoMoreChanges(decoder);

    QCOMPARE(sampleCount, testFileSampleCount);
    QCOMPARE(finishSpy.size(), 1);
}

void tst_QAudioDecoderBackend::indirectReadingByBufferAvailableSignal() {
    CHECK_SELECTED_URL(m_wavFile);

    QAudioDecoder decoder;
    if (decoder.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    int sampleCount = 0;

    connect(&decoder, &QAudioDecoder::bufferAvailableChanged, this, [&](bool available) {
        QCOMPARE(decoder.bufferAvailable(), available);

        if (!available)
            return;

        while (decoder.bufferAvailable()) {
            auto buffer = decoder.read();
            QVERIFY(buffer.isValid());

            sampleCount += buffer.sampleCount();
        }
    });

    QSignalSpy decodingSpy(&decoder, &QAudioDecoder::isDecodingChanged);
    QSignalSpy finishSpy(&decoder, &QAudioDecoder::finished);

    decoder.setSource(*m_wavFile);
    QVERIFY(!decoder.isDecoding());
    QVERIFY(!decoder.bufferAvailable());

    decoder.start();
    QTRY_VERIFY(decodingSpy.size() >= 1);

    QTRY_VERIFY(finishSpy.size() == 1);
    QVERIFY(!decoder.isDecoding());

    checkNoMoreChanges(decoder);

    QCOMPARE(sampleCount, testFileSampleCount);
    QCOMPARE(finishSpy.size(), 1);
}

void tst_QAudioDecoderBackend::stopOnBufferReady()
{
    CHECK_SELECTED_URL(m_wavFile);

    QAudioDecoder decoder;
    if (decoder.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    connect(&decoder, &QAudioDecoder::bufferReady, this, [&]() {
        decoder.read(); // run next reading
        decoder.stop();
    });

    QSignalSpy finishSpy(&decoder, &QAudioDecoder::finished);
    QSignalSpy bufferReadySpy(&decoder, &QAudioDecoder::bufferReady);

    decoder.setSource(*m_wavFile);
    decoder.start();

    bufferReadySpy.wait();
    QVERIFY(!decoder.isDecoding());

    checkNoMoreChanges(decoder);

    QCOMPARE(bufferReadySpy.size(), 1);
}

void tst_QAudioDecoderBackend::restartOnBufferReady()
{
    QSKIP_GSTREAMER("QTBUG-124005: failures on gstreamer");

    CHECK_SELECTED_URL(m_wavFile);

    QAudioDecoder decoder;
    if (decoder.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    int sampleCount = 0;

    std::once_flag restartOnce;
    connect(&decoder, &QAudioDecoder::bufferReady, this, [&]() {
        QVERIFY(decoder.bufferAvailable());

        auto buffer = decoder.read();
        QVERIFY(buffer.isValid());
        QVERIFY(!decoder.bufferAvailable());

        sampleCount += buffer.sampleCount();

        std::call_once(restartOnce, [&]() {
            sampleCount = 0;
            decoder.stop();
            decoder.start();
        });
    });

    QSignalSpy finishSpy(&decoder, &QAudioDecoder::finished);

    decoder.setSource(*m_wavFile);
    decoder.start();

    QTRY_VERIFY2(finishSpy.size() == 2, "Wait for signals after restart and after finishing");
    QVERIFY(!decoder.isDecoding());

    checkNoMoreChanges(decoder);

    QCOMPARE(sampleCount, testFileSampleCount);
}

void tst_QAudioDecoderBackend::restartOnFinish()
{
    CHECK_SELECTED_URL(m_wavFile);

    QAudioDecoder decoder;
    if (decoder.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    int sampleCount = 0;

    connect(&decoder, &QAudioDecoder::bufferReady, this, [&]() {
        auto buffer = decoder.read();
        QVERIFY(buffer.isValid());

        sampleCount += buffer.sampleCount();
    });

    QSignalSpy finishSpy(&decoder, &QAudioDecoder::finished);

    std::once_flag restartOnce;
    connect(&decoder, &QAudioDecoder::finished, this, [&]() {
        QVERIFY(!decoder.bufferAvailable());
        QVERIFY(!decoder.isDecoding());

        std::call_once(restartOnce, [&]() {
            sampleCount = 0;
            decoder.start();
        });
    });

    decoder.setSource(*m_wavFile);
    decoder.start();

    QTRY_VERIFY(finishSpy.size() == 2);

    QVERIFY(!decoder.isDecoding());

    checkNoMoreChanges(decoder);
    QCOMPARE(sampleCount, testFileSampleCount);
}

void tst_QAudioDecoderBackend::fileTest()
{
    CHECK_SELECTED_URL(m_wavFile);

    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");
    QAudioBuffer buffer;
    quint64 duration = 0;
    int byteCount = 0;
    int sampleCount = 0;

    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);
    QCOMPARE(d.source(), QUrl{});
    QVERIFY(d.audioFormat() == QAudioFormat());

    // Test local file

    d.setSource(*m_wavFile);
    QVERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.source(), *m_wavFile);

    QSignalSpy readySpy(&d, &QAudioDecoder::bufferReady);
    QSignalSpy bufferChangedSpy(&d, &QAudioDecoder::bufferAvailableChanged);
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy isDecodingSpy(&d, &QAudioDecoder::isDecodingChanged);
    QSignalSpy durationSpy(&d, &QAudioDecoder::durationChanged);
    QSignalSpy finishedSpy(&d, &QAudioDecoder::finished);
    QSignalSpy positionSpy(&d, &QAudioDecoder::positionChanged);

    d.start();

    QTRY_VERIFY(!isDecodingSpy.isEmpty());
    QTRY_VERIFY(!readySpy.isEmpty());
    QTRY_VERIFY(!bufferChangedSpy.isEmpty());
    QVERIFY(d.bufferAvailable());
    QTRY_VERIFY(!durationSpy.isEmpty());

    QVERIFY(qAbs(durationSpy.front().front().value<qint64>() - 1000) < 20);
    if (finishedSpy.empty())
        QVERIFY(qAbs(d.duration() - 1000) < 20);
    else
        QCOMPARE(d.duration(), -1);

    buffer = d.read();
    QVERIFY(buffer.isValid());

    // Test file is 44.1K 16bit mono, 44094 samples
    QCOMPARE(buffer.format().channelCount(), 1);
    QCOMPARE(buffer.format().sampleRate(), testFileSampleRate);
    QCOMPARE(buffer.format().sampleFormat(), QAudioFormat::Int16);
    QCOMPARE(buffer.byteCount(), buffer.sampleCount() * 2); // 16bit mono

    // The decoder should still have no format set
    QVERIFY(d.audioFormat() == QAudioFormat());
    QVERIFY(errorSpy.isEmpty());

    duration += buffer.duration();
    sampleCount += buffer.sampleCount();
    byteCount += buffer.byteCount();

    // Now drain the decoder
    if (sampleCount < testFileSampleCount) {
        QTRY_COMPARE(d.bufferAvailable(), true);
    }

    auto durationToMs = [](uint64_t dur) {
        if (isGStreamerPlatform())
            return std::round(dur / 1000.0);
        else
            return dur / 1000.0;
    };

    while (d.bufferAvailable()) {
        buffer = d.read();
        QVERIFY(buffer.isValid());
        QTRY_VERIFY(!positionSpy.isEmpty());
        QCOMPARE(positionSpy.takeLast().at(0).toLongLong(), qint64(durationToMs(duration)));

        duration += buffer.duration();
        sampleCount += buffer.sampleCount();
        byteCount += buffer.byteCount();

        if (sampleCount < testFileSampleCount) {
            QTRY_COMPARE(d.bufferAvailable(), true);
        }
    }

    // Make sure the duration is roughly correct (+/- 20ms)
    QCOMPARE(sampleCount, testFileSampleCount);
    QCOMPARE(byteCount, testFileSampleCount * 2);
    QVERIFY(qAbs(qint64(duration) - 1000000) < 20000);
    QVERIFY(qAbs((d.position() + (buffer.duration() / 1000)) - 1000) < 20);
    QTRY_COMPARE(finishedSpy.size(), 1);
    QVERIFY(!d.bufferAvailable());
    QTRY_VERIFY(!d.isDecoding());

    d.stop();
    QTRY_VERIFY(!d.isDecoding());
    QTRY_COMPARE(durationSpy.size(), 2);
    QCOMPARE(d.duration(), qint64(-1));
    QVERIFY(!d.bufferAvailable());
    readySpy.clear();
    bufferChangedSpy.clear();
    isDecodingSpy.clear();
    durationSpy.clear();
    finishedSpy.clear();
    positionSpy.clear();

#ifdef Q_OS_ANDROID
    QSKIP("Setting a desired audio format is not yet supported on Android");
#endif
    // change output audio format
    QAudioFormat format;
    format.setChannelCount(2);
    format.setSampleRate(11050);
    format.setSampleFormat(QAudioFormat::UInt8);

    d.setAudioFormat(format);

    // We expect 1 second still, at 11050 * 2 samples == 22k samples.
    // (at 1 byte/sample -> 22kb)

    // Make sure it stuck
    QVERIFY(d.audioFormat() == format);

    duration = 0;
    sampleCount = 0;
    byteCount = 0;

    d.start();
    QTRY_VERIFY(!isDecodingSpy.isEmpty());
    QTRY_VERIFY(!readySpy.isEmpty());
    QTRY_VERIFY(!bufferChangedSpy.isEmpty());
    QVERIFY(d.bufferAvailable());
    QTRY_VERIFY(!durationSpy.isEmpty());
    QVERIFY(qAbs(durationSpy.front().front().value<qint64>() - 1000) < 20);
    if (finishedSpy.empty())
        QVERIFY(qAbs(d.duration() - 1000) < 20);
    else
        QCOMPARE(d.duration(), -1);

    buffer = d.read();
    QVERIFY(buffer.isValid());
    // See if we got the right format
    QVERIFY(buffer.format() == format);

    // The decoder should still have the same format
    QVERIFY(d.audioFormat() == format);

    QVERIFY(errorSpy.isEmpty());

    duration += buffer.duration();
    sampleCount += buffer.sampleCount();
    byteCount += buffer.byteCount();

    while (finishedSpy.isEmpty() || d.bufferAvailable()) {
        if (!d.bufferAvailable()) {
            QTest::qWait(std::chrono::milliseconds(10));
            continue;
        }

        buffer = d.read();
        QVERIFY(buffer.isValid());
        QTRY_VERIFY(!positionSpy.isEmpty());
        QCOMPARE(positionSpy.takeLast().at(0).toLongLong(), qlonglong(durationToMs(duration)));
        QCOMPARE_LT(d.position() - durationToMs(duration), 20u);

        duration += buffer.duration();
        sampleCount += buffer.sampleCount();
        byteCount += buffer.byteCount();
    }

    // Resampling might end up with fewer or more samples
    // so be a bit sloppy
    QCOMPARE_LT(qAbs(sampleCount - 22047), 100);
    QCOMPARE_LT(qAbs(byteCount - 22047), 100);
    QCOMPARE_LT(qAbs(qint64(duration) - testFileDurationUs), 20000);
    QCOMPARE_LT(qAbs((d.position() + (buffer.duration() / 1000)) - 1000), 20);
    QVERIFY(!d.bufferAvailable());
    QVERIFY(!d.isDecoding());

    d.stop();
    QTRY_VERIFY(!d.isDecoding());
    QTRY_COMPARE(durationSpy.size(), 2);
    QCOMPARE(d.duration(), qint64(-1));
    QVERIFY(!d.bufferAvailable());
}

void tst_QAudioDecoderBackend::qrcFileTest()
{
    CHECK_SELECTED_URL(m_wavFile);

    QAudioDecoder decoder;
    if (decoder.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    const QUrl qrcUrl(QStringLiteral("qrc:/testdata/test.wav"));

    int sampleCount = 0;
    QSignalSpy decodingSpy(&decoder, &QAudioDecoder::isDecodingChanged);
    QSignalSpy errorSpy(&decoder, SIGNAL(error(QAudioDecoder::Error)));

    decoder.setSource(qrcUrl);
    QCOMPARE(decoder.source(), qrcUrl);
    QVERIFY(!decoder.isDecoding());
    QVERIFY(!decoder.bufferAvailable());

    decoder.start();
    QTRY_VERIFY(decodingSpy.size() >= 1); // Wait until decoding starts

    auto waitAndCheck = [](auto &&predicate) { QVERIFY(QTest::qWaitFor(predicate)); };

    auto waitForBufferAvailable = [&]() {
        waitAndCheck([&]() { return !decoder.isDecoding() || decoder.bufferAvailable(); });

        return decoder.bufferAvailable();
    };

    while (waitForBufferAvailable()) {
        auto buffer = decoder.read();
        QVERIFY(buffer.isValid());

        sampleCount += buffer.sampleCount();
    }

    checkNoMoreChanges(decoder);

    QVERIFY(errorSpy.isEmpty());
    QCOMPARE(sampleCount, testFileSampleCount);
}

/*
 The avi file has an audio stream not supported by any codec.
*/
void tst_QAudioDecoderBackend::unsupportedFileTest()
{
    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");
    QAudioBuffer buffer;

    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);
    QCOMPARE(d.source(), QUrl{});
    QVERIFY(d.audioFormat() == QAudioFormat());

    // Test local file
    QUrl url = testFileUrl(TEST_UNSUPPORTED_FILE_NAME);
    d.setSource(url);
    QVERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.source(), url);

    QSignalSpy readySpy(&d, &QAudioDecoder::bufferReady);
    QSignalSpy bufferChangedSpy(&d, &QAudioDecoder::bufferAvailableChanged);
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy isDecodingSpy(&d, &QAudioDecoder::isDecodingChanged);
    QSignalSpy durationSpy(&d, &QAudioDecoder::durationChanged);
    QSignalSpy finishedSpy(&d, &QAudioDecoder::finished);
    QSignalSpy positionSpy(&d, &QAudioDecoder::positionChanged);

    d.start();
    QTRY_VERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.audioFormat(), QAudioFormat());
    QCOMPARE(d.duration(), qint64(-1));
    QCOMPARE(d.position(), qint64(-1));

    // Check the error code.
    QTRY_VERIFY(!errorSpy.isEmpty());

    // Have to use qvariant_cast, toInt will return 0 because unrecognized type;
    QAudioDecoder::Error errorCode = qvariant_cast<QAudioDecoder::Error>(errorSpy.takeLast().at(0));
    QCOMPARE(errorCode, QAudioDecoder::FormatError);
    QCOMPARE(d.error(), QAudioDecoder::FormatError);

    // Check all other spies.
    QVERIFY(readySpy.isEmpty());
    QVERIFY(bufferChangedSpy.isEmpty());
    QVERIFY(!d.isDecoding());
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(positionSpy.isEmpty());
    // Either reject the file directly, or set the duration to 5secs on setUrl() and back to -1 on start()
    QVERIFY(durationSpy.isEmpty() || durationSpy.size() == 2);

    errorSpy.clear();

    // Try read even if the file is not supported to test robustness.
    buffer = d.read();
    QTRY_VERIFY(!d.isDecoding());
    QVERIFY(!buffer.isValid());
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.position(), qint64(-1));

    QVERIFY(errorSpy.isEmpty());
    QVERIFY(readySpy.isEmpty());
    QVERIFY(bufferChangedSpy.isEmpty());
    QVERIFY(!d.isDecoding());
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(positionSpy.isEmpty());
    QVERIFY(durationSpy.isEmpty() || durationSpy.size() == 2);


    d.stop();
    QTRY_VERIFY(!d.isDecoding());
    QCOMPARE(d.duration(), qint64(-1));
    QVERIFY(!d.bufferAvailable());
}

/*
 The corrupted file is generated by copying a few random numbers
 from /dev/random on a linux machine.
*/
void tst_QAudioDecoderBackend::corruptedFileTest()
{
    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");
    QAudioBuffer buffer;

    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);
    QCOMPARE(d.source(), QUrl());
    QVERIFY(d.audioFormat() == QAudioFormat());

    // Test local file
    QUrl url = testFileUrl(TEST_CORRUPTED_FILE_NAME);
    d.setSource(url);
    QVERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.source(), url);

    QSignalSpy readySpy(&d, &QAudioDecoder::bufferReady);
    QSignalSpy bufferChangedSpy(&d, &QAudioDecoder::bufferAvailableChanged);
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy durationSpy(&d, &QAudioDecoder::durationChanged);
    QSignalSpy finishedSpy(&d, &QAudioDecoder::finished);
    QSignalSpy positionSpy(&d, &QAudioDecoder::positionChanged);

    d.start();
    QTRY_VERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.audioFormat(), QAudioFormat());
    QCOMPARE(d.duration(), qint64(-1));
    QCOMPARE(d.position(), qint64(-1));

    // Check the error code.
    QTRY_VERIFY(!errorSpy.isEmpty());

    // Have to use qvariant_cast, toInt will return 0 because unrecognized type;
    QAudioDecoder::Error errorCode = qvariant_cast<QAudioDecoder::Error>(errorSpy.takeLast().at(0));
    QCOMPARE(errorCode, QAudioDecoder::FormatError);
    QCOMPARE(d.error(), QAudioDecoder::FormatError);

    // Check all other spies.
    QVERIFY(readySpy.isEmpty());
    QVERIFY(bufferChangedSpy.isEmpty());
    QVERIFY(!d.isDecoding());
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(positionSpy.isEmpty());
    QVERIFY(durationSpy.isEmpty());

    errorSpy.clear();

    // Try read even if the file is corrupted to test the robustness.
    buffer = d.read();
    QTRY_VERIFY(!d.isDecoding());
    QVERIFY(!buffer.isValid());
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.position(), qint64(-1));

    QVERIFY(errorSpy.isEmpty());
    QVERIFY(readySpy.isEmpty());
    QVERIFY(bufferChangedSpy.isEmpty());
    QVERIFY(!d.isDecoding());
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(positionSpy.isEmpty());
    QVERIFY(durationSpy.isEmpty());

    d.stop();
    QTRY_VERIFY(!d.isDecoding());
    QCOMPARE(d.duration(), qint64(-1));
    QVERIFY(!d.bufferAvailable());
}

void tst_QAudioDecoderBackend::invalidSource()
{
    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");
    QAudioBuffer buffer;

    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);
    QCOMPARE(d.source(), QUrl());
    QVERIFY(d.audioFormat() == QAudioFormat());

    // Test invalid file source
    QFileInfo fileInfo(TEST_INVALID_SOURCE);
    QUrl url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
    d.setSource(url);
    QVERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.source(), url);

    QSignalSpy readySpy(&d, &QAudioDecoder::bufferReady);
    QSignalSpy bufferChangedSpy(&d, &QAudioDecoder::bufferAvailableChanged);
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy durationSpy(&d, &QAudioDecoder::durationChanged);
    QSignalSpy finishedSpy(&d, &QAudioDecoder::finished);
    QSignalSpy positionSpy(&d, &QAudioDecoder::positionChanged);

    d.start();
    QTRY_VERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.audioFormat(), QAudioFormat());
    QCOMPARE(d.duration(), qint64(-1));
    QCOMPARE(d.position(), qint64(-1));

    // Check the error code.
    QTRY_VERIFY(!errorSpy.isEmpty());

    // Have to use qvariant_cast, toInt will return 0 because unrecognized type;
    QAudioDecoder::Error errorCode = qvariant_cast<QAudioDecoder::Error>(errorSpy.takeLast().at(0));

    if (isDarwinPlatform()) {
        QCOMPARE(errorCode, QAudioDecoder::FormatError);
        QCOMPARE(d.error(), QAudioDecoder::FormatError);
    } else {
        QCOMPARE(errorCode, QAudioDecoder::ResourceError);
        QCOMPARE(d.error(), QAudioDecoder::ResourceError);
    }

    // Check all other spies.
    QVERIFY(readySpy.isEmpty());
    QVERIFY(bufferChangedSpy.isEmpty());
    QVERIFY(!d.isDecoding());
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(positionSpy.isEmpty());
    QVERIFY(durationSpy.isEmpty());

    errorSpy.clear();

    d.stop();
    QTRY_VERIFY(!d.isDecoding());
    QCOMPARE(d.duration(), qint64(-1));
    QVERIFY(!d.bufferAvailable());

    QFile file;
    file.setFileName(TEST_INVALID_SOURCE);
    QTEST_ASSERT(!file.open(QIODevice::ReadOnly));
    d.setSourceDevice(&file);

    d.start();
    QTRY_VERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.audioFormat(), QAudioFormat());
    QCOMPARE(d.duration(), qint64(-1));
    QCOMPARE(d.position(), qint64(-1));

    // Check the error code.
    QTRY_VERIFY(!errorSpy.isEmpty());
    errorCode = qvariant_cast<QAudioDecoder::Error>(errorSpy.takeLast().at(0));
    QCOMPARE(errorCode, QAudioDecoder::ResourceError);
    QCOMPARE(d.error(), QAudioDecoder::ResourceError);
    QVERIFY(!d.isDecoding());
    // Check all other spies.
    QVERIFY(readySpy.isEmpty());
    QVERIFY(bufferChangedSpy.isEmpty());
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(positionSpy.isEmpty());
    QVERIFY(durationSpy.isEmpty());

    errorSpy.clear();

    d.stop();
    QTRY_VERIFY(!d.isDecoding());
    QCOMPARE(d.duration(), qint64(-1));
    QVERIFY(!d.bufferAvailable());
}

void tst_QAudioDecoderBackend::invalidQrcSourceTest()
{
    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    const QUrl url(QStringLiteral("qrc:/testdata/does-not-exist.wav"));

    QSignalSpy readySpy(&d, &QAudioDecoder::bufferReady);
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy finishedSpy(&d, &QAudioDecoder::finished);

    d.setSource(url);
    QCOMPARE(d.source(), url);
    QVERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());

    d.start();
    QTRY_VERIFY(!errorSpy.isEmpty());
    QTRY_VERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());

    QVERIFY(readySpy.isEmpty());
    QVERIFY(finishedSpy.isEmpty());
}

void tst_QAudioDecoderBackend::deviceTest()
{
    using namespace std::chrono;
    CHECK_SELECTED_URL(m_wavFile);

    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");
    QAudioBuffer buffer;
    quint64 duration = 0;
    int sampleCount = 0;

    QSignalSpy readySpy(&d, &QAudioDecoder::bufferReady);
    QSignalSpy bufferChangedSpy(&d, &QAudioDecoder::bufferAvailableChanged);
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy isDecodingSpy(&d, &QAudioDecoder::isDecodingChanged);
    QSignalSpy durationSpy(&d, &QAudioDecoder::durationChanged);
    QSignalSpy finishedSpy(&d, &QAudioDecoder::finished);
    QSignalSpy positionSpy(&d, &QAudioDecoder::positionChanged);

    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);
    QCOMPARE(d.source(), QUrl{});
    QVERIFY(d.audioFormat() == QAudioFormat());
    QFile file(m_wavFile->toString());
    QVERIFY(file.open(QIODevice::ReadOnly));
    d.setSourceDevice(&file);

    QVERIFY(d.sourceDevice() == &file);
    QVERIFY(d.source().isEmpty());

    // We haven't set the format yet
    QVERIFY(d.audioFormat() == QAudioFormat());

    d.start();

    QTRY_VERIFY(!isDecodingSpy.isEmpty());
    QTRY_VERIFY(!readySpy.isEmpty());
    QTRY_VERIFY(!bufferChangedSpy.isEmpty());
    QVERIFY(d.bufferAvailable());
    QTRY_VERIFY(!durationSpy.isEmpty());
    if (finishedSpy.empty())
        QVERIFY(qAbs(d.duration() - 1000) < 20);
    else
        QCOMPARE(d.duration(), -1);

    buffer = d.read();
    QVERIFY(buffer.isValid());

    // Test file is 44.1K 16bit mono
    QCOMPARE(buffer.format().channelCount(), 1);
    QCOMPARE(buffer.format().sampleRate(), testFileSampleRate);
    QCOMPARE(buffer.format().sampleFormat(), QAudioFormat::Int16);

    QVERIFY(errorSpy.isEmpty());

    duration += buffer.duration();
    sampleCount += buffer.sampleCount();

    // Now drain the decoder
    if (sampleCount < testFileSampleCount) {
        QTRY_COMPARE(d.bufferAvailable(), true);
    }

    while (d.bufferAvailable()) {
        buffer = d.read();
        QVERIFY(buffer.isValid());
        QTRY_VERIFY(!positionSpy.isEmpty());
        if (isGStreamerPlatform())
            QCOMPARE_EQ(positionSpy.takeLast().at(0).toLongLong(),
                        round<milliseconds>(microseconds{ duration }).count());
        else
            QCOMPARE_EQ(positionSpy.takeLast().at(0).toLongLong(),
                        floor<milliseconds>(microseconds{ duration }).count());

        QVERIFY(d.position() - (duration / 1000) < 20);

        duration += buffer.duration();
        sampleCount += buffer.sampleCount();
        if (sampleCount < testFileSampleCount) {
            QTRY_COMPARE(d.bufferAvailable(), true);
        }
    }

    // Make sure the duration is roughly correct (+/- 20ms)
    QCOMPARE(sampleCount, testFileSampleCount);
    QVERIFY(qAbs(qint64(duration) - 1000000) < 20000);
    QVERIFY(qAbs((d.position() + (buffer.duration() / 1000)) - 1000) < 20);
    QTRY_COMPARE(finishedSpy.size(), 1);
    QVERIFY(!d.bufferAvailable());
    QTRY_VERIFY(!d.isDecoding());

    d.stop();
    QTRY_VERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());
    QTRY_COMPARE(durationSpy.size(), 2);
    QCOMPARE(d.duration(), qint64(-1));
    readySpy.clear();
    bufferChangedSpy.clear();
    isDecodingSpy.clear();
    durationSpy.clear();
    finishedSpy.clear();
    positionSpy.clear();

#ifdef Q_OS_ANDROID
    QSKIP("Setting a desired audio format is not yet supported on Android");
#endif
    // Now try changing formats
    QAudioFormat format;
    format.setChannelCount(2);
    format.setSampleRate(8000);
    format.setSampleFormat(QAudioFormat::UInt8);

    d.setAudioFormat(format);

    // Make sure it stuck
    QVERIFY(d.audioFormat() == format);

    d.start();
    QVERIFY(d.error() == QAudioDecoder::NoError);
    QTRY_VERIFY(!isDecodingSpy.isEmpty());
    QTRY_VERIFY(!readySpy.isEmpty());
    QTRY_VERIFY(!bufferChangedSpy.isEmpty());
    QVERIFY(d.bufferAvailable());
    QTRY_VERIFY(!durationSpy.isEmpty());

    QVERIFY(qAbs(durationSpy.front().front().value<qint64>() - 1000) < 20);
    if (finishedSpy.empty())
        QVERIFY(qAbs(d.duration() - 1000) < 20);
    else
        QCOMPARE(d.duration(), -1);

    buffer = d.read();
    QVERIFY(buffer.isValid());
    // See if we got the right format
    QVERIFY(buffer.format() == format);

    // The decoder should still have the same format
    QVERIFY(d.audioFormat() == format);

    QVERIFY(errorSpy.isEmpty());

    d.stop();
    QTRY_VERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());
    QTRY_COMPARE(durationSpy.size(), 2);
    QCOMPARE(d.duration(), qint64(-1));
}

void tst_QAudioDecoderBackend::readIntoDifferentSampleFormat()
{
    CHECK_SELECTED_URL(m_wavFile);

    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    // Request float samples at same rate and channels
    QAudioFormat format;
    format.setChannelCount(1);
    format.setSampleRate(testFileSampleRate);
    format.setSampleFormat(QAudioFormat::Float);

    // Decode native int16
    std::vector<qint16> native = decodeToInt16(*m_wavFile);

    // Decode requested Float format then convert to int16 for sample-wise comparison
    std::optional<QAudioFormat> desired = format;
    std::vector<qint16> converted = decodeToInt16(*m_wavFile, desired);

    // Compare sample-wise
    QCOMPARE(qsizetype(native.size()), qsizetype(converted.size()));
    for (qsizetype i = 0; i < qsizetype(native.size()); ++i) {
        int diff = std::abs(native[i] - converted[i]);
        QVERIFY2(diff <= 1,
                 qPrintable(QString::fromUtf8("Sample mismatch at index %1: ref=%2 got=%3 diff=%4")
                                    .arg(i)
                                    .arg(native[i])
                                    .arg(converted[i])
                                    .arg(diff)));
    }
}

void tst_QAudioDecoderBackend::readIntoDifferentChannelCount()
{
    CHECK_SELECTED_URL(m_wavFile);

    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    // Request stereo output instead of mono
    QAudioFormat format;
    format.setChannelCount(2);
    format.setSampleRate(testFileSampleRate);
    format.setSampleFormat(QAudioFormat::Int16);

    // Decode native int16
    std::vector<qint16> native = decodeToInt16(*m_wavFile);

    // Decode requested stereo format then convert to int16 for comparison
    std::optional<QAudioFormat> desired = format;
    std::vector<qint16> converted = decodeToInt16(*m_wavFile, desired);

    // other should be interleaved stereo, compare left channel to native mono
    compareMonoReferenceToInterleaved(
            native, converted, format.channelCount(),
            1.41401273885f); // sqrt(2) is the expected amplification when upmixing mono to stereo
}

void tst_QAudioDecoderBackend::read24bitFile()
{
    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    QUrl url = testFileUrl(TEST_24BIT_FILE_NAME);
    d.setSource(url);

    QSignalSpy readySpy(&d, &QAudioDecoder::bufferReady);

    d.start();

    QTRY_VERIFY(!readySpy.isEmpty());

    // decode 24bit files into 32bit integers
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    auto buffer = d.read();
    QVERIFY(buffer.isValid());
    auto sf = buffer.format().sampleFormat();
    QVERIFY(sf == QAudioFormat::Int32);
    // ensure no error signal emitted
    QVERIFY(errorSpy.isEmpty());
}

void tst_QAudioDecoderBackend::readFloat32File()
{
    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    QUrl url = testFileUrl(TEST_FLOAT32_FILE_NAME);
    d.setSource(url);

    QSignalSpy readySpy(&d, &QAudioDecoder::bufferReady);

    d.start();

    QTRY_VERIFY(!readySpy.isEmpty());

    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    auto buffer = d.read();
    QVERIFY(buffer.isValid());
    QCOMPARE(buffer.format().sampleFormat(), QAudioFormat::Float);
    QCOMPARE(buffer.format().channelCount(), 1);
    QCOMPARE(buffer.format().sampleRate(), 44100);
    // ensure no error signal emitted
    QVERIFY(errorSpy.isEmpty());
}

void tst_QAudioDecoderBackend::readMp3File()
{
    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    QUrl url = testFileUrl("testdata/nokia-tune.mp3");
    d.setSource(url);

    QSignalSpy readySpy(&d, &QAudioDecoder::bufferReady);
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));

    d.start();

    QTRY_VERIFY(!readySpy.isEmpty());

    auto buffer = d.read();
    QVERIFY(buffer.isValid());
    QCOMPARE(buffer.format().channelCount(), 2);
    QCOMPARE(buffer.format().sampleRate(), 48000);
    QVERIFY(errorSpy.isEmpty());
}

void tst_QAudioDecoderBackend::readOggFile()
{
    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    QUrl url = testFileUrl("testdata/test.ogg");
    d.setSource(url);

    QSignalSpy readySpy(&d, &QAudioDecoder::bufferReady);
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));

    d.start();

    QTRY_VERIFY(!readySpy.isEmpty());

    auto buffer = d.read();
    QVERIFY(buffer.isValid());
    QCOMPARE(buffer.format().channelCount(), 1);
    QCOMPARE(buffer.format().sampleRate(), 44100);
    QVERIFY(errorSpy.isEmpty());
}

void tst_QAudioDecoderBackend::readAacFile()
{
    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    QUrl url = testFileUrl("testdata/test.aac");
    d.setSource(url);

    QSignalSpy readySpy(&d, &QAudioDecoder::bufferReady);
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));

    d.start();

    QTRY_VERIFY(!readySpy.isEmpty());

    auto buffer = d.read();
    QVERIFY(buffer.isValid());
    QCOMPARE(buffer.format().channelCount(), 1);
    QCOMPARE(buffer.format().sampleRate(), 44100);
    QVERIFY(errorSpy.isEmpty());
}

void tst_QAudioDecoderBackend::readFlacFile()
{
    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    QUrl url = testFileUrl("testdata/test.flac");
    d.setSource(url);

    QSignalSpy readySpy(&d, &QAudioDecoder::bufferReady);
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));

    d.start();

    QTRY_VERIFY(!readySpy.isEmpty());

    auto buffer = d.read();
    QVERIFY(buffer.isValid());
    QCOMPARE(buffer.format().channelCount(), 1);
    QCOMPARE(buffer.format().sampleRate(), 44100);
    QVERIFY(errorSpy.isEmpty());
}

void tst_QAudioDecoderBackend::play_emitsFormatError_whenMediaHasNoAudioTrack()
{
    QAudioDecoder decoder;

    QSignalSpy errors{ &decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error) };

    decoder.setSource(testFileUrl(TEST_NO_AUDIO_TRACK));
    decoder.start();

    QTRY_VERIFY(!errors.empty());

    QCOMPARE_EQ(decoder.error(), QAudioDecoder::Error::FormatError);
}

QTEST_MAIN(tst_QAudioDecoderBackend)

#include "tst_qaudiodecoderbackend.moc"
