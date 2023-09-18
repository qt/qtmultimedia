// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QtCore/qlocale.h>
#include <QtCore/QTemporaryDir>
#include <QtCore/QSharedPointer>
#include <QtCore/QScopedPointer>

#include <qaudiosink.h>
#include <qaudiodevice.h>
#include <qaudioformat.h>
#include <qaudio.h>
#include <qmediadevices.h>
#include <qwavedecoder.h>

#define AUDIO_BUFFER 192000

class tst_QAudioSink : public QObject
{
    Q_OBJECT
public:
    tst_QAudioSink(QObject* parent=nullptr) : QObject(parent) {}

private slots:
    void initTestCase();
    void format();
    void invalidFormat_data();
    void invalidFormat();

    void bufferSize_data();
    void bufferSize();

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

private:
    using FilePtr = QSharedPointer<QFile>;

    static QString formatToFileName(const QAudioFormat &format);
    void createSineWaveData(const QAudioFormat &format, qint64 length, int sampleRate = 440);
    static QString dumpStateSignalSpy(const QSignalSpy &stateSignalSpy);

    static qint64 wavDataSize(QIODevice &input);

    template<typename Checker>
    static void pushDataToAudioSink(QAudioSink &sink, QIODevice &input, QIODevice &feed,
                                    qint64 &allWritten, qint64 writtenLimit, Checker &&checker,
                                    bool checkOnlyFirst = false);

    void generate_audiofile_testrows();

    QAudioDevice audioDevice;
    QList<QAudioFormat> testFormats;
    QList<FilePtr> audioFiles;
    QScopedPointer<QTemporaryDir> m_temporaryDir;

    QScopedPointer<QByteArray> m_byteArray;
    QScopedPointer<QBuffer> m_buffer;
};

QString tst_QAudioSink::formatToFileName(const QAudioFormat &format)
{
    return QString("%1_%2_%3")
        .arg(format.sampleRate())
        .arg(format.bytesPerSample())
        .arg(format.channelCount());
}

void tst_QAudioSink::createSineWaveData(const QAudioFormat &format, qint64 length, int sampleRate)
{
    const int channelBytes = format.bytesPerSample();
    const int sampleBytes = format.bytesPerFrame();

    Q_ASSERT(length % sampleBytes == 0);
    Q_UNUSED(sampleBytes); // suppress warning in release builds

    m_byteArray.reset(new QByteArray(length, 0));
    unsigned char *ptr = reinterpret_cast<unsigned char *>(m_byteArray->data());
    int sampleIndex = 0;

    while (length) {
        const qreal x = qSin(2 * M_PI * sampleRate * qreal(sampleIndex % format.sampleRate()) / format.sampleRate());
        for (int i = 0; i < format.channelCount(); ++i) {
            switch (format.sampleFormat()) {
            case QAudioFormat::UInt8: {
                const quint8 value = static_cast<quint8>((1.0 + x) / 2 * 255);
                *reinterpret_cast<quint8 *>(ptr) = value;
                break;
            }
            case QAudioFormat::Int16: {
                qint16 value = static_cast<qint16>(x * 32767);
                *reinterpret_cast<qint16 *>(ptr) = value;
                break;
            }
            case QAudioFormat::Int32: {
                quint32 value = static_cast<quint32>(x) * std::numeric_limits<qint32>::max();
                *reinterpret_cast<qint32 *>(ptr) = value;
                break;
            }
            case QAudioFormat::Float:
                *reinterpret_cast<float *>(ptr) = x;
                break;
            case QAudioFormat::Unknown:
            case QAudioFormat::NSampleFormats:
                break;
            }

            ptr += channelBytes;
            length -= channelBytes;
        }
        ++sampleIndex;
    }

    m_buffer.reset(new QBuffer(m_byteArray.data(), this));
    Q_ASSERT(m_buffer->open(QIODevice::ReadOnly));
}

QString tst_QAudioSink::dumpStateSignalSpy(const QSignalSpy& stateSignalSpy) {
    QString result = "[";
    bool first = true;
    for (auto& params : stateSignalSpy)
    {
        if (!std::exchange(first, false))
            result += ',';
        result += QString::number(params.front().value<QAudio::State>());
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
        QTest::newRow(QString("Audio File %1").arg(i).toUtf8().constData())
                << audioFiles.at(i) << testFormats.at(i);
    }
}

