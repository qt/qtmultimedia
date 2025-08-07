// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinaudiosink_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/qdebug.h>
#include <QtGui/qguiapplication.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qaudio_qiodevice_support_p.h>
#include <QtMultimedia/private/qaudio_rtsan_support_p.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>
#include <QtMultimedia/private/qaudioringbuffer_p.h>
#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>
#include <QtMultimedia/private/qautoresetevent_p.h>
#include <QtMultimedia/private/qcoreaudioutils_p.h>
#include <QtMultimedia/private/qdarwinaudiodevice_p.h>
#include <QtMultimedia/private/qdarwinaudiodevices_p.h>

#include <AudioUnit/AudioUnit.h>
#ifdef Q_OS_MACOS
#  include <AudioUnit/AudioComponent.h>
#  include <QtMultimedia/private/qmacosaudiodatautils_p.h>
#else
#  include <QtMultimedia/private/qcoreaudiosessionmanager_p.h>
#endif

#include <variant>

QT_BEGIN_NAMESPACE

////////////////////////////////////////////////////////////////////////////////////////////////////

QCoreAudioSinkStream::QCoreAudioSinkStream(QAudioDevice audioDevice, const QAudioFormat& format,
                                           std::optional<qsizetype> ringbufferSize,
                                           QDarwinAudioSink *parent, float volume,
                                           std::optional<int32_t> hardwareBufferFrames,
                                           AudioEndpointRole)
    : QPlatformAudioSinkStream {
          std::move(audioDevice), format, ringbufferSize, hardwareBufferFrames, volume,
      },
      m_parent(parent)
{
}

bool QCoreAudioSinkStream::open()
{
    using namespace QCoreAudioUtils;

#ifdef Q_OS_MACOS
    // Find the the most recent CoreAudio AudioDeviceID for the current device
    // to start the audio stream.
    std::optional<AudioDeviceID> audioDeviceId = findAudioDeviceId(m_audioDevice);
    if (!audioDeviceId) {
        qWarning() << "QAudioSource: Unable to use find most recent CoreAudio AudioDeviceID for "
                      "given device-id. The device might not be connected.";
        return false;
    }
    const AudioDeviceID nativeDeviceId = audioDeviceId.value();
#endif

    if (auto audioUnit = makeAudioUnitForIO())
        m_audioUnit = std::move(*audioUnit);
    else
        return false;

#ifdef Q_OS_MACOS
    std::optional<int> bestNominalSamplingRate =
            audioObjectFindBestNominalSampleRate(nativeDeviceId, QAudioDevice::Output, m_format.sampleRate());

    if (bestNominalSamplingRate) {
        if (!audioObjectSetSamplingRate(nativeDeviceId, *bestNominalSamplingRate))
            return false;
    }

    // register listener
    if (!addDisconnectListener(*audioDeviceId))
        return false;

    // Set Audio Device
    audioUnitSetCurrentDevice(m_audioUnit, nativeDeviceId);

    if (m_hardwareBufferFrames)
        audioObjectSetFramesPerBuffer(*audioDeviceId, *m_hardwareBufferFrames);
#endif

    // Set stream format
    const AudioStreamBasicDescription streamFormat = toAudioStreamBasicDescription(m_format);
    if (!audioUnitSetInputStreamFormat(m_audioUnit, 0, streamFormat))
        return false;

    return m_audioUnit.initialize();
}

bool QCoreAudioSinkStream::start(QIODevice *device)
{
    auto renderCallback = [](void *self, [[maybe_unused]] AudioUnitRenderActionFlags *ioActionFlags,
                             [[maybe_unused]] const AudioTimeStamp *inTimeStamp,
                             [[maybe_unused]] UInt32 inBusNumber,
                             [[maybe_unused]] UInt32 inNumberFrames,
                             AudioBufferList *ioData) -> OSStatus {
        return reinterpret_cast<QCoreAudioSinkStream *>(self)->processRingbuffer(inNumberFrames,
                                                                                 ioData);
    };

    AURenderCallbackStruct callback{
        .inputProc = renderCallback,
        .inputProcRefCon = this,
    };
    if (!audioUnitSetRenderCallback(m_audioUnit, callback))
        return false;

    setQIODevice(device);
    pullFromQIODevice();

    const OSStatus status = AudioOutputUnitStart(m_audioUnit.get());
    if (status != noErr) {
        qDebug() << "AudioOutputUnitStart failed:" << status;
        return false;
    }

    m_audioUnitRunning = true;

    createQIODeviceConnections(device);

    return true;
}

QIODevice *QCoreAudioSinkStream::start()
{
    QIODevice *reader = createRingbufferWriterDevice();

    bool success = start(reader);
    if (success)
        return reader;
    else
        return nullptr;
}

