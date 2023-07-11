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

    m_dataSize = 0;

    m_bufferList = reinterpret_cast<AudioBufferList*>(malloc(sizeof(AudioBufferList) +
                                                            (sizeof(AudioBuffer) * numberOfBuffers)));

    m_bufferList->mNumberBuffers = numberOfBuffers;
    for (int i = 0; i < numberOfBuffers; ++i) {
        m_bufferList->mBuffers[i].mNumberChannels = isInterleaved ? numberOfBuffers : 1;
        m_bufferList->mBuffers[i].mDataByteSize = 0;
        m_bufferList->mBuffers[i].mData = 0;
    }
}

QCoreAudioBufferList::QCoreAudioBufferList(const AudioStreamBasicDescription &streamFormat, char *buffer, int bufferSize)
    : m_owner(false)
    , m_streamDescription(streamFormat)
    , m_bufferList(0)
{
    m_dataSize = bufferSize;

    m_bufferList = reinterpret_cast<AudioBufferList*>(malloc(sizeof(AudioBufferList) + sizeof(AudioBuffer)));

    m_bufferList->mNumberBuffers = 1;
    m_bufferList->mBuffers[0].mNumberChannels = 1;
    m_bufferList->mBuffers[0].mDataByteSize = m_dataSize;
    m_bufferList->mBuffers[0].mData = buffer;
}

QCoreAudioBufferList::QCoreAudioBufferList(const AudioStreamBasicDescription &streamFormat, int framesToBuffer)
    : m_owner(true)
    , m_streamDescription(streamFormat)
    , m_bufferList(0)
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
    m_position = 0;
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

QDarwinAudioSourceBuffer::QDarwinAudioSourceBuffer(int bufferSize, int maxPeriodSize, const AudioStreamBasicDescription &inputFormat, const AudioStreamBasicDescription &outputFormat, QObject *parent)
    : QObject(parent)
    , m_deviceError(false)
    , m_device(0)
    , m_audioConverter(0)
    , m_inputFormat(inputFormat)
    , m_outputFormat(outputFormat)
    , m_volume(qreal(1.0f))
{
    m_maxPeriodSize = maxPeriodSize;
    m_periodTime = m_maxPeriodSize / m_outputFormat.mBytesPerFrame * 1000 / m_outputFormat.mSampleRate;

    m_buffer = new CoreAudioRingBuffer(bufferSize);

    m_inputBufferList = new QCoreAudioBufferList(m_inputFormat);

    m_flushTimer = new QTimer(this);
    connect(m_flushTimer, SIGNAL(timeout()), this, SLOT(flushBuffer()));

    if (CoreAudioUtils::toQAudioFormat(inputFormat) != CoreAudioUtils::toQAudioFormat(outputFormat)) {
        if (AudioConverterNew(&m_inputFormat, &m_outputFormat, &m_audioConverter) != noErr) {
            qWarning() << "QAudioSource: Unable to create an Audio Converter";
            m_audioConverter = 0;
        }
    }

    m_qFormat = CoreAudioUtils::toQAudioFormat(inputFormat); // we adjust volume before conversion
}

QDarwinAudioSourceBuffer::~QDarwinAudioSourceBuffer()
{
    delete m_buffer;
}

qreal QDarwinAudioSourceBuffer::volume() const
{
    return m_volume;
}

void QDarwinAudioSourceBuffer::setVolume(qreal v)
{
    m_volume = v;
}

