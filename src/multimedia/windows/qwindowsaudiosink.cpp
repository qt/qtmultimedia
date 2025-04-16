// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsaudiosink_p.h"

#include <QtCore/qthread.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/private/quniquehandle_types_p.h>
#include <QtCore/private/qsystemerror_p.h>

#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>
#include <QtMultimedia/private/qwindowsaudiodevice_p.h>
#include <QtMultimedia/private/qwindowsaudioutils_p.h>

#include <audioclient.h>
#include <mmdeviceapi.h>

QT_BEGIN_NAMESPACE

using QtMultimediaPrivate::AudioEndpointRole;
using QtMultimediaPrivate::QPlatformAudioIOStream;
using QtMultimediaPrivate::QPlatformAudioSinkStream;
using QWindowsAudioUtils::audioClientErrorString;
using namespace std::chrono_literals;

struct QWASAPIAudioSinkStream final : std::enable_shared_from_this<QWASAPIAudioSinkStream>,
                                      QPlatformAudioSinkStream
{
    using SampleFormat = QAudioFormat::SampleFormat;
    enum class StreamType : uint8_t {
        Ringbuffer,
        Callback,
    };

    QWASAPIAudioSinkStream(QAudioDevice, QWindowsAudioSink *parent, const QAudioFormat &,
                           std::optional<qsizetype> ringbufferSize,
                           std::optional<int32_t> hardwareBufferSize, float volume);

    ~QWASAPIAudioSinkStream() = default;

    using QPlatformAudioSinkStream::bytesFree;
    using QPlatformAudioSinkStream::processedDuration;
    using QPlatformAudioSinkStream::ringbufferSizeInBytes;
    using QPlatformAudioSinkStream::setVolume;

    bool start(QIODevice *, ComPtr<IMMDevice>, AudioEndpointRole);
    QIODevice *start(ComPtr<IMMDevice>, AudioEndpointRole);
    bool start(AudioCallback, ComPtr<IMMDevice>, AudioEndpointRole);

    void suspend();
    void resume();
    void stop(ShutdownPolicy);

    void updateStreamIdle(bool) override;

private:
    bool openAudioClient(ComPtr<IMMDevice>, AudioEndpointRole);
    bool startAudioClient(StreamType);

    void fillInitialHostBuffer();
    void runProcessRingbufferLoop();
    void runProcessCallbackLoop();
    bool processRingbuffer() noexcept QT_MM_NONBLOCKING;
    bool processCallback() noexcept QT_MM_NONBLOCKING;

    void handleAudioClientError();

    ComPtr<IAudioClient3> m_audioClient;
    ComPtr<IAudioRenderClient> m_renderClient;

    QWindowsAudioUtils::reference_time m_periodSize;
    qsizetype m_audioClientFrames;

    std::atomic_bool m_suspended{};
    std::atomic<ShutdownPolicy> m_shutdownPolicy{ ShutdownPolicy::DiscardRingbuffer };
    QAutoResetEvent m_ringbufferDrained;

    const QUniqueWin32NullHandle m_wasapiHandle;
    std::unique_ptr<QThread> m_workerThread;

    AudioCallback m_audioCallback;

    QWindowsAudioSink *m_parent;
};

QWASAPIAudioSinkStream::QWASAPIAudioSinkStream(QAudioDevice device, QWindowsAudioSink *parent, const QAudioFormat &format,
                                               std::optional<qsizetype> ringbufferSize,
                                               std::optional<int32_t> hardwareBufferFrames, float volume):
    QPlatformAudioSinkStream{
        std::move(device),
        format,
        ringbufferSize,
        hardwareBufferFrames,
        volume,
    },
    m_wasapiHandle {
        CreateEvent(0, false, false, nullptr),
    },
    m_parent{parent}
{
}

bool QWASAPIAudioSinkStream::start(QIODevice *ioDevice, ComPtr<IMMDevice> immDevice,
                                   AudioEndpointRole role)
{
    bool clientOpen = openAudioClient(std::move(immDevice), role);
    if (!clientOpen)
        return false;

    setQIODevice(ioDevice);
    createQIODeviceConnections(ioDevice);
    pullFromQIODevice();

    bool started = startAudioClient(StreamType::Ringbuffer);
    if (!started)
        return false;

    return true;
}

QIODevice *QWASAPIAudioSinkStream::start(ComPtr<IMMDevice> immDevice, AudioEndpointRole role)
{
    bool clientOpen = openAudioClient(std::move(immDevice), role);
    if (!clientOpen)
        return nullptr;

    QIODevice *ioDevice = createRingbufferReaderDevice();

    m_parent->updateStreamIdle(true, QWindowsAudioSink::EmitStateSignal::False);

    setQIODevice(ioDevice);
    createQIODeviceConnections(ioDevice);

    bool started = startAudioClient(StreamType::Ringbuffer);
    if (!started)
        return nullptr;

    return ioDevice;
}