bool QCoreAudioSinkStream::start(AudioCallback cb)
{
    auto renderCallback = [](void *self, [[maybe_unused]] AudioUnitRenderActionFlags *ioActionFlags,
                             [[maybe_unused]] const AudioTimeStamp *inTimeStamp,
                             [[maybe_unused]] UInt32 inBusNumber,
                             [[maybe_unused]] UInt32 inNumberFrames,
                             AudioBufferList *ioData) -> OSStatus {
        return reinterpret_cast<QCoreAudioSinkStream *>(self)->processAudioCallback(inNumberFrames,
                                                                                    ioData);
    };

    m_audioCallback = std::move(cb);

    AURenderCallbackStruct callback;
    callback.inputProc = renderCallback;
    callback.inputProcRefCon = this;
    if (!audioUnitSetRenderCallback(m_audioUnit, callback))
        return false;

    const OSStatus status = AudioOutputUnitStart(m_audioUnit.get());
    if (status != noErr) {
        qDebug() << "AudioOutputUnitStart failed:" << status;
        return false;
    }
    return true;
}

void QCoreAudioSinkStream::stop(ShutdownPolicy policy)
{
    m_parent = nullptr;

    if (m_audioCallback) {
        stopStream();
        return;
    }

    disconnectQIODeviceConnections();
    stopIdleDetection();

    switch (policy) {
    case ShutdownPolicy::DrainRingbuffer:
        stopStreamWhenBufferDrained();
        break;
    case ShutdownPolicy::DiscardRingbuffer:
        stopStream();
        break;
    default:
        Q_UNREACHABLE_RETURN();
    }
}

void QCoreAudioSinkStream::stopStreamWhenBufferDrained()
{
    if (this->isIdle())
        return stopStream();

    // we keep the stream alive until the idle handler is called by keeping a shared_ptr
    // reference alive. Once the stream finishes, the idle handler will stop the audio unit
    // and delete the stream.
    connectIdleHandler([this, ownedSelf = shared_from_this()]() mutable {
        if (!isIdle())
            return;

        // stop on application thread once ringbuffer is empty
        stopStream();

        ownedSelf = nullptr; // might delete the instance
    });
}

void QCoreAudioSinkStream::stopStream()
{
#ifdef Q_OS_MACOS
    removeDisconnectListener();
#endif
    requestStop();
    stopAudioUnit();
}

void QCoreAudioSinkStream::suspend()
{
    const auto status = AudioOutputUnitStop(m_audioUnit.get());
    if (status == noErr)
        return;
    else
        qDebug() << "AudioOutputUnitStop failed:" << status;
}

void QCoreAudioSinkStream::resume()
{
    const auto status = AudioOutputUnitStart(m_audioUnit.get());
    if (status == noErr)
        return;
    else
        qDebug() << "AudioOutputUnitStart failed:" << status;
}

void QCoreAudioSinkStream::resumeIfNecessary()
{
    if (!QCoreAudioUtils::audioUnitIsRunning(m_audioUnit))
        resume();
}

OSStatus QCoreAudioSinkStream::processRingbuffer(uint32_t numberOfFrames,
                                                 AudioBufferList *ioData) noexcept QT_MM_NONBLOCKING
{
    Q_ASSERT(int64_t(ioData->mBuffers[0].mDataByteSize) == m_format.bytesForFrames(numberOfFrames));

    QSpan audioBufferSpan{
        reinterpret_cast<std::byte *>(ioData->mBuffers[0].mData),
        ioData->mBuffers[0].mDataByteSize,
    };

    QPlatformAudioSinkStream::process(audioBufferSpan, numberOfFrames);

    return noErr;
}

OSStatus QCoreAudioSinkStream::processAudioCallback(uint32_t numberOfFrames,
                                                    AudioBufferList *ioData) noexcept QT_MM_NONBLOCKING
{
    using namespace QtMultimediaPrivate;

    Q_ASSERT(int64_t(ioData->mBuffers[0].mDataByteSize) == m_format.bytesForFrames(numberOfFrames));

    QSpan<std::byte> inputSpan{
        reinterpret_cast<std::byte *>(ioData->mBuffers[0].mData),
        ioData->mBuffers[0].mDataByteSize,
    };

    runAudioCallback(*m_audioCallback, inputSpan, m_format, volume());

    return noErr;
}


void QCoreAudioSinkStream::updateStreamIdle(bool arg)
{
    m_parent->updateStreamIdle(arg);
}

void QCoreAudioSinkStream::stopAudioUnit()
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

#ifdef Q_OS_MACOS
bool QCoreAudioSinkStream::addDisconnectListener(AudioObjectID id)
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
        requestStop();
        handleIOError(m_parent);
    });

    return true;
}

void QCoreAudioSinkStream::removeDisconnectListener()
{
    m_stopOnDisconnected.cancel();
    m_disconnectMonitor.removeDisconnectListener();
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

QDarwinAudioSink::QDarwinAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent)
    : BaseClass(std::move(device), format, parent)
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

void QDarwinAudioSink::resumeStreamIfNecessary()
{
    if (m_stream)
        m_stream->resumeIfNecessary();
}

QT_END_NAMESPACE
