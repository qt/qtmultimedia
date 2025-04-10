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

class QCoreAudioSinkStream final : public std::enable_shared_from_this<QCoreAudioSinkStream>,
                                   QtMultimediaPrivate::QPlatformAudioSinkStream
{
    using QPlatformAudioSinkStream = QtMultimediaPrivate::QPlatformAudioSinkStream;

public:
    explicit QCoreAudioSinkStream(QAudioDevice, QAudioFormat, std::optional<int> ringbufferSize,
                                  QDarwinAudioSink *parent, float volume,
                                  std::optional<int32_t> hardwareBufferFrames);

    bool open();
    bool start(QIODevice *device);
    QIODevice *start();
    void stop();
    void reset();

    void suspend();
    void resume();

    using QPlatformAudioSinkStream::bytesFree;
    using QPlatformAudioSinkStream::processedDuration;
    using QPlatformAudioSinkStream::setVolume;

    void resumeIfNecessary();

private:
    static OSStatus renderCallback(void *self,
                                   [[maybe_unused]] AudioUnitRenderActionFlags *ioActionFlags,
                                   [[maybe_unused]] const AudioTimeStamp *inTimeStamp,
                                   [[maybe_unused]] UInt32 inBusNumber,
                                   [[maybe_unused]] UInt32 inNumberFrames, AudioBufferList *ioData);

    OSStatus process(uint32_t numberOfFrames, AudioBufferList *ioData);

    void updateStreamIdle(bool arg) override;
    void stopAudioUnit();

#ifdef Q_OS_MACOS
    bool addDisconnectListener(AudioObjectID id);
    void removeDisconnectListener();

    QCoreAudioUtils::DeviceDisconnectMonitor m_disconnectMonitor;
    QFuture<void> m_stopOnDisconnected;
#endif

    std::unique_ptr<QIODevice> m_reader;
    QCoreAudioUtils::AudioUnitHandle m_audioUnit;
    bool m_audioUnitRunning{};

    QDarwinAudioSink *m_parent;
    std::shared_ptr<QCoreAudioSinkStream> m_self;
};

QCoreAudioSinkStream::QCoreAudioSinkStream(QAudioDevice audioDevice, QAudioFormat format,
                                           std::optional<int> ringbufferSize,
                                           QDarwinAudioSink *parent, float volume,
                                           std::optional<int32_t> hardwareBufferFrames)
    : QPlatformAudioSinkStream{
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

    // register callback
    AURenderCallbackStruct callback;
    callback.inputProc = renderCallback;
    callback.inputProcRefCon = this;

    if (AudioUnitSetProperty(m_audioUnit.get(), kAudioUnitProperty_SetRenderCallback,
                             kAudioUnitScope_Global, 0, &callback, sizeof(callback))
        != noErr) {
        qWarning() << "QAudioOutput: Failed to set AudioUnit callback";
        return false;
    }

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
    QIODevice *reader = createRingbufferReaderDevice();

    bool success = start(reader);
    if (success)
        return reader;
    else
        return nullptr;
}

void QCoreAudioSinkStream::stop()
{
    if (this->isIdle())
        return reset();

    requestStop();
#ifdef Q_OS_MACOS
    removeDisconnectListener();
#endif
    disconnectQIODeviceConnections();

    stopIdleDetection();

    connectIdleHandler([this] {
        if (!isIdle())
            return;

        // stop on application thread once ringbuffer is empty
        stopAudioUnit();

        m_self = nullptr; // might delete the instance
    });

    m_parent = nullptr;

    // take ownership and
    m_self = shared_from_this();
}

