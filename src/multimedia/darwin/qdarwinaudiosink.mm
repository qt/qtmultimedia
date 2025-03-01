// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinaudiosink_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtGui/qguiapplication.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qcoreaudioutils_p.h>
#include <QtMultimedia/private/qdarwinaudiodevice_p.h>
#include <QtMultimedia/private/qdarwinaudiodevices_p.h>

#include <AudioUnit/AudioUnit.h>
#if defined(Q_OS_MACOS)
#  include <AudioUnit/AudioComponent.h>
#  include <QtMultimedia/private/qmacosaudiodatautils_p.h>
#else
#  include <QtMultimedia/private/qaudiohelpers_p.h>
#  include <QtMultimedia/private/qcoreaudiosessionmanager_p.h>
#endif


QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcDarwinAudioSink, "qt.multimedia.darwin.audiosink")

static int audioRingBufferSize(int bufferSize, int maxPeriodSize)
{
    // TODO: review this code
    return bufferSize
            + (bufferSize % maxPeriodSize == 0 ? 0 : maxPeriodSize - (bufferSize % maxPeriodSize));
}

QDarwinAudioSinkBuffer::QDarwinAudioSinkBuffer(int bufferSize, int maxPeriodSize,
                                               const QAudioFormat &audioFormat)
    : m_maxPeriodSize(maxPeriodSize),
      m_bytesPerFrame(audioFormat.bytesPerFrame()),
      m_periodTime(maxPeriodSize / m_bytesPerFrame * 1000 / audioFormat.sampleRate()),
      m_buffer(audioRingBufferSize(bufferSize, maxPeriodSize))
{
    m_fillTimer = new QTimer(this);
    m_fillTimer->setTimerType(Qt::PreciseTimer);
    m_fillTimer->setInterval(m_buffer.size() / 2 / m_maxPeriodSize * m_periodTime);
    connect(m_fillTimer, &QTimer::timeout, this, &QDarwinAudioSinkBuffer::fillBuffer);
}

QDarwinAudioSinkBuffer::~QDarwinAudioSinkBuffer() = default;

qint64 QDarwinAudioSinkBuffer::readFrames(char *data, qint64 maxFrames)
{
    bool    wecan = true;
    qint64  framesRead = 0;

    while (wecan && framesRead < maxFrames) {
        QSpan<const char> region =
                m_buffer.acquireReadRegion((maxFrames - framesRead) * m_bytesPerFrame);

        if (region.size() > 0) {
            // Ensure that we only read whole frames.
            region = region.first(region.size() - region.size() % m_bytesPerFrame);

            if (region.size() > 0) {
                memcpy(data + (framesRead * m_bytesPerFrame), region.data(), region.size());
                framesRead += region.size() / m_bytesPerFrame;
            } else
                wecan = false; // If there is only a partial frame left we should exit.
        } else
            wecan = false;

        if (region.size())
            m_buffer.releaseReadRegion(region.size());
    }

    if (framesRead == 0 && m_deviceError)
        framesRead = -1;

    return framesRead;
}

qint64 QDarwinAudioSinkBuffer::writeBytes(const char *data, qint64 maxSize)
{
    maxSize -= maxSize % m_bytesPerFrame; // only write full frames;
    qint64 bytesWritten = m_buffer.write(QSpan<const char>(data, maxSize));

    if (bytesWritten > 0)
        emit readyRead();

    return bytesWritten;
}

int QDarwinAudioSinkBuffer::available() const
{
    return m_buffer.free();
}

bool QDarwinAudioSinkBuffer::deviceAtEnd() const
{
    return m_deviceAtEnd;
}

void QDarwinAudioSinkBuffer::reset()
{
    m_buffer.reset();
    setFillingEnabled(false);
    setPrefetchDevice(nullptr);
}

void QDarwinAudioSinkBuffer::setPrefetchDevice(QIODevice *device)
{
    if (std::exchange(m_device, device) == device)
        return;

    m_deviceError = false;
    m_deviceAtEnd = device && device->atEnd();
    const auto wasFillingEnabled = m_fillingEnabled;
    setFillingEnabled(false);
    setFillingEnabled(wasFillingEnabled);
}

QIODevice *QDarwinAudioSinkBuffer::prefetchDevice() const
{
    return m_device;
}

