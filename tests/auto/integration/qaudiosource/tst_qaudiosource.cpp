// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qsemaphore.h>
#include <QtCore/qtemporarydir.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qaudiosource.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qwavedecoder.h>
#include <QtMultimedia/private/qaudiosystem_p.h>

#include <private/mediabackendutils_p.h>
#include <private/qmockiodevice_p.h>

#include <memory>

#define RANGE_ERR 0.5

namespace {

template<typename T> inline bool qTolerantCompare(T value, T expected)
{
    return qAbs(value - expected) < (RANGE_ERR * expected);
}

bool isPulseAudioBackend()
{
    return QPlatformMediaIntegration::audioBackendName() == "PulseAudio";
}

} // namespace

using AudioSourceInitializer = bool (*)(QAudioSource &);

class tst_QAudioSource : public QObject
{
    Q_OBJECT
public:
    tst_QAudioSource(QObject* parent=nullptr) : QObject(parent) {}

private slots:
    void initTestCase();

    void format();
    void invalidFormat_data();
    void invalidFormat();
    void nullFormat();

    void bufferSize();
    void bufferSize_getValidDefault();
    void bufferSize_setAfterStart();
    void bufferSize_updatedAfterStart();

    void stopWhileStopped();
    void suspendWhileStopped();
    void resumeWhileStopped();

    void pull_data(){generate_audiofile_testrows();}
    void pull();

    void pullSuspendResume_data(){generate_audiofile_testrows();}
    void pullSuspendResume();

    void push_data(){generate_audiofile_testrows();}
    void push();

    void pushSuspendResume_data(){generate_audiofile_testrows();}
    void pushSuspendResume();

    void reset_data(){generate_audiofile_testrows();}
    void reset();

    void volume_data(){generate_audiofile_testrows();}
    void volume();

    void stop_finishesPushMode_whenInvokedUponReadyReadSignal();

    void stop_stopsAudioSource_whenInvokedUponFirstStateChange_data();
    void stop_stopsAudioSource_whenInvokedUponFirstStateChange();

    void stateChanged_stringBasedConnect();


    void start_withSamplingRate_data();
    void start_withSamplingRate();

    void callbackAPI();
    void callbackAPI_startFailsWithWrongType();

private:
    using FilePtr = std::shared_ptr<QFile>;

    QString formatToFileName(const QAudioFormat &format);

    void generate_audiofile_testrows();

    QAudioDevice audioDevice;
    QList<QAudioFormat> testFormats;
    QList<FilePtr> audioFiles;
    std::unique_ptr<QTemporaryDir> m_temporaryDir;

    std::unique_ptr<QByteArray> m_byteArray;
    std::unique_ptr<QBuffer> m_buffer;

    bool m_inCISystem = isCI();
};

void tst_QAudioSource::generate_audiofile_testrows()
{
    QTest::addColumn<FilePtr>("audioFile");
    QTest::addColumn<QAudioFormat>("audioFormat");

    for (int i=0; i<audioFiles.size(); i++) {
        QTest::newRow(QStringLiteral("%1").arg(i).toUtf8().constData())
                << audioFiles.at(i) << testFormats.at(i);
    }
}

QString tst_QAudioSource::formatToFileName(const QAudioFormat &format)
{
    return QStringLiteral("%1_%2_%3")
            .arg(format.sampleRate())
            .arg(format.bytesPerSample())
            .arg(format.channelCount());
}

void tst_QAudioSource::initTestCase()
{
    if (m_inCISystem)
        QSKIP("SKIP initTestCase on CI. To be fixed");

    // Only perform tests if audio input device exists
    const QList<QAudioDevice> devices = QMediaDevices::audioInputs();

    if (devices.size() <= 0)
        QSKIP("No audio inputs found");

    audioDevice = QMediaDevices::defaultAudioInput();


    QAudioFormat format;
    format.setChannelCount(1);

    if (audioDevice.isFormatSupported(audioDevice.preferredFormat())) {
        if (format.sampleFormat() == QAudioFormat::Int16)
            testFormats.append(audioDevice.preferredFormat());
    }

    format.setSampleFormat(QAudioFormat::Int16);
    for (int channels : { 1, 2 }) {
        format.setChannelCount(channels);
        for (int rate : { 44100, 48000 }) {
            format.setSampleRate(rate);

            if (audioDevice.isFormatSupported(format))
                testFormats.append(format);
        }
    }

    QVERIFY(testFormats.size());

    const QChar slash = QLatin1Char('/');
    QString temporaryPattern = QDir::tempPath();
    if (!temporaryPattern.endsWith(slash))
        temporaryPattern += slash;
    temporaryPattern += "tst_qaudioinputXXXXXX";
    m_temporaryDir.reset(new QTemporaryDir(temporaryPattern));
    m_temporaryDir->setAutoRemove(true);
    QVERIFY(m_temporaryDir->isValid());

    const QString temporaryAudioPath = m_temporaryDir->path() + slash;
    for (const QAudioFormat &format : std::as_const(testFormats)) {
        const QString fileName = temporaryAudioPath + formatToFileName(format) + QStringLiteral(".wav");
        audioFiles.append(std::make_shared<QFile>(fileName));
    }
}

