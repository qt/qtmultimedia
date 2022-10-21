// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QtCore/qlocale.h>
#include <QtCore/QTemporaryDir>
#include <QtCore/QSharedPointer>
#include <QtCore/QScopedPointer>

#include <qaudiosource.h>
#include <qaudiodevice.h>
#include <qaudioformat.h>
#include <qaudio.h>
#include <qmediadevices.h>

#include <qwavedecoder.h>

//TESTED_COMPONENT=src/multimedia

#define RANGE_ERR 0.5

template<typename T> inline bool qTolerantCompare(T value, T expected)
{
    return qAbs(value - expected) < (RANGE_ERR * expected);
}

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

    void bufferSize();

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

private:
    using FilePtr = QSharedPointer<QFile>;

    QString formatToFileName(const QAudioFormat &format);

    void generate_audiofile_testrows();

    QAudioDevice audioDevice;
    QList<QAudioFormat> testFormats;
    QList<FilePtr> audioFiles;
    QScopedPointer<QTemporaryDir> m_temporaryDir;

    QScopedPointer<QByteArray> m_byteArray;
    QScopedPointer<QBuffer> m_buffer;

    bool m_inCISystem = false;
};

void tst_QAudioSource::generate_audiofile_testrows()
{
    QTest::addColumn<FilePtr>("audioFile");
    QTest::addColumn<QAudioFormat>("audioFormat");

    for (int i=0; i<audioFiles.size(); i++) {
        QTest::newRow(QString("%1").arg(i).toUtf8().constData())
                << audioFiles.at(i) << testFormats.at(i);

        // Only run first format in CI system to reduce test times
        if (m_inCISystem)
            break;
    }
}

QString tst_QAudioSource::formatToFileName(const QAudioFormat &format)
{
    return QString("%1_%2_%3")
        .arg(format.sampleRate())
        .arg(format.bytesPerSample())
        .arg(format.channelCount());
}

void tst_QAudioSource::initTestCase()
{
    m_inCISystem = qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci";

    if (m_inCISystem)
        QSKIP("SKIP initTestCase on CI. To be fixed");

    // Only perform tests if audio output device exists
    const QList<QAudioDevice> devices = QMediaDevices::audioOutputs();

    if (devices.size() <= 0)
        QSKIP("No audio backend");

    audioDevice = QMediaDevices::defaultAudioInput();


    QAudioFormat format;
    format.setChannelCount(1);

    if (audioDevice.isFormatSupported(audioDevice.preferredFormat())) {
        if (format.sampleFormat() == QAudioFormat::Int16)
            testFormats.append(audioDevice.preferredFormat());
    }

    // PCM 11025 mono S16LE
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
    temporaryPattern += "tst_qaudioinputXXXXXX";
    m_temporaryDir.reset(new QTemporaryDir(temporaryPattern));
    m_temporaryDir->setAutoRemove(true);
    QVERIFY(m_temporaryDir->isValid());

    const QString temporaryAudioPath = m_temporaryDir->path() + slash;
    for (const QAudioFormat &format : std::as_const(testFormats)) {
        const QString fileName = temporaryAudioPath + formatToFileName(format) + QStringLiteral(".wav");
        audioFiles.append(FilePtr::create(fileName));
    }
}

void tst_QAudioSource::format()
{
    QAudioSource audioInput(audioDevice.preferredFormat(), this);

    QAudioFormat requested = audioDevice.preferredFormat();
    QAudioFormat actual    = audioInput.format();

    QVERIFY2((requested.channelCount() == actual.channelCount()),
            QString("channels: requested=%1, actual=%2").arg(requested.channelCount()).arg(actual.channelCount()).toUtf8().constData());
    QVERIFY2((requested.sampleRate() == actual.sampleRate()),
            QString("sampleRate: requested=%1, actual=%2").arg(requested.sampleRate()).arg(actual.sampleRate()).toUtf8().constData());
    QVERIFY2((requested.sampleFormat() == actual.sampleFormat()),
            QString("sampleFormat: requested=%1, actual=%2").arg((ushort)requested.sampleFormat()).arg((ushort)actual.sampleFormat()).toUtf8().constData());
    QCOMPARE(actual, requested);
}

void tst_QAudioSource::invalidFormat_data()
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

