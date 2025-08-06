// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsaudiosink_p.h"

#include <QtCore/private/qsystemerror_p.h>
#include <QtCore/private/qfunctions_win_p.h>
#include <QtMultimedia/private/qmemory_resource_tlsf_p.h>
#include <QtMultimedia/private/qwindowsaudiodevice_p.h>
#include <QtMultimedia/private/qwindowsresampler_p.h>

#include <audioclient.h>
#include <mmdeviceapi.h>

QT_BEGIN_NAMESPACE

namespace QtWASAPI {

using QWindowsAudioUtils::audioClientErrorString;
using namespace std::chrono_literals;

namespace {

QAudioFormat makeHostFormatForSink(const QAudioDevice &device, const QAudioFormat &format)
{
    const QWindowsAudioDevice *winDevice = QAudioDevicePrivate::handle<QWindowsAudioDevice>(device);

    QAudioFormat hostFormat = format;
    const int requestedChannelCount = format.channelCount();
    auto [minProbedChannels, maxProbedChannels] = winDevice->m_probedChannelCountRange;

    if (requestedChannelCount < device.minimumChannelCount()) {
        hostFormat.setChannelCount(minProbedChannels);
        hostFormat.setChannelConfig(
                QAudioFormat::defaultChannelConfigForChannelCount(minProbedChannels));
    } else if (requestedChannelCount > device.maximumChannelCount()) {
        hostFormat.setChannelCount(maxProbedChannels);
        hostFormat.setChannelConfig(
                QAudioFormat::defaultChannelConfigForChannelCount(maxProbedChannels));
    }

    return hostFormat;
}

} // namespace

QWASAPIAudioSinkStream::QWASAPIAudioSinkStream(QAudioDevice device, const QAudioFormat &format, std::optional<qsizetype> ringbufferSize,
                                               QWindowsAudioSink *parent, float volume, std::optional<int32_t> hardwareBufferFrames, AudioEndpointRole role):
    QPlatformAudioSinkStream{
        std::move(device),
        format,
        ringbufferSize,
        hardwareBufferFrames,
        volume,
    },
    m_role{
          role,
    },
    m_wasapiHandle {
        CreateEvent(0, false, false, nullptr),
    },
    m_parent{
        parent
    },
    m_hostFormat {
        makeHostFormatForSink(m_audioDevice, format),
    }
{
}

bool QWASAPIAudioSinkStream::open()
{
    return true;
}

bool QWASAPIAudioSinkStream::start(QIODevice *ioDevice)
{
    auto immDevice = QAudioDevicePrivate::handle<QWindowsAudioDevice>(m_audioDevice)->open();
    bool clientOpen = openAudioClient(std::move(immDevice), m_role);
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

QIODevice *QWASAPIAudioSinkStream::start()
{
    auto immDevice = QAudioDevicePrivate::handle<QWindowsAudioDevice>(m_audioDevice)->open();
    bool clientOpen = openAudioClient(std::move(immDevice), m_role);
    if (!clientOpen)
        return nullptr;

    QIODevice *ioDevice = createRingbufferWriterDevice();

    m_parent->updateStreamIdle(true, QWindowsAudioSink::EmitStateSignal::False);

    setQIODevice(ioDevice);
    createQIODeviceConnections(ioDevice);

    bool started = startAudioClient(StreamType::Ringbuffer);
    if (!started)
        return nullptr;

    return ioDevice;
}

bool QWASAPIAudioSinkStream::start(AudioCallback audioCallback)
{
    auto immDevice = QAudioDevicePrivate::handle<QWindowsAudioDevice>(m_audioDevice)->open();
    bool clientOpen = openAudioClient(std::move(immDevice), m_role);
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
        m_ringbufferDrained.callOnActivated([self = shared_from_this()]() mutable {
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
            createAudioClient(device, m_hostFormat, m_hardwareBufferFrames, m_wasapiHandle, role);

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

    if (m_audioDevice.preferredFormat().sampleRate() != m_hostFormat.sampleRate())
        audioClientSetRate(m_audioClient, m_hostFormat.sampleRate());

    return true;
}

bool QWASAPIAudioSinkStream::startAudioClient(StreamType streamType)
{
    using namespace QWindowsAudioUtils;
    m_workerThread.reset(QThread::create([this, streamType] {
        setMCSSForPeriodSize(m_periodSize);
        fillInitialHostBuffer();
        std::optional<QComHelper> m_comHelper;

        if (m_hostFormat != m_format) {
            m_comHelper.emplace();
            m_resampler = std::make_unique<QWindowsResampler>();
            m_resampler->setup(m_format, m_hostFormat);

            m_memoryResource = std::make_unique<QTlsfMemoryResource>(512 * 1024);
        }

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

template <typename Functor>
bool QWASAPIAudioSinkStream::visitAudioClientBuffer(Functor &&f)
{
    uint32_t numFramesPadding;
    HRESULT hr = m_audioClient->GetCurrentPadding(&numFramesPadding);
    if (FAILED(hr)) {
        qWarning() << "IAudioClient3::GetCurrentPadding failed" << audioClientErrorString(hr);
        return false;
    }

    const uint32_t requiredFrames = m_audioClientFrames - numFramesPadding;
    if (requiredFrames == 0)
        return true;

    // Grab the next empty buffer from the audio device.
    unsigned char *hostBuffer{};
    hr = m_renderClient->GetBuffer(requiredFrames, &hostBuffer);
    if (FAILED(hr)) {
        qWarning() << "IAudioRenderClient::getBuffer failed" << audioClientErrorString(hr);
        return false;
    }

    QSpan<std::byte> hostBufferSpan{
        reinterpret_cast<std::byte *>(hostBuffer),
        m_hostFormat.bytesForFrames(requiredFrames),
    };

    uint64_t consumedFrames;
    if (m_resampler) {
        Q_UNLIKELY_BRANCH;

        std::pmr::vector<std::byte> resampleBuffer{
            size_t(m_format.bytesForFrames(requiredFrames)),
            m_memoryResource.get(),
        };
        consumedFrames = f(as_writable_bytes(QSpan{ resampleBuffer }), requiredFrames);

        auto resampledBuffer = m_resampler->resample(resampleBuffer, m_memoryResource.get());

        Q_ASSERT(resampledBuffer.size() == size_t(hostBufferSpan.size()));
        std::copy_n(resampledBuffer.data(), resampledBuffer.size(), hostBufferSpan.data());
    } else {
        consumedFrames = f(hostBufferSpan, requiredFrames);
    }

    const DWORD flags = consumedFrames != 0 ? 0 : AUDCLNT_BUFFERFLAGS_SILENT;

    hr = m_renderClient->ReleaseBuffer(requiredFrames, flags);
    if (FAILED(hr)) {
        qWarning() << "IAudioRenderClient::ReleaseBuffer failed" << audioClientErrorString(hr);
        return false;
    }

    return true;
}

bool QWASAPIAudioSinkStream::processRingbuffer() noexcept QT_MM_NONBLOCKING
{
    return visitAudioClientBuffer([&](QSpan<std::byte> hostBuffer, uint32_t requiredFrames) {
        uint64_t consumedFrames = QPlatformAudioSinkStream::process(hostBuffer, requiredFrames);
        return consumedFrames;
    });
}

bool QWASAPIAudioSinkStream::processCallback() noexcept QT_MM_NONBLOCKING
{
    return visitAudioClientBuffer([&](QSpan<std::byte> hostBuffer, uint32_t requiredFrames) {
        runAudioCallback(m_audioCallback, hostBuffer, m_format, volume());
        return requiredFrames;
    });
}

void QWASAPIAudioSinkStream::handleAudioClientError()
{
    using namespace QWindowsAudioUtils;
    audioClientStop(m_audioClient);
    audioClientReset(m_audioClient);

    invokeOnAppThread([this] {
        handleIOError(m_parent);
    });
}

QWindowsAudioSink::QWindowsAudioSink(QAudioDevice audioDevice, const QAudioFormat &fmt,
                                     QObject *parent)
    : BaseClass(std::move(audioDevice), fmt, parent)
{
}

} // namespace QtWASAPI

QT_END_NAMESPACE
