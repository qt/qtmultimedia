// Copyright (C) 2022 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinaudiosource_p.h"

#include <QtCore/qdatastream.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtGui/qguiapplication.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qaudio_qiodevice_support_p.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>
#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>
#include <QtMultimedia/private/qautoresetevent_p.h>
#include <QtMultimedia/private/qcoreaudioutils_p.h>
#include <QtMultimedia/private/qdarwinaudiodevice_p.h>
#include <QtMultimedia/private/qdarwinaudiodevices_p.h>

#ifdef Q_OS_MACOS
#  include <AudioUnit/AudioComponent.h>
#  include <QtMultimedia/private/qmacosaudiodatautils_p.h>
#else
#  include <QtMultimedia/private/qcoreaudiosessionmanager_p.h>
#endif

QT_BEGIN_NAMESPACE

class QCoreAudioSourceStream final : QtMultimediaPrivate::QPlatformAudioSourceStream
{
    using QPlatformAudioSourceStream = QtMultimediaPrivate::QPlatformAudioSourceStream;

public:
    explicit QCoreAudioSourceStream(QAudioDevice, const QAudioFormat &,
                                    std::optional<int> ringbufferSize, QDarwinAudioSource *parent,
                                    float volume, std::optional<int32_t> hardwareBufferFrames);
    ~QCoreAudioSourceStream();

    bool open();

    bool start(QIODevice *);
    QIODevice *start();
    void stop(ShutdownPolicy);

    void suspend();
    void resume();

    using QPlatformAudioSourceStream::bytesReady;
    using QPlatformAudioSourceStream::deviceIsRingbufferReader;
    using QPlatformAudioSourceStream::processedDuration;
    using QPlatformAudioSourceStream::ringbufferSizeInBytes;
    using QPlatformAudioSourceStream::setVolume;

    void resumeIfNecessary();

private:
    void updateStreamIdle(bool idle) override;
    void stopAudioUnit();

    static OSStatus inputCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                                  const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
                                  UInt32 inNumberFrames, AudioBufferList *ioData);

    OSStatus process(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *,
                     UInt32 inBusNumber, UInt32 inNumberFrames,
                     AudioBufferList *ioData) noexcept QT_MM_NONBLOCKING;

#ifdef Q_OS_MACOS
    bool addDisconnectListener(AudioObjectID id);
    void removeDisconnectListener();

    QCoreAudioUtils::DeviceDisconnectMonitor m_disconnectMonitor;
    QFuture<void> m_stopOnDisconnected;
#endif

    QCoreAudioUtils::AudioUnitHandle m_audioUnit;
    bool m_audioUnitRunning{};

    QDarwinAudioSource *m_parent;

    AudioBufferList m_bufferList{};
};

QCoreAudioSourceStream::QCoreAudioSourceStream(QAudioDevice audioDevice,
                                               const QAudioFormat &format,
                                               std::optional<int> ringbufferSize,
                                               QDarwinAudioSource *parent, float volume, std::optional<int32_t> hardwareBufferFrames)
    : QPlatformAudioSourceStream{
          std::move(audioDevice),
          format,
          ringbufferSize,
          hardwareBufferFrames,
          volume,
      },
      m_parent(parent)
{
}

QCoreAudioSourceStream::~QCoreAudioSourceStream()
{
    free(m_bufferList.mBuffers[0].mData);
}

bool QCoreAudioSourceStream::open()
{
    using namespace QCoreAudioUtils;

    if (auto audioUnit = makeAudioUnitForIO())
        m_audioUnit = std::move(*audioUnit);
    else
        return false;

    audioUnitSetInputEnabled(m_audioUnit, true);
    audioUnitSetOutputEnabled(m_audioUnit, false);

    // register callback
    AURenderCallbackStruct callback;
    callback.inputProc = inputCallback;
    callback.inputProcRefCon = this;

    if (AudioUnitSetProperty(m_audioUnit.get(), kAudioOutputUnitProperty_SetInputCallback,
                             kAudioUnitScope_Global, 0, &callback, sizeof(callback))
        != noErr) {
        qWarning() << "QAudioSource: Failed to set AudioUnit callback";
        return false;
    }

#ifdef Q_OS_MACOS
    // Find the the most recent CoreAudio AudioDeviceID for the current device
    // to start the audio stream.
    const std::optional<AudioDeviceID> nativeDeviceId = findAudioDeviceId(m_audioDevice);
    if (!nativeDeviceId) {
        qWarning() << "QAudioSource: Unable to use find most recent CoreAudio AudioDeviceID for "
                      "given device-id. The device might not be connected.";
        return false;
    }
    if (!addDisconnectListener(*nativeDeviceId))
        return false;

    // Set Audio Device
    if (!audioUnitSetCurrentDevice(m_audioUnit, *nativeDeviceId))
        return false;

    std::optional<int> bestNominalSamplingRate = audioObjectFindBestNominalSampleRate(
            *nativeDeviceId, QAudioDevice::Input, m_format.sampleRate());

    if (bestNominalSamplingRate) {
        if (!audioObjectSetSamplingRate(*nativeDeviceId, *bestNominalSamplingRate))
            return false;
    }

    if (m_hardwareBufferFrames)
        audioObjectSetFramesPerBuffer(*nativeDeviceId, *m_hardwareBufferFrames);
#endif

    AudioStreamBasicDescription streamFormat = toAudioStreamBasicDescription(m_format);

    audioUnitSetInputStreamFormat(m_audioUnit, 0, streamFormat);
    audioUnitSetOutputStreamFormat(m_audioUnit, 1, streamFormat);

    std::optional<int> framesPerBuffer = audioUnitGetFramesPerSlice(m_audioUnit);

    m_bufferList.mNumberBuffers = 1;
    m_bufferList.mBuffers[0].mNumberChannels = m_format.channelCount();
    m_bufferList.mBuffers[0].mDataByteSize =
            m_format.bytesForFrames(framesPerBuffer.value_or(2048));
    m_bufferList.mBuffers[0].mData = malloc(m_bufferList.mBuffers[0].mDataByteSize);

    return m_audioUnit.initialize();
}