void tst_QAudioSource::invalidFormat()
{
    QFETCH(QAudioFormat, invalidFormat);

    QVERIFY2(!audioDevice.isFormatSupported(invalidFormat),
            "isFormatSupported() is returning true on an invalid format");

    QAudioSource audioInput(invalidFormat, this);

    // Check that we are in the default state before calling start
    QVERIFY2((audioInput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");

    audioInput.start();

    // Check that error is raised
    QTRY_VERIFY2((audioInput.error() == QAudio::OpenError),"error() was not set to QAudio::OpenError after start()");
}

void tst_QAudioSource::bufferSize()
{
    QAudioSource audioInput(audioDevice.preferredFormat(), this);

    QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError on creation");

    audioInput.setBufferSize(512);
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() is not QAudio::NoError after setBufferSize(512)");
    QVERIFY2((audioInput.bufferSize() == 512),
            QString("bufferSize: requested=512, actual=%2").arg(audioInput.bufferSize()).toUtf8().constData());

    audioInput.setBufferSize(4096);
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() is not QAudio::NoError after setBufferSize(4096)");
    QVERIFY2((audioInput.bufferSize() == 4096),
            QString("bufferSize: requested=4096, actual=%2").arg(audioInput.bufferSize()).toUtf8().constData());

    audioInput.setBufferSize(8192);
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() is not QAudio::NoError after setBufferSize(8192)");
    QVERIFY2((audioInput.bufferSize() == 8192),
            QString("bufferSize: requested=8192, actual=%2").arg(audioInput.bufferSize()).toUtf8().constData());
}

void tst_QAudioSource::stopWhileStopped()
{
    // Calls QAudioSource::stop() when object is already in StoppedState
    // Checks that
    //  - No state change occurs
    //  - No error is raised (QAudioSource::error() returns QAudio::NoError)

    QAudioSource audioInput(audioDevice.preferredFormat(), this);

    QVERIFY2((audioInput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");

    QSignalSpy stateSignal(&audioInput, SIGNAL(stateChanged(QAudio::State)));
    audioInput.stop();

    // Check that no state transition occurred
    QVERIFY2((stateSignal.size() == 0), "stop() while stopped is emitting a signal and it shouldn't");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError after stop()");
}

void tst_QAudioSource::suspendWhileStopped()
{
    // Calls QAudioSource::suspend() when object is already in StoppedState
    // Checks that
    //  - No state change occurs
    //  - No error is raised (QAudioSource::error() returns QAudio::NoError)

    QAudioSource audioInput(audioDevice.preferredFormat(), this);

    QVERIFY2((audioInput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");

    QSignalSpy stateSignal(&audioInput, SIGNAL(stateChanged(QAudio::State)));
    audioInput.suspend();

    // Check that no state transition occurred
    QVERIFY2((stateSignal.size() == 0), "stop() while suspended is emitting a signal and it shouldn't");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError after stop()");
}

void tst_QAudioSource::resumeWhileStopped()
{
    // Calls QAudioSource::resume() when object is already in StoppedState
    // Checks that
    //  - No state change occurs
    //  - No error is raised (QAudioSource::error() returns QAudio::NoError)

    QAudioSource audioInput(audioDevice.preferredFormat(), this);

    QVERIFY2((audioInput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");

    QSignalSpy stateSignal(&audioInput, SIGNAL(stateChanged(QAudio::State)));
    audioInput.resume();

    // Check that no state transition occurred
    QVERIFY2((stateSignal.size() == 0), "resume() while stopped is emitting a signal and it shouldn't");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError after resume()");
}

void tst_QAudioSource::pull()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSource audioInput(audioFormat, this);

    QSignalSpy stateSignal(&audioInput, SIGNAL(stateChanged(QAudio::State)));

    // Check that we are in the default state before calling start
    QVERIFY2((audioInput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioInput.elapsedUSecs() == qint64(0)),"elapsedUSecs() not zero on creation");

    audioFile->close();
    audioFile->open(QIODevice::WriteOnly);
    QWaveDecoder waveDecoder(audioFile.data(), audioFormat);
    if (!waveDecoder.open(QIODevice::WriteOnly)) {
        waveDecoder.close();
        audioFile->close();
        QSKIP("Audio format not supported for writing to WAV file.");
    }
    QCOMPARE(waveDecoder.size(), QWaveDecoder::headerLength());

    audioInput.start(audioFile.data());

    // Check that QAudioSource immediately transitions to ActiveState or IdleState
    QTRY_VERIFY2((stateSignal.size() > 0),"didn't emit signals on start()");
    QVERIFY2((audioInput.state() == QAudio::ActiveState || audioInput.state() == QAudio::IdleState),
             "didn't transition to ActiveState or IdleState after start()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTRY_VERIFY2((audioInput.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");
    QTRY_VERIFY2((audioInput.processedUSecs() > 0), "elapsedUSecs() is still zero after start()");

    // Allow some recording to happen
    QTest::qWait(300); // .3 seconds should be plenty

    stateSignal.clear();

    qint64 processedUs = audioInput.processedUSecs();
    QVERIFY2(qTolerantCompare(processedUs, 300000LL),
             QString("processedUSecs() doesn't fall in acceptable range, should be 300000 (%1)").arg(processedUs).toUtf8().constData());

    audioInput.stop();
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QString("didn't emit StoppedState signal after stop(), got %1 signals instead").arg(stateSignal.size()).toUtf8().constData());
    QVERIFY2((audioInput.state() == QAudio::StoppedState), "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioInput.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioInput.elapsedUSecs() == (qint64)0), "elapsedUSecs() not equal to zero in StoppedState");

    //QWaveHeader::writeDataLength(*audioFile, audioFile->pos() - WavHeader::headerLength());
    //waveDecoder.writeDataLength();
    waveDecoder.close();
    audioFile->close();

}

void tst_QAudioSource::pullSuspendResume()
{
#ifdef Q_OS_LINUX
    if (m_inCISystem)
        QSKIP("QTBUG-26504 Fails 20% of time with pulseaudio backend");
#endif
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSource audioInput(audioFormat, this);

    QSignalSpy stateSignal(&audioInput, SIGNAL(stateChanged(QAudio::State)));

    // Check that we are in the default state before calling start
    QVERIFY2((audioInput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioInput.elapsedUSecs() == qint64(0)),"elapsedUSecs() not zero on creation");

    audioFile->close();
    audioFile->open(QIODevice::WriteOnly);
    QWaveDecoder waveDecoder(audioFile.get(), audioFormat);
    if (!waveDecoder.open(QIODevice::WriteOnly)) {
        waveDecoder.close();
        audioFile->close();
        QSKIP("Audio format not supported for writing to WAV file.");
    }
    QCOMPARE(waveDecoder.size(), QWaveDecoder::headerLength());

    audioInput.start(audioFile.data());

    // Check that QAudioSource immediately transitions to ActiveState or IdleState
    QTRY_VERIFY2((stateSignal.size() > 0),"didn't emit signals on start()");
    QVERIFY2((audioInput.state() == QAudio::ActiveState || audioInput.state() == QAudio::IdleState),
             "didn't transition to ActiveState or IdleState after start()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTRY_VERIFY2((audioInput.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");
    QTRY_VERIFY2((audioInput.processedUSecs() > 0), "elapsedUSecs() is still zero after start()");

    // Allow some recording to happen
    QTest::qWait(300); // .3 seconds should be plenty

    QVERIFY2((audioInput.state() == QAudio::ActiveState),
             "didn't transition to ActiveState after some recording");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after some recording");

    stateSignal.clear();

    audioInput.suspend();

    QTRY_VERIFY2((stateSignal.size() == 1),
             QString("didn't emit SuspendedState signal after suspend(), got %1 signals instead").arg(stateSignal.size()).toUtf8().constData());
    QVERIFY2((audioInput.state() == QAudio::SuspendedState), "didn't transitions to SuspendedState after stop()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    stateSignal.clear();

    // Check that only 'elapsed', and not 'processed' increases while suspended
    qint64 elapsedUs = audioInput.elapsedUSecs();
    qint64 processedUs = audioInput.processedUSecs();
    QVERIFY2(qTolerantCompare(processedUs, 300000LL),
             QString("processedUSecs() doesn't fall in acceptable range, should be 300000 (%1)").arg(processedUs).toUtf8().constData());
    QTRY_VERIFY(audioInput.elapsedUSecs() > elapsedUs);
    QVERIFY(audioInput.processedUSecs() == processedUs);

    audioInput.resume();

    // Check that QAudioSource immediately transitions to ActiveState
    QTRY_VERIFY2((stateSignal.size() == 1),
             QString("didn't emit signal after resume(), got %1 signals instead").arg(stateSignal.size()).toUtf8().constData());
    QVERIFY2((audioInput.state() == QAudio::ActiveState), "didn't transition to ActiveState after resume()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after resume()");
    stateSignal.clear();

    audioInput.stop();
    QTest::qWait(40);
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QString("didn't emit StoppedState signal after stop(), got %1 signals instead").arg(stateSignal.size()).toUtf8().constData());
    QVERIFY2((audioInput.state() == QAudio::StoppedState), "didn't transitions to StoppedState after stop()");

    QVERIFY2((audioInput.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioInput.elapsedUSecs() == (qint64)0), "elapsedUSecs() not equal to zero in StoppedState");

    //WavHeader::writeDataLength(*audioFile,audioFile->pos()-WavHeader::headerLength());
    //waveDecoder.writeDataLength();
    waveDecoder.close();
    audioFile->close();
}

void tst_QAudioSource::push()
{
    QFETCH(FilePtr, audioFile);
    QFETCH(QAudioFormat, audioFormat);

    QAudioSource audioInput(audioFormat, this);

    QSignalSpy stateSignal(&audioInput, SIGNAL(stateChanged(QAudio::State)));

    // Check that we are in the default state before calling start
    QVERIFY2((audioInput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioInput.elapsedUSecs() == qint64(0)),"elapsedUSecs() not zero on creation");

    audioFile->close();
    audioFile->open(QIODevice::WriteOnly);
    QWaveDecoder waveDecoder(audioFile.get(), audioFormat);
    if (!waveDecoder.open(QIODevice::WriteOnly)) {
        waveDecoder.close();
        audioFile->close();
        QSKIP("Audio format not supported for writing to WAV file.");
    }
    QCOMPARE(waveDecoder.size(), QWaveDecoder::headerLength());

    // Set a large buffer to avoid underruns during QTest::qWaits
    audioInput.setBufferSize(audioFormat.bytesForDuration(100000));

    QIODevice* feed = audioInput.start();

    // Check that QAudioSource immediately transitions to IdleState
    QTRY_VERIFY2((stateSignal.size() == 1),"didn't emit IdleState signal on start()");
    QVERIFY2((audioInput.state() == QAudio::IdleState),
             "didn't transition to IdleState after start()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTest::qWait(40);
    QVERIFY2((audioInput.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");

    qint64 totalBytesRead = 0;
    bool firstBuffer = true;
    qint64 len = audioFormat.sampleRate()*audioFormat.bytesPerFrame()/2; // .5 seconds
    while (totalBytesRead < len) {
        QTRY_VERIFY_WITH_TIMEOUT(audioInput.bytesAvailable() > 0, 1000);
        QByteArray buffer = feed->readAll();
        audioFile->write(buffer);
        totalBytesRead += buffer.size();
        if (firstBuffer && buffer.size()) {
            // Check for transition to ActiveState when data is provided
            QTRY_VERIFY2((stateSignal.size() == 1),"didn't emit ActiveState signal on data");
            QVERIFY2((audioInput.state() == QAudio::ActiveState),
                     "didn't transition to ActiveState after data");
            QVERIFY2((audioInput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
            firstBuffer = false;
        }
    }

    stateSignal.clear();

    qint64 processedUs = audioInput.processedUSecs();

    audioInput.stop();
    QTRY_VERIFY2((stateSignal.size() == 1),
                 QString("didn't emit StoppedState signal after stop(), got %1 signals instead").arg(stateSignal.size()).toUtf8().constData());
    QVERIFY2((audioInput.state() == QAudio::StoppedState), "didn't transitions to StoppedState after stop()");

    QVERIFY2(qTolerantCompare(processedUs, 500000LL),
             QString("processedUSecs() doesn't fall in acceptable range, should be 500000 (%1)").arg(processedUs).toUtf8().constData());
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    QVERIFY2((audioInput.elapsedUSecs() == (qint64)0), "elapsedUSecs() not equal to zero in StoppedState");

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
    QAudioSource audioInput(audioFormat, this);

    audioInput.setBufferSize(audioFormat.bytesForDuration(100000));

    QSignalSpy stateSignal(&audioInput, SIGNAL(stateChanged(QAudio::State)));

    // Check that we are in the default state before calling start
    QVERIFY2((audioInput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");
    QVERIFY2((audioInput.elapsedUSecs() == qint64(0)),"elapsedUSecs() not zero on creation");

    audioFile->close();
    audioFile->open(QIODevice::WriteOnly);
    QWaveDecoder waveDecoder(audioFile.get(), audioFormat);
    if (!waveDecoder.open(QIODevice::WriteOnly)) {
        waveDecoder.close();
        audioFile->close();
        QSKIP("Audio format not supported for writing to WAV file.");
    }
    QCOMPARE(waveDecoder.size(), QWaveDecoder::headerLength());

    QIODevice* feed = audioInput.start();

    // Check that QAudioSource immediately transitions to IdleState
    QTRY_VERIFY2((stateSignal.size() == 1),"didn't emit IdleState signal on start()");
    QVERIFY2((audioInput.state() == QAudio::IdleState),
             "didn't transition to IdleState after start()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
    stateSignal.clear();

    // Check that 'elapsed' increases
    QTRY_VERIFY2((audioInput.elapsedUSecs() > 0), "elapsedUSecs() is still zero after start()");

    qint64 totalBytesRead = 0;
    bool firstBuffer = true;
    qint64 len = audioFormat.sampleRate() * audioFormat.bytesPerFrame() / 2; // .5 seconds
    while (totalBytesRead < len) {
        QTRY_VERIFY_WITH_TIMEOUT(audioInput.bytesAvailable() > 0, 1000);
        auto buffer = feed->readAll();
        audioFile->write(buffer);
        totalBytesRead += buffer.size();
        if (firstBuffer && buffer.size()) {
            // Check for transition to ActiveState when data is provided
            QTRY_VERIFY2((stateSignal.size() == 1),"didn't emit ActiveState signal on data");
            QVERIFY2((audioInput.state() == QAudio::ActiveState),
                     "didn't transition to ActiveState after data");
            QVERIFY2((audioInput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
            firstBuffer = false;
        }
    }
    stateSignal.clear();

    audioInput.suspend();

    QTRY_VERIFY2((stateSignal.size() == 1),
             QString("didn't emit SuspendedState signal after suspend(), got %1 signals instead").arg(stateSignal.size()).toUtf8().constData());
    QVERIFY2((audioInput.state() == QAudio::SuspendedState), "didn't transitions to SuspendedState after stop()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error() is not QAudio::NoError after stop()");
    stateSignal.clear();

    // Check that only 'elapsed', and not 'processed' increases while suspended
    qint64 elapsedUs = audioInput.elapsedUSecs();
    qint64 processedUs = audioInput.processedUSecs();
    QTRY_VERIFY(audioInput.elapsedUSecs() > elapsedUs);
    QVERIFY(audioInput.processedUSecs() == processedUs);

    // Drain any data, in case we run out of space when resuming
    while (feed->readAll().size() > 0)
        ;
    QCOMPARE(audioInput.bytesAvailable(), 0);

    audioInput.resume();

    // Check that QAudioSource immediately transitions to Active or IdleState
    QTRY_VERIFY2((stateSignal.size() > 0),"didn't emit signals on resume()");
    QVERIFY2((audioInput.state() == QAudio::ActiveState || audioInput.state() == QAudio::IdleState),
             "didn't transition to ActiveState or IdleState after resume()");
    QVERIFY2((audioInput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after resume()");

    stateSignal.clear();

    // Read another seconds worth
    totalBytesRead = 0;
    firstBuffer = true;
    while (totalBytesRead < len && audioInput.state() != QAudio::StoppedState) {
        QTRY_VERIFY(audioInput.bytesAvailable() > 0);
        auto buffer = feed->readAll();
        audioFile->write(buffer);
        totalBytesRead += buffer.size();
    }
    stateSignal.clear();

    processedUs = audioInput.processedUSecs();

    audioInput.stop();
    QTRY_VERIFY2((stateSignal.size() == 1),
             QString("didn't emit StoppedState signal after stop(), got %1 signals instead").arg(stateSignal.size()).toUtf8().constData());
    QVERIFY2((audioInput.state() == QAudio::StoppedState), "didn't transitions to StoppedState after stop()");

    QVERIFY2(qTolerantCompare(processedUs, 1000000LL),
             QString("processedUSecs() doesn't fall in acceptable range, should be 2040000 (%1)").arg(processedUs).toUtf8().constData());
    QVERIFY2((audioInput.elapsedUSecs() == (qint64)0), "elapsedUSecs() not equal to zero in StoppedState");

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
        QAudioSource audioInput(audioFormat, this);

        QSignalSpy stateSignal(&audioInput, SIGNAL(stateChanged(QAudio::State)));

        // Check that we are in the default state before calling start
        QVERIFY2((audioInput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
        QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");
        QVERIFY2((audioInput.elapsedUSecs() == qint64(0)),"elapsedUSecs() not zero on creation");

        QIODevice* device = audioInput.start();
        // Check that QAudioSource immediately transitions to IdleState
        QTRY_VERIFY2((stateSignal.size() == 1),"didn't emit IdleState signal on start()");
        QVERIFY2((audioInput.state() == QAudio::IdleState), "didn't transition to IdleState after start()");
        QVERIFY2((audioInput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
        QTRY_VERIFY2_WITH_TIMEOUT((audioInput.bytesAvailable() > 0), "no bytes available after starting", 10000);

        // Trigger a read
        QByteArray data = device->readAll();
        QVERIFY2((audioInput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
        stateSignal.clear();

        audioInput.reset();
        QTRY_VERIFY2((stateSignal.size() == 1),"didn't emit StoppedState signal after reset()");
        QVERIFY2((audioInput.state() == QAudio::StoppedState), "didn't transitions to StoppedState after reset()");
        QVERIFY2((audioInput.bytesAvailable() == 0), "buffer not cleared after reset()");
    }

    {
        QAudioSource audioInput(audioFormat, this);
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);

        QSignalSpy stateSignal(&audioInput, SIGNAL(stateChanged(QAudio::State)));

        // Check that we are in the default state before calling start
        QVERIFY2((audioInput.state() == QAudio::StoppedState), "state() was not set to StoppedState before start()");
        QVERIFY2((audioInput.error() == QAudio::NoError), "error() was not set to QAudio::NoError before start()");
        QVERIFY2((audioInput.elapsedUSecs() == qint64(0)),"elapsedUSecs() not zero on creation");

        audioInput.start(&buffer);

        // Check that QAudioSource immediately transitions to ActiveState
        QTRY_VERIFY2((stateSignal.size() >= 1),"didn't emit state changed signal on start()");
        QTRY_VERIFY2((audioInput.state() == QAudio::ActiveState), "didn't transition to ActiveState after start()");
        QVERIFY2((audioInput.error() == QAudio::NoError), "error state is not equal to QAudio::NoError after start()");
        stateSignal.clear();

        audioInput.reset();
        QTRY_VERIFY2((stateSignal.size() >= 1),"didn't emit StoppedState signal after reset()");
        QVERIFY2((audioInput.state() == QAudio::StoppedState), "didn't transitions to StoppedState after reset()");
        QVERIFY2((audioInput.bytesAvailable() == 0), "buffer not cleared after reset()");
    }
}

void tst_QAudioSource::volume()
{
    QFETCH(QAudioFormat, audioFormat);

    const qreal half(0.5f);
    const qreal one(1.0f);

    QAudioSource audioInput(audioFormat, this);

    qreal volume = audioInput.volume();
    audioInput.setVolume(half);
    QTRY_VERIFY(qRound(audioInput.volume()*10.0f) == 5);

    audioInput.setVolume(one);
    QTRY_VERIFY(qRound(audioInput.volume()*10.0f) == 10);

    audioInput.setVolume(half);
    audioInput.start();
    QTRY_VERIFY(qRound(audioInput.volume()*10.0f) == 5);
    audioInput.setVolume(one);
    QTRY_VERIFY(qRound(audioInput.volume()*10.0f) == 10);

    audioInput.setVolume(volume);
}

QTEST_MAIN(tst_QAudioSource)

#include "tst_qaudiosource.moc"
