// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QDebug>
#include "qaudiodecoder.h"

#ifdef WAV_SUPPORT_NOT_FORCED
#include "../shared/mediafileselector.h"
#endif

#define TEST_FILE_NAME "testdata/test.wav"
#define TEST_UNSUPPORTED_FILE_NAME "testdata/test-unsupported.avi"
#define TEST_CORRUPTED_FILE_NAME "testdata/test-corrupted.wav"
#define TEST_INVALID_SOURCE "invalid"

QT_USE_NAMESPACE

/*
 This is the backend conformance test.

 Since it relies on platform media framework
 it may be less stable.
*/

class tst_QAudioDecoderBackend : public QObject
{
    Q_OBJECT
public slots:
    void init();
    void cleanup();
    void initTestCase();

private slots:
    void directBruteForceReading();
    void indirectReadingByBufferReadySignal();
    void indirectReadingByBufferAvailableSignal();
    void stopOnBufferReady();
    void restartOnBufferReady();
    void restartOnFinish();
    void fileTest();
    void unsupportedFileTest();
    void corruptedFileTest();
    void invalidSource();
    void deviceTest();

private:
    bool isWavSupported();
    QUrl testFileUrl(const QString filePath);
    void checkNoMoreChanges(QAudioDecoder &decoder);
#ifdef Q_OS_ANDROID
    QTemporaryFile *temporaryFile = nullptr;
#endif
};

void tst_QAudioDecoderBackend::init()
{
}