qint64 QDarwinAudioSourceBuffer::renderFromDevice(AudioUnit audioUnit, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames)
{
    const bool  pullMode = m_device == 0;

    OSStatus    err;
    qint64      framesRendered = 0;

    m_inputBufferList->reset();
    err = AudioUnitRender(audioUnit,
                          ioActionFlags,
                          inTimeStamp,
                          inBusNumber,
                          inNumberFrames,
                          m_inputBufferList->audioBufferList());

    // adjust volume, if necessary
    if (!qFuzzyCompare(m_volume, qreal(1.0f))) {
        QAudioHelperInternal::qMultiplySamples(m_volume,
                                               m_qFormat,
                                               m_inputBufferList->data(), /* input */
                                               m_inputBufferList->data(), /* output */
                                               m_inputBufferList->bufferSize());
    }

    if (m_audioConverter != 0) {
        QCoreAudioPacketFeeder  feeder(m_inputBufferList);

        int     copied = 0;
        const int available = m_buffer->free();

        while (err == noErr && !feeder.empty()) {
            CoreAudioRingBuffer::Region region = m_buffer->acquireWriteRegion(available - copied);

            if (region.second == 0)
                break;

            AudioBufferList     output;
            output.mNumberBuffers = 1;
            output.mBuffers[0].mNumberChannels = 1;
            output.mBuffers[0].mDataByteSize = region.second;
            output.mBuffers[0].mData = region.first;

            UInt32  packetSize = region.second / m_outputFormat.mBytesPerPacket;
            err = AudioConverterFillComplexBuffer(m_audioConverter,
                                                  converterCallback,
                                                  &feeder,
                                                  &packetSize,
                                                  &output,
                                                  0);
            region.second = output.mBuffers[0].mDataByteSize;
            copied += region.second;

            m_buffer->releaseWriteRegion(region);
        }

        framesRendered += copied / m_outputFormat.mBytesPerFrame;
    }
    else {
        const int available = m_inputBufferList->bufferSize();
        bool    wecan = true;
        int     copied = 0;

        while (wecan && copied < available) {
            CoreAudioRingBuffer::Region region = m_buffer->acquireWriteRegion(available - copied);

            if (region.second > 0) {
                memcpy(region.first, m_inputBufferList->data() + copied, region.second);
                copied += region.second;
            }
            else
                wecan = false;

            m_buffer->releaseWriteRegion(region);
        }

        framesRendered = copied / m_outputFormat.mBytesPerFrame;
    }

    if (pullMode && framesRendered > 0)
        emit readyRead();

    return framesRendered;
}

qint64 QDarwinAudioSourceBuffer::readBytes(char *data, qint64 len)
{
    bool    wecan = true;
    qint64  bytesCopied = 0;

    len -= len % m_maxPeriodSize;
    while (wecan && bytesCopied < len) {
        CoreAudioRingBuffer::Region region = m_buffer->acquireReadRegion(len - bytesCopied);

        if (region.second > 0) {
            memcpy(data + bytesCopied, region.first, region.second);
            bytesCopied += region.second;
        }
        else
            wecan = false;

        m_buffer->releaseReadRegion(region);
    }

    return bytesCopied;
}

void QDarwinAudioSourceBuffer::setFlushDevice(QIODevice *device)
{
    if (m_device != device)
        m_device = device;
}

void QDarwinAudioSourceBuffer::startFlushTimer()
{
    if (m_device != 0) {
        // We use the period time for the timer, since that's
        // around the buffer size (pre conversion >.>)
        m_flushTimer->start(qMax(1, m_periodTime));
    }
}

void QDarwinAudioSourceBuffer::stopFlushTimer()
{
    m_flushTimer->stop();
}

void QDarwinAudioSourceBuffer::flush(bool all)
{
    if (m_device == 0)
        return;

    const int used = m_buffer->used();
    const int readSize = all ? used : used - (used % m_maxPeriodSize);

    if (readSize > 0) {
        bool    wecan = true;
        int     flushed = 0;

        while (!m_deviceError && wecan && flushed < readSize) {
            CoreAudioRingBuffer::Region region = m_buffer->acquireReadRegion(readSize - flushed);

            if (region.second > 0) {
                int bytesWritten = m_device->write(region.first, region.second);
                if (bytesWritten < 0) {
                    stopFlushTimer();
                    m_deviceError = true;
                }
                else {
                    region.second = bytesWritten;
                    flushed += bytesWritten;
                    wecan = bytesWritten != 0;
                }
            }
            else
                wecan = false;

            m_buffer->releaseReadRegion(region);
        }
    }
}

void QDarwinAudioSourceBuffer::reset()
{
    m_buffer->reset();
    m_deviceError = false;
}

int QDarwinAudioSourceBuffer::available() const
{
    return m_buffer->free();
}