void tst_QAudioSink::initTestCase()
{
    // Only perform tests if audio output device exists
    const QList<QAudioDevice> devices = QMediaDevices::audioOutputs();

    if (devices.size() <= 0)
        QSKIP("No audio backend");

    audioDevice = QMediaDevices::defaultAudioOutput();


    QAudioFormat format;

    if (audioDevice.isFormatSupported(audioDevice.preferredFormat())) {
        if (format.sampleFormat() == QAudioFormat::Int16)
            testFormats.append(audioDevice.preferredFormat());
    }

    // PCM 11025 mono S16LE
    format.setChannelCount(1);
    format.setSampleRate(11025);
    format.setSampleFormat(QAudioFormat::Int16);
    if (audioDevice.isFormatSupported(format))
        testFormats.append(format);

    // PCM 22050 mono S16LE
    format.setSampleRate(22050);
    if (audioDevice.isFormatSupported(format))
        testFormats.append(format);

    // PCM 22050 stereo S16LE
    format.setChannelCount(2);
    if (audioDevice.isFormatSupported(format))
        testFormats.append(format);

    // PCM 44100 stereo S16LE
    format.setSampleRate(44100);
    if (audioDevice.isFormatSupported(format))
        testFormats.append(format);

    // PCM 48000 stereo S16LE
    format.setSampleRate(48000);
    if (audioDevice.isFormatSupported(format))
        testFormats.append(format);

    QVERIFY(testFormats.size());

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
        qint64 len = format.sampleRate()*format.bytesPerFrame(); // 1 second
        createSineWaveData(format, len);
        // Write generate sine wave data to file
        const QString fileName = temporaryAudioPath + QStringLiteral("generated")
                                 + formatToFileName(format) + QStringLiteral(".wav");
        FilePtr file(new QFile(fileName));
        QVERIFY2(file->open(QIODevice::WriteOnly), qPrintable(file->errorString()));
        QWaveDecoder waveDecoder(file.data(), format);
        if (waveDecoder.open(QIODevice::WriteOnly)) {
            waveDecoder.write(m_byteArray->data(), len);
            waveDecoder.close();
        }
        file->close();
        audioFiles.append(file);
    }
}

void tst_QAudioSink::format()
{
    QAudioSink audioOutput(audioDevice.preferredFormat(), this);

    QAudioFormat requested = audioDevice.preferredFormat();
    QAudioFormat actual    = audioOutput.format();

    QVERIFY2((requested.channelCount() == actual.channelCount()),
            QString("channels: requested=%1, actual=%2").arg(requested.channelCount()).arg(actual.channelCount()).toUtf8().constData());
    QVERIFY2((requested.sampleRate() == actual.sampleRate()),
            QString("sampleRate: requested=%1, actual=%2").arg(requested.sampleRate()).arg(actual.sampleRate()).toUtf8().constData());
    QVERIFY2((requested.sampleFormat() == actual.sampleFormat()),
            QString("sampleFormat: requested=%1, actual=%2").arg((ushort)requested.sampleFormat()).arg((ushort)actual.sampleFormat()).toUtf8().constData());
    QVERIFY(requested == actual);
}

void tst_QAudioSink::invalidFormat_data()
{
    QTest::addColumn<QAudioFormat>("invalidFormat");

    QAudioFormat format;

    QTest::newRow("Null Format")
            << format;

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

    QAudioSink audioOutput(invalidFormat, this);

    // Check that we are in the default state before calling start
    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");

    audioOutput.start();
    // Check that error is raised
    QTRY_VERIFY2((audioOutput.error() == QAudio::OpenError),"error() was not set to QAudio::OpenError after start()");
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
    QAudioSink audioOutput(audioDevice.preferredFormat(), this);

    QVERIFY2((audioOutput.error() == QAudio::NoError), QString("error() was not set to QAudio::NoError on creation(%1)").arg(bufferSize).toUtf8().constData());

    audioOutput.setBufferSize(bufferSize);
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() is not QAudio::NoError after setBufferSize");
    QVERIFY2((audioOutput.bufferSize() == bufferSize),
             QString("bufferSize: requested=%1, actual=%2").arg(bufferSize).arg(audioOutput.bufferSize()).toUtf8().constData());
}

