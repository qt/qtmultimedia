// Copyright (C) 2022 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qdarwinaudiosource_p.h"
#include "qcoreaudiosessionmanager_p.h"
#include "qdarwinaudiodevice_p.h"
#include "qcoreaudioutils_p.h"
#include "qdarwinmediadevices_p.h"
#include <qmediadevices.h>

#if defined(Q_OS_MACOS)
# include <AudioUnit/AudioComponent.h>
#endif

#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
# include "qcoreaudiosessionmanager_p.h"
#endif

#include <QtMultimedia/private/qaudiohelpers_p.h>
#include <QtCore/QDataStream>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

static const int DEFAULT_BUFFER_SIZE = 4 * 1024;

QCoreAudioBufferList::QCoreAudioBufferList(const AudioStreamBasicDescription &streamFormat)
    : m_owner(false)
    , m_streamDescription(streamFormat)
{
    const bool isInterleaved = (m_streamDescription.mFormatFlags & kAudioFormatFlagIsNonInterleaved) == 0;
    const int numberOfBuffers = isInterleaved ? 1 : m_streamDescription.mChannelsPerFrame;

    m_bufferList = reinterpret_cast<AudioBufferList*>(malloc(sizeof(AudioBufferList) +
                                                            (sizeof(AudioBuffer) * numberOfBuffers)));

    m_bufferList->mNumberBuffers = numberOfBuffers;
    for (int i = 0; i < numberOfBuffers; ++i) {
        m_bufferList->mBuffers[i].mNumberChannels = isInterleaved ? numberOfBuffers : 1;
        m_bufferList->mBuffers[i].mDataByteSize = 0;
        m_bufferList->mBuffers[i].mData = 0;
    }
}

QCoreAudioBufferList::QCoreAudioBufferList(const AudioStreamBasicDescription &streamFormat,
                                           char *buffer, int bufferSize)
    : m_owner(false), m_dataSize(bufferSize), m_streamDescription(streamFormat)
{
    m_bufferList = reinterpret_cast<AudioBufferList*>(malloc(sizeof(AudioBufferList) + sizeof(AudioBuffer)));

    m_bufferList->mNumberBuffers = 1;
    m_bufferList->mBuffers[0].mNumberChannels = 1;
    m_bufferList->mBuffers[0].mDataByteSize = m_dataSize;
    m_bufferList->mBuffers[0].mData = buffer;
}

QCoreAudioBufferList::QCoreAudioBufferList(const AudioStreamBasicDescription &streamFormat,
                                           int framesToBuffer)
    : m_owner(true), m_streamDescription(streamFormat)
{
    const bool isInterleaved = (m_streamDescription.mFormatFlags & kAudioFormatFlagIsNonInterleaved) == 0;
    const int numberOfBuffers = isInterleaved ? 1 : m_streamDescription.mChannelsPerFrame;

    m_dataSize = framesToBuffer * m_streamDescription.mBytesPerFrame;

    m_bufferList = reinterpret_cast<AudioBufferList*>(malloc(sizeof(AudioBufferList) +
                                                            (sizeof(AudioBuffer) * numberOfBuffers)));
    m_bufferList->mNumberBuffers = numberOfBuffers;
    for (int i = 0; i < numberOfBuffers; ++i) {
        m_bufferList->mBuffers[i].mNumberChannels = isInterleaved ? numberOfBuffers : 1;
        m_bufferList->mBuffers[i].mDataByteSize = m_dataSize;
        m_bufferList->mBuffers[i].mData = malloc(m_dataSize);
    }
}

QCoreAudioBufferList::~QCoreAudioBufferList()
{
    if (m_owner) {
        for (UInt32 i = 0; i < m_bufferList->mNumberBuffers; ++i)
            free(m_bufferList->mBuffers[i].mData);
    }

    free(m_bufferList);
}

char *QCoreAudioBufferList::data(int buffer) const
{
    return static_cast<char*>(m_bufferList->mBuffers[buffer].mData);
}

qint64 QCoreAudioBufferList::bufferSize(int buffer) const
{
    return m_bufferList->mBuffers[buffer].mDataByteSize;
}