int QDarwinAudioSourceBuffer::used() const
{
    return m_buffer->used();
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
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    connect(m_audioBuffer, SIGNAL(readyRead()), this, SIGNAL(readyRead()));
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
    : QPlatformAudioSource(parent)
    , m_audioDeviceInfo(device)
    , m_isOpen(false)
    , m_internalBufferSize(DEFAULT_BUFFER_SIZE)
    , m_totalFrames(0)
    , m_audioUnit(0)
    , m_clockFrequency(CoreAudioUtils::frequency() / 1000)
    , m_errorCode(QAudio::NoError)
    , m_stateCode(QAudio::StoppedState)
    , m_audioBuffer(nullptr)
    , m_volume(1.0)
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
        QMutexLocker lock(m_audioBuffer);
        m_audioBuffer = new QDarwinAudioSourceBuffer(m_internalBufferSize * 4,
                                            m_periodSizeBytes,
                                            m_deviceFormat,
                                            m_streamFormat,
                                            this);

        m_audioBuffer->setVolume(m_volume);
    }
    m_audioIO = new QDarwinAudioSourceDevice(m_audioBuffer, this);

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
    stop();
    if (m_audioUnit != 0) {
        AudioOutputUnitStop(m_audioUnit);
        AudioUnitUninitialize(m_audioUnit);
        AudioComponentInstanceDispose(m_audioUnit);
    }

    delete m_audioBuffer;
    m_audioBuffer = nullptr;
    m_isOpen = false;
}

void QDarwinAudioSource::start(QIODevice *device)
{
    QIODevice* op = device;

    if (!m_audioDeviceInfo.isFormatSupported(m_audioFormat) || !open()) {
        m_stateCode = QAudio::StoppedState;
        m_errorCode = QAudio::OpenError;
        return;
    }

    reset();
    {
        QMutexLocker lock(m_audioBuffer);
        m_audioBuffer->reset();
        m_audioBuffer->setFlushDevice(op);
    }

    if (op == 0)
        op = m_audioIO;

    // Start
    m_totalFrames = 0;

    m_stateCode = QAudio::IdleState;
    m_errorCode = QAudio::NoError;
    emit stateChanged(m_stateCode);

    audioThreadStart();
}


QIODevice *QDarwinAudioSource::start()
{
    QIODevice* op = 0;

    if (!m_audioDeviceInfo.isFormatSupported(m_audioFormat) || !open()) {
        m_stateCode = QAudio::StoppedState;
        m_errorCode = QAudio::OpenError;
        return m_audioIO;
    }

    reset();
    {
        QMutexLocker lock(m_audioBuffer);
        m_audioBuffer->reset();
        m_audioBuffer->setFlushDevice(op);
    }

    if (op == 0)
        op = m_audioIO;

    // Start
    m_totalFrames = 0;

    m_stateCode = QAudio::IdleState;
    m_errorCode = QAudio::NoError;
    emit stateChanged(m_stateCode);

    audioThreadStart();

    return op;
}


void QDarwinAudioSource::stop()
{
    QMutexLocker lock(m_audioBuffer);
    if (m_stateCode != QAudio::StoppedState) {
        audioThreadStop();
        m_audioBuffer->flush(true);

        m_errorCode = QAudio::NoError;
        m_stateCode = QAudio::StoppedState;
        QMetaObject::invokeMethod(this, "stateChanged", Qt::QueuedConnection, Q_ARG(QAudio::State, m_stateCode));
    }
}


void QDarwinAudioSource::reset()
{
    QMutexLocker lock(m_audioBuffer);
    if (m_stateCode != QAudio::StoppedState) {
        audioThreadStop();

        m_errorCode = QAudio::NoError;
        m_stateCode = QAudio::StoppedState;
        m_audioBuffer->reset();
        QMetaObject::invokeMethod(this, "stateChanged", Qt::QueuedConnection, Q_ARG(QAudio::State, m_stateCode));
    }
}


void QDarwinAudioSource::suspend()
{
    QMutexLocker lock(m_audioBuffer);
    if (m_stateCode == QAudio::ActiveState || m_stateCode == QAudio::IdleState) {
        audioThreadStop();

        m_errorCode = QAudio::NoError;
        m_stateCode = QAudio::SuspendedState;
        QMetaObject::invokeMethod(this, "stateChanged", Qt::QueuedConnection, Q_ARG(QAudio::State, m_stateCode));
    }
}