bool QWASAPIAudioSinkStream::start(AudioCallback audioCallback, ComPtr<IMMDevice> immDevice,
                                   AudioEndpointRole role)
{
    bool clientOpen = openAudioClient(std::move(immDevice), role);
    if (!clientOpen)
        return false;

    m_audioCallback = std::move(audioCallback);

    return startAudioClient(StreamType::Callback);
}

void QWASAPIAudioSinkStream::suspend()
{
    m_suspended = true;
    QWindowsAudioUtils::audioClientStop(m_audioClient);
}

void QWASAPIAudioSinkStream::resume()
{
    m_suspended = false;
    QWindowsAudioUtils::audioClientStart(m_audioClient);
}

void QWASAPIAudioSinkStream::stop(ShutdownPolicy shutdownPolicy)
{
    using namespace QWindowsAudioUtils;

    m_parent = nullptr;
    m_shutdownPolicy = shutdownPolicy;

    switch (shutdownPolicy) {
    case ShutdownPolicy::DiscardRingbuffer: {
        requestStop();
        audioClientStop(m_audioClient);
        m_workerThread->wait();
        m_workerThread = {};
        audioClientReset(m_audioClient);

        return;
    }
    case ShutdownPolicy::DrainRingbuffer: {
        QObject::connect(&m_ringbufferDrained, &QAutoResetEvent::activated, &m_ringbufferDrained,
                         [self = shared_from_this()]() mutable {
            self->m_workerThread->wait();
            self = {};
        });
        return;
    }
    default:
        Q_UNREACHABLE_RETURN();
    }
}

void QWASAPIAudioSinkStream::updateStreamIdle(bool streamIsIdle)
{
    if (m_parent)
        m_parent->updateStreamIdle(streamIsIdle);
}

bool QWASAPIAudioSinkStream::openAudioClient(ComPtr<IMMDevice> device, AudioEndpointRole role)
{
    using namespace QWindowsAudioUtils;

    std::optional<AudioClientCreationResult> clientData =
            createAudioClient(device, m_format, m_hardwareBufferFrames, m_wasapiHandle, role);

    if (!clientData)
        return false;

    m_audioClient = std::move(clientData->client);
    m_periodSize = clientData->periodSize;
    m_audioClientFrames = clientData->audioClientFrames;

    HRESULT hr = m_audioClient->GetService(IID_PPV_ARGS(m_renderClient.GetAddressOf()));
    if (FAILED(hr)) {
        qWarning() << "IAudioClient3::GetService failed to obtain IAudioRenderClient"
                   << audioClientErrorString(hr);
        return false;
    }

    if (m_audioDevice.preferredFormat().sampleRate() != m_format.sampleRate())
        audioClientSetRate(m_audioClient, m_format.sampleRate());

    return true;
}

bool QWASAPIAudioSinkStream::startAudioClient(StreamType streamType)
{
    using namespace QWindowsAudioUtils;
    m_workerThread.reset(QThread::create([this, streamType] {
        setMCSSForPeriodSize(m_periodSize);
        fillInitialHostBuffer();

        switch (streamType) {
        case StreamType::Ringbuffer:
            return runProcessRingbufferLoop();
        case StreamType::Callback:
            return runProcessCallbackLoop();
        }
    }));
    m_workerThread->setObjectName(u"QWASAPIAudioSinkStream");
    m_workerThread->start();

    return QWindowsAudioUtils::audioClientStart(m_audioClient);
}

void QWASAPIAudioSinkStream::fillInitialHostBuffer()
{
    processRingbuffer();
}

void QWASAPIAudioSinkStream::runProcessRingbufferLoop()
{
    using namespace QWindowsAudioUtils;

    for (;;) {
        constexpr std::chrono::milliseconds timeout = 2s;
        DWORD retval = WaitForSingleObject(m_wasapiHandle.get(), timeout.count());
        if (retval != WAIT_OBJECT_0) {
            if (m_suspended)
                continue;

            handleAudioClientError();
            return;
        }

        if (isStopRequested()) {
            switch (m_shutdownPolicy.load(std::memory_order_relaxed)) {
            case ShutdownPolicy::DiscardRingbuffer:
                return;
            case ShutdownPolicy::DrainRingbuffer: {
                bool bufferDrained = visitRingbuffer([](const auto &ringbuffer) {
                    return ringbuffer.used() == 0;
                });
                if (bufferDrained) {
                    audioClientStop(m_audioClient);
                    audioClientReset(m_audioClient);

                    m_ringbufferDrained.set();
                    return;
                }
                break;
            }
            default:
                Q_UNREACHABLE_RETURN();
            }
        }

        bool success = processRingbuffer();
        if (!success) {
            handleAudioClientError();
            return;
        }
    }
}