bool QCoreAudioSourceStream::start(QIODevice *device)
{
    setQIODevice(device);

    const OSStatus status = AudioOutputUnitStart(m_audioUnit.get());
    if (status != noErr) {
        qDebug() << "AudioOutputUnitStart failed:" << status;
        return false;
    }

    m_audioUnitRunning = true;
    createQIODeviceConnections(device);

    return true;
}

QIODevice *QCoreAudioSourceStream::start()
{
    QIODevice *device = createRingbufferReaderDevice();
    bool opened = start(device);
    if (!opened)
        return nullptr;

    return device;
}

void QCoreAudioSourceStream::stop(ShutdownPolicy shutdownPolicy)
{
    requestStop();

    stopAudioUnit();

    disconnectQIODeviceConnections();

    finalizeQIODevice(shutdownPolicy);
    if (shutdownPolicy == ShutdownPolicy::DiscardRingbuffer)
        emptyRingbuffer();
}

void QCoreAudioSourceStream::suspend()
{
    const auto status = AudioOutputUnitStop(m_audioUnit.get());
    if (status == noErr)
        return;
    else
        qDebug() << "AudioOutputUnitStop failed:" << status;
}

void QCoreAudioSourceStream::resume()
{
    const auto status = AudioOutputUnitStart(m_audioUnit.get());
    if (status == noErr)
        return;
    else
        qDebug() << "AudioOutputUnitStart failed:" << status;
}

void QCoreAudioSourceStream::resumeIfNecessary()
{
    if (!audioUnitIsRunning(m_audioUnit))
        resume();
}

void QCoreAudioSourceStream::updateStreamIdle(bool idle)
{
    if (m_parent)
        m_parent->updateStreamIdle(idle);
}

void QCoreAudioSourceStream::stopAudioUnit()
{
    const auto status = AudioOutputUnitStop(m_audioUnit.get());
    if (status != noErr)
        qDebug() << "AudioOutputUnitStop failed:" << status;

    m_audioUnitRunning = false;

#ifdef Q_OS_MACOS
    removeDisconnectListener();
#endif
    m_audioUnit = {};
}