int QCoreAudioBufferList::frameCount(int buffer) const
{
    return m_bufferList->mBuffers[buffer].mDataByteSize / m_streamDescription.mBytesPerFrame;
}

int QCoreAudioBufferList::packetCount(int buffer) const
{
    return m_bufferList->mBuffers[buffer].mDataByteSize / m_streamDescription.mBytesPerPacket;
}

int QCoreAudioBufferList::packetSize() const
{
    return m_streamDescription.mBytesPerPacket;
}

void QCoreAudioBufferList::reset()
{
    for (UInt32 i = 0; i < m_bufferList->mNumberBuffers; ++i) {
        m_bufferList->mBuffers[i].mDataByteSize = m_dataSize;
        m_bufferList->mBuffers[i].mData = 0;
    }
}

QCoreAudioPacketFeeder::QCoreAudioPacketFeeder(QCoreAudioBufferList *abl)
    : m_audioBufferList(abl)
{
    m_totalPackets = m_audioBufferList->packetCount();
}

bool QCoreAudioPacketFeeder::feed(AudioBufferList &dst, UInt32 &packetCount)
{
    if (m_position == m_totalPackets) {
        dst.mBuffers[0].mDataByteSize = 0;
        packetCount = 0;
        return false;
    }

    if (m_totalPackets - m_position < packetCount)
        packetCount = m_totalPackets - m_position;

    dst.mBuffers[0].mDataByteSize = packetCount * m_audioBufferList->packetSize();
    dst.mBuffers[0].mData = m_audioBufferList->data() + (m_position * m_audioBufferList->packetSize());

    m_position += packetCount;

    return true;
}

bool QCoreAudioPacketFeeder::empty() const
{
    return m_position == m_totalPackets;
}

QDarwinAudioSourceBuffer::QDarwinAudioSourceBuffer(const QDarwinAudioSource &audioSource,
                                                   int bufferSize, int maxPeriodSize,
                                                   const AudioStreamBasicDescription &inputFormat,
                                                   const AudioStreamBasicDescription &outputFormat,
                                                   QObject *parent)
    : QObject(parent),
      m_audioSource(audioSource),
      m_maxPeriodSize(maxPeriodSize),
      m_buffer(bufferSize),
      m_inputBufferList(inputFormat),
      m_outputFormat(outputFormat)
{
    m_flushTimer = new QTimer(this);
    connect(m_flushTimer, &QTimer::timeout, this, &QDarwinAudioSourceBuffer::flushBuffer);
    m_flushTimer->setTimerType(Qt::PreciseTimer);

    const int periodTime =
            m_maxPeriodSize / m_outputFormat.mBytesPerFrame * 1000 / m_outputFormat.mSampleRate;
    m_flushTimer->start(qMax(1, periodTime));

    if (CoreAudioUtils::toQAudioFormat(inputFormat) != CoreAudioUtils::toQAudioFormat(outputFormat)) {
        if (AudioConverterNew(&inputFormat, &m_outputFormat, &m_audioConverter) != noErr) {
            qWarning() << "QAudioSource: Unable to create an Audio Converter";
            m_audioConverter = 0;
        }
    }

    m_qFormat = CoreAudioUtils::toQAudioFormat(inputFormat); // we adjust volume before conversion
}

qint64 QDarwinAudioSourceBuffer::renderFromDevice(AudioUnit audioUnit, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames)
{
    const bool pullMode = m_device == nullptr;

    OSStatus    err;
    qint64      framesRendered = 0;

    m_inputBufferList.reset();
    err = AudioUnitRender(audioUnit, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames,
                          m_inputBufferList.audioBufferList());

    // adjust volume, if necessary
    const qreal volume = m_audioSource.volume();
    if (!qFuzzyCompare(volume, qreal(1.0f))) {
        QAudioHelperInternal::qMultiplySamples(volume, m_qFormat,
                                               m_inputBufferList.data(), /* input */
                                               m_inputBufferList.data(), /* output */
                                               m_inputBufferList.bufferSize());
    }

    if (m_audioConverter != 0) {
        QCoreAudioPacketFeeder feeder(&m_inputBufferList);

        int bytesConsumed = m_buffer.consume(m_buffer.free(), [&](QSpan<const char> region) {
            AudioBufferList output;
            output.mNumberBuffers = 1;
            output.mBuffers[0].mNumberChannels = 1;
            output.mBuffers[0].mDataByteSize = region.size();
            output.mBuffers[0].mData = (void *)region.data();

            UInt32 packetSize = region.size() / m_outputFormat.mBytesPerPacket;
            err = AudioConverterFillComplexBuffer(m_audioConverter, converterCallback, &feeder,
                                                  &packetSize, &output, 0);
        });

        framesRendered += bytesConsumed / m_outputFormat.mBytesPerFrame;
    }
    else {
        int bytesCopied = m_buffer.write(QSpan<const char>{
                reinterpret_cast<const char *>(m_inputBufferList.data()),
                m_inputBufferList.bufferSize(),
        });

        framesRendered = bytesCopied / m_outputFormat.mBytesPerFrame;
    }

    if (pullMode && framesRendered > 0)
        emit readyRead();

    return framesRendered;
}