void tst_QAudioSource::format()
{
    QAudioSource audioSource(audioDevice.preferredFormat(), this);

    QAudioFormat requested = audioDevice.preferredFormat();
    QAudioFormat actual = audioSource.format();

    QVERIFY2((requested.channelCount() == actual.channelCount()),
             QStringLiteral("channels: requested=%1, actual=%2")
                     .arg(requested.channelCount())
                     .arg(actual.channelCount())
                     .toUtf8()
                     .constData());
    QVERIFY2((requested.sampleRate() == actual.sampleRate()),
             QStringLiteral("sampleRate: requested=%1, actual=%2")
                     .arg(requested.sampleRate())
                     .arg(actual.sampleRate())
                     .toUtf8()
                     .constData());
    QVERIFY2((requested.sampleFormat() == actual.sampleFormat()),
             QStringLiteral("sampleFormat: requested=%1, actual=%2")
                     .arg((ushort)requested.sampleFormat())
                     .arg((ushort)actual.sampleFormat())
                     .toUtf8()
                     .constData());
    QCOMPARE(actual, requested);
}

void tst_QAudioSource::invalidFormat_data()
{
    QTest::addColumn<QAudioFormat>("invalidFormat");

    QAudioFormat format;

    format = audioDevice.preferredFormat();
    format.setChannelCount(0);
    QTest::newRow("Channel count 0")
            << format;

    format = audioDevice.preferredFormat();
    format.setSampleRate(0);
    QTest::newRow("Sample rate 0")
            << format;

    format = audioDevice.preferredFormat();
    format.setSampleFormat(QAudioFormat::Unknown);
    QTest::newRow("Sample size 0")
            << format;
}

void tst_QAudioSource::invalidFormat()
{
    QFETCH(QAudioFormat, invalidFormat);

    QVERIFY2(!audioDevice.isFormatSupported(invalidFormat),
            "isFormatSupported() is returning true on an invalid format");

    QAudioSource audioSource(invalidFormat, this);

    // Check that we are in the default state before calling start
    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");

    QTest::ignoreMessage(QtWarningMsg, "QAudioSource::start: QAudioFormat not valid");
    audioSource.start();

    // Check that error is raised
    QTRY_VERIFY2((audioSource.error() == QAudio::OpenError),
                 "error() was not set to QAudio::OpenError after start()");
}

void tst_QAudioSource::nullFormat()
{
    QAudioDevice audioDevice = QMediaDevices::defaultAudioInput();
    if (audioDevice.isNull())
        QSKIP("No audio inputs found");

    {
        QAudioSource audioSource;
        QCOMPARE(audioSource.format(), audioDevice.preferredFormat());
    }
    {
        QAudioSource audioSource(audioDevice);
        QCOMPARE(audioSource.format(), audioDevice.preferredFormat());
    }
}

void tst_QAudioSource::bufferSize()
{
    QAudioSource audioSource(audioDevice.preferredFormat(), this);

    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError on creation");

    audioSource.setBufferSize(512);
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() is not QAudio::NoError after setBufferSize(512)");
    QVERIFY2((audioSource.bufferSize() == 512),
             QStringLiteral("bufferSize: requested=512, actual=%2")
                     .arg(audioSource.bufferSize())
                     .toUtf8()
                     .constData());

    audioSource.setBufferSize(4096);
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() is not QAudio::NoError after setBufferSize(4096)");
    QVERIFY2((audioSource.bufferSize() == 4096),
             QStringLiteral("bufferSize: requested=4096, actual=%2")
                     .arg(audioSource.bufferSize())
                     .toUtf8()
                     .constData());

    audioSource.setBufferSize(8192);
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() is not QAudio::NoError after setBufferSize(8192)");
    QVERIFY2((audioSource.bufferSize() == 8192),
             QStringLiteral("bufferSize: requested=8192, actual=%2")
                     .arg(audioSource.bufferSize())
                     .toUtf8()
                     .constData());
}