OSStatus QCoreAudioSourceStream::inputCallback(void *inRefCon,
                                               AudioUnitRenderActionFlags *ioActionFlags,
                                               const AudioTimeStamp *inTimeStamp,
                                               UInt32 inBusNumber, UInt32 inNumberFrames,
                                               AudioBufferList *ioData)
{
    return reinterpret_cast<QCoreAudioSourceStream *>(inRefCon)->process(
            ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
}

OSStatus QCoreAudioSourceStream::process(AudioUnitRenderActionFlags *ioActionFlags,
                                         const AudioTimeStamp *timeStamp, UInt32 inBusNumber,
                                         UInt32 inNumberFrames,
                                         AudioBufferList * /*ioData*/) noexcept QT_MM_NONBLOCKING
{
    OSStatus status = AudioUnitRender(m_audioUnit.get(), ioActionFlags, timeStamp, inBusNumber,
                                      inNumberFrames, &m_bufferList);

    switch (status) {
    case noErr:
        break;

    case kAudioUnitErr_CannotDoInCurrentContext:
        // it seems that during warmup, kAudioUnitErr_CannotDoInCurrentContext can occur for a few
        // times at startup
        return status;

    default:
        qDebug() << "AudioUnitRender failed" << status;
        return status;
    }

    QSpan<const std::byte> inputSpan{
        reinterpret_cast<const std::byte *>(m_bufferList.mBuffers[0].mData),
        m_bufferList.mBuffers[0].mDataByteSize,
    };

    QPlatformAudioSourceStream::process(inputSpan, inNumberFrames);

    return noErr;
}

#ifdef Q_OS_MACOS
bool QCoreAudioSourceStream::addDisconnectListener(AudioObjectID id)
{
    m_stopOnDisconnected.cancel();

    if (!m_disconnectMonitor.addDisconnectListener(id))
        return false;

    m_stopOnDisconnected = m_disconnectMonitor.then(m_parent, [this] {
        // Coreaudio will pause for a bit and restart the audio unit with a different device.
        // This is problematic, as it switches kAudioOutputUnitProperty_CurrentDevice and
        // invalidates the native device ID (and the disconnect handler). furthermore, we don't have
        // a way to re-synchronize the audio stream. so we explicitly stop the audio unit

        stopAudioUnit();
        finalizeQIODevice(ShutdownPolicy::DrainRingbuffer);

        QPlatformAudioSourceStream::handleIOError(m_parent);
    });

    return true;
}

void QCoreAudioSourceStream::removeDisconnectListener()
{
    m_stopOnDisconnected.cancel();
    m_disconnectMonitor.removeDisconnectListener();
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

QDarwinAudioSource::QDarwinAudioSource(QAudioDevice device, const QAudioFormat &format,
                                       QObject *parent)
    : QPlatformAudioSource(std::move(device), format, parent)
{
#ifndef Q_OS_MACOS
    if (qGuiApp)
        QObject::connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
                         [this](Qt::ApplicationState state) {
            if (state == Qt::ApplicationState::ApplicationActive)
                resumeStreamIfNecessary();
        });
#endif
}


QDarwinAudioSource::~QDarwinAudioSource()
{
    stop();
}

void QDarwinAudioSource::start(QIODevice *device)
{
    if (!device) {
        setError(QAudio::IOError);
        return;
    }

    m_stream = std::make_shared<QCoreAudioSourceStream>(
            m_audioDevice, m_format, m_internalBufferSize, this, volume(), m_hardwareBufferFrames);

    if (!m_stream->open()) {
        setError(QAudio::OpenError);
        m_stream = {};
        return;
    }

    m_stream->start(device);
    updateStreamState(QAudio::ActiveState);
}

QIODevice *QDarwinAudioSource::start()
{
    m_stream = std::make_shared<QCoreAudioSourceStream>(
            m_audioDevice, m_format, m_internalBufferSize, this, volume(), m_hardwareBufferFrames);

    if (!m_stream->open()) {
        setError(QAudio::OpenError);
        m_stream = {};
        return nullptr;
    }

    QIODevice *device = m_stream->start();
    QObject::connect(device, &QIODevice::readyRead, this, [this] {
        updateStreamIdle(false);
    });
    updateStreamIdle(true, EmitStateSignal::False);
    updateStreamState(QAudio::ActiveState);
    return device;
}

void QDarwinAudioSource::stop()
{
    if (!m_stream)
        return;

    if (m_stream->deviceIsRingbufferReader())
        // we own the qiodevice, so let's keep it alive to allow users to drain the ringbuffer
        m_retiredStream = m_stream;

    using ShutdownPolicy = QtMultimediaPrivate::QPlatformAudioIOStream::ShutdownPolicy;
    m_stream->stop(ShutdownPolicy::DrainRingbuffer);
    m_stream = {};
    updateStreamState(QAudio::StoppedState);
}

void QDarwinAudioSource::reset()
{
    m_retiredStream = {};

    if (!m_stream)
        return;

    using ShutdownPolicy = QtMultimediaPrivate::QPlatformAudioIOStream::ShutdownPolicy;
    m_stream->stop(ShutdownPolicy::DiscardRingbuffer);
    m_stream = {};
    updateStreamState(QAudio::StoppedState);
}

void QDarwinAudioSource::suspend()
{
    if (m_stream) {
        m_stream->suspend();
        updateStreamState(QAudio::SuspendedState);
    }
}

void QDarwinAudioSource::resume()
{
    if (m_stream) {
        updateStreamState(QAudio::ActiveState);
        m_stream->resume();
    }
}

qsizetype QDarwinAudioSource::bytesReady() const
{
    return m_stream ? m_stream->bytesReady() : 0;
}

void QDarwinAudioSource::setBufferSize(qsizetype value)
{
    m_internalBufferSize = value;
}

qsizetype QDarwinAudioSource::bufferSize() const
{
    if (m_stream)
        return m_stream->ringbufferSizeInBytes();

    return QtMultimediaPrivate::QPlatformAudioIOStream::inferRingbufferBytes(
            m_internalBufferSize, m_hardwareBufferFrames, format());
}

void QDarwinAudioSource::setHardwareBufferFrames(int32_t arg)
{
    if (arg > 0)
        m_hardwareBufferFrames = arg;
    else
        m_hardwareBufferFrames = std::nullopt;
}

int32_t QDarwinAudioSource::hardwareBufferFrames()
{
    return m_hardwareBufferFrames.value_or(-1);
}

qint64 QDarwinAudioSource::processedUSecs() const
{
    return m_stream ? m_stream->processedDuration().count() : 0;
}

void QDarwinAudioSource::setVolume(float volume)
{
    QPlatformAudioEndpointBase::setVolume(volume);
    if (m_stream)
        m_stream->setVolume(volume);
}

void QDarwinAudioSource::resumeStreamIfNecessary()
{
    if (m_stream)
        m_stream->resumeIfNecessary();
}

QT_END_NAMESPACE
