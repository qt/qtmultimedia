// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// INTERNAL USE ONLY: Do NOT use for any other purpose.
//

#include "qwindowsaudiosink_p.h"
#include "qwindowsaudioutils_p.h"
#include "qwindowsmultimediautils_p.h"
#include "qcomtaskresource_p.h"

#include <QtCore/QDataStream>
#include <QtCore/qtimer.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qpointer.h>

#include <private/qaudiohelpers_p.h>

#include <audioclient.h>
#include <mmdeviceapi.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcAudioOutput, "qt.multimedia.audiooutput")

using namespace QWindowsMultimediaUtils;
using namespace QWindowsAudioUtils;

class OutputPrivate : public QIODevice
{
    Q_OBJECT
public:
    OutputPrivate(QWindowsAudioSink &audio) : QIODevice(&audio), audioDevice(audio) {}
    ~OutputPrivate() override = default;

    qint64 readData(char *, qint64) override { return 0; }
    qint64 writeData(const char *data, qint64 len) override { return audioDevice.push(data, len); }

private:
    QWindowsAudioSink &audioDevice;
};


std::unique_ptr<AudioClient> AudioClient::create(const ComPtr<IMMDevice> &device,
                                                 const QAudioFormat &format, qsizetype &bufferSize)
{
    std::unique_ptr<AudioClient> client{ //
        new AudioClient{ device, format }
    }; // No make_unique with private ctor

    if (client->create(bufferSize))
        return client;

    return {};
}

AudioClient::AudioClient(const ComPtr<IMMDevice> &device, const QAudioFormat &format)
    : m_device{ device }, m_inputFormat{ format }
{
}

bool AudioClient::create(qsizetype &bufferSize)
{
    HRESULT hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, nullptr,
                                    reinterpret_cast<void**>(m_audioClient.GetAddressOf()));
    if (FAILED(hr)) {
        qCWarning(qLcAudioOutput) << "Failed to activate audio device" << errorString(hr);
        return false;
    }

    QComTaskResource<WAVEFORMATEX> mixFormat;
    hr = m_audioClient->GetMixFormat(mixFormat.address());
    if (FAILED(hr)) {
        qCWarning(qLcAudioOutput) << "Format unsupported" << errorString(hr);
        return false;
    }

    m_outputFormat = waveFormatExToFormat(*mixFormat);

    if (!resetResampler())
        return false;

    if (bufferSize == 0)
        bufferSize = m_inputFormat.sampleRate() * m_inputFormat.bytesPerFrame() / 2;

    REFERENCE_TIME requestedDuration =
            m_inputFormat.durationForBytes(static_cast<qint32>(bufferSize)) * 10;

    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, requestedDuration, 0, mixFormat.get(),
                                   nullptr);

    if (FAILED(hr)) {
        qCWarning(qLcAudioOutput) << "Failed to initialize audio client" << errorString(hr);
        return false;
    }

    auto framesAllocated = allocatedFrames(m_audioClient.Get());
    if (!framesAllocated) {
        qCWarning(qLcAudioOutput) << "Failed to get audio client buffer size";
        return false;
    }

    bufferSize = m_inputFormat.bytesForDuration(
            m_outputFormat.durationForFrames(static_cast<qint32>(*framesAllocated)));

    hr = m_audioClient->GetService(IID_PPV_ARGS(m_renderClient.GetAddressOf()));
    if (FAILED(hr)) {
        qCWarning(qLcAudioOutput) << "Failed to obtain audio client rendering service"
                                  << errorString(hr);
        return false;
    }

    return true;
}

std::chrono::microseconds AudioClient::remainingPlayTime()
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    std::optional<quint32> framesInUse = usedFrames(m_audioClient.Get());
    if (!framesInUse)
        return 0us;

    const qint32 frameCount = static_cast<qint32>(*framesInUse);
    const qint64 durationUs = m_outputFormat.durationForFrames(frameCount);
    return microseconds{ durationUs };
}

void AudioClient::start()
{
    m_audioClient->Start();
}

void AudioClient::stop()
{
    m_audioClient->Stop();
}

std::optional<quint32> AudioClient::availableFrameCount() const
{
    const auto framesAllocated = allocatedFrames(m_audioClient.Get());
    const auto framesInUse = usedFrames(m_audioClient.Get());

    if (framesAllocated && framesInUse)
        return *framesAllocated - *framesInUse;
    return {};
}

bool AudioClient::resetResampler()
{
    const bool success = m_resampler.setup(m_inputFormat, m_outputFormat);
    if (!success)
        qCWarning(qLcAudioOutput) << "Failed to set up resampler";
    return success;
}