void QCoreAudioSinkStream::reset()
{
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

OSStatus QCoreAudioSinkStream::renderCallback(void *self,
                                              AudioUnitRenderActionFlags * /*ioActionFlags*/,
                                              const AudioTimeStamp * /*inTimeStamp*/,
                                              UInt32 /*inBusNumber*/, UInt32 inNumberFrames,
                                              AudioBufferList *ioData)
{
    return reinterpret_cast<QCoreAudioSinkStream *>(self)->process(inNumberFrames, ioData);
}

OSStatus QCoreAudioSinkStream::process(uint32_t numberOfFrames,
                                       AudioBufferList *ioData) QT_MM_NONBLOCKING
{
    const uint32_t bytesPerFrame = m_format.bytesPerFrame();

    Q_ASSERT(ioData->mBuffers[0].mDataByteSize / bytesPerFrame == numberOfFrames);

    QSpan audioBufferSpan{
        reinterpret_cast<std::byte *>(ioData->mBuffers[0].mData),
        ioData->mBuffers[0].mDataByteSize,
    };

    QPlatformAudioSinkStream::process(audioBufferSpan, numberOfFrames);

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

        m_parent->setError(QtAudio::IOError);
        m_parent->updateStreamState(QtAudio::State::StoppedState);
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
    : QPlatformAudioSink(std::move(device), parent), m_audioFormat(format)
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

QDarwinAudioSink::~QDarwinAudioSink()
{
    stop();
}

void QDarwinAudioSink::start(QIODevice *device)
{
    if (!device) {
        setError(QAudio::IOError);
        return;
    }

    m_stream = std::make_shared<QCoreAudioSinkStream>(m_audioDevice, m_audioFormat,
                                                      m_internalBufferSize, this, m_volume,
                                                      m_hardwareBufferFrames);

    if (!m_stream->open()) {
        setError(QAudio::OpenError);
        return;
    }

    m_stream->start(device);
    updateStreamState(QAudio::ActiveState);
}

QIODevice *QDarwinAudioSink::start()
{
    m_stream = std::make_shared<QCoreAudioSinkStream>(m_audioDevice, m_audioFormat,
                                                      m_internalBufferSize, this, m_volume,
                                                      m_hardwareBufferFrames);

    if (!m_stream->open()) {
        setError(QAudio::OpenError);
        return nullptr;
    }

    QIODevice *device = m_stream->start();
    QObject::connect(device, &QIODevice::readyRead, this, [this] {
        updateStreamIdle(false);
    });
    updateStreamIdle(true);
    updateStreamState(QAudio::ActiveState);
    return device;
}

void QDarwinAudioSink::stop()
{
    if (m_stream) {
        m_stream->stop();
        m_stream = {};
        updateStreamState(QAudio::StoppedState);
    }
}

void QDarwinAudioSink::reset()
{
    if (m_stream) {
        m_stream->reset();
        m_stream = {};
        updateStreamState(QAudio::StoppedState);
    }
}

void QDarwinAudioSink::suspend()
{
    if (m_stream) {
        m_stream->suspend();
        updateStreamState(QAudio::SuspendedState);
    }
}

void QDarwinAudioSink::resume()
{
    if (m_stream) {
        updateStreamState(QAudio::ActiveState);
        m_stream->resume();
    }
}

qsizetype QDarwinAudioSink::bytesFree() const
{
    if (m_stream)
        return m_stream->bytesFree();
    return 0;
}

void QDarwinAudioSink::setBufferSize(qsizetype value)
{
    m_internalBufferSize = value;
}

qsizetype QDarwinAudioSink::bufferSize() const
{
    return m_internalBufferSize.value_or(-1);
}

void QDarwinAudioSink::setHardwareBufferFrames(int32_t arg)
{
    if (arg > 0)
        m_hardwareBufferFrames = arg;
    else
        m_hardwareBufferFrames = std::nullopt;
}

int32_t QDarwinAudioSink::hardwareBufferFrames()
{
    return m_hardwareBufferFrames.value_or(-1);
}

qint64 QDarwinAudioSink::processedUSecs() const
{
    if (m_stream)
        return m_stream->processedDuration().count();

    return 0;
}

QAudioFormat QDarwinAudioSink::format() const
{
    return m_audioFormat;
}

void QDarwinAudioSink::setVolume(float volume)
{
    m_volume = volume;
    if (m_stream)
        m_stream->setVolume(m_volume);
}

float QDarwinAudioSink::volume() const
{
    return m_volume;
}

void QDarwinAudioSink::resumeStreamIfNecessary()
{
    if (m_stream)
        m_stream->resumeIfNecessary();
}

QT_END_NAMESPACE