void tst_QAudioDecoderBackend::initTestCase()
{
    QAudioDecoder d;
    if (!d.isSupported())
        QSKIP("Audio decoder service is not available");
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

bool tst_QAudioDecoderBackend::isWavSupported()
{
#ifdef WAV_SUPPORT_NOT_FORCED
    return !MediaFileSelector::selectMediaFile(QStringList() << QFINDTESTDATA(TEST_FILE_NAME)).isNull();
#else
    return true;
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

void tst_QAudioDecoderBackend::directBruteForceReading()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioDecoder decoder;
    if (decoder.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    int sampleCount = 0;

    decoder.setSource(testFileUrl(TEST_FILE_NAME));
    QVERIFY(!decoder.isDecoding());
    QVERIFY(!decoder.bufferAvailable());

    decoder.start();
    QTRY_VERIFY(decoder.isDecoding());

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

    QCOMPARE(sampleCount, 44094);
}

void tst_QAudioDecoderBackend::indirectReadingByBufferReadySignal()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioDecoder decoder;
    if (decoder.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    int sampleCount = 0;

    connect(&decoder, &QAudioDecoder::bufferReady, this, [&]() {
        QVERIFY(decoder.bufferAvailable());
        QVERIFY(decoder.isDecoding());

        auto buffer = decoder.read();
        QVERIFY(buffer.isValid());
        QVERIFY(!decoder.bufferAvailable());

        sampleCount += buffer.sampleCount();
    });

    QSignalSpy decodingSpy(&decoder, &QAudioDecoder::isDecodingChanged);
    QSignalSpy finishSpy(&decoder, &QAudioDecoder::finished);

    decoder.setSource(testFileUrl(TEST_FILE_NAME));
    QVERIFY(!decoder.isDecoding());
    QVERIFY(!decoder.bufferAvailable());

    decoder.start();
    QTRY_VERIFY(decodingSpy.size() >= 1);

    QTRY_VERIFY(finishSpy.size() == 1);
    QVERIFY(!decoder.isDecoding());

    checkNoMoreChanges(decoder);

    QCOMPARE(sampleCount, 44094);
    QCOMPARE(finishSpy.size(), 1);
}

void tst_QAudioDecoderBackend::indirectReadingByBufferAvailableSignal() {
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioDecoder decoder;
    if (decoder.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    int sampleCount = 0;

    connect(&decoder, &QAudioDecoder::bufferAvailableChanged, this, [&](bool available) {
        QCOMPARE(decoder.bufferAvailable(), available);

        if (!available)
            return;

        QVERIFY(decoder.isDecoding());

        while (decoder.bufferAvailable()) {
            auto buffer = decoder.read();
            QVERIFY(buffer.isValid());

            sampleCount += buffer.sampleCount();
        }
    });

    QSignalSpy decodingSpy(&decoder, &QAudioDecoder::isDecodingChanged);
    QSignalSpy finishSpy(&decoder, &QAudioDecoder::finished);

    decoder.setSource(testFileUrl(TEST_FILE_NAME));
    QVERIFY(!decoder.isDecoding());
    QVERIFY(!decoder.bufferAvailable());

    decoder.start();
    QTRY_VERIFY(decodingSpy.size() >= 1);

    QTRY_VERIFY(finishSpy.size() == 1);
    QVERIFY(!decoder.isDecoding());

    checkNoMoreChanges(decoder);

    QCOMPARE(sampleCount, 44094);
    QCOMPARE(finishSpy.size(), 1);
}

void tst_QAudioDecoderBackend::stopOnBufferReady()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioDecoder decoder;
    if (decoder.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");

    connect(&decoder, &QAudioDecoder::bufferReady, this, [&]() {
        decoder.read(); // run next reading
        decoder.stop();
    });

    QSignalSpy finishSpy(&decoder, &QAudioDecoder::finished);
    QSignalSpy bufferReadySpy(&decoder, &QAudioDecoder::bufferReady);

    decoder.setSource(testFileUrl(TEST_FILE_NAME));
    decoder.start();

    bufferReadySpy.wait();
    QVERIFY(!decoder.isDecoding());

    checkNoMoreChanges(decoder);

    QCOMPARE(bufferReadySpy.size(), 1);
}

void tst_QAudioDecoderBackend::restartOnBufferReady()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

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

    decoder.setSource(testFileUrl(TEST_FILE_NAME));
    decoder.start();

    QTRY_VERIFY2(finishSpy.size() == 2, "Wait for signals after restart and after finishing");
    QVERIFY(!decoder.isDecoding());

    checkNoMoreChanges(decoder);

    QCOMPARE(sampleCount, 44094);
}

void tst_QAudioDecoderBackend::restartOnFinish()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

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

    decoder.setSource(testFileUrl(TEST_FILE_NAME));
    decoder.start();

    QTRY_VERIFY(finishSpy.size() == 2);

    QVERIFY(!decoder.isDecoding());

    checkNoMoreChanges(decoder);
    QCOMPARE(sampleCount, 44094);
}

void tst_QAudioDecoderBackend::fileTest()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");
    QAudioBuffer buffer;
    quint64 duration = 0;
    int byteCount = 0;
    int sampleCount = 0;

    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);
    QCOMPARE(d.source(), QString(""));
    QVERIFY(d.audioFormat() == QAudioFormat());

    // Test local file
    QUrl url = testFileUrl(TEST_FILE_NAME);
    d.setSource(url);
    QVERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.source(), url);

    QSignalSpy readySpy(&d, SIGNAL(bufferReady()));
    QSignalSpy bufferChangedSpy(&d, SIGNAL(bufferAvailableChanged(bool)));
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy isDecodingSpy(&d, SIGNAL(isDecodingChanged(bool)));
    QSignalSpy durationSpy(&d, SIGNAL(durationChanged(qint64)));
    QSignalSpy finishedSpy(&d, SIGNAL(finished()));
    QSignalSpy positionSpy(&d, SIGNAL(positionChanged(qint64)));

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
    QCOMPARE(buffer.format().sampleRate(), 44100);
    QCOMPARE(buffer.format().sampleFormat(), QAudioFormat::Int16);
    QCOMPARE(buffer.byteCount(), buffer.sampleCount() * 2); // 16bit mono

    // The decoder should still have no format set
    QVERIFY(d.audioFormat() == QAudioFormat());
    QVERIFY(errorSpy.isEmpty());

    duration += buffer.duration();
    sampleCount += buffer.sampleCount();
    byteCount += buffer.byteCount();

    // Now drain the decoder
    if (sampleCount < 44094) {
        QTRY_COMPARE(d.bufferAvailable(), true);
    }

    while (d.bufferAvailable()) {
        buffer = d.read();
        QVERIFY(buffer.isValid());
        QTRY_VERIFY(!positionSpy.isEmpty());
        QCOMPARE(positionSpy.takeLast().at(0).toLongLong(), qint64(duration / 1000));

        duration += buffer.duration();
        sampleCount += buffer.sampleCount();
        byteCount += buffer.byteCount();

        if (sampleCount < 44094) {
            QTRY_COMPARE(d.bufferAvailable(), true);
        }
    }

    // Make sure the duration is roughly correct (+/- 20ms)
    QCOMPARE(sampleCount, 44094);
    QCOMPARE(byteCount, 44094 * 2);
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
    QSKIP("Setting a desired audio format is not yet supported on Android", QTest::SkipSingle);
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

    // Now drain the decoder
    if (duration < 996000) {
        QTRY_COMPARE(d.bufferAvailable(), true);
    }

    while (d.bufferAvailable()) {
        buffer = d.read();
        QVERIFY(buffer.isValid());
        QTRY_VERIFY(!positionSpy.isEmpty());
        QCOMPARE(positionSpy.takeLast().at(0).toLongLong(), qint64(duration / 1000));
        QVERIFY(d.position() - (duration / 1000) < 20);

        duration += buffer.duration();
        sampleCount += buffer.sampleCount();
        byteCount += buffer.byteCount();

        if (duration < 996000) {
            QTRY_COMPARE(d.bufferAvailable(), true);
        }
    }

    // Resampling might end up with fewer or more samples
    // so be a bit sloppy
    QVERIFY(qAbs(sampleCount - 22047) < 100);
    QVERIFY(qAbs(byteCount - 22047) < 100);
    QVERIFY(qAbs(qint64(duration) - 1000000) < 20000);
    QVERIFY(qAbs((d.position() + (buffer.duration() / 1000)) - 1000) < 20);
    QTRY_COMPARE(finishedSpy.size(), 1);
    QVERIFY(!d.bufferAvailable());
    QVERIFY(!d.isDecoding());

    d.stop();
    QTRY_VERIFY(!d.isDecoding());
    QTRY_COMPARE(durationSpy.size(), 2);
    QCOMPARE(d.duration(), qint64(-1));
    QVERIFY(!d.bufferAvailable());
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
    QCOMPARE(d.source(), QString(""));
    QVERIFY(d.audioFormat() == QAudioFormat());

    // Test local file
    QUrl url = testFileUrl(TEST_UNSUPPORTED_FILE_NAME);
    d.setSource(url);
    QVERIFY(!d.isDecoding());
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.source(), url);

    QSignalSpy readySpy(&d, SIGNAL(bufferReady()));
    QSignalSpy bufferChangedSpy(&d, SIGNAL(bufferAvailableChanged(bool)));
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy isDecodingSpy(&d, SIGNAL(isDecodingChanged(bool)));
    QSignalSpy durationSpy(&d, SIGNAL(durationChanged(qint64)));
    QSignalSpy finishedSpy(&d, SIGNAL(finished()));
    QSignalSpy positionSpy(&d, SIGNAL(positionChanged(qint64)));

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
    QVERIFY(isDecodingSpy.isEmpty());
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
    QVERIFY(isDecodingSpy.isEmpty());
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

    QSignalSpy readySpy(&d, SIGNAL(bufferReady()));
    QSignalSpy bufferChangedSpy(&d, SIGNAL(bufferAvailableChanged(bool)));
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy isDecodingSpy(&d, SIGNAL(isDecodingChanged(bool)));
    QSignalSpy durationSpy(&d, SIGNAL(durationChanged(qint64)));
    QSignalSpy finishedSpy(&d, SIGNAL(finished()));
    QSignalSpy positionSpy(&d, SIGNAL(positionChanged(qint64)));

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
    QVERIFY(isDecodingSpy.isEmpty());
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
    QVERIFY(isDecodingSpy.isEmpty());
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

    QSignalSpy readySpy(&d, SIGNAL(bufferReady()));
    QSignalSpy bufferChangedSpy(&d, SIGNAL(bufferAvailableChanged(bool)));
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy isDecodingSpy(&d, SIGNAL(isDecodingChanged(bool)));
    QSignalSpy durationSpy(&d, SIGNAL(durationChanged(qint64)));
    QSignalSpy finishedSpy(&d, SIGNAL(finished()));
    QSignalSpy positionSpy(&d, SIGNAL(positionChanged(qint64)));

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
    QCOMPARE(errorCode, QAudioDecoder::ResourceError);
    QCOMPARE(d.error(), QAudioDecoder::ResourceError);

    // Check all other spies.
    QVERIFY(readySpy.isEmpty());
    QVERIFY(bufferChangedSpy.isEmpty());
    QVERIFY(isDecodingSpy.isEmpty());
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
    file.open(QIODevice::ReadOnly);
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
    // Check all other spies.
    QVERIFY(readySpy.isEmpty());
    QVERIFY(bufferChangedSpy.isEmpty());
    QVERIFY(isDecodingSpy.isEmpty());
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(positionSpy.isEmpty());
    QVERIFY(durationSpy.isEmpty());

    errorSpy.clear();

    d.stop();
    QTRY_VERIFY(!d.isDecoding());
    QCOMPARE(d.duration(), qint64(-1));
    QVERIFY(!d.bufferAvailable());
}