void tst_QAudioSource::bufferSize_getValidDefault()
{
#if !(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
    QSKIP("bufferSize validation only fully implemented on Mac and Windows");
#endif

    QAudioSource audioSource(audioDevice.preferredFormat(), this);
    QCOMPARE_GE(audioSource.bufferSize(),
                audioDevice.preferredFormat().bytesForDuration(250'000)); // 250ms
}

void tst_QAudioSource::bufferSize_setAfterStart()
{
#if !(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
    QSKIP("bufferSize validation only fully implemented on Mac and Windows");
#endif

    QAudioSource audioSource(audioDevice.preferredFormat(), this);
    audioSource.start();
    QCOMPARE_GE(audioSource.bufferSize(), audioDevice.preferredFormat().bytesForFrames(32));
}

void tst_QAudioSource::bufferSize_updatedAfterStart()
{
#if !(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
    QSKIP("bufferSize validation only fully implemented on Mac and Windows");
#endif

    QAudioSource audioSource(audioDevice.preferredFormat(), this);
    audioSource.setBufferSize(1); // small enough force a size increase
    audioSource.start();
    QCOMPARE_GE(audioSource.bufferSize(), audioDevice.preferredFormat().bytesForFrames(32));
}

void tst_QAudioSource::stopWhileStopped()
{
    // Calls QAudioSource::stop() when object is already in StoppedState
    // Checks that
    //  - No state change occurs
    //  - No error is raised (QAudioSource::error() returns QAudio::NoError)

    QAudioSource audioSource(audioDevice.preferredFormat(), this);

    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");

    QSignalSpy stateSignal(&audioSource, &QAudioSource::stateChanged);
    audioSource.stop();

    // Check that no state transition occurred
    QVERIFY2((stateSignal.size() == 0), "stop() while stopped is emitting a signal and it shouldn't");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError after stop()");
}

void tst_QAudioSource::suspendWhileStopped()
{
    // Calls QAudioSource::suspend() when object is already in StoppedState
    // Checks that
    //  - No state change occurs
    //  - No error is raised (QAudioSource::error() returns QAudio::NoError)

    QAudioSource audioSource(audioDevice.preferredFormat(), this);

    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");

    QSignalSpy stateSignal(&audioSource, &QAudioSource::stateChanged);
    audioSource.suspend();

    // Check that no state transition occurred
    QVERIFY2((stateSignal.size() == 0), "stop() while suspended is emitting a signal and it shouldn't");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError after stop()");
}

void tst_QAudioSource::resumeWhileStopped()
{
    // Calls QAudioSource::resume() when object is already in StoppedState
    // Checks that
    //  - No state change occurs
    //  - No error is raised (QAudioSource::error() returns QAudio::NoError)

    QAudioSource audioSource(audioDevice.preferredFormat(), this);

    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");

    QSignalSpy stateSignal(&audioSource, &QAudioSource::stateChanged);
    audioSource.resume();

    // Check that no state transition occurred
    QVERIFY2((stateSignal.size() == 0), "resume() while stopped is emitting a signal and it shouldn't");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError after resume()");
}

void tst_QAudioSource::pull()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSource audioSource(audioFormat, this);

    QSignalSpy stateSignal(&audioSource, &QAudioSource::stateChanged);

    // Check that we are in the default state before calling start
    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioSource.elapsedUSecs() == qint64(0)), "elapsedUSecs() not zero on creation");

    audioFile->close();
    QTEST_ASSERT(audioFile->open(QIODevice::WriteOnly));
    QWaveDecoder waveDecoder(audioFile.get(), audioFormat);
    if (!waveDecoder.open(QIODevice::WriteOnly)) {
        waveDecoder.close();
        audioFile->close();
        QSKIP("Audio format not supported for writing to WAV file.");
    }
    QCOMPARE(waveDecoder.size(), QWaveDecoder::headerLength());

    audioSource.start(audioFile.get());

    // Check that QAudioSource immediately transitions to ActiveState or IdleState
    QTRY_VERIFY2((stateSignal.size() > 0),"didn't emit signals on start()");
    QVERIFY2((audioSource.state() == QAudio::ActiveState
              || audioSource.state() == QAudio::IdleState),
             "didn't transition to ActiveState or IdleState after start()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTRY_COMPARE_GT(audioSource.elapsedUSecs(), 0);
    QTRY_COMPARE_GT(audioSource.processedUSecs(), 0);

    // Allow some recording to happen
    QTest::qWait(3000); // 3 seconds should be plenty

    stateSignal.clear();

    if (!isPulseAudioBackend()) {
        // QTBUG-138000 ... pulseaudio has a rather odd timing behaviour
        qint64 processedUs = audioSource.processedUSecs();
        QVERIFY2(
                qTolerantCompare(processedUs, 3000000LL),
                QStringLiteral(
                        "processedUSecs() doesn't fall in acceptable range, should be 3000000 (%1)")
                        .arg(processedUs)
                        .toUtf8()
                        .constData());
    }

    audioSource.stop();
    QTRY_VERIFY2(
            (stateSignal.size() == 1),
            QStringLiteral("didn't emit StoppedState signal after stop(), got %1 signals instead")
                    .arg(stateSignal.size())
                    .toUtf8()
                    .constData());
    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioSource.elapsedUSecs() == (qint64)0),
             "elapsedUSecs() not equal to zero in StoppedState");

    //QWaveHeader::writeDataLength(*audioFile, audioFile->pos() - WavHeader::headerLength());
    //waveDecoder.writeDataLength();
    waveDecoder.close();
    audioFile->close();

}

