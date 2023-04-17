// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qdarwinaudiosink_p.h"
#include "qcoreaudiosessionmanager_p.h"
#include "qdarwinaudiodevice_p.h"
#include "qcoreaudioutils_p.h"
#include "qdarwinmediadevices_p.h"
#include <qmediadevices.h>

#include <QtCore/QDataStream>
#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#if defined(Q_OS_MACOS)
# include <AudioUnit/AudioComponent.h>
#endif

#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
# include <QtMultimedia/private/qaudiohelpers_p.h>
#endif

QT_BEGIN_NAMESPACE

static int audioRingBufferSize(int bufferSize, int maxPeriodSize)
{
    // TODO: review this code
    return bufferSize
            + (bufferSize % maxPeriodSize == 0 ? 0 : maxPeriodSize - (bufferSize % maxPeriodSize));
}

QDarwinAudioSinkBuffer::QDarwinAudioSinkBuffer(int bufferSize, int maxPeriodSize,
                                               const QAudioFormat &audioFormat)
    : m_deviceError(false),
      m_maxPeriodSize(maxPeriodSize),
      m_device(nullptr),
      m_buffer(
              std::make_unique<CoreAudioRingBuffer>(audioRingBufferSize(bufferSize, maxPeriodSize)))
{
    m_bytesPerFrame = audioFormat.bytesPerFrame();
    m_periodTime = m_maxPeriodSize / m_bytesPerFrame * 1000 / audioFormat.sampleRate();

    m_fillTimer = new QTimer(this);
    m_fillTimer->setTimerType(Qt::PreciseTimer);
    connect(m_fillTimer, SIGNAL(timeout()), SLOT(fillBuffer()));
}

QDarwinAudioSinkBuffer::~QDarwinAudioSinkBuffer() = default;

qint64 QDarwinAudioSinkBuffer::readFrames(char *data, qint64 maxFrames)
{
    bool    wecan = true;
    qint64  framesRead = 0;

    while (wecan && framesRead < maxFrames) {
        CoreAudioRingBuffer::Region region = m_buffer->acquireReadRegion((maxFrames - framesRead) * m_bytesPerFrame);

        if (region.second > 0) {
            // Ensure that we only read whole frames.
            region.second -= region.second % m_bytesPerFrame;

            if (region.second > 0) {
                memcpy(data + (framesRead * m_bytesPerFrame), region.first, region.second);
                framesRead += region.second / m_bytesPerFrame;
            } else
                wecan = false; // If there is only a partial frame left we should exit.
        }
        else
            wecan = false;

        m_buffer->releaseReadRegion(region);
    }

    if (framesRead == 0 && m_deviceError)
        framesRead = -1;

    return framesRead;
}

qint64 QDarwinAudioSinkBuffer::writeBytes(const char *data, qint64 maxSize)
{
    bool    wecan = true;
    qint64  bytesWritten = 0;

    maxSize -= maxSize % m_bytesPerFrame;
    while (wecan && bytesWritten < maxSize) {
        CoreAudioRingBuffer::Region region = m_buffer->acquireWriteRegion(maxSize - bytesWritten);

        if (region.second > 0) {
            memcpy(region.first, data + bytesWritten, region.second);
            bytesWritten += region.second;
        }
        else
            wecan = false;

        m_buffer->releaseWriteRegion(region);
    }

    if (bytesWritten > 0)
        emit readyRead();

    return bytesWritten;
}

int QDarwinAudioSinkBuffer::available() const
{
    return m_buffer->free();
}

void QDarwinAudioSinkBuffer::reset()
{
    m_buffer->reset();
    m_device = nullptr;
    m_deviceError = false;
}

void QDarwinAudioSinkBuffer::setPrefetchDevice(QIODevice *device)
{
    if (std::exchange(m_device, device) != device && device)
        fillBuffer();
}

QIODevice *QDarwinAudioSinkBuffer::prefetchDevice() const
{
    return m_device;
}

void QDarwinAudioSinkBuffer::startFillTimer()
{
    if (m_device)
        m_fillTimer->start(m_buffer->size() / 2 / m_maxPeriodSize * m_periodTime);
}

void QDarwinAudioSinkBuffer::stopFillTimer()
{
    m_fillTimer->stop();
}

