// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtCore/qsemaphore.h>
#include <QtCore/qtemporarydir.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qaudiosink.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qwavedecoder.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#include <private/audiogenerationutils_p.h>
#include <private/mediabackendutils_p.h>
#include <private/multimedia_debug_support_p.h>
#include <private/osdetection_p.h>
#include <private/qmockiodevice_p.h>

#include <memory>

using AudioSinkInitializer = bool (*)(QAudioSink &);

class AudioPullSource : public QIODevice
{
public:
    AudioPullSource(bool isContinuous = false)
        : available(isContinuous ? std::numeric_limits<int>::max() : 0),
          m_isContinuous(isContinuous)
    {
    }

    qint64 readData(char *data, qint64 len) override
    {
        qint64 read = qMin(len, available);
        if (!m_isContinuous)
            available -= read;
        memset(data, 0, read);
        return read;
    }
    qint64 writeData(const char *, qint64) override { return 0; }
    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return available; }
    bool atEnd() const override { return signalEnd && available == 0; }

    qint64 available = 0;
    bool signalEnd = false;

private:
    bool m_isContinuous;
};

static bool isPipewireBackend()
{
    return QPlatformMediaIntegration::audioBackendName() == "PipeWire";
}

static bool isPulseAudioBackend()
{
    return QPlatformMediaIntegration::audioBackendName() == "PulseAudio";
}

static bool underrunIsAnError()
{
#if defined(Q_OS_APPLE) || defined(Q_OS_ANDROID)
    return false;
#endif

    return !(isPipewireBackend() || isPulseAudioBackend());
}

class tst_QAudioSink : public QObject
{
    Q_OBJECT
public:
    tst_QAudioSink(QObject* parent=nullptr) : QObject(parent) {}

private slots:
    void initTestCase();
    void cleanupTestCase();

    void format();
    void invalidFormat_data();
    void invalidFormat();
    void nullFormat();

    void bufferSize_data();
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
    void pullResumeFromUnderrun();

    void push_data(){generate_audiofile_testrows();}
    void push();

    void pushSuspendResume_data(){generate_audiofile_testrows();}
    void pushSuspendResume();

    void pushResetResume();

    void pushUnderrun_data(){generate_audiofile_testrows();}
    void pushUnderrun();

    void volume_data();
    void volume();

    void stop_stopsAudioSink_whenInvokedUponFirstStateChange_data();
    void stop_stopsAudioSink_whenInvokedUponFirstStateChange();

    void stateChanged_stringBasedConnect();

    void callbackAPI();
    void callbackAPI_startFailsWithWrongType();
    void callbackAPI_startWithMoveOnlyFunctor();

    void multipleSinks_data() { generate_multiple_sinks_testrows(); }
    void multipleSinks();
    void start_afterStopAndReset_data() { generate_multiple_sinks_testrows(); }
    void start_afterStopAndReset();

    void destroy_while_running(); // should be last test to catch crash on exit

private:
    using FilePtr = std::shared_ptr<QFile>;

    static QString formatToFileName(const QAudioFormat &format);
    static QString dumpStateSignalSpy(const QSignalSpy &stateSignalSpy);

    static qint64 wavDataSize(QIODevice &input);

    template<typename Checker>
    static void pushDataToAudioSink(QAudioSink &sink, QIODevice &input, QIODevice &feed,
                                    qint64 &allWritten, qint64 writtenLimit, Checker &&checker,
                                    bool checkOnlyFirst = false);

    void generate_audiofile_testrows();
    void generate_multiple_sinks_testrows();

    QAudioDevice audioDevice;
    std::optional<QAudioDevice> secondDevice;
    QList<QAudioFormat> testFormats;
    QList<FilePtr> audioFiles;
    std::unique_ptr<QTemporaryDir> m_temporaryDir;
};

QString tst_QAudioSink::formatToFileName(const QAudioFormat &format)
{
    return QStringLiteral("%1_%2_%3")
            .arg(format.sampleRate())
            .arg(format.bytesPerSample())
            .arg(format.channelCount());
}

QString tst_QAudioSink::dumpStateSignalSpy(const QSignalSpy& stateSignalSpy) {
    QString result = "[";
    bool first = true;
    for (auto& params : stateSignalSpy)
    {
        if (!std::exchange(first, false))
            result += ',';
        result += qAsQString(params.front().value<QAudio::State>());
    }
    result.append(']');
    return result;
}

qint64 tst_QAudioSink::wavDataSize(QIODevice &input)
{
    return input.size() - QWaveDecoder::headerLength();
}

template<typename Checker>
void tst_QAudioSink::pushDataToAudioSink(QAudioSink &sink, QIODevice &input, QIODevice &feed,
                                         qint64 &allWritten, qint64 writtenLimit, Checker &&checker,
                                         bool checkOnlyFirst)
{
    bool firstBuffer = true;
    qint64 offset = 0;
    QByteArray buffer;

    while ((allWritten < writtenLimit || writtenLimit < 0) && !input.atEnd()
           && !QTest::currentTestFailed()) {
        if (sink.bytesFree() > 0) {
            if (buffer.isNull())
                buffer = input.read(sink.bytesFree());

            const auto written = feed.write(buffer);
            allWritten += written;
            offset += written;

            if (offset >= buffer.size()) {
                offset = 0;
                buffer.clear();
            }

            if (!checkOnlyFirst || firstBuffer)
                checker();

            firstBuffer = false;
        } else {
            // wait a bit to ensure some the sink has consumed some data
            // The delay getting might need some improvements
            const auto delay = qMin(10, sink.format().durationForBytes(sink.bufferSize()) / 1000 / 2);
            QTest::qWait(delay);
        }
    }
}