void tst_QAudioDecoderBackend::deviceTest()
{
    if (!isWavSupported())
        QSKIP("Sound format is not supported");

    QAudioDecoder d;
    if (d.error() == QAudioDecoder::NotSupportedError)
        QSKIP("There is no audio decoding support on this platform.");
    QAudioBuffer buffer;
    quint64 duration = 0;
    int sampleCount = 0;

    QSignalSpy readySpy(&d, SIGNAL(bufferReady()));
    QSignalSpy bufferChangedSpy(&d, SIGNAL(bufferAvailableChanged(bool)));
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy isDecodingSpy(&d, SIGNAL(isDecodingChanged(bool)));
    QSignalSpy durationSpy(&d, SIGNAL(durationChanged(qint64)));
    QSignalSpy finishedSpy(&d, SIGNAL(finished()));
    QSignalSpy positionSpy(&d, SIGNAL(positionChanged(qint64)));

    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);
    QCOMPARE(d.source(), QString(""));
    QVERIFY(d.audioFormat() == QAudioFormat());
#ifndef Q_OS_ANDROID
    QFileInfo fileInfo(QFINDTESTDATA(TEST_FILE_NAME));
    QFile file(fileInfo.absoluteFilePath());
#else
    QFile file(":/" TEST_FILE_NAME);
