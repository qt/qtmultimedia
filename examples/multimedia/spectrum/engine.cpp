// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "engine.h"
#include "tonegenerator.h"
#include "utils.h"

#include <QAudioSink>
#include <QAudioSource>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QMetaObject>
#include <QSet>
#include <QThread>

#if QT_CONFIG(permissions)
  #include <QPermission>
#endif

#include <math.h>

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

const qint64 BufferDurationUs = 10 * 1000000;

// Size of the level calculation window in microseconds
const int LevelWindowUs = 0.1 * 1000000;

//-----------------------------------------------------------------------------
// Constructor and destructor
//-----------------------------------------------------------------------------

Engine::Engine(QObject *parent)
    : QObject(parent),
      m_mode(QAudioDevice::Input),
      m_state(QAudio::StoppedState),
      m_devices(new QMediaDevices(this)),
      m_generateTone(false),
      m_file(nullptr),
      m_analysisFile(nullptr),
      m_audioInput(nullptr),
      m_audioInputIODevice(nullptr),
      m_recordPosition(0),
      m_audioOutput(nullptr),
      m_playPosition(0),
      m_bufferPosition(0),
      m_bufferLength(0),
      m_dataLength(0),
      m_levelBufferLength(0),
      m_rmsLevel(0.0),
      m_peakLevel(0.0),
      m_spectrumBufferLength(0),
      m_spectrumPosition(0),
      m_count(0)
{
    connect(&m_spectrumAnalyser,
            QOverload<const FrequencySpectrum &>::of(&SpectrumAnalyser::spectrumChanged), this,
            QOverload<const FrequencySpectrum &>::of(&Engine::spectrumChanged));

    // This code might misinterpret things like "-something -category".  But
    // it's unlikely that that needs to be supported so we'll let it go.
    QStringList arguments = QCoreApplication::instance()->arguments();
    for (int i = 0; i < arguments.count(); ++i) {
        if (arguments.at(i) == QStringLiteral("--"))
            break;
    }

    initAudioDevices();

    initialize();

#ifdef DUMP_DATA
    createOutputDir();
#endif

#ifdef DUMP_SPECTRUM
    m_spectrumAnalyser.setOutputPath(outputPath());
#endif

    m_notifyTimer = new QTimer(this);
    m_notifyTimer->setInterval(1000);
    connect(m_notifyTimer, &QTimer::timeout, this, &Engine::audioNotify);
}

Engine::~Engine() = default;

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

bool Engine::loadFile(const QString &fileName)
{
    reset();
    bool result = false;
    Q_ASSERT(!m_generateTone);
    Q_ASSERT(!m_file);
    Q_ASSERT(!fileName.isEmpty());
    QIODevice *file = new QFile(fileName);
    if (file->open(QIODevice::ReadOnly)) {
        m_file = new QWaveDecoder(file, this);
        if (m_file->open(QIODevice::ReadOnly)) {
            if (m_file->audioFormat().sampleFormat() == QAudioFormat::Int16) {
                result = initialize();
            } else {
                emit errorMessage(tr("Audio format not supported"),
                                  formatToString(m_file->audioFormat()));
            }
        } else
            emit errorMessage(tr("Could not open WAV decoder for file"), fileName);
    } else {
        emit errorMessage(tr("Could not open file"), fileName);
    }
    if (result) {
        file->close();
        file->open(QIODevice::ReadOnly);
        m_analysisFile = new QWaveDecoder(file, this);
        m_analysisFile->open(QIODevice::ReadOnly);
    }
    return result;
}

bool Engine::generateTone(const Tone &tone)
{
    reset();
    Q_ASSERT(!m_generateTone);
    Q_ASSERT(!m_file);
    m_generateTone = true;
    m_tone = tone;
    ENGINE_DEBUG << "Engine::generateTone"
                 << "startFreq" << m_tone.startFreq << "endFreq" << m_tone.endFreq << "amp"
                 << m_tone.amplitude;
    return initialize();
}