void QDarwinAudioSinkBuffer::fillBuffer()
{
    const int free = m_buffer->free();
    const int writeSize = free - (free % m_maxPeriodSize);

    if (writeSize > 0) {
        bool    wecan = true;
        int     filled = 0;

        while (!m_deviceError && wecan && filled < writeSize) {
            CoreAudioRingBuffer::Region region = m_buffer->acquireWriteRegion(writeSize - filled);

            if (region.second > 0) {
                region.second = m_device->read(region.first, region.second);
                if (region.second > 0)
                    filled += region.second;
                else if (region.second == 0)
                    wecan = false;
                else if (region.second < 0) {
                    m_fillTimer->stop();
                    region.second = 0;
                    m_deviceError = true;
                }
            }
            else
                wecan = false;

            m_buffer->releaseWriteRegion(region);
        }

        if (filled > 0)
            emit readyRead();
    }
}

QDarwinAudioSinkDevice::QDarwinAudioSinkDevice(QDarwinAudioSinkBuffer *audioBuffer, QObject *parent)
    : QIODevice(parent)
    , m_audioBuffer(audioBuffer)
{
    open(QIODevice::WriteOnly | QIODevice::Unbuffered);
}

qint64 QDarwinAudioSinkDevice::readData(char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 QDarwinAudioSinkDevice::writeData(const char *data, qint64 len)
{
    return m_audioBuffer->writeBytes(data, len);
}

QDarwinAudioSink::QDarwinAudioSink(const QAudioDevice &device, QObject *parent)
    : QPlatformAudioSink(parent), m_audioDeviceInfo(device), m_stateMachine(*this)
{
    QAudioDevice di = device;
    if (di.isNull())
        di = QMediaDevices::defaultAudioOutput();
#if defined(Q_OS_MACOS)
    const QCoreAudioDeviceInfo *info = static_cast<const QCoreAudioDeviceInfo *>(di.handle());
    Q_ASSERT(info);
    m_audioDeviceId = info->deviceID();
#endif
    m_device = di.id();

    m_clockFrequency = CoreAudioUtils::frequency() / 1000;
}

QDarwinAudioSink::~QDarwinAudioSink()
{
    close();
}

void QDarwinAudioSink::start(QIODevice *device)
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

    auto guard = m_stateMachine.start();
    Q_ASSERT(guard);

    m_audioBuffer->reset();
    m_audioBuffer->setPrefetchDevice(device);

    m_pullMode = true;
    m_totalFrames = 0;

    audioThreadStart();
}

QIODevice *QDarwinAudioSink::start()
{
    reset();

    if (!m_audioDeviceInfo.isFormatSupported(m_audioFormat) || !open()) {
        m_stateMachine.setError(QAudio::OpenError);
        return m_audioIO;
    }

    auto guard = m_stateMachine.start(false);
    Q_ASSERT(guard);

    m_audioBuffer->reset();
    m_audioBuffer->setPrefetchDevice(nullptr);

    m_pullMode = false;
    m_totalFrames = 0;

    return m_audioIO;
}

void QDarwinAudioSink::stop()
{
    if (auto guard = m_stateMachine.stop(QAudio::NoError, true)) {
        stopTimers();

        if (guard.prevState() == QAudio::ActiveState) {
            m_stateMachine.waitForDrained(std::chrono::milliseconds(500));

            if (m_stateMachine.isDraining())
                qWarning() << "Failed wait for sink drained";

            audioDeviceStop();
        }
    }
}

void QDarwinAudioSink::reset()
{
    if (auto guard = m_stateMachine.stopOrUpdateError())
        audioThreadStop(guard.prevState());
}

void QDarwinAudioSink::suspend()
{
    if (auto guard = m_stateMachine.suspend())
        audioThreadStop(guard.prevState());
}

void QDarwinAudioSink::resume()
{
    if (auto guard = m_stateMachine.resume())
        audioThreadStart();
}

qsizetype QDarwinAudioSink::bytesFree() const
{
    return m_audioBuffer->available();
}

void QDarwinAudioSink::setBufferSize(qsizetype value)
{
    if (state() == QAudio::StoppedState)
        m_internalBufferSize = value;
}

qsizetype QDarwinAudioSink::bufferSize() const
{
    return m_internalBufferSize;
}

qint64 QDarwinAudioSink::processedUSecs() const
{
    return m_totalFrames * 1000000 / m_audioFormat.sampleRate();
}