void QDarwinAudioSource::resume()
{
    QMutexLocker lock(m_audioBuffer);
    if (m_stateCode == QAudio::SuspendedState) {
        audioThreadStart();

        m_errorCode = QAudio::NoError;
        m_stateCode = QAudio::ActiveState;
        QMetaObject::invokeMethod(this, "stateChanged", Qt::QueuedConnection, Q_ARG(QAudio::State, m_stateCode));
    }
}


qsizetype QDarwinAudioSource::bytesReady() const
{
    QMutexLocker lock(m_audioBuffer);
    if (!m_audioBuffer)
        return 0;
    return m_audioBuffer->used();
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
    return m_errorCode;
}


QAudio::State QDarwinAudioSource::state() const
{
    return m_stateCode;
}


void QDarwinAudioSource::setFormat(const QAudioFormat &format)
{
    if (m_stateCode == QAudio::StoppedState)
        m_audioFormat = format;
}


QAudioFormat QDarwinAudioSource::format() const
{
    return m_audioFormat;
}


void QDarwinAudioSource::setVolume(qreal volume)
{
    QMutexLocker lock(m_audioBuffer);
    m_volume = volume;
    if (m_audioBuffer)
        m_audioBuffer->setVolume(m_volume);
}


qreal QDarwinAudioSource::volume() const
{
    return m_volume;
}

void QDarwinAudioSource::deviceStoppped()
{
    stopTimers();
    emit stateChanged(m_stateCode);
}

void QDarwinAudioSource::audioThreadStart()
{
    startTimers();
    m_audioThreadState.storeRelaxed(Running);
    AudioOutputUnitStart(m_audioUnit);
}

void QDarwinAudioSource::audioThreadStop()
{
    stopTimers();
    if (m_audioThreadState.testAndSetAcquire(Running, Stopped))
        m_audioBuffer->wait();
}

void QDarwinAudioSource::audioDeviceStop()
{
    AudioOutputUnitStop(m_audioUnit);
    m_audioThreadState.storeRelaxed(Stopped);
    m_audioBuffer->wake();
}

void QDarwinAudioSource::audioDeviceActive()
{
    if (m_stateCode == QAudio::IdleState) {
        QMutexLocker lock(m_audioBuffer);
        m_stateCode = QAudio::ActiveState;
        emit stateChanged(m_stateCode);
    }
}

void QDarwinAudioSource::audioDeviceFull()
{
    if (m_stateCode == QAudio::ActiveState) {
        QMutexLocker lock(m_audioBuffer);
        m_errorCode = QAudio::UnderrunError;
        m_stateCode = QAudio::IdleState;
        emit stateChanged(m_stateCode);
    }
}

void QDarwinAudioSource::audioDeviceError()
{
    if (m_stateCode == QAudio::ActiveState) {
        QMutexLocker lock(m_audioBuffer);
        audioDeviceStop();

        m_errorCode = QAudio::IOError;
        m_stateCode = QAudio::StoppedState;
        QMetaObject::invokeMethod(this, "deviceStopped", Qt::QueuedConnection);
    }
}

void QDarwinAudioSource::startTimers()
{
    m_audioBuffer->startFlushTimer();
}

void QDarwinAudioSource::stopTimers()
{
    m_audioBuffer->stopFlushTimer();
}

OSStatus QDarwinAudioSource::inputCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
    Q_UNUSED(ioData);

    QDarwinAudioSource* d = static_cast<QDarwinAudioSource*>(inRefCon);

    const int threadState = d->m_audioThreadState.loadAcquire();
    if (threadState == Stopped)
        d->audioDeviceStop();
    else {
        qint64 framesWritten;

        {
            QMutexLocker locker(d->m_audioBuffer);
            framesWritten = d->m_audioBuffer->renderFromDevice(d->m_audioUnit,
                                                             ioActionFlags,
                                                             inTimeStamp,
                                                             inBusNumber,
                                                             inNumberFrames);
        }

        if (framesWritten > 0) {
            d->m_totalFrames += framesWritten;
            d->audioDeviceActive();
        } else if (framesWritten == 0)
            d->audioDeviceFull();
        else if (framesWritten < 0)
            d->audioDeviceError();
    }

    return noErr;
}

QT_END_NAMESPACE

#include "moc_qdarwinaudiosource_p.cpp"
