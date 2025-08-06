// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsaudiosource_p.h"

#include <QtCore/qthread.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qfunctions_win_p.h>
#include <QtCore/private/quniquehandle_types_p.h>
#include <QtMultimedia/private/qaudioformat_p.h>
#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>
#include <QtMultimedia/private/qmemory_resource_tlsf_p.h>
#include <QtMultimedia/private/qwindowsaudiodevice_p.h>
#include <QtMultimedia/private/qwindowsaudioutils_p.h>

#include <audioclient.h>
#include <mmdeviceapi.h>

QT_BEGIN_NAMESPACE

namespace QtWASAPI {

using QWindowsAudioUtils::audioClientErrorString;
using namespace std::chrono_literals;

namespace {
QAudioFormat makeHostFormatForSource(const QAudioDevice &device, const QAudioFormat &format)
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

    const int requestedSampleRate = format.sampleRate();
    auto [minProbedSampleRate, maxProbedSampleRate] = winDevice->m_probedSampleRateRange;

    if (requestedSampleRate < device.minimumSampleRate())
        hostFormat.setSampleRate(minProbedSampleRate);
    else if (requestedSampleRate > device.maximumSampleRate())
        hostFormat.setSampleRate(maxProbedSampleRate);

    return hostFormat;
}
} // namespace

QWASAPIAudioSourceStream::QWASAPIAudioSourceStream(QAudioDevice device, const QAudioFormat &format,
                                                   std::optional<qsizetype> ringbufferSize,
                                                   QWindowsAudioSource *parent,
                                                   float volume,
                                                   std::optional<int32_t> hardwareBufferFrames):
    QPlatformAudioSourceStream{
        std::move(device),
        format,
        ringbufferSize,
        hardwareBufferFrames,
        volume,
    },
    m_wasapiHandle {
        CreateEvent(0, false, false, nullptr),
    },
    m_parent{
        parent
    },
    m_hostFormat {
        makeHostFormatForSource(m_audioDevice, format),
    }
{
}

QWASAPIAudioSourceStream::~QWASAPIAudioSourceStream() = default;

bool QWASAPIAudioSourceStream::start(QIODevice *ioDevice)
{
    auto immDevice = QAudioDevicePrivate::handle<QWindowsAudioDevice>(m_audioDevice)->open();

    bool clientOpen = openAudioClient(std::move(immDevice));
    if (!clientOpen)
        return false;

    setQIODevice(ioDevice);
    createQIODeviceConnections(ioDevice);

    bool started = startAudioClient();
    if (!started)
        return false;

    return true;
}

QIODevice *QWASAPIAudioSourceStream::start()
{
    auto immDevice = QAudioDevicePrivate::handle<QWindowsAudioDevice>(m_audioDevice)->open();

    bool clientOpen = openAudioClient(std::move(immDevice));
    if (!clientOpen)
        return nullptr;

    QIODevice *ioDevice = createRingbufferReaderDevice();

    m_parent->updateStreamIdle(true, QWindowsAudioSource::EmitStateSignal::False);

    setQIODevice(ioDevice);
    createQIODeviceConnections(ioDevice);

    bool started = startAudioClient();
    if (!started)
        return nullptr;

    return ioDevice;
}

bool QWASAPIAudioSourceStream::start(AudioCallback &&cb)
{
    auto immDevice = QAudioDevicePrivate::handle<QWindowsAudioDevice>(m_audioDevice)->open();

    bool clientOpen = openAudioClient(std::move(immDevice));
    if (!clientOpen)
        return false;

    m_audioCallback = std::move(cb);

    return startAudioClient();
}

void QWASAPIAudioSourceStream::suspend()
{
    m_suspended = true;
    QWindowsAudioUtils::audioClientStop(m_audioClient);
}

void QWASAPIAudioSourceStream::resume()
{
    m_suspended = false;
    QWindowsAudioUtils::audioClientStart(m_audioClient);
}

void QWASAPIAudioSourceStream::stop(ShutdownPolicy shutdownPolicy)
{
    m_parent = nullptr;
    m_shutdownPolicy = shutdownPolicy;

    requestStop();
    disconnectQIODeviceConnections();

    QWindowsAudioUtils::audioClientStop(m_audioClient);
    m_workerThread->wait();
    QWindowsAudioUtils::audioClientReset(m_audioClient);

    finalizeQIODevice(shutdownPolicy);
    if (shutdownPolicy == ShutdownPolicy::DiscardRingbuffer)
        emptyRingbuffer();
}

void QWASAPIAudioSourceStream::updateStreamIdle(bool streamIsIdle)
{
    if (m_parent)
        m_parent->updateStreamIdle(streamIsIdle);
}