void tst_QAudioSink::stopWhileStopped()
{
    // Calls QAudioSink::stop() when object is already in StoppedState
    // Checks that
    //  - No state change occurs
    //  - No error is raised (QAudioSink::error() returns QAudio::NoError)

    QAudioSink audioOutput(audioDevice.preferredFormat(), this);

    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");

    QSignalSpy stateSignal(&audioOutput, SIGNAL(stateChanged(QAudio::State)));
    audioOutput.stop();

    // Check that no state transition occurred
    QVERIFY2((stateSignal.size() == 0), "stop() while stopped is emitting a signal and it shouldn't");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() was not set to QAudio::NoError after stop()");
}

void tst_QAudioSink::suspendWhileStopped()
{
    // Calls QAudioSink::suspend() when object is already in StoppedState
    // Checks that
    //  - No state change occurs
    //  - No error is raised (QAudioSink::error() returns QAudio::NoError)

    QAudioSink audioOutput(audioDevice.preferredFormat(), this);

    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");

    QSignalSpy stateSignal(&audioOutput, SIGNAL(stateChanged(QAudio::State)));
    audioOutput.suspend();

    // Check that no state transition occurred
    QVERIFY2((stateSignal.size() == 0), "stop() while suspended is emitting a signal and it shouldn't");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() was not set to QAudio::NoError after stop()");
}

void tst_QAudioSink::resumeWhileStopped()
{
    // Calls QAudioSink::resume() when object is already in StoppedState
    // Checks that
    //  - No state change occurs
    //  - No error is raised (QAudioSink::error() returns QAudio::NoError)

    QAudioSink audioOutput(audioDevice.preferredFormat(), this);

    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");

    QSignalSpy stateSignal(&audioOutput, SIGNAL(stateChanged(QAudio::State)));
    audioOutput.resume();

    // Check that no state transition occurred
    QVERIFY2((stateSignal.size() == 0), "resume() while stopped is emitting a signal and it shouldn't");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() was not set to QAudio::NoError after resume()");
}