void tst_QAudioSink::generate_audiofile_testrows()
{
    QTest::addColumn<FilePtr>("audioFile");
    QTest::addColumn<QAudioFormat>("audioFormat");

    for (int i=0; i<audioFiles.size(); i++) {
        QTest::newRow(QStringLiteral("Audio File %1").arg(i).toUtf8().constData())
                << audioFiles.at(i) << testFormats.at(i);
    }
}

void tst_QAudioSink::generate_multiple_sinks_testrows()
{
    QTest::addColumn<QAudioDevice>("firstSinkDevice");
    QTest::addColumn<QAudioDevice>("secondSinkDevice");

    QTest::newRow("Primary device only") << audioDevice << audioDevice;
    if (secondDevice) {
        QTest::newRow("Secondary device started after primary device")
                << audioDevice << *secondDevice;
        QTest::newRow("Primary device started after secondary device")
                << *secondDevice << audioDevice;
        QTest::newRow("Secondary device only") << *secondDevice << *secondDevice;
    }
}

void tst_QAudioSink::initTestCase()
{
    const QList<QAudioDevice> devices = QMediaDevices::audioOutputs();

    // Only perform tests if audio output device exists
    if (devices.isEmpty())
        QSKIP("No audio outputs found");

    audioDevice = QMediaDevices::defaultAudioOutput();

    // Some tests use a second device as well
    if (devices.size() > 1) {
        for (auto device : devices)
            if (!device.isDefault())
                secondDevice = device;
    }

    QAudioFormat format;

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

#ifdef Q_OS_ANDROID
    // Testset crash on emulator x86 with API 23 (Android 6) for 44,1 MHz.
    // It is not happen on x86 with API 24. What is more, there is no crash when
    // tested sample rate is 44,999 or any other value. Seems like problem on
    // emulator side. Let's turn off this frequency for API 23
    if (QNativeInterface::QAndroidApplication::sdkVersion() <= __ANDROID_API_M__)
        testFormats.removeIf([](const QAudioFormat &fmt) {
            return fmt.sampleRate() == 44100;
        });
#endif

    if (testFormats.empty())
        QSKIP("audio devices does not support test format"); // e.g RME Fireface / pipewire

    const QChar slash = QLatin1Char('/');
    QString temporaryPattern = QDir::tempPath();
    if (!temporaryPattern.endsWith(slash))
         temporaryPattern += slash;
    temporaryPattern += "tst_qaudiooutputXXXXXX";
    m_temporaryDir.reset(new QTemporaryDir(temporaryPattern));
    m_temporaryDir->setAutoRemove(true);
    QVERIFY(m_temporaryDir->isValid());

    const QString temporaryAudioPath = m_temporaryDir->path() + slash;
    for (const QAudioFormat &format : std::as_const(testFormats)) {
        QByteArray data = createSineWaveData(format, std::chrono::seconds(1));

        const QString fileName = temporaryAudioPath + QStringLiteral("generated")
                                 + formatToFileName(format) + QStringLiteral(".wav");
        FilePtr file(new QFile(fileName));
        QVERIFY2(file->open(QIODevice::WriteOnly), qPrintable(file->errorString()));
        QWaveDecoder waveDecoder(file.get(), format);
        if (waveDecoder.open(QIODevice::WriteOnly)) {
            waveDecoder.write(data);
            waveDecoder.close();
        }
        file->close();
        audioFiles.append(file);
    }
}

void tst_QAudioSink::cleanupTestCase()
{
    audioFiles.clear();
    testFormats.clear();
}

void tst_QAudioSink::format()
{
    QAudioSink audioSink(audioDevice.preferredFormat(), this);

    QAudioFormat requested = audioDevice.preferredFormat();
    QAudioFormat actual = audioSink.format();

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
    QVERIFY(requested == actual);
}

void tst_QAudioSink::invalidFormat_data()
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

void tst_QAudioSink::invalidFormat()
{
    QFETCH(QAudioFormat, invalidFormat);

    QVERIFY2(!audioDevice.isFormatSupported(invalidFormat),
            "isFormatSupported() is returning true on an invalid format");

    QAudioSink audioSink(invalidFormat, this);

    // Check that we are in the default state before calling start
    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");

    QTest::ignoreMessage(QtWarningMsg, "QAudioSink::start: QAudioFormat not valid");

    audioSink.start();
    // Check that error is raised
    QTRY_VERIFY2((audioSink.error() == QAudio::OpenError),
                 "error() was not set to QAudio::OpenError after start()");
}

void tst_QAudioSink::nullFormat()
{
    QAudioDevice audioDevice = QMediaDevices::defaultAudioOutput();
    if (audioDevice.isNull())
        QSKIP("No audio outputs found");

    {
        QAudioSink audioSink;
        QCOMPARE(audioSink.format(), audioDevice.preferredFormat());
    }
    {
        QAudioSink audioSink(audioDevice);
        QCOMPARE(audioSink.format(), audioDevice.preferredFormat());
    }
}