qint64 QDarwinAudioSourceBuffer::readBytes(char *data, qint64 len)
{
    return m_buffer.consume(len, [&](QSpan<const char> buffer) {
        memcpy(data, buffer.data(), buffer.size());
        data += buffer.size();
    });
}

void QDarwinAudioSourceBuffer::setFlushDevice(QIODevice *device)
{
    Q_ASSERT(!m_audioSource.audioUnitStarted());

    if (std::exchange(m_device, device) == device)
        return;

    m_deviceError = false;
    const auto wasFlushingEnabled = m_flushingEnabled;
    setFlushingEnabled(false);
    setFlushingEnabled(wasFlushingEnabled);
}

void QDarwinAudioSourceBuffer::setFlushingEnabled(bool enabled)
{
    if (std::exchange(m_flushingEnabled, enabled) == enabled)
        return;

    if (!enabled)
        m_flushTimer->stop();
    else if (m_device) {
        flushBuffer();
        m_flushTimer->start();
    }
}

void QDarwinAudioSourceBuffer::reset()
{
    Q_ASSERT(!m_audioSource.audioUnitStarted());

    m_buffer.reset();
    setFlushingEnabled(false);
    setFlushDevice(nullptr);
}

void QDarwinAudioSourceBuffer::flush(bool all)
{
    if (m_device == nullptr)
        return;

    const int used = m_buffer.used();
    const int readSize = all ? used : used - (used % m_maxPeriodSize);

    if (readSize > 0) {
        m_buffer.consume(readSize, [&](QSpan<const char> buffer) {
            const int bytesWritten = m_device->write(buffer.data(), buffer.size());
            if (bytesWritten < 0) {
                setFlushingEnabled(false);
                m_deviceError = true;
            }
        });
    }
}

int QDarwinAudioSourceBuffer::available() const
{
    return m_buffer.free();
}

int QDarwinAudioSourceBuffer::used() const
{
    return m_buffer.used();
}

void QDarwinAudioSourceBuffer::flushBuffer()
{
    flush();
}

OSStatus QDarwinAudioSourceBuffer::converterCallback(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets, AudioBufferList *ioData, AudioStreamPacketDescription **outDataPacketDescription, void *inUserData)
{
    Q_UNUSED(inAudioConverter);
    Q_UNUSED(outDataPacketDescription);

    QCoreAudioPacketFeeder* feeder = static_cast<QCoreAudioPacketFeeder*>(inUserData);

    if (!feeder->feed(*ioData, *ioNumberDataPackets))
        return as_empty;

    return noErr;
}

QDarwinAudioSourceDevice::QDarwinAudioSourceDevice(QDarwinAudioSourceBuffer *audioBuffer, QObject *parent)
    : QIODevice(parent)
    , m_audioBuffer(audioBuffer)
{
    Q_ASSERT(m_audioBuffer);
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    connect(m_audioBuffer, &QDarwinAudioSourceBuffer::readyRead, this,
            &QDarwinAudioSourceDevice::readyRead);
}

qint64 QDarwinAudioSourceDevice::readData(char *data, qint64 len)
{
    return m_audioBuffer->readBytes(data, len);
}

qint64 QDarwinAudioSourceDevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