quint64 AudioClient::bytesFree() const
{
    if (!m_audioClient)
        return 0;

    const auto framesAvailable = availableFrameCount();
    if (framesAvailable)
        return m_resampler.inputBufferSize(*framesAvailable * m_outputFormat.bytesPerFrame());
    return 0;
}

quint64 AudioClient::totalInputBytes() const
{
    return m_resampler.totalInputBytes();
}

qint64 AudioClient::render(const QAudioFormat &format, qreal volume, const char *data, qint64 len)
{
    Q_ASSERT(m_audioClient);
    Q_ASSERT(m_renderClient);

    qCDebug(qLcAudioOutput) << "render()" << len;

    auto framesAvailable = availableFrameCount();
    if (!framesAvailable)
        return -1;

    auto maxBytesCanWrite =
            format.bytesForDuration(m_outputFormat.durationForFrames(*framesAvailable));
    qsizetype writeSize = qMin(maxBytesCanWrite, len);

    QByteArray writeBytes = m_resampler.resample({ data, writeSize });
    qint32 writeFramesNum = m_outputFormat.framesForBytes(writeBytes.size());

    quint8 *buffer = nullptr;
    HRESULT hr = m_renderClient->GetBuffer(writeFramesNum, &buffer);
    if (FAILED(hr)) {
        qCWarning(qLcAudioOutput) << "Failed to get buffer" << errorString(hr);
        return -1;
    }

    if (volume < qreal(1.0))
        QAudioHelperInternal::qMultiplySamples(volume, m_outputFormat, writeBytes.data(), buffer,
                                               writeBytes.size());
    else
        std::memcpy(buffer, writeBytes.data(), writeBytes.size());

    DWORD flags = writeBytes.isEmpty() ? AUDCLNT_BUFFERFLAGS_SILENT : 0;
    hr = m_renderClient->ReleaseBuffer(writeFramesNum, flags);
    if (FAILED(hr)) {
        qCWarning(qLcAudioOutput) << "Failed to return buffer" << errorString(hr);
        return -1;
    }
    return writeSize;
};

QWindowsAudioSink::QWindowsAudioSink(ComPtr<IMMDevice> device, QObject *parent) :
    QPlatformAudioSink(parent),
    m_timer(new QTimer(this)),
    m_pushSource(new OutputPrivate(*this)),
    m_device{ std::move(device) }
{
    m_pushSource->open(QIODevice::WriteOnly|QIODevice::Unbuffered);
    m_timer->setSingleShot(true);
    m_timer->setTimerType(Qt::PreciseTimer);
}

QWindowsAudioSink::~QWindowsAudioSink()
{
    close();
}

void QWindowsAudioSink::deviceStateChange(QAudio::State state, QAudio::Error error)
{
    if (state != deviceState) {
        if (state == QAudio::ActiveState) {
            m_client->start();
            qCDebug(qLcAudioOutput) << "Audio client started";

        } else if (deviceState == QAudio::ActiveState) {
            m_timer->stop();
            m_client->stop();
            qCDebug(qLcAudioOutput) << "Audio client stopped";
        }

        QPointer<QWindowsAudioSink> thisGuard(this);
        deviceState = state;
        emit stateChanged(deviceState);
        if (!thisGuard)
            return;
    }

    if (error != errorState) {
        errorState = error;
        emit errorChanged(error);
    }
}

QAudioFormat QWindowsAudioSink::format() const
{
    return m_format;
}

void QWindowsAudioSink::setFormat(const QAudioFormat& fmt)
{
    if (deviceState == QAudio::StoppedState) {
        m_format = fmt;
        m_recreateClient = true;
    }
}

/*!
Pull data from audio source and render through WASAPI audio client.

A timer is used to call pullSource periodically. When the source has
no more data, pullSource will set the sink's state to idle.
*/
void QWindowsAudioSink::pullSource()
{
    using namespace std::chrono_literals;
    using namespace std::chrono;

    qCDebug(qLcAudioOutput) << "Pull source";
    if (!m_pullSource)
        return;

    const qint64 bytesAvailable = m_pullSource->isOpen() ? m_pullSource->bytesAvailable() : 0;
    const qint64 readLen = qMin(bytesFree(), bytesAvailable);
    if (readLen > 0) {
        const QByteArray samples = m_pullSource->read(readLen);
        if (samples.isEmpty()) {
            // Unexpected end of stream or IO error
            deviceStateChange(QAudio::IdleState, QAudio::IOError);
            return;
        }

        Q_ASSERT(m_client);

        m_client->render(m_format, m_volume, samples.data(), samples.size());
    }

    const microseconds playTime = m_client->remainingPlayTime();
    if (playTime == 0us) {
        deviceStateChange(QAudio::IdleState, m_pullSource->atEnd() ? QAudio::NoError : QAudio::UnderrunError);
    } else {
        // Note: deviceStateChange starts WASAPI audio client when transitioning into ActiveState
        // This is done after calling render, to ensure that the audio client has data to play.
        deviceStateChange(QAudio::ActiveState, QAudio::NoError);

        // Schedule next call to pullSource
        m_timer->start(duration_cast<milliseconds>(playTime / 2));
    }
}