QAudio::Error QDarwinAudioSink::error() const
{
    return m_stateMachine.error();
}

QAudio::State QDarwinAudioSink::state() const
{
    return m_stateMachine.state();
}

void QDarwinAudioSink::setFormat(const QAudioFormat &format)
{
    if (state() == QAudio::StoppedState)
        m_audioFormat = format;
}

QAudioFormat QDarwinAudioSink::format() const
{
    return m_audioFormat;
}

void QDarwinAudioSink::setVolume(qreal volume)
{
    m_cachedVolume = qBound(qreal(0.0), volume, qreal(1.0));
    if (!m_isOpen)
        return;

#if defined(Q_OS_MACOS)
    //on OS X the volume can be set directly on the AudioUnit
    if (AudioUnitSetParameter(m_audioUnit,
                              kHALOutputParam_Volume,
                              kAudioUnitScope_Global,
                              0 /* bus */,
                              m_cachedVolume,
                              0) == noErr)
        m_volume = m_cachedVolume;
#endif
}

qreal QDarwinAudioSink::volume() const
{
    return m_cachedVolume;
}

void QDarwinAudioSink::inputReady()
{
    if (auto guard = m_stateMachine.activateFromIdle())
        audioDeviceStart();
}

OSStatus QDarwinAudioSink::renderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
    Q_UNUSED(ioActionFlags);
    Q_UNUSED(inTimeStamp);
    Q_UNUSED(inBusNumber);
    Q_UNUSED(inNumberFrames);

    QDarwinAudioSink* d = static_cast<QDarwinAudioSink*>(inRefCon);

    const auto [drained, stopped] = d->m_stateMachine.getDrainedAndStopped();

    if (drained && stopped) {
        ioData->mBuffers[0].mDataByteSize = 0;
    } else {
        const UInt32 bytesPerFrame = d->m_streamFormat.mBytesPerFrame;
        qint64 framesRead;

        Q_ASSERT(ioData->mBuffers[0].mDataByteSize / bytesPerFrame == inNumberFrames);
        framesRead = d->m_audioBuffer->readFrames((char*)ioData->mBuffers[0].mData,
                                                 ioData->mBuffers[0].mDataByteSize / bytesPerFrame);

        if (framesRead > 0) {
            ioData->mBuffers[0].mDataByteSize = framesRead * bytesPerFrame;
            d->m_totalFrames += framesRead;

#if defined(Q_OS_MACOS)
            // If playback is already stopped.
            if (!drained) {
                qreal oldVolume = d->m_cachedVolume;
                // Decrease volume smoothly.
                d->setVolume(d->m_volume / 2);
                d->m_cachedVolume = oldVolume;
            }
#elif defined(Q_OS_IOS) || defined(Q_OS_TVOS)
            // on iOS we have to adjust the sound volume ourselves
            if (!qFuzzyCompare(d->m_cachedVolume, qreal(1.0f))) {
                QAudioHelperInternal::qMultiplySamples(d->m_cachedVolume,
                                                       d->m_audioFormat,
                                                       ioData->mBuffers[0].mData, /* input */
                                                       ioData->mBuffers[0].mData, /* output */
                                                       ioData->mBuffers[0].mDataByteSize);
            }
#endif

        }
        else {
            ioData->mBuffers[0].mDataByteSize = 0;
            if (framesRead == 0) {
                if (!drained)
                    d->onAudioDeviceDrained();
                else
                    d->onAudioDeviceIdle();
            }
            else
                d->onAudioDeviceError();
        }
    }

    return noErr;
}

bool QDarwinAudioSink::open()
{
#if defined(Q_OS_IOS)
    // Set default category to Ambient (implies MixWithOthers). This makes sure audio stops playing
    // if the screen is locked or if the Silent switch is toggled.
    CoreAudioSessionManager::instance().setCategory(CoreAudioSessionManager::Ambient, CoreAudioSessionManager::None);
    CoreAudioSessionManager::instance().setActive(true);
#endif

    if (error() != QAudio::NoError)
        return false;

    if (m_isOpen) {
        setVolume(m_cachedVolume);
        return true;
    }

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
        qWarning() << "QAudioOutput: Failed to find Output component";
        return false;
    }

    if (AudioComponentInstanceNew(component, &m_audioUnit) != noErr) {
        qWarning() << "QAudioOutput: Unable to Open Output Component";
        return false;
    }

    // register callback
    AURenderCallbackStruct callback;
    callback.inputProc = renderCallback;
    callback.inputProcRefCon = this;

    if (AudioUnitSetProperty(m_audioUnit,
                             kAudioUnitProperty_SetRenderCallback,
                             kAudioUnitScope_Global,
                             0,
                             &callback,
                             sizeof(callback)) != noErr) {
        qWarning() << "QAudioOutput: Failed to set AudioUnit callback";
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
        qWarning() << "QAudioOutput: Unable to use configured device";
        return false;
    }
