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

#include <AudioUnit/AudioComponent.h>
#include <AudioToolbox/AudioConverter.h>
#ifdef Q_OS_MACOS
#  include <QtMultimedia/private/qmacosaudiodatautils_p.h>
#else
#  import <AVFoundation/AVAudioSession.h>
#  include <QtMultimedia/private/qcoreaudiosessionmanager_p.h>
#endif

QT_BEGIN_NAMESPACE

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
#ifdef Q_OS_MACOS
    m_stopOnDisconnected.cancelChain();
#endif

    if (m_audioConverter)
        AudioConverterDispose(m_audioConverter);
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

    AudioStreamBasicDescription streamFormat = toAudioStreamBasicDescription(m_format);

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

    std::optional<int> deviceSamplingRate = audioObjectGetSamplingRate(*nativeDeviceId);

    if (!deviceSamplingRate) {
        qWarning() << "QAudioSource: Device does not support any sampling rate. This should not "
                      "happen";
        return false;
    }

    if (m_hardwareBufferFrames)
        audioObjectSetFramesPerBuffer(*nativeDeviceId, *m_hardwareBufferFrames);

    if (deviceSamplingRate != m_format.sampleRate()) {
        AudioStreamBasicDescription desiredFormat = streamFormat;

        streamFormat.mSampleRate = *deviceSamplingRate;

        OSStatus status = AudioConverterNew(&streamFormat, &desiredFormat, &m_audioConverter);
        if (status != noErr) {
            qWarning() << "QAudioSource: Failed to create AudioConverter:" << status;
            return false;
        }
    }

    audioUnitSetInputStreamFormat(m_audioUnit, 0, streamFormat);
    audioUnitSetOutputStreamFormat(m_audioUnit, 1, streamFormat);
#else

    AVAudioSession *session = [AVAudioSession sharedInstance];
    double hwRate = session.sampleRate;
    std::optional<int> deviceSamplingRate = int(hwRate);

    audioUnitSetInputStreamFormat(m_audioUnit, 0, streamFormat);
    audioUnitSetOutputStreamFormat(m_audioUnit, 1, streamFormat);
#endif

    std::optional<int> framesPerBuffer = audioUnitGetFramesPerSlice(m_audioUnit);

    m_bufferList.mNumberBuffers = 1;
    m_bufferList.mBuffers[0].mNumberChannels = m_format.channelCount();
    m_bufferList.mBuffers[0].mDataByteSize =
            m_format.bytesForFrames(framesPerBuffer.value_or(2048));
    m_bufferList.mBuffers[0].mData = malloc(m_bufferList.mBuffers[0].mDataByteSize);

    if (m_audioConverter) {
        size_t outputBufferSize = m_bufferList.mBuffers[0].mDataByteSize * m_format.sampleRate()
                        / static_cast<float>(*deviceSamplingRate)
                + 128 /*padding*/;
        m_outputBuffer.resize(outputBufferSize);
        m_outputBufferList.mNumberBuffers = 1;
        m_outputBufferList.mBuffers[0].mNumberChannels = m_format.channelCount();
        m_outputBufferList.mBuffers[0].mDataByteSize = outputBufferSize;
        m_outputBufferList.mBuffers[0].mData = m_outputBuffer.data();
    }

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

bool QCoreAudioSourceStream::start(AudioCallback &&cb)
{
    m_audioCallback = std::move(cb);

    const OSStatus status = AudioOutputUnitStart(m_audioUnit.get());
    if (status != noErr) {
        qDebug() << "AudioOutputUnitStart failed:" << status;
        return false;
    }

    m_audioUnitRunning = true;

    return true;
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
    auto *self = reinterpret_cast<QCoreAudioSourceStream *>(inRefCon);
    return self->processInput(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
}

OSStatus
QCoreAudioSourceStream::processInput(AudioUnitRenderActionFlags *ioActionFlags,
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

    QSpan<const std::byte> inputSpan;
    if (m_audioConverter) {
        // convert the data to the desired sample rate
        struct InputProcState
        {
            QCoreAudioSourceStream *self;
            UInt32 inNumberFrames;
        };

        InputProcState state{
            /*self:*/ this,
            /*inNumberFrames:*/ inNumberFrames,
        };

        auto inputProc = [](AudioConverterRef, UInt32 *ioNumberDataPackets, AudioBufferList *ioData,
                            AudioStreamPacketDescription **outDataPacketDescription,
                            void *inUserData) -> OSStatus {
            auto *state = static_cast<InputProcState *>(inUserData);
            *ioNumberDataPackets = state->inNumberFrames;
            ioData->mNumberBuffers = 1;
            ioData->mBuffers[0] = state->self->m_bufferList.mBuffers[0];
            if (outDataPacketDescription)
                *outDataPacketDescription = nullptr;
            return noErr;
        };

        UInt32 outputFrames = m_format.framesForBytes(m_outputBuffer.size());
        OSStatus convStatus = AudioConverterFillComplexBuffer(
                m_audioConverter, inputProc, &state, &outputFrames, &m_outputBufferList, nullptr);
        if (convStatus != noErr) {
            qDebug() << "AudioConverterFillComplexBuffer failed:" << convStatus;
            return convStatus;
        }

        uint32_t outputBytes = m_format.bytesForFrames(outputFrames);
        inputSpan = QSpan<const std::byte>{
            reinterpret_cast<const std::byte *>(m_outputBuffer.data()),
            outputBytes,
        };
        inNumberFrames = outputFrames;
    } else {
        inputSpan = QSpan<const std::byte>{
            reinterpret_cast<const std::byte *>(m_bufferList.mBuffers[0].mData),
            m_bufferList.mBuffers[0].mDataByteSize,
        };
    }

    return m_audioCallback ? processAudioCallback(inputSpan)
                           : processRingbuffer(inputSpan, inNumberFrames);
}

OSStatus
QCoreAudioSourceStream::processRingbuffer(QSpan<const std::byte> inputSpan,
                                          UInt32 inNumberFrames) noexcept QT_MM_NONBLOCKING
{
    QPlatformAudioSourceStream::process(inputSpan, inNumberFrames);
    return noErr;
}

OSStatus QCoreAudioSourceStream::processAudioCallback(QSpan<const std::byte> inputSpan) noexcept
        QT_MM_NONBLOCKING
{
    using namespace QtMultimediaPrivate;
    runAudioCallback(*m_audioCallback, inputSpan, m_format, volume());

    return noErr;
}

#ifdef Q_OS_MACOS
bool QCoreAudioSourceStream::addDisconnectListener(AudioObjectID id)
{
    m_stopOnDisconnected.cancel();

    auto disconnectionFuture = m_disconnectMonitor.addDisconnectListener(id);
    if (!disconnectionFuture)
        return false;

    m_stopOnDisconnected = disconnectionFuture->then(m_parent, [this] {
        // Coreaudio will pause for a bit and restart the audio unit with a different device.
        // This is problematic, as it switches kAudioOutputUnitProperty_CurrentDevice and
        // invalidates the native device ID (and the disconnect handler). furthermore, we don't have
        // a way to re-synchronize the audio stream. so we explicitly stop the audio unit

        requestStop();
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

QDarwinAudioSource::~QDarwinAudioSource()
    = default;

void QDarwinAudioSource::resumeStreamIfNecessary()
{
    if (m_stream)
        m_stream->resumeIfNecessary();
}

QT_END_NAMESPACE