void QDarwinAudioSinkBuffer::setFillingEnabled(bool enabled)
{
    if (std::exchange(m_fillingEnabled, enabled) == enabled)
        return;

    if (!enabled)
        m_fillTimer->stop();
    else if (m_device) {
        m_fillTimer->start();
        // call fillBuffer() after m_fillTimer->start() so that
        // it can stop the timer if an error has occurred.
        fillBuffer();
    }
}

void QDarwinAudioSinkBuffer::fillBuffer()
{
    const int free = m_buffer.free();
    const int writeSize = free - (free % m_maxPeriodSize);

    if (writeSize > 0) {
        bool    wecan = true;
        int     filled = 0;

        while (!m_deviceError && wecan && filled < writeSize) {
            QSpan<char> region = m_buffer.acquireWriteRegion(writeSize - filled);

            if (region.size() > 0) {
                const int bytesRead = m_device->read(region.data(), region.size());
                m_deviceAtEnd = m_device->atEnd();
                if (bytesRead > 0) {
                    filled += bytesRead;
                    m_buffer.releaseWriteRegion(bytesRead);
                } else if (bytesRead == 0)
                    wecan = false;
                else if (bytesRead < 0) {
                    setFillingEnabled(false);
                    m_deviceError = true;
                }
            } else
                wecan = false;
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

QDarwinAudioSink::QDarwinAudioSink(const QAudioDevice &device, const QAudioFormat &format,
                                   QObject *parent)
    : QPlatformAudioSink(parent),
      m_audioDevice(device),
      m_audioFormat(format),
      m_stateMachine(*this)
{
    connect(this, &QDarwinAudioSink::stateChanged, this, &QDarwinAudioSink::updateAudioDevice);
#ifdef Q_OS_IOS
    if (qGuiApp)
        connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &QDarwinAudioSink::appStateChanged);
#endif
}

QDarwinAudioSink::~QDarwinAudioSink()
{
    close();
}

void QDarwinAudioSink::start(QIODevice *device)
{
    reset();

    if (!m_audioDevice.isFormatSupported(m_audioFormat) || !open()) {
        m_stateMachine.setError(QAudio::OpenError);
        return;
    }

    if (!device) {
        m_stateMachine.setError(QAudio::IOError);
        return;
    }

    m_audioBuffer->setPrefetchDevice(device);

    m_totalFrames = 0;

    m_stateMachine.start();
}

QIODevice *QDarwinAudioSink::start()
{
    reset();

    if (!m_audioDevice.isFormatSupported(m_audioFormat) || !open()) {
        m_stateMachine.setError(QAudio::OpenError);
        return m_audioIO;
    }

    m_totalFrames = 0;

    m_stateMachine.start(QAudioStateMachine::RunningState::Idle);

    return m_audioIO;
}

void QDarwinAudioSink::stop()
{
    if (m_audioBuffer)
        m_audioBuffer->setFillingEnabled(false);

    if (auto notifier = m_stateMachine.stop(QAudio::NoError, true)) {
        Q_ASSERT((notifier.prevAudioState() == QAudio::ActiveState) == notifier.isDraining());
        if (notifier.isDraining() && !m_drainSemaphore.tryAcquire(1, 500)) {
            const bool wasDraining = m_stateMachine.onDrained();

            qWarning() << "Failed wait for getting sink drained; was draining:" << wasDraining;

            // handle a rare corner case when the audio thread managed to release the semaphore in
            // the time window between tryAcquire and onDrained
            if (!wasDraining)
                m_drainSemaphore.acquire();
        }
    }
}

void QDarwinAudioSink::reset()
{
    onAudioDeviceDrained();
    m_stateMachine.stopOrUpdateError();
}

void QDarwinAudioSink::suspend()
{
    m_stateMachine.suspend();
}

void QDarwinAudioSink::resume()
{
    m_stateMachine.resume();
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
    m_stateMachine.activateFromIdle();
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
            if (stopped) {
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
            if (framesRead == 0)
                d->onAudioDeviceIdle();
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

#if defined(Q_OS_MACOS)
    // Find the the most recent CoreAudio AudioDeviceID for the current device
    // to start the audio stream.
    std::optional<AudioDeviceID> audioDeviceId = QCoreAudioUtils::findAudioDeviceId(m_audioDevice);
    if (!audioDeviceId) {
        qWarning() << "QAudioSource: Unable to use find most recent CoreAudio AudioDeviceID for "
                      "given device-id. The device might not be connected.";
        return false;
    }
    const AudioDeviceID nativeDeviceId = audioDeviceId.value();
#endif

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

    AudioComponent component = AudioComponentFindNext(nullptr, &componentDescription);
    if (component == nullptr) {
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
    // register listener
    if (!addDisconnectListener(*audioDeviceId))
        return false;

    //Set Audio Device
    if (AudioUnitSetProperty(m_audioUnit,
                             kAudioOutputUnitProperty_CurrentDevice,
                             kAudioUnitScope_Global,
                             0,
                             &nativeDeviceId,
                             sizeof(nativeDeviceId))
        != noErr) {
        qWarning() << "QAudioOutput: Unable to use configured device";
        return false;
    }
#endif
    UInt32 size;


    // Set stream format
    m_streamFormat = QCoreAudioUtils::toAudioStreamBasicDescription(m_audioFormat);
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
    connect(m_audioBuffer.get(), &QDarwinAudioSinkBuffer::readyRead, this,
            &QDarwinAudioSink::inputReady); // Pull

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
    if (m_audioUnit != nullptr) {
        m_stateMachine.stop();

#if defined(Q_OS_MACOS)
        removeDisconnectListener();
#endif
        AudioUnitUninitialize(m_audioUnit);
        AudioComponentInstanceDispose(m_audioUnit);
        m_audioUnit = nullptr;
    }

    m_audioBuffer.reset();
}

void QDarwinAudioSink::onAudioDeviceIdle()
{
    const bool atEnd = m_audioBuffer->deviceAtEnd();
    if (!m_stateMachine.updateActiveOrIdle(QAudioStateMachine::RunningState::Idle,
                                           atEnd ? QAudio::NoError : QAudio::UnderrunError))
        onAudioDeviceDrained();
}

void QDarwinAudioSink::onAudioDeviceDrained()
{
    if (m_stateMachine.onDrained())
        m_drainSemaphore.release();
}

#if defined(Q_OS_MACOS)
bool QDarwinAudioSink::addDisconnectListener(AudioObjectID id)
{
    m_stopOnDisconnected.cancel();

    if (!m_disconnectMonitor.addDisconnectListener(id))
        return false;

    m_stopOnDisconnected = m_disconnectMonitor.then(this, [this] {
        m_stateMachine.stop(QAudio::IOError);
    });

    return true;
}

void QDarwinAudioSink::removeDisconnectListener()
{
    m_stopOnDisconnected.cancel();
    m_disconnectMonitor.removeDisconnectListener();
}
#endif

void QDarwinAudioSink::onAudioDeviceError()
{
    m_stateMachine.stop(QAudio::IOError);
}

void QDarwinAudioSink::updateAudioDevice()
{
    const auto state = m_stateMachine.state();
    const auto desiredAudioUnitState = state == QAudio::ActiveState ? AudioUnitState::Started : AudioUnitState::Stopped;

    Q_ASSERT(m_audioUnit);
    if (m_audioUnitState != desiredAudioUnitState) {
        if (desiredAudioUnitState == AudioUnitState::Started) {
            const auto status = AudioOutputUnitStart(m_audioUnit);
            if (status != noErr) {
                qCWarning(qLcDarwinAudioSink) << "AudioOutputUnitStart failed with error:"
                                              << status << "the current application state is:"
                                              << QGuiApplication::applicationState();
                m_audioUnitState = AudioUnitState::StartFailed;
            } else {
                m_audioUnitState = desiredAudioUnitState;
            }
        } else {
            if (m_audioUnitState == AudioUnitState::Started)
                AudioOutputUnitStop(m_audioUnit);
            m_audioUnitState = desiredAudioUnitState;
        }
    }

    Q_ASSERT(m_audioBuffer);
    if (state == QAudio::StoppedState) {
        m_audioBuffer->reset();
    } else {
        m_audioBuffer->setFillingEnabled(state != QAudio::SuspendedState
                                         && m_audioUnitState != AudioUnitState::StartFailed);
    }
}

void QDarwinAudioSink::appStateChanged(Qt::ApplicationState newState)
{
#ifdef Q_OS_IOS
    if (newState == Qt::ApplicationActive && m_audioUnitState == AudioUnitState::StartFailed) {
        // Retry audio unit start:
        updateAudioDevice();
        if (m_audioUnitState != AudioUnitState::Started) {
            // Failed again?
            m_stateMachine.forceSetState(QAudio::StoppedState, QAudio::IOError);
        }
    }
#else
    Q_UNUSED(newState);
#endif // Q_OS_IOS
}

QT_END_NAMESPACE

#include "moc_qdarwinaudiosink_p.cpp"