void QWASAPIAudioSinkStream::runProcessCallbackLoop()
{
    using namespace QWindowsAudioUtils;

    for (;;) {
        constexpr std::chrono::milliseconds timeout = 2s;
        DWORD retval = WaitForSingleObject(m_wasapiHandle.get(), timeout.count());
        if (retval != WAIT_OBJECT_0) {
            if (m_suspended)
                continue;

            handleAudioClientError();
            return;
        }

        if (isStopRequested())
            return;

        bool success = processCallback();
        if (!success) {
            handleAudioClientError();
            return;
        }
    }
}

bool QWASAPIAudioSinkStream::processRingbuffer() noexcept QT_MM_NONBLOCKING
{
    uint32_t numFramesPadding;
    HRESULT hr = m_audioClient->GetCurrentPadding(&numFramesPadding);
    if (FAILED(hr)) {
        qWarning() << "IAudioClient3::GetCurrentPadding failed" << audioClientErrorString(hr);
        return false;
    }

    uint32_t requiredFrames = m_audioClientFrames - numFramesPadding;
    if (requiredFrames == 0)
        return true;

    // Grab the next empty buffer from the audio device.
    unsigned char *hostBuffer{};
    hr = m_renderClient->GetBuffer(requiredFrames, &hostBuffer);
    if (FAILED(hr)) {
        qWarning() << "IAudioRenderClient::getBuffer failed" << audioClientErrorString(hr);
        return false;
    }

    uint32_t requiredDataSize = m_format.bytesForFrames(requiredFrames);
    auto hostBufferSpan = as_writable_bytes(QSpan{ hostBuffer, requiredDataSize });
    uint64_t consumedFrames = QPlatformAudioSinkStream::process(hostBufferSpan, requiredFrames);

    DWORD flags = consumedFrames != 0 ? 0 : AUDCLNT_BUFFERFLAGS_SILENT;

    hr = m_renderClient->ReleaseBuffer(requiredFrames, flags);
    if (FAILED(hr)) {
        qWarning() << "IAudioRenderClient::ReleaseBuffer failed" << audioClientErrorString(hr);
        return false;
    }

    return true;
}

bool QWASAPIAudioSinkStream::processCallback() noexcept QT_MM_NONBLOCKING
{
    using namespace QtMultimediaPrivate;

    uint32_t numFramesPadding;
    HRESULT hr = m_audioClient->GetCurrentPadding(&numFramesPadding);
    if (FAILED(hr)) {
        qWarning() << "IAudioClient3::GetCurrentPadding failed" << audioClientErrorString(hr);
        return false;
    }

    uint32_t requiredFrames = m_audioClientFrames - numFramesPadding;
    if (requiredFrames == 0)
        return true;

    uint32_t requiredSamples = requiredFrames * m_format.channelCount();

    // Grab the next empty buffer from the audio device.
    unsigned char *hostBuffer{};
    hr = m_renderClient->GetBuffer(requiredFrames, &hostBuffer);
    if (FAILED(hr)) {
        qWarning() << "IAudioRenderClient::getBuffer failed" << audioClientErrorString(hr);
        return false;
    }

    runAudioSinkCallback(m_audioCallback, reinterpret_cast<std::byte *>(hostBuffer),
                         requiredSamples, m_format);

    constexpr DWORD flags = 0;
    hr = m_renderClient->ReleaseBuffer(requiredFrames, flags);
    if (FAILED(hr)) {
        qWarning() << "IAudioRenderClient::ReleaseBuffer failed" << audioClientErrorString(hr);
        return false;
    }

    return true;
}

void QWASAPIAudioSinkStream::handleAudioClientError()
{
    using namespace QWindowsAudioUtils;
    audioClientStop(m_audioClient);
    audioClientReset(m_audioClient);

    QMetaObject::invokeMethod(&m_ringbufferDrained, [this] {
        handleIOError(m_parent);
    });
}

///////////////////////////////////////////////////////////////////////////////////////////////////

QWindowsAudioSink::QWindowsAudioSink(QAudioDevice audioDevice, const QAudioFormat &fmt,
                                     QObject *parent)
    : QPlatformAudioSink(std::move(audioDevice), fmt, parent)
{
}

QWindowsAudioSink::~QWindowsAudioSink()
{
    stop();
}