void QWindowsAudioSink::start(QIODevice* device)
{
    qCDebug(qLcAudioOutput) << "start(ioDevice)" << deviceState;
    if (deviceState != QAudio::StoppedState)
        close();

    if (device == nullptr)
        return;

    if (!open()) {
        errorState = QAudio::OpenError;
        emit errorChanged(QAudio::OpenError);
        return;
    }

    m_pullSource = device;

    connect(device, &QIODevice::readyRead, this, &QWindowsAudioSink::pullSource);
    m_timer->disconnect();
    m_timer->callOnTimeout(this, &QWindowsAudioSink::pullSource);
    pullSource();
}

qint64 QWindowsAudioSink::push(const char *data, qint64 len)
{
    using namespace std::chrono;

    if (deviceState == QAudio::StoppedState)
        return -1;

    Q_ASSERT(m_client);

    qint64 bytesRendered = m_client->render(m_format, m_volume, data, len);
    if (bytesRendered > 0) {
        deviceStateChange(QAudio::ActiveState, QAudio::NoError);
        m_timer->start(duration_cast<milliseconds>(m_client->remainingPlayTime()));
    }

    return bytesRendered;
}

QIODevice* QWindowsAudioSink::start()
{
    qCDebug(qLcAudioOutput) << "start()";
    if (deviceState != QAudio::StoppedState)
        close();

    if (!open()) {
        errorState = QAudio::OpenError;
        emit errorChanged(QAudio::OpenError);
        return nullptr;
    }

    deviceStateChange(QAudio::IdleState, QAudio::NoError);

    m_timer->disconnect();
    m_timer->callOnTimeout(this, [this](){
        deviceStateChange(QAudio::IdleState, QAudio::UnderrunError);
    });

    return m_pushSource.get();
}

bool QWindowsAudioSink::open()
{
    if (m_recreateClient) {
        m_client = nullptr;
        m_recreateClient = false;
    }

    if (m_client) {
        m_client->resetResampler();
        return true;
    }

    m_client = AudioClient::create(m_device, m_format, m_bufferSize);

    return m_client != nullptr;
}

void QWindowsAudioSink::close()
{
    qCDebug(qLcAudioOutput) << "close()";
    if (deviceState == QAudio::StoppedState)
        return;

    deviceStateChange(QAudio::StoppedState, QAudio::NoError);

    if (m_pullSource)
        disconnect(m_pullSource, &QIODevice::readyRead, this, &QWindowsAudioSink::pullSource);
    m_pullSource = nullptr;
}

qsizetype QWindowsAudioSink::bytesFree() const
{
    return static_cast<qsizetype>(m_client->bytesFree());
}

void QWindowsAudioSink::setBufferSize(qsizetype value)
{
    if (value != m_bufferSize) {
        m_bufferSize = value;
        m_recreateClient = true;
    }
}

qint64 QWindowsAudioSink::processedUSecs() const
{
    if (!m_client)
        return 0;

    return m_format.durationForBytes(m_client->totalInputBytes());
}

void QWindowsAudioSink::resume()
{
    using namespace std::chrono_literals;

    qCDebug(qLcAudioOutput) << "resume()";
    if (deviceState == QAudio::SuspendedState) {
        if (m_pullSource) {
            pullSource();
        } else {
            deviceStateChange(suspendedInState, QAudio::NoError);
            if (m_client->remainingPlayTime() > 0us)
                m_client->start();
        }
    }
}

void QWindowsAudioSink::suspend()
{
    qCDebug(qLcAudioOutput) << "suspend()";
    if (deviceState == QAudio::ActiveState || deviceState == QAudio::IdleState) {
        suspendedInState = deviceState;
        deviceStateChange(QAudio::SuspendedState, QAudio::NoError);
    }
}

void QWindowsAudioSink::setVolume(qreal v)
{
    if (qFuzzyCompare(m_volume, v))
        return;

    m_volume = qBound(qreal(0), v, qreal(1));
}

void QWindowsAudioSink::stop()
{
    close();
}

void QWindowsAudioSink::reset()
{
    close();
}

QT_END_NAMESPACE

#include "qwindowsaudiosink.moc"