QDarwinAudioSource::QDarwinAudioSource(const QAudioDevice &device, QObject *parent)
    : QPlatformAudioSource(parent),
      m_audioDeviceInfo(device),
      m_internalBufferSize(DEFAULT_BUFFER_SIZE),
      m_clockFrequency(CoreAudioUtils::frequency() / 1000),
      m_stateMachine(*this)
{
    QAudioDevice di = device;
    if (di.isNull())
        di = QMediaDevices::defaultAudioInput();
#if defined(Q_OS_MACOS)
    const QCoreAudioDeviceInfo *info = static_cast<const QCoreAudioDeviceInfo *>(di.handle());
    Q_ASSERT(info);
    m_audioDeviceId = info->deviceID();
#endif
    m_device = di.id();

    connect(this, &QDarwinAudioSource::stateChanged, this, &QDarwinAudioSource::updateAudioDevice);
}


QDarwinAudioSource::~QDarwinAudioSource()
{
    close();
}

bool QDarwinAudioSource::open()
{
#if defined(Q_OS_IOS)
    CoreAudioSessionManager::instance().setCategory(CoreAudioSessionManager::PlayAndRecord, CoreAudioSessionManager::MixWithOthers);
    CoreAudioSessionManager::instance().setActive(true);
#endif

    if (m_isOpen)
        return true;

    AudioComponentDescription componentDescription;
    componentDescription.componentType = kAudioUnitType_Output;
#if defined(Q_OS_MACOS)
    componentDescription.componentSubType = kAudioUnitSubType_HALOutput;
#else
    componentDescription.componentSubType = kAudioUnitSubType_RemoteIO;
#endif
    componentDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
    componentDescription.componentFlags = 0;
    componentDescription.componentFlagsMask = 0;

    AudioComponent component = AudioComponentFindNext(0, &componentDescription);
    if (component == 0) {
        qWarning() << "QAudioSource: Failed to find Output component";
        return false;
    }

    if (AudioComponentInstanceNew(component, &m_audioUnit) != noErr) {
        qWarning() << "QAudioSource: Unable to Open Output Component";
        return false;
    }

    // Set mode
    // switch to input mode
    UInt32 enable = 1;
    if (AudioUnitSetProperty(m_audioUnit,
                               kAudioOutputUnitProperty_EnableIO,
                               kAudioUnitScope_Input,
                               1,
                               &enable,
                               sizeof(enable)) != noErr) {
        qWarning() << "QAudioSource: Unable to switch to input mode (Enable Input)";
        return false;
    }

    enable = 0;
    if (AudioUnitSetProperty(m_audioUnit,
                            kAudioOutputUnitProperty_EnableIO,
                            kAudioUnitScope_Output,
                            0,
                            &enable,
                            sizeof(enable)) != noErr) {
        qWarning() << "QAudioSource: Unable to switch to input mode (Disable output)";
        return false;
    }

    // register callback
    AURenderCallbackStruct callback;
    callback.inputProc = inputCallback;
    callback.inputProcRefCon = this;

    if (AudioUnitSetProperty(m_audioUnit,
                               kAudioOutputUnitProperty_SetInputCallback,
                               kAudioUnitScope_Global,
                               0,
                               &callback,
                               sizeof(callback)) != noErr) {
        qWarning() << "QAudioSource: Failed to set AudioUnit callback";
        return false;
    }

#if defined(Q_OS_MACOS)
    //Set Audio Device
    if (AudioUnitSetProperty(m_audioUnit,
                             kAudioOutputUnitProperty_CurrentDevice,
                             kAudioUnitScope_Global,
                             0,
                             &m_audioDeviceId,
                             sizeof(m_audioDeviceId)) != noErr) {
        qWarning() << "QAudioSource: Unable to use configured device";
        return false;
    }
#endif

    //set format
    m_streamFormat = CoreAudioUtils::toAudioStreamBasicDescription(m_audioFormat);

#if defined(Q_OS_MACOS)
    UInt32 size = 0;

    if (m_audioFormat == m_audioDeviceInfo.preferredFormat()) {
#endif

    m_deviceFormat = m_streamFormat;
    AudioUnitSetProperty(m_audioUnit,
                         kAudioUnitProperty_StreamFormat,
                         kAudioUnitScope_Output,
                         1,
                         &m_deviceFormat,
                         sizeof(m_deviceFormat));
#if defined(Q_OS_MACOS)
    } else {
        size = sizeof(m_deviceFormat);
        if (AudioUnitGetProperty(m_audioUnit,
                                 kAudioUnitProperty_StreamFormat,
                                 kAudioUnitScope_Input,
                                 1,
                                 &m_deviceFormat,
                                 &size) != noErr) {
            qWarning() << "QAudioSource: Unable to retrieve device format";
            return false;
        }

        if (AudioUnitSetProperty(m_audioUnit,
                                 kAudioUnitProperty_StreamFormat,
                                 kAudioUnitScope_Output,
                                 1,
                                 &m_deviceFormat,
                                 sizeof(m_deviceFormat)) != noErr) {
            qWarning() << "QAudioSource: Unable to set device format";
            return false;
        }
    }
#endif

    //setup buffers
    UInt32 numberOfFrames;
#if defined(Q_OS_MACOS)
    size = sizeof(UInt32);
    if (AudioUnitGetProperty(m_audioUnit,
                             kAudioDevicePropertyBufferFrameSize,
                             kAudioUnitScope_Global,
                             0,
                             &numberOfFrames,
                             &size) != noErr) {
        qWarning() << "QAudioSource: Failed to get audio period size";
        return false;
    }
    //BUG: numberOfFrames gets ignored after this point

    AudioValueRange bufferRange;
    size = sizeof(AudioValueRange);

    if (AudioUnitGetProperty(m_audioUnit,
                             kAudioDevicePropertyBufferFrameSizeRange,
                             kAudioUnitScope_Global,
                             0,
                             &bufferRange,
                             &size) != noErr) {
        qWarning() << "QAudioSource: Failed to get audio period size range";
        return false;
    }

    // See if the requested buffer size is permissible
    numberOfFrames = qBound((UInt32)bufferRange.mMinimum, m_internalBufferSize / m_streamFormat.mBytesPerFrame, (UInt32)bufferRange.mMaximum);

    // Set it back
    if (AudioUnitSetProperty(m_audioUnit,
                             kAudioDevicePropertyBufferFrameSize,
                             kAudioUnitScope_Global,
                             0,
                             &numberOfFrames,
                             sizeof(UInt32)) != noErr) {
        qWarning() << "QAudioSource: Failed to set audio buffer size";
        return false;
    }
#else //iOS
    Float32 bufferSize = CoreAudioSessionManager::instance().currentIOBufferDuration();
    bufferSize *= m_streamFormat.mSampleRate;
    numberOfFrames = bufferSize;
#endif

    // Now allocate a few buffers to be safe.
    m_periodSizeBytes = m_internalBufferSize = numberOfFrames * m_streamFormat.mBytesPerFrame;

    {
        // LATER: we probably to derive the ringbuffer size from an absolute latency rather than
        // tying it to coreaudio
        const int ringbufferSize = m_internalBufferSize * 4;

        m_audioBuffer = std::make_unique<QDarwinAudioSourceBuffer>(
                *this, ringbufferSize, m_periodSizeBytes, m_deviceFormat, m_streamFormat, this);
    }
    m_audioIO = new QDarwinAudioSourceDevice(m_audioBuffer.get(), this);

    // Init
    if (AudioUnitInitialize(m_audioUnit) != noErr) {
        qWarning() << "QAudioSource: Failed to initialize AudioUnit";
        return false;
    }

    m_isOpen = true;

    return m_isOpen;

}