void tst_QAudioSource::pullSuspendResume()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSource audioSource(audioFormat, this);

    QSignalSpy stateSignal(&audioSource, &QAudioSource::stateChanged);

    // Check that we are in the default state before calling start
    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioSource.elapsedUSecs() == qint64(0)), "elapsedUSecs() not zero on creation");

    audioFile->close();
    QTEST_ASSERT(audioFile->open(QIODevice::WriteOnly));
    QWaveDecoder waveDecoder(audioFile.get(), audioFormat);
    if (!waveDecoder.open(QIODevice::WriteOnly)) {
        waveDecoder.close();
        audioFile->close();
        QSKIP("Audio format not supported for writing to WAV file.");
    }
    QCOMPARE(waveDecoder.size(), QWaveDecoder::headerLength());

    audioSource.start(audioFile.get());

    // Check that QAudioSource immediately transitions to ActiveState or IdleState
    QTRY_VERIFY2((stateSignal.size() > 0),"didn't emit signals on start()");
    QVERIFY2((audioSource.state() == QAudio::ActiveState
              || audioSource.state() == QAudio::IdleState),
             "didn't transition to ActiveState or IdleState after start()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTRY_VERIFY2((audioSource.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");
    QTRY_VERIFY2((audioSource.processedUSecs() > 0), "elapsedUSecs() is still zero after start()");

    // Allow some recording to happen
    QTest::qWait(3000); // 3 seconds should be plenty

    QVERIFY2((audioSource.state() == QAudio::ActiveState),
             "didn't transition to ActiveState after some recording");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after some recording");

    stateSignal.clear();

    audioSource.suspend();

    QTRY_VERIFY2(
            (stateSignal.size() == 1),
            QStringLiteral(
                    "didn't emit SuspendedState signal after suspend(), got %1 signals instead")
                    .arg(stateSignal.size())
                    .toUtf8()
                    .constData());
    QVERIFY2((audioSource.state() == QAudio::SuspendedState),
             "didn't transitions to SuspendedState after stop()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() is not QAudio::NoError after stop()");
    stateSignal.clear();

    // Check that only 'elapsed', and not 'processed' increases while suspended
    if (!isPulseAudioBackend()) {
        // QTBUG-138000 ... pulseaudio has a rather odd timing behaviour

        qint64 elapsedUs = audioSource.elapsedUSecs();
        qint64 processedUs = audioSource.processedUSecs();
        QVERIFY2(
                qTolerantCompare(processedUs, 3000000LL),
                QStringLiteral(
                        "processedUSecs() doesn't fall in acceptable range, should be 3000000 (%1)")
                        .arg(processedUs)
                        .toUtf8()
                        .constData());
        QTRY_COMPARE_GT(audioSource.elapsedUSecs(), elapsedUs);
        QCOMPARE(audioSource.processedUSecs(), processedUs);
    }

    audioSource.resume();

    // Check that QAudioSource immediately transitions to ActiveState
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QStringLiteral("didn't emit signal after resume(), got %1 signals instead")
                         .arg(stateSignal.size())
                         .toUtf8()
                         .constData());
    QVERIFY2((audioSource.state() == QAudio::ActiveState),
             "didn't transition to ActiveState after resume()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after resume()");
    stateSignal.clear();

    audioSource.stop();
    QTest::qWait(40);
    QTRY_VERIFY2(
            (stateSignal.size() == 1),
            QStringLiteral("didn't emit StoppedState signal after stop(), got %1 signals instead")
                    .arg(stateSignal.size())
                    .toUtf8()
                    .constData());
    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioSource.elapsedUSecs() == (qint64)0),
             "elapsedUSecs() not equal to zero in StoppedState");

    //WavHeader::writeDataLength(*audioFile,audioFile->pos()-WavHeader::headerLength());
    //waveDecoder.writeDataLength();
    waveDecoder.close();
    audioFile->close();
}