#endif
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
    QCOMPARE(buffer.format().sampleRate(), 44100);
    QCOMPARE(buffer.format().sampleFormat(), QAudioFormat::Int16);

    QVERIFY(errorSpy.isEmpty());

    duration += buffer.duration();
    sampleCount += buffer.sampleCount();

    // Now drain the decoder
    if (sampleCount < 44094) {
        QTRY_COMPARE(d.bufferAvailable(), true);
    }

    while (d.bufferAvailable()) {
        buffer = d.read();
        QVERIFY(buffer.isValid());
        QTRY_VERIFY(!positionSpy.isEmpty());
        QVERIFY(positionSpy.takeLast().at(0).toLongLong() == qint64(duration / 1000));
        QVERIFY(d.position() - (duration / 1000) < 20);

        duration += buffer.duration();
        sampleCount += buffer.sampleCount();
        if (sampleCount < 44094) {
            QTRY_COMPARE(d.bufferAvailable(), true);
        }
    }

    // Make sure the duration is roughly correct (+/- 20ms)
    QCOMPARE(sampleCount, 44094);
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
    QSKIP("Setting a desired audio format is not yet supported on Android", QTest::SkipSingle);
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

QTEST_MAIN(tst_QAudioDecoderBackend)

#include "tst_qaudiodecoderbackend.moc"