QIODevice *QWindowsAudioSink::start()
{
    auto immDevice = QAudioDevicePrivate::handle<QWindowsAudioDevice>(m_audioDevice)->open();
    if (!immDevice) {
        setError(QtAudio::OpenError);
        return nullptr;
    }

    m_stream = std::make_unique<QWASAPIAudioSinkStream>(m_audioDevice, this, m_format, m_bufferSize,
                                                        m_hardwareBufferFrames, volume());

    QIODevice *ioDevice = m_stream->start(std::move(immDevice), m_endpointRole);
    if (!ioDevice) {
        setError(QtAudio::OpenError);
        m_stream = {};
        return nullptr;
    }

    updateStreamState(QtAudio::State::ActiveState);

    return ioDevice;
}

void QWindowsAudioSink::start(QIODevice *iodevice)
{
    auto immDevice = QAudioDevicePrivate::handle<QWindowsAudioDevice>(m_audioDevice)->open();
    if (!immDevice) {
        setError(QtAudio::OpenError);
        return;
    }

    m_stream = std::make_unique<QWASAPIAudioSinkStream>(m_audioDevice, this, m_format, m_bufferSize,
                                                        m_hardwareBufferFrames, volume());

    bool started = m_stream->start(iodevice, std::move(immDevice), m_endpointRole);
    if (!started) {
        setError(QtAudio::OpenError);
        m_stream = {};
        return;
    }

    updateStreamState(QtAudio::State::ActiveState);
}

void QWindowsAudioSink::stop()
{
    if (m_stream) {
        m_stream->stop(QPlatformAudioIOStream::ShutdownPolicy::DrainRingbuffer);
        m_stream = {};
        updateStreamState(QtAudio::State::StoppedState);
    }
}

void QWindowsAudioSink::reset()
{
    if (m_stream) {
        m_stream->stop(QPlatformAudioIOStream::ShutdownPolicy::DiscardRingbuffer);
        m_stream = {};
        updateStreamState(QtAudio::State::StoppedState);
    }
}

void QWindowsAudioSink::suspend()
{
    if (!m_stream)
        return;

    m_stream->suspend();

    updateStreamState(QtAudio::State::SuspendedState);
}

void QWindowsAudioSink::resume()
{
    if (!m_stream)
        return;

    if (state() == QtAudio::State::ActiveState)
        return;

    m_stream->resume();

    updateStreamState(QtAudio::State::ActiveState);
}

qsizetype QWindowsAudioSink::bytesFree() const
{
    return m_stream ? m_stream->bytesFree() : -1;
}

void QWindowsAudioSink::setBufferSize(qsizetype value)
{
    m_bufferSize = value;
}

qsizetype QWindowsAudioSink::bufferSize() const
{
    if (m_stream)
        return m_stream->ringbufferSizeInBytes();

    return QPlatformAudioIOStream::inferRingbufferBytes(m_bufferSize, m_hardwareBufferFrames,
                                                        m_format);
}

void QWindowsAudioSink::setHardwareBufferFrames(int32_t arg)
{
    if (arg > 0)
        m_hardwareBufferFrames = arg;
    else
        m_hardwareBufferFrames = std::nullopt;
}

int32_t QWindowsAudioSink::hardwareBufferFrames()
{
    return m_hardwareBufferFrames.value_or(-1);
}

qint64 QWindowsAudioSink::processedUSecs() const
{
    return m_stream ? m_stream->processedDuration().count() : 0;
}

void QWindowsAudioSink::setVolume(float v)
{
    QPlatformAudioEndpointBase::setVolume(v);
    if (m_stream)
        m_stream->setVolume(v);
}

void QWindowsAudioSink::setRole(AudioEndpointRole role)
{
    m_endpointRole = role;
}

void QWindowsAudioSink::start(AudioCallback &&audioCallback)
{
    using namespace QtMultimediaPrivate;
    if (!validateAudioSinkCallback(audioCallback, m_format)) {
        setError(QAudio::OpenError);
        return;
    }

    auto immDevice = QAudioDevicePrivate::handle<QWindowsAudioDevice>(m_audioDevice)->open();
    if (!immDevice) {
        setError(QtAudio::OpenError);
        return;
    }

    m_stream = std::make_unique<QWASAPIAudioSinkStream>(m_audioDevice, this, m_format, m_bufferSize,
                                                        m_hardwareBufferFrames, volume());

    bool started = m_stream->start(std::move(audioCallback), std::move(immDevice), m_endpointRole);
    if (!started) {
        setError(QtAudio::OpenError);
        return;
    }

    updateStreamState(QtAudio::State::ActiveState);
}

bool QWindowsAudioSink::hasCallbackAPI()
{
    return true;
}

QT_END_NAMESPACE