void QDarwinAudioSource::close()
{
    if (m_audioUnit != 0) {
        m_stateMachine.stop();

        AudioUnitUninitialize(m_audioUnit);
        AudioComponentInstanceDispose(m_audioUnit);
        m_audioUnit = 0;
    }

    m_audioBuffer.reset();
    m_isOpen = false;
}

void QDarwinAudioSource::onAudioDeviceError()
{
    m_stateMachine.stop(QAudio::IOError);
}

void QDarwinAudioSource::updateAudioDevice()
{
    const QtAudio::State state = m_stateMachine.state();

    Q_ASSERT(m_audioBuffer);
    Q_ASSERT(m_audioUnit);

    // stop before m_audioBuffer->reset to get around potential
    // race conditions
    const bool unitStarted = state == QAudio::ActiveState || state == QAudio::IdleState;
    if (m_audioUnitStarted != unitStarted) {
        m_audioUnitStarted = unitStarted;
        if (unitStarted)
            AudioOutputUnitStart(m_audioUnit);
        else
            AudioOutputUnitStop(m_audioUnit);
    }

    if (state == QAudio::StoppedState)
        m_audioBuffer->reset();
    else
        m_audioBuffer->setFlushingEnabled(state != QAudio::SuspendedState);
}

void QDarwinAudioSource::start(QIODevice *device)
{
    reset();

    if (!m_audioDeviceInfo.isFormatSupported(m_audioFormat) || !open()) {
        m_stateMachine.setError(QAudio::OpenError);
        return;
    }

    if (!device) {
        m_stateMachine.setError(QAudio::IOError);
        return;
    }

    m_audioBuffer->setFlushDevice(device);

    // Start
    m_totalFrames = 0;
    m_stateMachine.start(false);
}