bool Engine::generateSweptTone(qreal amplitude)
{
    Q_ASSERT(!m_generateTone);
    Q_ASSERT(!m_file);
    m_generateTone = true;
    m_tone.startFreq = 1;
    m_tone.endFreq = 0;
    m_tone.amplitude = amplitude;
    ENGINE_DEBUG << "Engine::generateSweptTone"
                 << "startFreq" << m_tone.startFreq << "amp" << m_tone.amplitude;
    return initialize();
}

bool Engine::initializeRecord()
{
    reset();
    ENGINE_DEBUG << "Engine::initializeRecord";
    Q_ASSERT(!m_generateTone);
    Q_ASSERT(!m_file);
    m_generateTone = false;
    m_tone = SweptTone();
    return initialize();
}

qint64 Engine::bufferLength() const
{
    return m_file ? m_file->getDevice()->size() : m_bufferLength;
}

void Engine::setWindowFunction(WindowFunction type)
{
    m_spectrumAnalyser.setWindowFunction(type);
}

//-----------------------------------------------------------------------------
// Public slots
//-----------------------------------------------------------------------------

void Engine::startRecording()
{
    if (m_audioInput) {
        if (QAudioDevice::Input == m_mode && QAudio::SuspendedState == m_state) {
            m_audioInput->resume();
        } else {
            m_spectrumAnalyser.cancelCalculation();
            emit spectrumChanged(0, 0, FrequencySpectrum());

            m_buffer.fill(0);
            setRecordPosition(0, true);
            stopPlayback();
            m_mode = QAudioDevice::Input;
            connect(m_audioInput, &QAudioSource::stateChanged, this, &Engine::audioStateChanged);

            m_count = 0;
            m_dataLength = 0;
            emit dataLengthChanged(0);
            m_audioInputIODevice = m_audioInput->start();
            connect(m_audioInputIODevice, &QIODevice::readyRead, this, &Engine::audioDataReady);
        }
        m_notifyTimer->start();
    }
}

void Engine::startPlayback()
{
    if (!m_audioOutput)
        initialize();

    if (m_audioOutput) {
        if (QAudioDevice::Output == m_mode && QAudio::SuspendedState == m_state) {
#ifdef Q_OS_WIN
            // The Windows backend seems to internally go back into ActiveState
            // while still returning SuspendedState, so to ensure that it doesn't
            // ignore the resume() call, we first re-suspend
            m_audioOutput->suspend();
#endif
            m_audioOutput->resume();
        } else {
            m_spectrumAnalyser.cancelCalculation();
            emit spectrumChanged(0, 0, FrequencySpectrum());
            setPlayPosition(0, true);
            stopRecording();
            m_mode = QAudioDevice::Output;
            connect(m_audioOutput, &QAudioSink::stateChanged, this, &Engine::audioStateChanged);

            m_count = 0;
            if (m_file) {
                m_file->seek(0);
                m_bufferPosition = 0;
                m_dataLength = 0;
                m_audioOutput->start(m_file->getDevice());
            } else {
                m_audioOutputIODevice.close();
                m_audioOutputIODevice.setBuffer(&m_buffer);
                m_audioOutputIODevice.open(QIODevice::ReadOnly);
                m_audioOutput->start(&m_audioOutputIODevice);
            }
        }
        m_notifyTimer->start();
    }
}

void Engine::suspend()
{
    if (QAudio::ActiveState == m_state || QAudio::IdleState == m_state) {
        switch (m_mode) {
        case QAudioDevice::Input:
            m_audioInput->suspend();
            break;
        case QAudioDevice::Output:
            m_audioOutput->suspend();
            break;
        default:
            break;
        }
        m_notifyTimer->stop();
    }
}

void Engine::setAudioInputDevice(const QAudioDevice &device)
{
    if (device.id() != m_audioInputDevice.id()) {
        m_audioInputDevice = device;
        initialize();
    }
}

void Engine::setAudioOutputDevice(const QAudioDevice &device)
{
    if (device.id() != m_audioOutputDevice.id()) {
        m_audioOutputDevice = device;
        initialize();
    }
}

//-----------------------------------------------------------------------------
// Private slots
//-----------------------------------------------------------------------------