void tst_QAudioSource::push()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSource audioSource(audioFormat, this);

    QSignalSpy stateSignal(&audioSource, &QAudioSource::stateChanged);

    // Check that we are in the default state before calling start
    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioSource.elapsedUSecs() == qint64(0)), "elapsedUSecs() not zero on creation");

    audioFile->close();
    QTEST_ASSERT(audioFile->open(QIODevice::WriteOnly));
    QWaveDecoder waveDecoder(audioFile.get(), audioFormat);
    if (!waveDecoder.open(QIODevice::WriteOnly)) {
        waveDecoder.close();
        audioFile->close();
        QSKIP("Audio format not supported for writing to WAV file.");
    }
    QCOMPARE(waveDecoder.size(), QWaveDecoder::headerLength());

    // Set a large buffer to avoid underruns during QTest::qWaits
    audioSource.setBufferSize(audioFormat.bytesForDuration(100000));

    QIODevice *feed = audioSource.start();

    // Check that QAudioSource immediately transitions to IdleState
    QTRY_VERIFY2((stateSignal.size() == 1),"didn't emit IdleState signal on start()");
    QVERIFY2((audioSource.state() == QAudio::IdleState),
             "didn't transition to IdleState after start()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTest::qWait(40);
    QVERIFY2((audioSource.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");

    qint64 totalBytesRead = 0;
    bool firstBuffer = true;
    qint64 len = audioFormat.sampleRate()*audioFormat.bytesPerFrame()/2; // .5 seconds
    while (totalBytesRead < len) {
        QTRY_VERIFY_WITH_TIMEOUT(audioSource.bytesAvailable() > 0, 1000);
        QByteArray buffer = feed->readAll();
        audioFile->write(buffer);
        totalBytesRead += buffer.size();
        if (firstBuffer && buffer.size()) {
            // Check for transition to ActiveState when data is provided
            QTRY_VERIFY2((stateSignal.size() == 1),"didn't emit ActiveState signal on data");
            QVERIFY2((audioSource.state() == QAudio::ActiveState),
                     "didn't transition to ActiveState after data");
            QVERIFY2((audioSource.error() == QAudio::NoError),
                     "error state is not equal to QAudio::NoError after start()");
            firstBuffer = false;
        }
    }

    stateSignal.clear();

    qint64 processedUs = audioSource.processedUSecs();

    audioSource.stop();
    QTRY_VERIFY2(
            (stateSignal.size() == 1),
            QStringLiteral("didn't emit StoppedState signal after stop(), got %1 signals instead")
                    .arg(stateSignal.size())
                    .toUtf8()
                    .constData());
    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "didn't transitions to StoppedState after stop()");

    QVERIFY2(qTolerantCompare(processedUs, 500000LL),
             QStringLiteral(
                     "processedUSecs() doesn't fall in acceptable range, should be 500000 (%1)")
                     .arg(processedUs)
                     .toUtf8()
                     .constData());
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioSource.elapsedUSecs() == (qint64)0),
             "elapsedUSecs() not equal to zero in StoppedState");

    //WavHeader::writeDataLength(*audioFile,audioFile->pos()-WavHeader::headerLength());
    //waveDecoder.writeDataLength();
    waveDecoder.close();
    audioFile->close();
}