void tst_QAudioSink::bufferSize_data()
{
    QTest::addColumn<int>("bufferSize");
    QTest::newRow("Buffer size 512") << 512;
    QTest::newRow("Buffer size 4096") << 4096;
    QTest::newRow("Buffer size 8192") << 8192;
}

void tst_QAudioSink::bufferSize()
{
    QFETCH(int, bufferSize);
    QAudioSink audioSink(audioDevice.preferredFormat(), this);

    QVERIFY2((audioSink.error() == QAudio::NoError),
             QStringLiteral("error() was not set to QAudio::NoError on creation(%1)")
                     .arg(bufferSize)
                     .toUtf8()
                     .constData());

    audioSink.setBufferSize(bufferSize);
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() is not QAudio::NoError after setBufferSize");
    QVERIFY2((audioSink.bufferSize() == bufferSize),
             QStringLiteral("bufferSize: requested=%1, actual=%2")
                     .arg(bufferSize)
                     .arg(audioSink.bufferSize())
                     .toUtf8()
                     .constData());
}

void tst_QAudioSink::bufferSize_getValidDefault()
{
#if !(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
    QSKIP("bufferSize validation only fully implemented on Mac and Windows");
#endif

    QAudioSink audioSink(audioDevice.preferredFormat(), this);
    QCOMPARE_GE(audioSink.bufferSize(),
                audioDevice.preferredFormat().bytesForDuration(250'000)); // 250ms
}

void tst_QAudioSink::bufferSize_setAfterStart()
{
#if !(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
    QSKIP("bufferSize validation only fully implemented on Mac and Windows");
#endif

    QAudioSink audioSink(audioDevice.preferredFormat(), this);
    audioSink.start();
    QCOMPARE_GE(audioSink.bufferSize(), audioDevice.preferredFormat().bytesForFrames(32));
}

void tst_QAudioSink::bufferSize_updatedAfterStart()
{
#if !(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
    QSKIP("bufferSize validation only fully implemented on Mac and Windows");
#endif

    QAudioSink audioSink(audioDevice.preferredFormat(), this);
    audioSink.setBufferSize(1); // small enough force a size increase
    audioSink.start();
    QCOMPARE_GE(audioSink.bufferSize(), audioDevice.preferredFormat().bytesForFrames(32));
}

void tst_QAudioSink::stopWhileStopped()
{
    // Calls QAudioSink::stop() when object is already in StoppedState
    // Checks that
    //  - No state change occurs
    //  - No error is raised (QAudioSink::error() returns QAudio::NoError)

    QAudioSink audioSink(audioDevice.preferredFormat(), this);

    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");

    QSignalSpy stateSignal(&audioSink, &QAudioSink::stateChanged);
    audioSink.stop();

    // Check that no state transition occurred
    QVERIFY2((stateSignal.size() == 0), "stop() while stopped is emitting a signal and it shouldn't");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError after stop()");
}

void tst_QAudioSink::suspendWhileStopped()
{
    // Calls QAudioSink::suspend() when object is already in StoppedState
    // Checks that
    //  - No state change occurs
    //  - No error is raised (QAudioSink::error() returns QAudio::NoError)

    QAudioSink audioSink(audioDevice.preferredFormat(), this);

    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");

    QSignalSpy stateSignal(&audioSink, &QAudioSink::stateChanged);
    audioSink.suspend();

    // Check that no state transition occurred
    QVERIFY2((stateSignal.size() == 0), "stop() while suspended is emitting a signal and it shouldn't");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError after stop()");
}

void tst_QAudioSink::resumeWhileStopped()
{
    // Calls QAudioSink::resume() when object is already in StoppedState
    // Checks that
    //  - No state change occurs
    //  - No error is raised (QAudioSink::error() returns QAudio::NoError)

    QAudioSink audioSink(audioDevice.preferredFormat(), this);

    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");

    QSignalSpy stateSignal(&audioSink, &QAudioSink::stateChanged);
    audioSink.resume();

    // Check that no state transition occurred
    QVERIFY2((stateSignal.size() == 0), "resume() while stopped is emitting a signal and it shouldn't");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError after resume()");
}

void tst_QAudioSink::pull()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSink audioSink(audioFormat, this);

    audioSink.setVolume(0.1f);

    QSignalSpy stateSignal(&audioSink, &QAudioSink::stateChanged);

    // Check that we are in the default state before calling start
    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioSink.elapsedUSecs() == qint64(0)), "elapsedUSecs() not zero on creation");

    audioFile->close();
    QVERIFY(audioFile->open(QIODevice::ReadOnly));
    audioFile->seek(QWaveDecoder::headerLength());

    audioSink.start(audioFile.get());

    // Check that QAudioSink immediately transitions to ActiveState
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QStringLiteral("didn't emit signal on start(), got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
    QVERIFY2((audioSink.state() == QAudio::ActiveState),
             "didn't transition to ActiveState after start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTest::qWait(40);
    QVERIFY2((audioSink.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");

    // Wait until playback finishes using a high timeout to avoid flakiness in CI
    QTRY_VERIFY2_WITH_TIMEOUT(audioFile->atEnd(), "didn't play to EOF", 60s);
    QTRY_VERIFY(stateSignal.size() > 0);
    QTRY_COMPARE(qvariant_cast<QAudio::State>(stateSignal.last().at(0)), QAudio::IdleState);
    QVERIFY2((audioSink.state() == QAudio::IdleState),
             "didn't transitions to IdleState when at EOF");
    stateSignal.clear();

    QTRY_COMPARE(audioSink.processedUSecs(), 1000000);

    audioSink.stop();
    QTest::qWait(40);
    QVERIFY2((stateSignal.size() == 1),
             QStringLiteral("didn't emit StoppedState signal after stop(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioSink.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioSink.elapsedUSecs() == (qint64)0),
             "elapsedUSecs() not equal to zero in StoppedState");

    audioFile->close();
}

void tst_QAudioSink::pullSuspendResume()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);
    QAudioSink audioSink(audioFormat, this);

    audioSink.setVolume(0.1f);

    QSignalSpy stateSignal(&audioSink, &QAudioSink::stateChanged);

    // Check that we are in the default state before calling start
    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioSink.elapsedUSecs() == qint64(0)), "elapsedUSecs() not zero on creation");

    audioFile->close();
    QVERIFY(audioFile->open(QIODevice::ReadOnly));
    audioFile->seek(QWaveDecoder::headerLength());

    audioSink.start(audioFile.get());
    // Check that QAudioSink immediately transitions to ActiveState
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QStringLiteral("didn't emit signal on start(), got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
    QVERIFY2((audioSink.state() == QAudio::ActiveState),
             "didn't transition to ActiveState after start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after start()");

    stateSignal.clear();
    // Wait for half of clip to play
    QTest::qWait(500);

    audioSink.suspend();
    QTest::qWait(100);

    QTRY_VERIFY2(
            (stateSignal.size() == 1),
            QStringLiteral(
                    "didn't emit SuspendedState signal after suspend(), got %1 signals instead")
                    .arg(dumpStateSignalSpy(stateSignal))
                    .toUtf8()
                    .constData());
    QVERIFY2((audioSink.state() == QAudio::SuspendedState),
             "didn't transition to SuspendedState after suspend()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after suspend()");
    stateSignal.clear();

    // Check that only 'elapsed', and not 'processed' increases while suspended
    qint64 elapsedUs = audioSink.elapsedUSecs();
    qint64 processedUs = audioSink.processedUSecs();
    QTest::qWait(100);
    QVERIFY(audioSink.elapsedUSecs() > elapsedUs);
    if (isPulseAudioBackend())
        // suspend is asynchronous on pulseaudio, so we give it 50ms to stop
        QCOMPARE_LE(audioSink.processedUSecs(), processedUs + 50'000);
    else
        QCOMPARE(audioSink.processedUSecs(), processedUs);

    audioSink.resume();

    // Check that QAudioSink immediately transitions to ActiveState
    QVERIFY2((stateSignal.size() == 1),
             QStringLiteral("didn't emit signal after resume(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioSink.state() == QAudio::ActiveState),
             "didn't transition to ActiveState after resume()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after resume()");
    stateSignal.clear();

    // Wait until playback finishes
    QTRY_VERIFY2(audioFile->atEnd(), "didn't play to EOF");
    QTRY_VERIFY(stateSignal.size() > 0);
    QTRY_COMPARE(qvariant_cast<QAudio::State>(stateSignal.last().at(0)), QAudio::IdleState);
    QVERIFY2((audioSink.state() == QAudio::IdleState),
             "didn't transitions to IdleState when at EOF");
    stateSignal.clear();

    QTRY_COMPARE(audioSink.processedUSecs(), 1000000);

    audioSink.stop();
    QTest::qWait(40);
    QVERIFY2((stateSignal.size() == 1),
             QStringLiteral("didn't emit StoppedState signal after stop(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioSink.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioSink.elapsedUSecs() == (qint64)0),
             "elapsedUSecs() not equal to zero in StoppedState");

    audioFile->close();
}

void tst_QAudioSink::pullResumeFromUnderrun()
{
    constexpr int chunkSize = 128;

    QAudioDevice output = QMediaDevices::defaultAudioOutput();
    if (output.isNull())
        QSKIP("no audio output detected");

    QAudioFormat format = output.preferredFormat();

    AudioPullSource audioSource;
    QAudioSink audioSink(format, this);
    QSignalSpy stateSignal(&audioSink, &QAudioSink::stateChanged);

    audioSource.open(QIODeviceBase::ReadOnly);
    audioSource.available = chunkSize;
    QCOMPARE(audioSink.state(), QAudio::StoppedState);
    audioSink.start(&audioSource);

    QTRY_COMPARE(stateSignal.size(), 1);
    QCOMPARE(audioSink.state(), QAudio::ActiveState);
    QCOMPARE(audioSink.error(), QAudio::NoError);
    stateSignal.clear();

    QTRY_COMPARE(stateSignal.size(), 1);
    QCOMPARE(audioSink.state(), QAudio::IdleState);
    if (underrunIsAnError())
        QCOMPARE(audioSink.error(), QAudio::UnderrunError);
    stateSignal.clear();

    QTest::qWait(300);
    audioSource.available = chunkSize;
    audioSource.signalEnd = true;

    // Resume pull
    emit audioSource.readyRead();

    QTRY_COMPARE(stateSignal.size(), 2);
    QCOMPARE(stateSignal.at(0).front().value<QAudio::State>(), QAudio::ActiveState);
    QCOMPARE(stateSignal.at(1).front().value<QAudio::State>(), QAudio::IdleState);

    QCOMPARE(audioSink.error(), QAudio::NoError);
    QCOMPARE(audioSink.state(), QAudio::IdleState);

    // we played two chunks, sample rate is per second
    const int expectedUSecs =
            (double(chunkSize) / double(format.sampleRate()) / format.bytesPerFrame()) * 2 * 1000
            * 1000;
    QTRY_COMPARE(audioSink.processedUSecs(), expectedUSecs);
}

void tst_QAudioSink::push()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSink audioSink(audioFormat, this);

    audioSink.setVolume(0.1f);

    QSignalSpy stateSignal(&audioSink, &QAudioSink::stateChanged);

    // Check that we are in the default state before calling start
    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioSink.elapsedUSecs() == qint64(0)), "elapsedUSecs() not zero on creation");

    audioFile->close();
    QVERIFY(audioFile->open(QIODevice::ReadOnly));
    audioFile->seek(QWaveDecoder::headerLength());

    QIODevice *feed = audioSink.start();
    QSignalSpy dataWrittenToDeviceSpy(feed, &QIODevice::bytesWritten);

    // Check that QAudioSink immediately transitions to IdleState
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QStringLiteral("didn't emit signal on start(), got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
    QVERIFY2((audioSink.state() == QAudio::IdleState),
             "didn't transition to IdleState after start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTest::qWait(40);
    QVERIFY2((audioSink.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");
    QVERIFY2((audioSink.processedUSecs() == qint64(0)),
             "processedUSecs() is not zero after start()");

    qint64 written = 0;

    auto checker = [&]() {
        // Check for transition to ActiveState when data is provided
        QVERIFY2((stateSignal.size() == 1),
                 QStringLiteral("didn't emit signal after receiving data, got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
        QVERIFY2((audioSink.state() == QAudio::ActiveState),
                 "didn't transition to ActiveState after receiving data");
        QVERIFY2((audioSink.error() == QAudio::NoError),
                 "error state is not equal to QAudio::NoError after receiving data");
        stateSignal.clear();
    };

    pushDataToAudioSink(audioSink, *audioFile, *feed, written, wavDataSize(*audioFile), checker,
                        true);

    // Wait until playback finishes
    QVERIFY2(audioFile->atEnd(), "didn't play to EOF");
    QTRY_VERIFY(audioSink.state() == QAudio::IdleState);
    QTRY_VERIFY(stateSignal.size() > 0);
    QTRY_COMPARE(qvariant_cast<QAudio::State>(stateSignal.last().at(0)), QAudio::IdleState);
    QVERIFY2((audioSink.state() == QAudio::IdleState),
             "didn't transitions to IdleState when at EOF");
    stateSignal.clear();

    QTRY_COMPARE(audioSink.processedUSecs(), 1000000);

    audioSink.stop();
    QTest::qWait(40);
    QVERIFY2((stateSignal.size() == 1),
             QStringLiteral("didn't emit StoppedState signal after stop(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioSink.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioSink.elapsedUSecs() == (qint64)0),
             "elapsedUSecs() not equal to zero in StoppedState");

    if (isWindows || isMacOS || isPulseAudioBackend() || isPipewireBackend()) {
        QVERIFY(!dataWrittenToDeviceSpy.empty());
        auto allBytesWritten = [&] {
            qint64 total = 0;
            for (const auto &params : dataWrittenToDeviceSpy) {
                total += params.at(0).toLongLong();
            }
            return total;
        }();

        QCOMPARE_EQ(allBytesWritten, audioFile->pos() - QWaveDecoder::headerLength());
    }
}

void tst_QAudioSink::pushSuspendResume()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSink audioSink(audioFormat, this);

    audioSink.setVolume(0.1f);

    QSignalSpy stateSignal(&audioSink, &QAudioSink::stateChanged);

    // Check that we are in the default state before calling start
    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioSink.elapsedUSecs() == qint64(0)), "elapsedUSecs() not zero on creation");

    audioFile->close();
    QVERIFY(audioFile->open(QIODevice::ReadOnly));
    audioFile->seek(QWaveDecoder::headerLength());

    QIODevice *feed = audioSink.start();

    // Check that QAudioSink immediately transitions to IdleState
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QStringLiteral("didn't emit signal on start(), got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
    QVERIFY2((audioSink.state() == QAudio::IdleState),
             "didn't transition to IdleState after start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTest::qWait(40);
    QVERIFY2((audioSink.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");
    QVERIFY2((audioSink.processedUSecs() == qint64(0)),
             "processedUSecs() is not zero after start()");

    auto firstHalfChecker = [&]() {
        QVERIFY2((stateSignal.size() == 1),
                 QStringLiteral("didn't emit signal after receiving data, got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
        QVERIFY2((audioSink.state() == QAudio::ActiveState),
                 "didn't transition to ActiveState after receiving data");
        QVERIFY2((audioSink.error() == QAudio::NoError),
                 "error state is not equal to QAudio::NoError after receiving data");
    };

    qint64 written = 0;
    // Play half of the clip
    pushDataToAudioSink(audioSink, *audioFile, *feed, written, wavDataSize(*audioFile) / 2,
                        firstHalfChecker, true);

    stateSignal.clear();

    const auto suspendedInState = audioSink.state();
    audioSink.suspend();

    QTRY_VERIFY2(
            (stateSignal.size() == 1),
            QStringLiteral(
                    "didn't emit SuspendedState signal after suspend(), got %1 signals instead")
                    .arg(dumpStateSignalSpy(stateSignal))
                    .toUtf8()
                    .constData());
    QVERIFY2((audioSink.state() == QAudio::SuspendedState),
             "didn't transition to SuspendedState after suspend()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after suspend()");
    stateSignal.clear();

    // Check that only 'elapsed', and not 'processed' increases while suspended
    const qint64 elapsedUs = audioSink.elapsedUSecs();
    const qint64 processedUs = audioSink.processedUSecs();
    QTest::qWait(100);
    QCOMPARE_GT(audioSink.elapsedUSecs(), elapsedUs);
    if (isPulseAudioBackend())
        // suspend is asynchronous on pulseaudio, so we give it 50ms to stop
        QCOMPARE_LE(audioSink.processedUSecs(), processedUs + 50'000);
    else
        QCOMPARE(audioSink.processedUSecs(), processedUs);

    audioSink.resume();

    // Give backends running in separate threads a chance to resume
    // but not too much or the rest of the file may be processed
    QTest::qWait(20);

    // Check that QAudioSink immediately transitions to IdleState
    QVERIFY2((stateSignal.size() == 1),
             QStringLiteral("didn't emit signal after resume(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioSink.state() == suspendedInState),
             "resume() didn't transition to state before suspend()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after resume()");
    stateSignal.clear();

    // Play rest of the clip

    auto restChecker = [&]() {
        QVERIFY2((audioSink.state() == QAudio::ActiveState),
                 "didn't transition to ActiveState after writing audio data");
    };

    pushDataToAudioSink(audioSink, *audioFile, *feed, written, -1, restChecker);

    QVERIFY(audioSink.state() != QAudio::IdleState);
    stateSignal.clear();

    QVERIFY2(audioFile->atEnd(), "didn't play to EOF");
    QTRY_VERIFY(stateSignal.size() > 0);
    QTRY_COMPARE(qvariant_cast<QAudio::State>(stateSignal.last().at(0)), QAudio::IdleState);
    QVERIFY2((audioSink.state() == QAudio::IdleState),
             "didn't transitions to IdleState when at EOF");
    stateSignal.clear();

    QTRY_COMPARE(audioSink.processedUSecs(), 1000000);

    audioSink.stop();
    QTest::qWait(40);
    QVERIFY2((stateSignal.size() == 1),
             QStringLiteral("didn't emit StoppedState signal after stop(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioSink.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioSink.elapsedUSecs() == (qint64)0),
             "elapsedUSecs() not equal to zero in StoppedState");

    audioFile->close();
}

void tst_QAudioSink::pushResetResume()
{
    auto audioFile = audioFiles.at(0);
    auto audioFormat = testFormats.at(0);

    QAudioSink audioSink(audioFormat, this);

    audioSink.setBufferSize(8192);
    audioSink.setVolume(0.1f);

    audioFile->close();
    QVERIFY(audioFile->open(QIODevice::ReadOnly));
    audioFile->seek(QWaveDecoder::headerLength());

    QPointer<QIODevice> feed = audioSink.start();

    QTest::qWait(20);

    auto buffer = audioFile->read(audioSink.bytesFree());
    feed->write(buffer);

    QTest::qWait(20);
    QTRY_COMPARE(audioSink.state(), QAudio::ActiveState);

    audioSink.reset();
    QCOMPARE(audioSink.state(), QAudio::StoppedState);
    QCOMPARE(audioSink.error(), QAudio::NoError);

    const auto processedUSecs = audioSink.processedUSecs();

    audioSink.resume();
    QTest::qWait(40);

    // Nothing changed if resume after reset
    QCOMPARE(audioSink.state(), QAudio::StoppedState);
    QCOMPARE(audioSink.error(), QAudio::NoError);

    QCOMPARE(audioSink.processedUSecs(), processedUSecs);
}

void tst_QAudioSink::pushUnderrun()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSink audioSink(audioFormat, this);

    audioSink.setVolume(0.1f);

    QSignalSpy stateSignal(&audioSink, &QAudioSink::stateChanged);

    // Check that we are in the default state before calling start
    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "state() was not set to StoppedState before start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioSink.elapsedUSecs() == qint64(0)), "elapsedUSecs() not zero on creation");

    audioFile->close();
    QVERIFY(audioFile->open(QIODevice::ReadOnly));
    audioFile->seek(QWaveDecoder::headerLength());

    QIODevice *feed = audioSink.start();

    // Check that QAudioSink immediately transitions to IdleState
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QStringLiteral("didn't emit signal on start(), got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
    QVERIFY2((audioSink.state() == QAudio::IdleState),
             "didn't transition to IdleState after start()");
    QVERIFY2((audioSink.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTest::qWait(40);
    QCOMPARE_GT(audioSink.elapsedUSecs(), 0);
    QCOMPARE(audioSink.processedUSecs(), qint64(0));

    qint64 written = 0;

    // Play half of the clip

    auto firstHalfChecker = [&]() {
        QVERIFY2((stateSignal.size() == 1),
                 QStringLiteral("didn't emit signal after receiving data, got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
        QVERIFY2((audioSink.state() == QAudio::ActiveState),
                 "didn't transition to ActiveState after receiving data");
        QVERIFY2((audioSink.error() == QAudio::NoError),
                 "error state is not equal to QAudio::NoError after receiving data");
    };

    pushDataToAudioSink(audioSink, *audioFile, *feed, written, wavDataSize(*audioFile) / 2,
                        firstHalfChecker, true);

    stateSignal.clear();

    // Wait for data to be played
    QTest::qWait(700);

    QVERIFY2((stateSignal.size() == 1),
             QStringLiteral("didn't emit IdleState signal after suspend(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioSink.state() == QAudio::IdleState), "didn't transition to IdleState, no data");
    if (underrunIsAnError())
        QVERIFY2((audioSink.error() == QAudio::UnderrunError),
                 "error state is not equal to QAudio::UnderrunError, no data");
    stateSignal.clear();

    // Play rest of the clip
    auto restChecker = [&]() {
        QVERIFY2((stateSignal.size() == 1),
                 QStringLiteral("didn't emit signal after receiving data, got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
        QVERIFY2((audioSink.state() == QAudio::ActiveState),
                 "didn't transition to ActiveState after receiving data");
        QVERIFY2((audioSink.error() == QAudio::NoError),
                 "error state is not equal to QAudio::NoError after receiving data");
    };
    pushDataToAudioSink(audioSink, *audioFile, *feed, written, -1, restChecker, true);

    stateSignal.clear();

    // Wait until playback finishes
    QVERIFY2(audioFile->atEnd(), "didn't play to EOF");
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QStringLiteral("didn't emit IdleState signal when at EOF, got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
    QVERIFY2((audioSink.state() == QAudio::IdleState),
             "didn't transitions to IdleState when at EOF");
    stateSignal.clear();

    QTRY_COMPARE(audioSink.processedUSecs(), 1000000);

    audioSink.stop();
    QTest::qWait(40);
    QVERIFY2((stateSignal.size() == 1),
             QStringLiteral("didn't emit StoppedState signal after stop(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioSink.state() == QAudio::StoppedState),
             "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioSink.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QCOMPARE(audioSink.elapsedUSecs(), 0);

    audioFile->close();
}

void tst_QAudioSink::volume_data()
{
    QTest::addColumn<float>("actualFloat");
    QTest::addColumn<int>("expectedInt");
    QTest::newRow("Volume 0.3") << 0.3f << 3;
    QTest::newRow("Volume 0.6") << 0.6f << 6;
    QTest::newRow("Volume 0.9") << 0.9f << 9;
}

void tst_QAudioSink::volume()
{
    QFETCH(float, actualFloat);
    QFETCH(int, expectedInt);
    QAudioSink audioSink(audioDevice.preferredFormat(), this);

    audioSink.setVolume(actualFloat);
    QTRY_VERIFY(qRound(audioSink.volume() * 10.0f) == expectedInt);
    // Wait a while to see if this changes
    QTest::qWait(500);
    QTRY_VERIFY(qRound(audioSink.volume() * 10.0f) == expectedInt);
}

void tst_QAudioSink::stop_stopsAudioSink_whenInvokedUponFirstStateChange_data()
{
    QTest::addColumn<AudioSinkInitializer>("initializer");

    AudioSinkInitializer initPullMode = [](QAudioSink &sink) {
        QIODevice *device = new MockIODevice(&sink);
        device->open(QIODevice::ReadOnly);
        sink.start(device);
        return sink.error() == QtAudio::NoError;
    };

    AudioSinkInitializer initPushMode = [](QAudioSink &sink) {
        QIODevice *device = sink.start();
        return device && sink.error() == QtAudio::NoError;
    };

    QTest::newRow("pullMode") << initPullMode;
    QTest::newRow("pushMode") << initPushMode;
}

void tst_QAudioSink::stop_stopsAudioSink_whenInvokedUponFirstStateChange()
{
    QFETCH(const AudioSinkInitializer, initializer);

    QAudioSink audioSink(testFormats.at(0));

    auto stop = [&audioSink]() {
        audioSink.stop();
        QCOMPARE(audioSink.state(), QtAudio::State::StoppedState);
    };

    connect(&audioSink, &QAudioSink::stateChanged, this, stop, Qt::SingleShotConnection);

    if (!initializer(audioSink))
        QSKIP("Cannot start the audio sink"); // Pulse audio backend fails on some Linux CI.
                                              // TODO: replace with QVERIFY

    QTRY_COMPARE(audioSink.state(), QtAudio::State::StoppedState);
}

void tst_QAudioSink::stateChanged_stringBasedConnect()
{
    const QAudioDevice defaultAudioOutputDevice = QMediaDevices::defaultAudioOutput();

    QAudioSink audiosink(defaultAudioOutputDevice);

    QSignalSpy stateSignal(&audiosink, SIGNAL(stateChanged(QAudio::State)));

    audiosink.start();
    QTRY_VERIFY(!stateSignal.empty());
}

void tst_QAudioSink::callbackAPI()
{
#if QT_CONFIG(thread)
    using namespace std::chrono_literals;

    QAudioFormat format = audioDevice.preferredFormat();
    format.setSampleFormat(QAudioFormat::SampleFormat::Float);

    QAudioSink audioSink(audioDevice, format);
    QPlatformAudioSink *platformSink = QPlatformAudioSink::get(audioSink);
    if (!platformSink->hasCallbackAPI())
        QSKIP("Callback API not supported by this backend");

    QSemaphore sync;

    platformSink->start([&](QSpan<float> outputBuffer) {
        QCOMPARE_GT(outputBuffer.size(), 0);
        sync.release();
    });
    QCOMPARE(audioSink.error(), QAudio::Error::NoError);

    bool callbackExecuted = sync.try_acquire_for(1s);
    QVERIFY(callbackExecuted);
#else
    QSKIP("Threading not configured");
#endif
}

void tst_QAudioSink::callbackAPI_startFailsWithWrongType()
{
    using namespace std::chrono_literals;

    QAudioFormat format = audioDevice.preferredFormat();
    format.setSampleFormat(QAudioFormat::SampleFormat::Float);

    QAudioSink audioSink(audioDevice, format);
    QPlatformAudioSink *platformSink = QPlatformAudioSink::get(audioSink);
    if (!platformSink->hasCallbackAPI())
        QSKIP("Callback API not supported by this backend");

    platformSink->start([&](QSpan<int32_t>) {
    });
    QCOMPARE(audioSink.error(), QAudio::Error::OpenError);
}

void tst_QAudioSink::callbackAPI_startWithMoveOnlyFunctor()
{
#if QT_CONFIG(thread) && defined(__cpp_lib_move_only_function)
    using namespace std::chrono_literals;

    QAudioFormat format = audioDevice.preferredFormat();
    format.setSampleFormat(QAudioFormat::SampleFormat::Float);

    QAudioSink audioSink(audioDevice, format);
    QPlatformAudioSink *platformSink = QPlatformAudioSink::get(audioSink);
    if (!platformSink->hasCallbackAPI())
        QSKIP("Callback API not supported by this backend");

    QSemaphore sync;

    platformSink->start([&, dummy = std::make_unique<int>(1)](QSpan<float> outputBuffer) {
        QCOMPARE_GT(outputBuffer.size(), 0);
        sync.release();
    });
    QCOMPARE(audioSink.error(), QAudio::Error::NoError);

    bool callbackExecuted = sync.try_acquire_for(1s);
    QVERIFY(callbackExecuted);
#else
    QSKIP("Threading not configured or move-only functions not available");
#endif
}

void tst_QAudioSink::multipleSinks()
{
    QFETCH(QAudioDevice, firstSinkDevice);
    QFETCH(QAudioDevice, secondSinkDevice);

    auto format1 = firstSinkDevice.preferredFormat();
    auto sink1 = std::make_unique<QAudioSink>(firstSinkDevice, format1, this);
    AudioPullSource source1(true);
    source1.open(QIODeviceBase::ReadOnly);

    auto format2 = secondSinkDevice.preferredFormat();
    auto sink2 = std::make_unique<QAudioSink>(secondSinkDevice, format2, this);
    AudioPullSource source2(true);
    source2.open(QIODeviceBase::ReadOnly);

    sink1->start(&source1);
    QTRY_COMPARE_GT(sink1->processedUSecs(), 0);

    sink2->start(&source2);
    QTRY_COMPARE_GT(sink2->processedUSecs(), 0);

    QTest::qWait(1000);

    // Check both sinks are active
    QCOMPARE(sink1->state(), QAudio::State::ActiveState);
    QCOMPARE(sink2->state(), QAudio::State::ActiveState);
    QCOMPARE(sink1->error(), QAudio::Error::NoError);
    QCOMPARE(sink2->error(), QAudio::Error::NoError);

    // Stop sink1
    sink1->stop();

    // Check sink2 is still active after sink1 stopped
    QTest::qWait(1000);
    QCOMPARE(sink2->state(), QAudio::State::ActiveState);
    QCOMPARE(sink2->error(), QAudio::Error::NoError);
}

void tst_QAudioSink::start_afterStopAndReset()
{
    QFETCH(QAudioDevice, firstSinkDevice);
    QFETCH(QAudioDevice, secondSinkDevice);

    auto format1 = firstSinkDevice.preferredFormat();
    auto sink = std::make_unique<QAudioSink>(firstSinkDevice, format1, this);
    AudioPullSource source1(true);
    source1.open(QIODeviceBase::ReadOnly);
    sink->start(&source1);
    QTRY_COMPARE_GT(sink->processedUSecs(), 0);

    sink->stop();
    // Reset immediately
    auto format2 = secondSinkDevice.preferredFormat();
    sink.reset(new QAudioSink(secondSinkDevice, format2, this));
    AudioPullSource source2(true);
    source2.open(QIODeviceBase::ReadOnly);
    sink->start(&source2);
    QTRY_COMPARE_GT(sink->processedUSecs(), 0);
}

void tst_QAudioSink::destroy_while_running()
{
    AudioPullSource src;
    src.open(QIODeviceBase::ReadOnly);
    src.available = 44100 * 10; // 10 seconds of audio

    QAudioSink sink(audioDevice, audioDevice.preferredFormat());

    sink.start(&src);
}

QTEST_MAIN(tst_QAudioSink)

#include "tst_qaudiosink.moc"