bool QWASAPIAudioSourceStream::openAudioClient(ComPtr<IMMDevice> device)
{
    using namespace QWindowsAudioUtils;

    std::optional<AudioClientCreationResult> clientData =
            createAudioClient(device, m_hostFormat, m_hardwareBufferFrames, m_wasapiHandle);

    if (!clientData)
        return false;

    m_audioClient = std::move(clientData->client);
    m_periodSize = clientData->periodSize;
    m_audioClientFrames = clientData->audioClientFrames;

    HRESULT hr = m_audioClient->GetService(IID_PPV_ARGS(m_captureClient.GetAddressOf()));
    if (FAILED(hr)) {
        qWarning() << "IAudioClient3::GetService failed to obtain IAudioCaptureClient"
                   << audioClientErrorString(hr);
        return false;
    }

    return true;
}

bool QWASAPIAudioSourceStream::startAudioClient()
{
    using namespace QWindowsAudioUtils;
    m_workerThread.reset(QThread::create([this] {
        setMCSSForPeriodSize(m_periodSize);

        std::optional<QComHelper> m_comHelper;

        if (m_hostFormat != m_format) {
            m_comHelper.emplace();
            m_resampler = std::make_unique<QWindowsResampler>();
            m_resampler->setup(m_hostFormat, m_format);

            m_memoryResource = std::make_unique<QTlsfMemoryResource>(512 * 1024);
        }

        runProcessLoop();
    }));

    m_workerThread->setObjectName(u"QWASAPIAudioSourceStream");
    m_workerThread->start();

    return audioClientStart(m_audioClient);
}

void QWASAPIAudioSourceStream::runProcessLoop()
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
            return; // TODO: distinguish between stop/reset?

        bool success = m_audioCallback ? processCallback() : processRingbuffer();
        if (!success) {
            handleAudioClientError();
            return;
        }
    }
}

template <typename Functor>
bool QWASAPIAudioSourceStream::visitAudioClientBuffer(Functor &&f)
{
    for (;;) {
        unsigned char *hostBuffer;
        uint32_t hostBufferFrames;
        DWORD flags;
        uint64_t devicePosition;
        uint64_t QPCPosition;
        HRESULT hr = m_captureClient->GetBuffer(&hostBuffer, &hostBufferFrames, &flags,
                                                &devicePosition, &QPCPosition);
        if (FAILED(hr)) {
            qWarning() << "IAudioCaptureClient::GetBuffer failed" << audioClientErrorString(hr);
            return false;
        }

        // Note: when starting the capture client, we an initial wakeup, despite no buffer being
        // available
        if (hostBufferFrames == 0)
            return true;

        QSpan hostBufferSpan{
            hostBuffer,
            m_hostFormat.bytesForFrames(hostBufferFrames),
        };

        if (m_resampler) {
            Q_UNLIKELY_BRANCH;
            auto resampledBuffer =
                    m_resampler->resample(as_bytes(hostBufferSpan), m_memoryResource.get());
            QPlatformAudioSourceStream::process(resampledBuffer,
                                                m_format.framesForBytes(resampledBuffer.size()));
        } else {
            f(as_bytes(hostBufferSpan), hostBufferFrames);
        }

        hr = m_captureClient->ReleaseBuffer(hostBufferFrames);

        if (FAILED(hr)) {
            qWarning() << "IAudioCaptureClient::ReleaseBuffer failed" << audioClientErrorString(hr);
            return false;
        }
    }
}

bool QWASAPIAudioSourceStream::processRingbuffer() noexcept QT_MM_NONBLOCKING
{
    return visitAudioClientBuffer(
            [&](QSpan<const std::byte> hostBuffer, uint32_t hostBufferFrames) {
        uint64_t framesWritten = QPlatformAudioSourceStream::process(hostBuffer, hostBufferFrames);
        if (framesWritten != hostBufferFrames)
            updateStreamIdle(true);
    });
}

bool QWASAPIAudioSourceStream::processCallback() noexcept QT_MM_NONBLOCKING
{
    return visitAudioClientBuffer([&](QSpan<const std::byte> hostBuffer, uint32_t) {
        runAudioCallback(*m_audioCallback, as_bytes(hostBuffer), m_format, volume());
    });
}

void QWASAPIAudioSourceStream::handleAudioClientError()
{
    using namespace QWindowsAudioUtils;
    audioClientStop(m_audioClient);
    audioClientReset(m_audioClient);
    requestStop();

    invokeOnAppThread([this] {
        handleIOError(m_parent);
    });
}

///////////////////////////////////////////////////////////////////////////////////////////////////

QWindowsAudioSource::QWindowsAudioSource(QAudioDevice audioDevice, const QAudioFormat &fmt,
                                         QObject *parent)
    : BaseClass(std::move(audioDevice), fmt, parent)
{
}

} // namespace QtWASAPI

QT_END_NAMESPACE