void tst_QAudioSource::pushSuspendResume()
{
#ifdef Q_OS_LINUX
    if (m_inCISystem)
        QSKIP("QTBUG-26504 Fails 20% of time with pulseaudio backend");
#endif
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);
    QAudioSource audioSource(audioFormat, this);

    audioSource.setBufferSize(audioFormat.bytesForDuration(100000));

    QSignalSpy stateSignal(&audioSource, &QAudioSource::stateChanged);

    // Check that we are in the default state before calling start
    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioSource.elapsedUSecs() == qint64(0)), "elapsedUSecs() not zero on creation");

    audioFile->close();
    QTEST_ASSERT(audioFile->open(QIODevice::WriteOnly));
    QWaveDecoder waveDecoder(audioFile.get(), audioFormat);
    if (!waveDecoder.open(QIODevice::WriteOnly)) {
        waveDecoder.close();
        audioFile->close();
        QSKIP("Audio format not supported for writing to WAV file.");
    }
    QCOMPARE(waveDecoder.size(), QWaveDecoder::headerLength());

    QIODevice *feed = audioSource.start();

    // Check that QAudioSource immediately transitions to IdleState
    QTRY_VERIFY2((stateSignal.size() == 1),"didn't emit IdleState signal on start()");
    QVERIFY2((audioSource.state() == QAudio::IdleState),
             "didn't transition to IdleState after start()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTRY_VERIFY2((audioSource.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");

    qint64 totalBytesRead = 0;
    bool firstBuffer = true;
    qint64 len = audioFormat.sampleRate() * audioFormat.bytesPerFrame() / 2; // .5 seconds
    while (totalBytesRead < len) {
        QTRY_VERIFY_WITH_TIMEOUT(audioSource.bytesAvailable() > 0, 1000);
        auto buffer = feed->readAll();
        audioFile->write(buffer);
        totalBytesRead += buffer.size();
        if (firstBuffer && buffer.size()) {
            // Check for transition to ActiveState when data is provided
            QTRY_VERIFY2((stateSignal.size() == 1),"didn't emit ActiveState signal on data");
            QVERIFY2((audioSource.state() == QAudio::ActiveState),
                     "didn't transition to ActiveState after data");
            QVERIFY2((audioSource.error() == QAudio::NoError),
                     "error state is not equal to QAudio::NoError after start()");
            firstBuffer = false;
        }
    }
    stateSignal.clear();

    audioSource.suspend();

    QTRY_VERIFY2(
            (stateSignal.size() == 1),
            QStringLiteral(
                    "didn't emit SuspendedState signal after suspend(), got %1 signals instead")
                    .arg(stateSignal.size())
                    .toUtf8()
                    .constData());
    QVERIFY2((audioSource.state() == QAudio::SuspendedState),
             "didn't transitions to SuspendedState after stop()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error() is not QAudio::NoError after stop()");
    stateSignal.clear();

    // Check that only 'elapsed', and not 'processed' increases while suspended
    qint64 elapsedUs = audioSource.elapsedUSecs();
    qint64 processedUs = audioSource.processedUSecs();
    QTRY_COMPARE_GT(audioSource.elapsedUSecs(), elapsedUs);
    QCOMPARE(audioSource.processedUSecs(), processedUs);

    // Drain any data, in case we run out of space when resuming
    while (feed->readAll().size() > 0)
        ;
    QCOMPARE(audioSource.bytesAvailable(), 0);

    audioSource.resume();

    // Check that QAudioSource immediately transitions to Active or IdleState
    QTRY_VERIFY2((stateSignal.size() > 0),"didn't emit signals on resume()");
    QVERIFY2((audioSource.state() == QAudio::ActiveState
              || audioSource.state() == QAudio::IdleState),
             "didn't transition to ActiveState or IdleState after resume()");
    QVERIFY2((audioSource.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after resume()");

    stateSignal.clear();

    // Read another seconds worth
    totalBytesRead = 0;
    firstBuffer = true;
    while (totalBytesRead < len && audioSource.state() != QAudio::StoppedState) {
        QTRY_VERIFY(audioSource.bytesAvailable() > 0);
        auto buffer = feed->readAll();
        audioFile->write(buffer);
        totalBytesRead += buffer.size();
    }
    stateSignal.clear();

    processedUs = audioSource.processedUSecs();

    audioSource.stop();
    QTRY_VERIFY2(
            (stateSignal.size() == 1),
            QStringLiteral("didn't emit StoppedState signal after stop(), got %1 signals instead")
                    .arg(stateSignal.size())
                    .toUtf8()
                    .constData());
    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "didn't transitions to StoppedState after stop()");

    QVERIFY2(qTolerantCompare(processedUs, 1000000LL),
             QStringLiteral(
                     "processedUSecs() doesn't fall in acceptable range, should be 2040000 (%1)")
                     .arg(processedUs)
                     .toUtf8()
                     .constData());
    QVERIFY2((audioSource.elapsedUSecs() == (qint64)0),
             "elapsedUSecs() not equal to zero in StoppedState");

    //WavHeader::writeDataLength(*audioFile,audioFile->pos()-WavHeader::headerLength());
    //waveDecoder.writeDataLength();
    waveDecoder.close();
    audioFile->close();
}