QIODevice *QDarwinAudioSource::start()
{
    reset();

    if (!m_audioDeviceInfo.isFormatSupported(m_audioFormat) || !open()) {
        m_stateMachine.setError(QAudio::OpenError);
        return m_audioIO;
    }

    m_audioBuffer->setFlushDevice(nullptr);

    // Start
    m_totalFrames = 0;
    m_stateMachine.start(false);

    return m_audioIO;
}


void QDarwinAudioSource::stop()
{
    if (m_audioBuffer)
        m_audioBuffer->flushAll();

    m_stateMachine.stop(QAudio::NoError);
}

void QDarwinAudioSource::reset()
{
    m_stateMachine.stopOrUpdateError();
}

void QDarwinAudioSource::suspend()
{
    m_stateMachine.suspend();
}

void QDarwinAudioSource::resume()
{
    m_stateMachine.resume();
}

qsizetype QDarwinAudioSource::bytesReady() const
{
    return m_audioBuffer ? m_audioBuffer->used() : 0;
}

void QDarwinAudioSource::setBufferSize(qsizetype value)
{
    m_internalBufferSize = value;
}

qsizetype QDarwinAudioSource::bufferSize() const
{
    return m_internalBufferSize;
}

qint64 QDarwinAudioSource::processedUSecs() const
{
    return m_totalFrames * 1000000 / m_audioFormat.sampleRate();
}

QAudio::Error QDarwinAudioSource::error() const
{
    return m_stateMachine.error();
}

QAudio::State QDarwinAudioSource::state() const
{
    return m_stateMachine.state();
}

void QDarwinAudioSource::setFormat(const QAudioFormat &format)
{
    if (state() == QAudio::StoppedState)
        m_audioFormat = format;
}

QAudioFormat QDarwinAudioSource::format() const
{
    return m_audioFormat;
}

void QDarwinAudioSource::setVolume(qreal volume)
{
    m_volume = volume;
}

qreal QDarwinAudioSource::volume() const
{
    return m_volume;
}

void QDarwinAudioSource::onAudioDeviceActive()
{
    m_stateMachine.updateActiveOrIdle(true);
}

void QDarwinAudioSource::onAudioDeviceFull()
{
    m_stateMachine.updateActiveOrIdle(false, QAudio::UnderrunError);
}

OSStatus QDarwinAudioSource::inputCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
    Q_UNUSED(ioData);

    QDarwinAudioSource* d = static_cast<QDarwinAudioSource*>(inRefCon);

    if (d->state() != QtAudio::State::StoppedState) {
        const qint64 framesWritten = d->m_audioBuffer->renderFromDevice(
                d->m_audioUnit, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames);

        if (framesWritten > 0) {
            d->m_totalFrames += framesWritten;
            d->onAudioDeviceActive();
        } else if (framesWritten == 0)
            d->onAudioDeviceFull();
        else if (framesWritten < 0)
            d->onAudioDeviceError();
    }

    return noErr;
}

QT_END_NAMESPACE

#include "moc_qdarwinaudiosource_p.cpp"
