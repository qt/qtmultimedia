// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsaudiosource_p.h"

#include <QtCore/qthread.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/private/quniquehandle_types_p.h>
#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>
#include <QtMultimedia/private/qwindowsaudiodevice_p.h>
#include <QtMultimedia/private/qwindowsaudioutils_p.h>

#include <audioclient.h>
#include <mmdeviceapi.h>

QT_BEGIN_NAMESPACE

using QtMultimediaPrivate::QPlatformAudioIOStream;
using QtMultimediaPrivate::QPlatformAudioSourceStream;
using QWindowsAudioUtils::audioClientErrorString;
using namespace std::chrono_literals;

struct QWASAPIAudioSourceStream final : std::enable_shared_from_this<QWASAPIAudioSourceStream>,
                                        QPlatformAudioSourceStream
{
    using SampleFormat = QAudioFormat::SampleFormat;

    QWASAPIAudioSourceStream(QAudioDevice, QWindowsAudioSource *parent, const QAudioFormat &,
                             std::optional<qsizetype> ringbufferSize,
                             std::optional<int32_t> hardwareBufferFrames, float volume);

    ~QWASAPIAudioSourceStream();

    using QPlatformAudioSourceStream::bytesReady;
    using QPlatformAudioSourceStream::deviceIsRingbufferReader;
    using QPlatformAudioSourceStream::processedDuration;
    using QPlatformAudioSourceStream::ringbufferSizeInBytes;
    using QPlatformAudioSourceStream::setVolume;

    bool start(QIODevice *, ComPtr<IMMDevice>);
    QIODevice *start(ComPtr<IMMDevice>);

    void suspend();
    void resume();
    void stop(ShutdownPolicy);

    void updateStreamIdle(bool) override;

private:
    bool openAudioClient(ComPtr<IMMDevice> device);
    bool startAudioClient();

    void runProcessLoop();
    bool process() noexcept QT_MM_NONBLOCKING;
    void handleAudioClientError();

    ComPtr<IAudioClient3> m_audioClient;
    ComPtr<IAudioCaptureClient> m_captureClient;

    QWindowsAudioUtils::reference_time m_periodSize;
    qsizetype m_audioClientFrames;

    std::atomic_bool m_suspended{};
    std::atomic<ShutdownPolicy> m_shutdownPolicy{ ShutdownPolicy::DiscardRingbuffer };
    QAutoResetEvent m_ringbufferDrained;

    const QUniqueWin32NullHandle m_wasapiHandle;
    std::unique_ptr<QThread> m_workerThread;

    QWindowsAudioSource *m_parent;
};

QWASAPIAudioSourceStream::~QWASAPIAudioSourceStream() = default;

void QWASAPIAudioSourceStream::updateStreamIdle(bool streamIsIdle)
{
    if (m_parent)
        m_parent->updateStreamIdle(streamIsIdle);
}

bool QWASAPIAudioSourceStream::openAudioClient(ComPtr<IMMDevice> device)
{
    using namespace QWindowsAudioUtils;

    std::optional<AudioClientCreationResult> clientData =
            createAudioClient(device, m_format, m_hardwareBufferFrames, m_wasapiHandle);

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

    if (m_audioDevice.preferredFormat().sampleRate() != m_format.sampleRate())
        audioClientSetRate(m_audioClient, m_format.sampleRate());

    return true;
}

bool QWASAPIAudioSourceStream::startAudioClient()
{
    using namespace QWindowsAudioUtils;
    m_workerThread.reset(QThread::create([this] {
        setMCSSForPeriodSize(m_periodSize);
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

        bool success = process();
        if (!success) {
            handleAudioClientError();
            return;
        }
    }
}

bool QWASAPIAudioSourceStream::process() noexcept QT_MM_NONBLOCKING
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

        QSpan hostBufferSpan{
            hostBuffer,
            m_format.bytesForFrames(hostBufferFrames),
        };

        uint64_t framesWritten =
                QPlatformAudioSourceStream::process(as_bytes(hostBufferSpan), hostBufferFrames);
        if (framesWritten != hostBufferFrames)
            updateStreamIdle(true);

        hr = m_captureClient->ReleaseBuffer(hostBufferFrames);
        if (FAILED(hr)) {
            qWarning() << "IAudioCaptureClient::ReleaseBuffer failed" << audioClientErrorString(hr);
            return false;
        }

        uint32_t framesInNextPacket;
        hr = m_captureClient->GetNextPacketSize(&framesInNextPacket);

        if (FAILED(hr)) {
            qWarning() << "IAudioCaptureClient::GetNextPacketSize failed"
                       << audioClientErrorString(hr);
            return false;
        }

        if (framesInNextPacket == 0)
            return true;
    }
}

void QWASAPIAudioSourceStream::handleAudioClientError()
{
    using namespace QWindowsAudioUtils;
    audioClientStop(m_audioClient);
    audioClientReset(m_audioClient);

    QMetaObject::invokeMethod(&m_ringbufferDrained, [this] {
        handleIOError(m_parent);
    });
}