void tst_QAudioSource::reset()
{
    QFETCH(QAudioFormat, audioFormat);

    // Try both push/pull.. the vagaries of Active vs Idle are tested elsewhere
    {
        QAudioSource audioSource(audioFormat, this);

        QSignalSpy stateSignal(&audioSource, &QAudioSource::stateChanged);

        // Check that we are in the default state before calling start
        QVERIFY2((audioSource.state() == QAudio::StoppedState),
                 "state() was not set to StoppedState before start()");
        QVERIFY2((audioSource.error() == QAudio::NoError),
                 "error() was not set to QAudio::NoError before start()");
        QVERIFY2((audioSource.elapsedUSecs() == qint64(0)), "elapsedUSecs() not zero on creation");

        QIODevice *device = audioSource.start();
        // Check that QAudioSource immediately transitions to IdleState
        QTRY_VERIFY2((stateSignal.size() == 1),"didn't emit IdleState signal on start()");
        QVERIFY2((audioSource.state() == QAudio::IdleState),
                 "didn't transition to IdleState after start()");
        QVERIFY2((audioSource.error() == QAudio::NoError),
                 "error state is not equal to QAudio::NoError after start()");
        QTRY_VERIFY2_WITH_TIMEOUT((audioSource.bytesAvailable() > 0),
                                  "no bytes available after starting", 10000);

        // Trigger a read
        QByteArray data = device->readAll();
        QVERIFY2((audioSource.error() == QAudio::NoError),
                 "error state is not equal to QAudio::NoError after start()");
        stateSignal.clear();

        audioSource.reset();
        QTRY_VERIFY2((stateSignal.size() == 1),"didn't emit StoppedState signal after reset()");
        QVERIFY2((audioSource.state() == QAudio::StoppedState),
                 "didn't transitions to StoppedState after reset()");
        QVERIFY2((audioSource.bytesAvailable() == 0), "buffer not cleared after reset()");
    }

    {
        QAudioSource audioSource(audioFormat, this);
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);

        QSignalSpy stateSignal(&audioSource, &QAudioSource::stateChanged);

        // Check that we are in the default state before calling start
        QVERIFY2((audioSource.state() == QAudio::StoppedState),
                 "state() was not set to StoppedState before start()");
        QVERIFY2((audioSource.error() == QAudio::NoError),
                 "error() was not set to QAudio::NoError before start()");
        QVERIFY2((audioSource.elapsedUSecs() == qint64(0)), "elapsedUSecs() not zero on creation");

        audioSource.start(&buffer);

        // Check that QAudioSource immediately transitions to ActiveState
        QTRY_VERIFY2((stateSignal.size() >= 1),"didn't emit state changed signal on start()");
        QTRY_VERIFY2((audioSource.state() == QAudio::ActiveState),
                     "didn't transition to ActiveState after start()");
        QVERIFY2((audioSource.error() == QAudio::NoError),
                 "error state is not equal to QAudio::NoError after start()");
        stateSignal.clear();

        audioSource.reset();
        QTRY_VERIFY2((stateSignal.size() >= 1),"didn't emit StoppedState signal after reset()");
        QVERIFY2((audioSource.state() == QAudio::StoppedState),
                 "didn't transitions to StoppedState after reset()");
        QVERIFY2((audioSource.bytesAvailable() == 0), "buffer not cleared after reset()");
    }
}

void tst_QAudioSource::volume()
{
    QFETCH(QAudioFormat, audioFormat);

    const qreal half(0.5f);
    const qreal one(1.0f);

    QAudioSource audioSource(audioFormat, this);

    qreal volume = audioSource.volume();
    audioSource.setVolume(half);
    QTRY_VERIFY(qRound(audioSource.volume() * 10.0f) == 5);

    audioSource.setVolume(one);
    QTRY_VERIFY(qRound(audioSource.volume() * 10.0f) == 10);

    audioSource.setVolume(half);
    audioSource.start();
    QTRY_VERIFY(qRound(audioSource.volume() * 10.0f) == 5);
    audioSource.setVolume(one);
    QTRY_VERIFY(qRound(audioSource.volume() * 10.0f) == 10);

    audioSource.setVolume(volume);
}

void tst_QAudioSource::stop_finishesPushMode_whenInvokedUponReadyReadSignal()
{
    const auto defaultAudioInputDevice = QMediaDevices::defaultAudioInput();

    QAudioFormat audioFormat;
    audioFormat.setSampleFormat(QAudioFormat::Int16);
    audioFormat.setSampleRate(qBound(defaultAudioInputDevice.minimumSampleRate(), 48000,
                                     defaultAudioInputDevice.maximumSampleRate()));
    audioFormat.setChannelCount(qBound(defaultAudioInputDevice.minimumChannelCount(), 2,
                                       defaultAudioInputDevice.maximumChannelCount()));

    const auto isFormatSupported = defaultAudioInputDevice.isFormatSupported(audioFormat);
    QCOMPARE(isFormatSupported, true);

    QAudioSource audioSource(audioFormat, this);

    const auto audioInputDevice = audioSource.start();

    auto isReadyReadReceived = false;
    connect(audioInputDevice, &QIODevice::readyRead, this, [&]() {
        audioSource.stop();
        isReadyReadReceived = true;
    });

    const auto awaitedValue = QTest::qWaitFor([&] { return isReadyReadReceived; });
    QVERIFY2(awaitedValue, "didn't receive readyRead signal");

    QVERIFY2((audioSource.state() == QAudio::StoppedState),
             "didn't transitions to StoppedState after close()");
}