void tst_QAudioSink::pull()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSink audioOutput(audioFormat, this);

    audioOutput.setVolume(0.1f);

    QSignalSpy stateSignal(&audioOutput, SIGNAL(stateChanged(QAudio::State)));

    // Check that we are in the default state before calling start
    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioOutput.elapsedUSecs() == qint64(0)),"elapsedUSecs() not zero on creation");

    audioFile->close();
    audioFile->open(QIODevice::ReadOnly);
    audioFile->seek(QWaveDecoder::headerLength());

    audioOutput.start(audioFile.data());

    // Check that QAudioSink immediately transitions to ActiveState
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QString("didn't emit signal on start(), got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
    QVERIFY2((audioOutput.state() == QAudio::ActiveState), "didn't transition to ActiveState after start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTest::qWait(40);
    QVERIFY2((audioOutput.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");

    // Wait until playback finishes
    QTRY_VERIFY2(audioFile->atEnd(), "didn't play to EOF");
    QTRY_VERIFY(stateSignal.size() > 0);
    QTRY_COMPARE(qvariant_cast<QAudio::State>(stateSignal.last().at(0)), QAudio::IdleState);
    QVERIFY2((audioOutput.state() == QAudio::IdleState), "didn't transitions to IdleState when at EOF");
    stateSignal.clear();

    QTRY_COMPARE(audioOutput.processedUSecs(), 1000000);

    audioOutput.stop();
    QTest::qWait(40);
    QVERIFY2((stateSignal.size() == 1),
             QString("didn't emit StoppedState signal after stop(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioOutput.elapsedUSecs() == (qint64)0), "elapsedUSecs() not equal to zero in StoppedState");

    audioFile->close();
}

void tst_QAudioSink::pullSuspendResume()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);
    QAudioSink audioOutput(audioFormat, this);

    audioOutput.setVolume(0.1f);

    QSignalSpy stateSignal(&audioOutput, SIGNAL(stateChanged(QAudio::State)));

    // Check that we are in the default state before calling start
    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioOutput.elapsedUSecs() == qint64(0)),"elapsedUSecs() not zero on creation");

    audioFile->close();
    audioFile->open(QIODevice::ReadOnly);
    audioFile->seek(QWaveDecoder::headerLength());

    audioOutput.start(audioFile.data());
    // Check that QAudioSink immediately transitions to ActiveState
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QString("didn't emit signal on start(), got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
    QVERIFY2((audioOutput.state() == QAudio::ActiveState), "didn't transition to ActiveState after start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError),
             "error state is not equal to QAudio::NoError after start()");

    stateSignal.clear();
    // Wait for half of clip to play
    QTest::qWait(500);

    audioOutput.suspend();
    QTest::qWait(100);

    QTRY_VERIFY2((stateSignal.size() == 1),
             QString("didn't emit SuspendedState signal after suspend(), got %1 signals instead")
             .arg(dumpStateSignalSpy(stateSignal)).toUtf8().constData());
    QVERIFY2((audioOutput.state() == QAudio::SuspendedState), "didn't transition to SuspendedState after suspend()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after suspend()");
    stateSignal.clear();

    // Check that only 'elapsed', and not 'processed' increases while suspended
    qint64 elapsedUs = audioOutput.elapsedUSecs();
    qint64 processedUs = audioOutput.processedUSecs();
    QTest::qWait(100);
    QVERIFY(audioOutput.elapsedUSecs() > elapsedUs);
    QVERIFY(audioOutput.processedUSecs() == processedUs);

    audioOutput.resume();

    // Check that QAudioSink immediately transitions to ActiveState
    QVERIFY2((stateSignal.size() == 1),
             QString("didn't emit signal after resume(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioOutput.state() == QAudio::ActiveState), "didn't transition to ActiveState after resume()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after resume()");
    stateSignal.clear();

    // Wait until playback finishes
    QTRY_VERIFY2(audioFile->atEnd(), "didn't play to EOF");
    QTRY_VERIFY(stateSignal.size() > 0);
    QTRY_COMPARE(qvariant_cast<QAudio::State>(stateSignal.last().at(0)), QAudio::IdleState);
    QVERIFY2((audioOutput.state() == QAudio::IdleState), "didn't transitions to IdleState when at EOF");
    stateSignal.clear();

    QTRY_COMPARE(audioOutput.processedUSecs(), 1000000);

    audioOutput.stop();
    QTest::qWait(40);
    QVERIFY2((stateSignal.size() == 1),
             QString("didn't emit StoppedState signal after stop(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioOutput.elapsedUSecs() == (qint64)0), "elapsedUSecs() not equal to zero in StoppedState");

    audioFile->close();
}

void tst_QAudioSink::pullResumeFromUnderrun()
{
    class AudioPullSource : public QIODevice
    {
    public:
        qint64 readData(char *data, qint64 len) override {
            qint64 read = qMin(len, available);
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
    };

    constexpr int chunkSize = 128;

    QAudioFormat format;
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::UInt8);
    format.setSampleRate(8000);

    AudioPullSource audioSource;
    QAudioSink audioOutput(format, this);
    QSignalSpy stateSignal(&audioOutput, SIGNAL(stateChanged(QAudio::State)));

    audioSource.open(QIODeviceBase::ReadOnly);
    audioSource.available = chunkSize;
    QCOMPARE(audioOutput.state(), QAudio::StoppedState);
    audioOutput.start(&audioSource);

    QTRY_COMPARE(stateSignal.size(), 1);
    QCOMPARE(audioOutput.state(), QAudio::ActiveState);
    QCOMPARE(audioOutput.error(), QAudio::NoError);
    stateSignal.clear();

    QTRY_COMPARE(stateSignal.size(), 1);
    QCOMPARE(audioOutput.state(), QAudio::IdleState);
    QCOMPARE(audioOutput.error(), QAudio::UnderrunError);
    stateSignal.clear();

    QTest::qWait(300);
    audioSource.available = chunkSize;
    audioSource.signalEnd = true;

    // Resume pull
    emit audioSource.readyRead();

    QTRY_COMPARE(stateSignal.size(), 2);
    QCOMPARE(stateSignal.at(0).front().value<QAudio::State>(), QAudio::ActiveState);
    QCOMPARE(stateSignal.at(1).front().value<QAudio::State>(), QAudio::IdleState);

    QCOMPARE(audioOutput.error(), QAudio::NoError);
    QCOMPARE(audioOutput.state(), QAudio::IdleState);

    // we played two chunks, sample rate is per second
    const int expectedUSecs = (double(chunkSize) / double(format.sampleRate()))
                            * 2 * 1000 * 1000;
    QTRY_COMPARE(audioOutput.processedUSecs(), expectedUSecs);
}

void tst_QAudioSink::push()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSink audioOutput(audioFormat, this);

    audioOutput.setVolume(0.1f);

    QSignalSpy stateSignal(&audioOutput, SIGNAL(stateChanged(QAudio::State)));

    // Check that we are in the default state before calling start
    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioOutput.elapsedUSecs() == qint64(0)),"elapsedUSecs() not zero on creation");

    audioFile->close();
    audioFile->open(QIODevice::ReadOnly);
    audioFile->seek(QWaveDecoder::headerLength());

    QIODevice* feed = audioOutput.start();

    // Check that QAudioSink immediately transitions to IdleState
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QString("didn't emit signal on start(), got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
    QVERIFY2((audioOutput.state() == QAudio::IdleState), "didn't transition to IdleState after start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTest::qWait(40);
    QVERIFY2((audioOutput.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");
    QVERIFY2((audioOutput.processedUSecs() == qint64(0)), "processedUSecs() is not zero after start()");

    qint64 written = 0;

    auto checker = [&]() {
        // Check for transition to ActiveState when data is provided
        QVERIFY2((stateSignal.size() == 1),
                 QString("didn't emit signal after receiving data, got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
        QVERIFY2((audioOutput.state() == QAudio::ActiveState),
                 "didn't transition to ActiveState after receiving data");
        QVERIFY2((audioOutput.error() == QAudio::NoError),
                 "error state is not equal to QAudio::NoError after receiving data");
        stateSignal.clear();
    };

    pushDataToAudioSink(audioOutput, *audioFile, *feed, written, wavDataSize(*audioFile), checker,
                        true);

    // Wait until playback finishes
    QVERIFY2(audioFile->atEnd(), "didn't play to EOF");
    QTRY_VERIFY(audioOutput.state() == QAudio::IdleState);
    QTRY_VERIFY(stateSignal.size() > 0);
    QTRY_COMPARE(qvariant_cast<QAudio::State>(stateSignal.last().at(0)), QAudio::IdleState);
    QVERIFY2((audioOutput.state() == QAudio::IdleState), "didn't transitions to IdleState when at EOF");
    stateSignal.clear();

    QTRY_COMPARE(audioOutput.processedUSecs(), 1000000);

    audioOutput.stop();
    QTest::qWait(40);
    QVERIFY2((stateSignal.size() == 1),
             QString("didn't emit StoppedState signal after stop(), got %1 signals instead").arg(dumpStateSignalSpy(stateSignal)).toUtf8().constData());
    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioOutput.elapsedUSecs() == (qint64)0), "elapsedUSecs() not equal to zero in StoppedState");

    audioFile->close();
}

void tst_QAudioSink::pushSuspendResume()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSink audioOutput(audioFormat, this);

    audioOutput.setVolume(0.1f);

    QSignalSpy stateSignal(&audioOutput, SIGNAL(stateChanged(QAudio::State)));

    // Check that we are in the default state before calling start
    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioOutput.elapsedUSecs() == qint64(0)),"elapsedUSecs() not zero on creation");

    audioFile->close();
    audioFile->open(QIODevice::ReadOnly);
    audioFile->seek(QWaveDecoder::headerLength());

    QIODevice* feed = audioOutput.start();

    // Check that QAudioSink immediately transitions to IdleState
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QString("didn't emit signal on start(), got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
    QVERIFY2((audioOutput.state() == QAudio::IdleState), "didn't transition to IdleState after start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTest::qWait(40);
    QVERIFY2((audioOutput.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");
    QVERIFY2((audioOutput.processedUSecs() == qint64(0)), "processedUSecs() is not zero after start()");

    auto firstHalfChecker = [&]() {
        QVERIFY2((stateSignal.size() == 1),
                 QString("didn't emit signal after receiving data, got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
        QVERIFY2((audioOutput.state() == QAudio::ActiveState),
                 "didn't transition to ActiveState after receiving data");
        QVERIFY2((audioOutput.error() == QAudio::NoError),
                 "error state is not equal to QAudio::NoError after receiving data");
    };

    qint64 written = 0;
    // Play half of the clip
    pushDataToAudioSink(audioOutput, *audioFile, *feed, written, wavDataSize(*audioFile) / 2,
                        firstHalfChecker, true);

    stateSignal.clear();

    const auto suspendedInState = audioOutput.state();
    audioOutput.suspend();

    QTRY_VERIFY2((stateSignal.size() == 1),
             QString("didn't emit SuspendedState signal after suspend(), got %1 signals instead")
             .arg(dumpStateSignalSpy(stateSignal)).toUtf8().constData());
    QVERIFY2((audioOutput.state() == QAudio::SuspendedState), "didn't transition to SuspendedState after suspend()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after suspend()");
    stateSignal.clear();

    // Check that only 'elapsed', and not 'processed' increases while suspended
    qint64 elapsedUs = audioOutput.elapsedUSecs();
    qint64 processedUs = audioOutput.processedUSecs();
    QTest::qWait(100);
    QVERIFY(audioOutput.elapsedUSecs() > elapsedUs);
    QVERIFY(audioOutput.processedUSecs() == processedUs);

    audioOutput.resume();

    // Give backends running in separate threads a chance to resume
    // but not too much or the rest of the file may be processed
    QTest::qWait(20);

    // Check that QAudioSink immediately transitions to IdleState
    QVERIFY2((stateSignal.size() == 1),
             QString("didn't emit signal after resume(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioOutput.state() == suspendedInState), "resume() didn't transition to state before suspend()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after resume()");
    stateSignal.clear();

    // Play rest of the clip

    auto restChecker = [&]() {
        QVERIFY2((audioOutput.state() == QAudio::ActiveState),
                 "didn't transition to ActiveState after writing audio data");
    };

    pushDataToAudioSink(audioOutput, *audioFile, *feed, written, -1, restChecker);

    QVERIFY(audioOutput.state() != QAudio::IdleState);
    stateSignal.clear();

    QVERIFY2(audioFile->atEnd(), "didn't play to EOF");
    QTRY_VERIFY(stateSignal.size() > 0);
    QTRY_COMPARE(qvariant_cast<QAudio::State>(stateSignal.last().at(0)), QAudio::IdleState);
    QVERIFY2((audioOutput.state() == QAudio::IdleState), "didn't transitions to IdleState when at EOF");
    stateSignal.clear();

    QTRY_COMPARE(audioOutput.processedUSecs(), 1000000);

    audioOutput.stop();
    QTest::qWait(40);
    QVERIFY2((stateSignal.size() == 1),
             QString("didn't emit StoppedState signal after stop(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioOutput.elapsedUSecs() == (qint64)0), "elapsedUSecs() not equal to zero in StoppedState");

    audioFile->close();
}

void tst_QAudioSink::pushResetResume()
{
    auto audioFile = audioFiles.at(0);
    auto audioFormat = testFormats.at(0);

    QAudioSink audioOutput(audioFormat, this);

    audioOutput.setBufferSize(8192);
    audioOutput.setVolume(0.1f);

    audioFile->close();
    audioFile->open(QIODevice::ReadOnly);
    audioFile->seek(QWaveDecoder::headerLength());

    QPointer<QIODevice> feed = audioOutput.start();

    QTest::qWait(20);

    auto buffer = audioFile->read(audioOutput.bytesFree());
    feed->write(buffer);

    QTest::qWait(20);
    QTRY_COMPARE(audioOutput.state(), QAudio::ActiveState);

    audioOutput.reset();
    QCOMPARE(audioOutput.state(), QAudio::StoppedState);
    QCOMPARE(audioOutput.error(), QAudio::NoError);

    const auto processedUSecs = audioOutput.processedUSecs();

    audioOutput.resume();
    QTest::qWait(40);

    // Nothing changed if resume after reset
    QCOMPARE(audioOutput.state(), QAudio::StoppedState);
    QCOMPARE(audioOutput.error(), QAudio::NoError);

    QCOMPARE(audioOutput.processedUSecs(), processedUSecs);
}

void tst_QAudioSink::pushUnderrun()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSink audioOutput(audioFormat, this);

    audioOutput.setVolume(0.1f);

    QSignalSpy stateSignal(&audioOutput, SIGNAL(stateChanged(QAudio::State)));

    // Check that we are in the default state before calling start
    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioOutput.elapsedUSecs() == qint64(0)),"elapsedUSecs() not zero on creation");

    audioFile->close();
    audioFile->open(QIODevice::ReadOnly);
    audioFile->seek(QWaveDecoder::headerLength());

    QIODevice* feed = audioOutput.start();

    // Check that QAudioSink immediately transitions to IdleState
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QString("didn't emit signal on start(), got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
    QVERIFY2((audioOutput.state() == QAudio::IdleState), "didn't transition to IdleState after start()");
    QVERIFY2((audioOutput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTest::qWait(40);
    QVERIFY2((audioOutput.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");
    QVERIFY2((audioOutput.processedUSecs() == qint64(0)), "processedUSecs() is not zero after start()");

    qint64 written = 0;

    // Play half of the clip

    auto firstHalfChecker = [&]() {
        QVERIFY2((stateSignal.size() == 1),
                 QString("didn't emit signal after receiving data, got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
        QVERIFY2((audioOutput.state() == QAudio::ActiveState),
                 "didn't transition to ActiveState after receiving data");
        QVERIFY2((audioOutput.error() == QAudio::NoError),
                 "error state is not equal to QAudio::NoError after receiving data");
    };

    pushDataToAudioSink(audioOutput, *audioFile, *feed, written, wavDataSize(*audioFile) / 2,
                        firstHalfChecker, true);

    stateSignal.clear();

    // Wait for data to be played
    QTest::qWait(700);

    QVERIFY2((stateSignal.size() == 1),
             QString("didn't emit IdleState signal after suspend(), got %1 signals instead")
             .arg(dumpStateSignalSpy(stateSignal)).toUtf8().constData());
    QVERIFY2((audioOutput.state() == QAudio::IdleState), "didn't transition to IdleState, no data");
    QVERIFY2((audioOutput.error() == QAudio::UnderrunError), "error state is not equal to QAudio::UnderrunError, no data");
    stateSignal.clear();

    // Play rest of the clip
    auto restChecker = [&]() {
        QVERIFY2((stateSignal.size() == 1),
                 QString("didn't emit signal after receiving data, got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
        QVERIFY2((audioOutput.state() == QAudio::ActiveState),
                 "didn't transition to ActiveState after receiving data");
        QVERIFY2((audioOutput.error() == QAudio::NoError),
                 "error state is not equal to QAudio::NoError after receiving data");
    };
    pushDataToAudioSink(audioOutput, *audioFile, *feed, written, -1, restChecker, true);

    stateSignal.clear();

    // Wait until playback finishes
    QVERIFY2(audioFile->atEnd(), "didn't play to EOF");
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QString("didn't emit IdleState signal when at EOF, got %1 signals instead")
                         .arg(dumpStateSignalSpy(stateSignal))
                         .toUtf8()
                         .constData());
    QVERIFY2((audioOutput.state() == QAudio::IdleState), "didn't transitions to IdleState when at EOF");
    stateSignal.clear();

    QTRY_COMPARE(audioOutput.processedUSecs(), 1000000);

    audioOutput.stop();
    QTest::qWait(40);
    QVERIFY2((stateSignal.size() == 1),
             QString("didn't emit StoppedState signal after stop(), got %1 signals instead")
                     .arg(dumpStateSignalSpy(stateSignal))
                     .toUtf8()
                     .constData());
    QVERIFY2((audioOutput.state() == QAudio::StoppedState), "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioOutput.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioOutput.elapsedUSecs() == (qint64)0), "elapsedUSecs() not equal to zero in StoppedState");

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
    QAudioSink audioOutput(audioDevice.preferredFormat(), this);

    audioOutput.setVolume(actualFloat);
    QTRY_VERIFY(qRound(audioOutput.volume()*10.0f) == expectedInt);
    // Wait a while to see if this changes
    QTest::qWait(500);
    QTRY_VERIFY(qRound(audioOutput.volume()*10.0f) == expectedInt);
}

QTEST_MAIN(tst_QAudioSink)

#include "tst_qaudiosink.moc"