///////////////////////////////////////////////////////////////////////////////////////////////////

QWindowsAudioSource::QWindowsAudioSource(QAudioDevice audioDevice, const QAudioFormat &fmt,
                                         QObject *parent)
    : QPlatformAudioSource(std::move(audioDevice), fmt, parent)
{
}

QWindowsAudioSource::~QWindowsAudioSource()
{
    stop();
}

QIODevice *QWindowsAudioSource::start()
{
    auto immDevice = QAudioDevicePrivate::handle<QWindowsAudioDevice>(m_audioDevice)->open();
    if (!immDevice) {
        setError(QtAudio::OpenError);
        m_stream = {};
        return nullptr;
    }

    m_stream = std::make_unique<QWASAPIAudioSourceStream>(
            m_audioDevice, this, m_format, m_bufferSize, m_hardwareBufferFrames, volume());

    QIODevice *ioDevice = m_stream->start(std::move(immDevice));
    if (!ioDevice) {
        setError(QtAudio::OpenError);
        return nullptr;
    }

    updateStreamState(QtAudio::State::ActiveState);

    return ioDevice;
}

void QWindowsAudioSource::start(QIODevice *iodevice)
{
    auto immDevice = QAudioDevicePrivate::handle<QWindowsAudioDevice>(m_audioDevice)->open();
    if (!immDevice) {
        setError(QtAudio::OpenError);
        return;
    }

    m_stream = std::make_unique<QWASAPIAudioSourceStream>(
            m_audioDevice, this, m_format, m_bufferSize, m_hardwareBufferFrames, volume());

    bool started = m_stream->start(iodevice, std::move(immDevice));
    if (!started) {
        setError(QtAudio::OpenError);
        m_stream = {};
        return;
    }

    updateStreamState(QtAudio::State::ActiveState);
}

void QWindowsAudioSource::stop()
{
    if (!m_stream)
        return;

    if (m_stream->deviceIsRingbufferReader())
        // we own the qiodevice, so let's keep it alive to allow users to drain the ringbuffer
        m_retiredStream = m_stream;

    m_stream->stop(QPlatformAudioIOStream::ShutdownPolicy::DrainRingbuffer);
    m_stream = {};
    updateStreamState(QtAudio::State::StoppedState);
}

void QWindowsAudioSource::reset()
{
    m_retiredStream = {};

    if (!m_stream)
        return;

    m_stream->stop(QPlatformAudioIOStream::ShutdownPolicy::DiscardRingbuffer);
    m_stream = {};
    updateStreamState(QtAudio::State::StoppedState);
}

void QWindowsAudioSource::suspend()
{
    if (!m_stream)
        return;

    m_stream->suspend();

    updateStreamState(QtAudio::State::SuspendedState);
}

void QWindowsAudioSource::resume()
{
    if (!m_stream)
        return;

    if (state() == QtAudio::State::ActiveState)
        return;

    m_stream->resume();

    updateStreamState(QtAudio::State::ActiveState);
}

qsizetype QWindowsAudioSource::bytesReady() const
{
    return m_stream ? m_stream->bytesReady() : 0;
}

void QWindowsAudioSource::setBufferSize(qsizetype value)
{
    m_bufferSize = value;
}

qsizetype QWindowsAudioSource::bufferSize() const
{
    if (m_stream)
        return m_stream->ringbufferSizeInBytes();

    return QPlatformAudioIOStream::inferRingbufferBytes(m_bufferSize, m_hardwareBufferFrames,
                                                        m_format);
}

void QWindowsAudioSource::setHardwareBufferFrames(int32_t arg)
{
    if (arg > 0)
        m_hardwareBufferFrames = arg;
    else
        m_hardwareBufferFrames = std::nullopt;
}

int32_t QWindowsAudioSource::hardwareBufferFrames()
{
    return m_hardwareBufferFrames.value_or(-1);
}

qint64 QWindowsAudioSource::processedUSecs() const
{
    return m_stream ? m_stream->processedDuration().count() : 0;
}

QWASAPIAudioSourceStream::QWASAPIAudioSourceStream(QAudioDevice device, QWindowsAudioSource *parent,
                                                   const QAudioFormat &format,
                                                   std::optional<qsizetype> ringbufferSize,
                                                   std::optional<int32_t> hardwareBufferFrames, float volume):
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
    }
{
}

bool QWASAPIAudioSourceStream::start(QIODevice *ioDevice, ComPtr<IMMDevice> immDevice)
{
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

QIODevice *QWASAPIAudioSourceStream::start(ComPtr<IMMDevice> immDevice)
{
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

void QWindowsAudioSource::setVolume(float v)
{
    QPlatformAudioEndpointBase::setVolume(v);
    if (m_stream)
        m_stream->setVolume(v);
}

QT_END_NAMESPACE