void Engine::initAudioDevices()
{
#if QT_CONFIG(permissions)
    QMicrophonePermission microphonePermission;
    switch (qApp->checkPermission(microphonePermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(microphonePermission, this, &Engine::initAudioDevices);
        return;
    case Qt::PermissionStatus::Denied:
        qWarning("Microphone permission is not granted!");
        return;
    case Qt::PermissionStatus::Granted:
        break;
    }
#endif
    m_availableAudioInputDevices = m_devices->audioInputs();
    m_audioInputDevice = m_devices->defaultAudioInput();
    m_availableAudioOutputDevices = m_devices->audioOutputs();
    m_audioOutputDevice = m_devices->defaultAudioOutput();
}

void Engine::audioNotify()
{
    switch (m_mode) {
    case QAudioDevice::Input: {
        const qint64 recordPosition =
                qMin(m_bufferLength, m_format.bytesForDuration(m_audioInput->processedUSecs()));
        setRecordPosition(recordPosition);
        const qint64 levelPosition = m_dataLength - m_levelBufferLength;
        if (levelPosition >= 0)
            calculateLevel(levelPosition, m_levelBufferLength);
        if (m_dataLength >= m_spectrumBufferLength) {
            const qint64 spectrumPosition = m_dataLength - m_spectrumBufferLength;
            calculateSpectrum(spectrumPosition);
        }
        emit bufferChanged(0, m_dataLength, m_buffer);
    } break;
    case QAudioDevice::Output: {
        const qint64 playPosition = m_format.bytesForDuration(m_audioOutput->processedUSecs());
        setPlayPosition(qMin(bufferLength(), playPosition));
        const qint64 levelPosition = playPosition - m_levelBufferLength;
        const qint64 spectrumPosition = playPosition - m_spectrumBufferLength;
        if (m_file) {
            if (levelPosition > m_bufferPosition || spectrumPosition > m_bufferPosition
                || qMax(m_levelBufferLength, m_spectrumBufferLength) > m_dataLength) {
                m_bufferPosition = 0;
                m_dataLength = 0;
                // Data needs to be read into m_buffer in order to be analysed
                const qint64 readPos = qMax(qint64(0), qMin(levelPosition, spectrumPosition));
                const qint64 readEnd = qMin(m_analysisFile->getDevice()->size(),
                                            qMax(levelPosition + m_levelBufferLength,
                                                 spectrumPosition + m_spectrumBufferLength));
                const qint64 readLen =
                        readEnd - readPos + m_format.bytesForDuration(WaveformWindowDuration);
                qDebug() << "Engine::audioNotify [1]"
                         << "analysisFileSize" << m_analysisFile->getDevice()->size() << "readPos"
                         << readPos << "readLen" << readLen;
                if (m_analysisFile->seek(readPos + m_analysisFile->headerLength())) {
                    m_buffer.resize(readLen);
                    m_bufferPosition = readPos;
                    m_dataLength = m_analysisFile->read(m_buffer.data(), readLen);
                    qDebug() << "Engine::audioNotify [2]"
                             << "bufferPosition" << m_bufferPosition << "dataLength"
                             << m_dataLength;
                } else {
                    qDebug() << "Engine::audioNotify [2]"
                             << "file seek error";
                }
                emit bufferChanged(m_bufferPosition, m_dataLength, m_buffer);
            }
        } else {
            if (playPosition >= m_dataLength)
                stopPlayback();
        }
        if (levelPosition >= 0
            && levelPosition + m_levelBufferLength < m_bufferPosition + m_dataLength)
            calculateLevel(levelPosition, m_levelBufferLength);
        if (spectrumPosition >= 0
            && spectrumPosition + m_spectrumBufferLength < m_bufferPosition + m_dataLength)
            calculateSpectrum(spectrumPosition);
    } break;
    default:
        break;
    }
}

void Engine::audioStateChanged(QAudio::State state)
{
    ENGINE_DEBUG << "Engine::audioStateChanged from" << m_state << "to" << state;

    if (QAudio::IdleState == state && m_file && m_file->pos() == m_file->getDevice()->size()) {
        stopPlayback();
    } else {
        if (QAudio::StoppedState == state) {
            // Check error
            QAudio::Error error = QAudio::NoError;
            switch (m_mode) {
            case QAudioDevice::Input:
                error = m_audioInput->error();
                break;
            case QAudioDevice::Output:
                error = m_audioOutput->error();
                break;
            default:
                break;
            }
            if (QAudio::NoError != error) {
                emitError(error);
                reset();
                return;
            }
        }
        setState(state);
    }
}

void Engine::audioDataReady()
{
    Q_ASSERT(0 == m_bufferPosition);
    const qint64 bytesReady = m_audioInput->bytesAvailable();
    const qint64 bytesSpace = m_buffer.size() - m_dataLength;
    const qint64 bytesToRead = qMin(bytesReady, bytesSpace);

    const qint64 bytesRead =
            m_audioInputIODevice->read(m_buffer.data() + m_dataLength, bytesToRead);

    if (bytesRead) {
        m_dataLength += bytesRead;
        emit dataLengthChanged(dataLength());
    }

    if (m_buffer.size() == m_dataLength)
        stopRecording();
}

void Engine::spectrumChanged(const FrequencySpectrum &spectrum)
{
    ENGINE_DEBUG << "Engine::spectrumChanged"
                 << "pos" << m_spectrumPosition;
    emit spectrumChanged(m_spectrumPosition, m_spectrumBufferLength, spectrum);
}

//-----------------------------------------------------------------------------
// Private functions
//-----------------------------------------------------------------------------

void Engine::resetAudioDevices()
{
    delete m_audioInput;
    m_audioInput = nullptr;
    m_audioInputIODevice = nullptr;
    setRecordPosition(0);
    delete m_audioOutput;
    m_audioOutput = nullptr;
    setPlayPosition(0);
    m_spectrumPosition = 0;
    setLevel(0.0, 0.0, 0);
}

void Engine::reset()
{
    stopRecording();
    stopPlayback();
    setState(QAudioDevice::Input, QAudio::StoppedState);
    m_generateTone = false;
    setFormat(QAudioFormat());
    delete m_file;
    m_file = nullptr;
    delete m_analysisFile;
    m_analysisFile = nullptr;
    m_buffer.clear();
    m_bufferPosition = 0;
    m_bufferLength = 0;
    m_dataLength = 0;
    emit dataLengthChanged(0);
    resetAudioDevices();
}

bool Engine::initialize()
{
    bool result = false;

    QAudioFormat format = m_format;

    if (selectFormat()) {
        if (m_format != format) {
            resetAudioDevices();
            if (m_file) {
                emit bufferLengthChanged(bufferLength());
                emit dataLengthChanged(dataLength());
                emit bufferChanged(0, 0, m_buffer);
                setRecordPosition(bufferLength());
                result = true;
            } else {
                m_bufferLength = m_format.bytesForDuration(BufferDurationUs);
                m_buffer.resize(m_bufferLength);
                m_buffer.fill(0);
                emit bufferLengthChanged(bufferLength());
                if (m_generateTone) {
                    if (0 == m_tone.endFreq) {
                        const qreal nyquist = nyquistFrequency(m_format);
                        m_tone.endFreq = qMin(qreal(SpectrumHighFreq), nyquist);
                    }
                    // Call function defined in utils.h, at global scope
                    ::generateTone(m_tone, m_format, m_buffer);
                    m_dataLength = m_bufferLength;
                    emit dataLengthChanged(dataLength());
                    emit bufferChanged(0, m_dataLength, m_buffer);
                    setRecordPosition(m_bufferLength);
                    result = true;
                } else {
                    emit bufferChanged(0, 0, m_buffer);
                    m_audioInput = new QAudioSource(m_audioInputDevice, m_format, this);
                    result = true;
                }
            }
            m_audioOutput = new QAudioSink(m_audioOutputDevice, m_format, this);
        }
    } else {
        if (m_file)
            emit errorMessage(tr("Audio format not supported"), formatToString(m_format));
        else if (m_generateTone)
            emit errorMessage(tr("No suitable format found"), "");
        else
            emit errorMessage(tr("No common input / output format found"), "");
    }

    ENGINE_DEBUG << "Engine::initialize"
                 << "m_bufferLength" << m_bufferLength;
    ENGINE_DEBUG << "Engine::initialize"
                 << "m_dataLength" << m_dataLength;
    ENGINE_DEBUG << "Engine::initialize"
                 << "format" << m_format;

    return result;
}

bool Engine::selectFormat()
{
    bool foundSupportedFormat = false;

    if (m_file || QAudioFormat() != m_format) {
        QAudioFormat format = m_format;
        if (m_file)
            // Header is read from the WAV file; just need to check whether
            // it is supported by the audio output device
            format = m_file->audioFormat();
        if (m_audioOutputDevice.isFormatSupported(format)) {
            setFormat(format);
            foundSupportedFormat = true;
        }
    } else {

        int minSampleRate = qMin(m_audioInputDevice.minimumSampleRate(),
                                 m_audioOutputDevice.minimumSampleRate());
        int maxSampleRate = qMin(m_audioInputDevice.maximumSampleRate(),
                                 m_audioOutputDevice.maximumSampleRate());
        int minChannelCount = qMin(m_audioInputDevice.minimumChannelCount(),
                                   m_audioOutputDevice.minimumChannelCount());
        int maxChannelCount = qMin(m_audioInputDevice.maximumChannelCount(),
                                   m_audioOutputDevice.maximumChannelCount());

        QAudioFormat format;
        format.setSampleFormat(QAudioFormat::Int16);
        format.setSampleRate(qBound(minSampleRate, 48000, maxSampleRate));
        format.setChannelCount(qBound(minChannelCount, 2, maxChannelCount));

        const bool inputSupport = m_audioInputDevice.isFormatSupported(format);
        const bool outputSupport = m_audioOutputDevice.isFormatSupported(format);
        if (inputSupport && outputSupport)
            foundSupportedFormat = true;

        setFormat(format);
    }

    return foundSupportedFormat;
}

void Engine::stopRecording()
{
    if (m_audioInput) {
        m_audioInput->stop();
        QCoreApplication::instance()->processEvents();
        m_audioInput->disconnect();
    }
    m_audioInputIODevice = nullptr;
    m_notifyTimer->stop();

#ifdef DUMP_AUDIO
    dumpData();
#endif
}

void Engine::stopPlayback()
{
    if (m_audioOutput) {
        m_audioOutput->stop();
        QCoreApplication::instance()->processEvents();
        m_audioOutput->disconnect();
        setPlayPosition(0);
    }
    m_notifyTimer->stop();
}

void Engine::setState(QAudio::State state)
{
    const bool changed = (m_state != state);
    m_state = state;
    if (changed)
        emit stateChanged(m_mode, m_state);
}

void Engine::setState(QAudioDevice::Mode mode, QAudio::State state)
{
    const bool changed = (m_mode != mode || m_state != state);
    m_mode = mode;
    m_state = state;
    if (changed)
        emit stateChanged(m_mode, m_state);
}

void Engine::setRecordPosition(qint64 position, bool forceEmit)
{
    const bool changed = (m_recordPosition != position);
    m_recordPosition = position;
    if (changed || forceEmit)
        emit recordPositionChanged(m_recordPosition);
}

void Engine::setPlayPosition(qint64 position, bool forceEmit)
{
    const bool changed = (m_playPosition != position);
    m_playPosition = position;
    if (changed || forceEmit)
        emit playPositionChanged(m_playPosition);
}

void Engine::calculateLevel(qint64 position, qint64 length)
{
#ifdef DISABLE_LEVEL
    Q_UNUSED(position);
    Q_UNUSED(length);
#else
    Q_ASSERT(position + length <= m_bufferPosition + m_dataLength);

    qreal peakLevel = 0.0;

    qreal sum = 0.0;
    const char *ptr = m_buffer.constData() + position - m_bufferPosition;
    const char *const end = ptr + length;
    while (ptr < end) {
        const qint16 value = *reinterpret_cast<const qint16 *>(ptr);
        const qreal fracValue = pcmToReal(value);
        peakLevel = qMax(peakLevel, fracValue);
        sum += fracValue * fracValue;
        ptr += 2;
    }
    const int numSamples = length / 2;
    qreal rmsLevel = sqrt(sum / numSamples);

    rmsLevel = qMax(qreal(0.0), rmsLevel);
    rmsLevel = qMin(qreal(1.0), rmsLevel);
    setLevel(rmsLevel, peakLevel, numSamples);

    ENGINE_DEBUG << "Engine::calculateLevel"
                 << "pos" << position << "len" << length << "rms" << rmsLevel << "peak"
                 << peakLevel;
#endif
}

void Engine::calculateSpectrum(qint64 position)
{
#ifdef DISABLE_SPECTRUM
    Q_UNUSED(position);
#else
    Q_ASSERT(position + m_spectrumBufferLength <= m_bufferPosition + m_dataLength);
    Q_ASSERT(0 == m_spectrumBufferLength % 2); // constraint of FFT algorithm

    // QThread::currentThread is marked 'for internal use only', but
    // we're only using it for debug output here, so it's probably OK :)
    ENGINE_DEBUG << "Engine::calculateSpectrum" << QThread::currentThread() << "count" << m_count
                 << "pos" << position << "len" << m_spectrumBufferLength
                 << "spectrumAnalyser.isReady" << m_spectrumAnalyser.isReady();

    if (m_spectrumAnalyser.isReady()) {
        m_spectrumBuffer = QByteArray::fromRawData(
                m_buffer.constData() + position - m_bufferPosition, m_spectrumBufferLength);
        m_spectrumPosition = position;
        m_spectrumAnalyser.calculate(m_spectrumBuffer, m_format);
    }
#endif
}

void Engine::setFormat(const QAudioFormat &format)
{
    const bool changed = (format != m_format);
    m_format = format;
    m_levelBufferLength = m_format.bytesForDuration(LevelWindowUs);
    m_spectrumBufferLength = SpectrumLengthSamples * format.bytesPerFrame();
    if (changed)
        emit formatChanged(m_format);
}

void Engine::setLevel(qreal rmsLevel, qreal peakLevel, int numSamples)
{
    m_rmsLevel = rmsLevel;
    m_peakLevel = peakLevel;
    emit levelChanged(m_rmsLevel, m_peakLevel, numSamples);
}

void Engine::emitError(QAudio::Error error)
{
    QString errorString;
    switch (error) {
    case QAudio::NoError:
        errorString = tr("NoError");
        break;
    case QAudio::OpenError:
        errorString = tr("OpenError: An error occurred opening the audio device.");
        break;
    case QAudio::IOError:
        errorString = tr("IOError: An error occurred during read/write of audio device.");
        break;
    case QAudio::UnderrunError:
        errorString = tr("UnderrunError: Audio data is not being fed"
                         "to the audio device at a fast enough rate.");
        break;
    case QAudio::FatalError:
        errorString = tr("FatalError: A non-recoverable error has occurred,"
                         "the audio device is not usable at this time.");
        break;
    }

    emit errorMessage(tr("Audio Device"), errorString);
}

#ifdef DUMP_DATA
void Engine::createOutputDir()
{
    m_outputDir.setPath("output");

    // Ensure output directory exists and is empty
    if (m_outputDir.exists()) {
        const QStringList files = m_outputDir.entryList(QDir::Files);
        for (const QString &file : files)
            m_outputDir.remove(file);
    } else {
        QDir::current().mkdir("output");
    }
}
#endif // DUMP_DATA

#ifdef DUMP_AUDIO
void Engine::dumpData()
{
    const QString txtFileName = m_outputDir.filePath("data.txt");
    QFile txtFile(txtFileName);
    txtFile.open(QFile::WriteOnly | QFile::Text);
    QTextStream stream(&txtFile);
    const qint16 *ptr = reinterpret_cast<const qint16 *>(m_buffer.constData());
    const int numSamples = m_dataLength / (2 * m_format.channels());
    for (int i = 0; i < numSamples; ++i) {
        stream << i << "\t" << *ptr << "\n";
        ptr += m_format.channels();
    }

    const QString pcmFileName = m_outputDir.filePath("data.pcm");
    QFile pcmFile(pcmFileName);
    pcmFile.open(QFile::WriteOnly);
    pcmFile.write(m_buffer.constData(), m_dataLength);
}
#endif // DUMP_AUDIO

#include "moc_engine.cpp"