void tst_QAudioSource::stop_stopsAudioSource_whenInvokedUponFirstStateChange_data()
{
    QTest::addColumn<AudioSourceInitializer>("initializer");

    AudioSourceInitializer initPullMode = [](QAudioSource &source) {
        QIODevice *device = new MockIODevice(&source);
        device->open(QIODevice::WriteOnly);
        source.start(device);
        return source.error() == QtAudio::NoError;
    };

    AudioSourceInitializer initPushMode = [](QAudioSource &source) {
        QIODevice *device = source.start();
        return device && source.error() == QtAudio::NoError;
    };

    QTest::newRow("pullMode") << initPullMode;
    QTest::newRow("pushMode") << initPushMode;
}

void tst_QAudioSource::stop_stopsAudioSource_whenInvokedUponFirstStateChange()
{
    QFETCH(const AudioSourceInitializer, initializer);

    const QAudioDevice defaultAudioInputDevice = QMediaDevices::defaultAudioInput();

    QAudioFormat audioFormat;
    audioFormat.setSampleFormat(QAudioFormat::Int16);
    audioFormat.setSampleRate(qBound(defaultAudioInputDevice.minimumSampleRate(), 48000,
                                     defaultAudioInputDevice.maximumSampleRate()));
    audioFormat.setChannelCount(qBound(defaultAudioInputDevice.minimumChannelCount(), 2,
                                       defaultAudioInputDevice.maximumChannelCount()));

    QAudioSource audioSource(audioFormat);

    auto stop = [&audioSource]() {
        audioSource.stop();
        QCOMPARE(audioSource.state(), QtAudio::State::StoppedState);
    };

    connect(&audioSource, &QAudioSource::stateChanged, this, stop, Qt::SingleShotConnection);

    if (!initializer(audioSource))
        QSKIP("Cannot start the audio source"); // Pulse audio backend fails on some Linux CI.
                                                // TODO: replace with QVERIFY, QTBUG-130272

    QTRY_COMPARE(audioSource.state(), QtAudio::State::StoppedState);
}

void tst_QAudioSource::stateChanged_stringBasedConnect()
{
    const QAudioDevice defaultAudioInputDevice = QMediaDevices::defaultAudioInput();

    QAudioSource audioSource(defaultAudioInputDevice);

    QSignalSpy stateSignal(&audioSource, SIGNAL(stateChanged(QAudio::State)));

    audioSource.start();
    QTRY_VERIFY(!stateSignal.empty());
}

void tst_QAudioSource::start_withSamplingRate_data()
{
    QTest::addColumn<int>("rate");

    QTest::newRow("minimum") << audioDevice.minimumSampleRate();
    QTest::newRow("preferred") << audioDevice.preferredFormat().sampleRate();
    QTest::newRow("maximum") << audioDevice.maximumSampleRate();
}

void tst_QAudioSource::start_withSamplingRate()
{
    QFETCH(int, rate);

    QAudioFormat format = audioDevice.preferredFormat();
    format.setSampleRate(rate);

    QAudioSource audioSource(format, this);
    audioSource.start();

    QTRY_COMPARE(audioSource.state(), QAudio::State::IdleState);
}

void tst_QAudioSource::callbackAPI()
{
#if QT_CONFIG(thread)
    using namespace std::chrono_literals;

    QAudioFormat format = audioDevice.preferredFormat();
    format.setSampleFormat(QAudioFormat::SampleFormat::Float);

    QAudioSource audioSource(audioDevice, format);
    QPlatformAudioSource *platformSource = QPlatformAudioSource::get(audioSource);
    if (!platformSource->hasCallbackAPI())
        QSKIP("Callback API not supported by this backend");

    QSemaphore sync;

    platformSource->start([&](QSpan<const float> outputBuffer) {
        QCOMPARE_GT(outputBuffer.size(), 0);
        sync.release();
    });
    QCOMPARE(audioSource.error(), QAudio::Error::NoError);

    bool callbackExecuted = sync.try_acquire_for(1s);
    QVERIFY(callbackExecuted);
#endif
}

void tst_QAudioSource::callbackAPI_startFailsWithWrongType()
{
    using namespace std::chrono_literals;

    QAudioFormat format = audioDevice.preferredFormat();
    format.setSampleFormat(QAudioFormat::SampleFormat::Float);

    QAudioSource audioSource(audioDevice, format);
    QPlatformAudioSource *platformSource = QPlatformAudioSource::get(audioSource);
    if (!platformSource->hasCallbackAPI())
        QSKIP("Callback API not supported by this backend");

    platformSource->start([&](QSpan<const int32_t>) {
    });
    QCOMPARE(audioSource.error(), QAudio::Error::OpenError);
}

QTEST_MAIN(tst_QAudioSource)

#include "tst_qaudiosource.moc"