#endif
    UInt32 size;


    // Set stream format
    m_streamFormat = CoreAudioUtils::toAudioStreamBasicDescription(m_audioFormat);
    size = sizeof(m_streamFormat);

    if (AudioUnitSetProperty(m_audioUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                0,
                                &m_streamFormat,
                                size) != noErr) {
        qWarning() << "QAudioOutput: Unable to Set Stream information";
        return false;
    }

    // Allocate buffer
    UInt32 numberOfFrames = 0;
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
#else //iOS
    Float32 bufferSize = CoreAudioSessionManager::instance().currentIOBufferDuration();
    bufferSize *= m_streamFormat.mSampleRate;
    numberOfFrames = bufferSize;
#endif

    m_periodSizeBytes = numberOfFrames * m_streamFormat.mBytesPerFrame;
    if (m_internalBufferSize < m_periodSizeBytes * 2)
        m_internalBufferSize = m_periodSizeBytes * 2;
    else
        m_internalBufferSize -= m_internalBufferSize % m_streamFormat.mBytesPerFrame;

    m_audioBuffer = std::make_unique<QDarwinAudioSinkBuffer>(m_internalBufferSize,
                                                             m_periodSizeBytes, m_audioFormat);
    connect(m_audioBuffer.get(), SIGNAL(readyRead()), SLOT(inputReady())); // Pull

    m_audioIO = new QDarwinAudioSinkDevice(m_audioBuffer.get(), this);

    //Init
    if (AudioUnitInitialize(m_audioUnit)) {
        qWarning() << "QAudioOutput: Failed to initialize AudioUnit";
        return false;
    }

    m_isOpen = true;

    setVolume(m_cachedVolume);

    return true;
}

void QDarwinAudioSink::close()
{
    if (m_audioUnit != 0) {
        if (auto guard = m_stateMachine.stop()) {
            audioThreadStop(guard.prevState());
            QSignalBlocker blocker(this);
            guard.reset();
        }
        AudioUnitUninitialize(m_audioUnit);
        AudioComponentInstanceDispose(m_audioUnit);
    }

    m_audioBuffer.reset();
}

void QDarwinAudioSink::audioThreadStart()
{
    startTimers();
    if (m_stateMachine.state() == QAudio::ActiveState)
        audioDeviceStart();
}

void QDarwinAudioSink::audioThreadStop(QAudio::State prevState)
{
    stopTimers();
    if (prevState == QAudio::ActiveState)
        audioDeviceStop();
}

void QDarwinAudioSink::audioDeviceStart()
{
    AudioOutputUnitStart(m_audioUnit);
}

void QDarwinAudioSink::audioDeviceStop()
{
    AudioOutputUnitStop(m_audioUnit);
}

void QDarwinAudioSink::onAudioDeviceIdle()
{
    if (auto guard = m_stateMachine.updateActiveOrIdle(false)) {
        auto device = m_audioBuffer ? m_audioBuffer->prefetchDevice() : nullptr;
        const bool atEnd = device && device->atEnd();
        guard.setError(atEnd ? QAudio::NoError : QAudio::UnderrunError);

        if (guard.prevState() == QAudio::ActiveState)
            audioDeviceStop();
    }
}

void QDarwinAudioSink::onAudioDeviceError()
{
    if (auto guard = m_stateMachine.stop(QAudio::IOError))
        audioThreadStop(guard.prevState());
}

void QDarwinAudioSink::onAudioDeviceDrained()
{
    m_stateMachine.onDrained();
}

void QDarwinAudioSink::startTimers()
{
    m_audioBuffer->startFillTimer();
}

void QDarwinAudioSink::stopTimers()
{
    m_audioBuffer->stopFillTimer();
}

QT_END_NAMESPACE

#include "moc_qdarwinaudiosink_p.cpp"
