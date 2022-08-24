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
#include "qwindowsaudiodevice_p.h"
#include "qwindowsaudioutils_p.h"
#include <QtEndian>
#include <QtCore/QDataStream>
#include <QtCore/qtimer.h>

#include <private/qaudiohelpers_p.h>

#include <qloggingcategory.h>

#include <system_error>
#include <audioclient.h>
#include <mmdeviceapi.h>


QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcAudioOutput, "qt.multimedia.audiooutput")

const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

class OutputPrivate : public QIODevice
{
    Q_OBJECT
public:
    OutputPrivate(QWindowsAudioSink& audio) : audioDevice(audio) {}
    ~OutputPrivate() override = default;

    qint64 readData(char *, qint64) override { return 0; }
    qint64 writeData(const char *data, qint64 len) override { return audioDevice.push(data, len); }

private:
    QWindowsAudioSink &audioDevice;
};

std::optional<quint32> audioClientFramesInUse(IAudioClient *client)
{
    Q_ASSERT(client);
    UINT32 framesPadding = 0;
    if (SUCCEEDED(client->GetCurrentPadding(&framesPadding)))
        return framesPadding;
    return {};
}

std::optional<quint32> audioClientFramesAllocated(IAudioClient *client)
{
    Q_ASSERT(client);
    UINT32 bufferFrameCount = 0;
    if (SUCCEEDED(client->GetBufferSize(&bufferFrameCount)))
        return bufferFrameCount;
    return {};
}

std::optional<quint32> audioClientFramesAvailable(IAudioClient *client)
{
    auto framesAllocated = audioClientFramesAllocated(client);
    auto framesInUse = audioClientFramesInUse(client);

    if (framesAllocated && framesInUse)
        return *framesAllocated - *framesInUse;
    return {};
}

QWindowsAudioSink::QWindowsAudioSink(QWindowsIUPointer<IMMDevice> device) :
    m_pushSource(new OutputPrivate(*this)),
    m_device(std::move(device))
{
    m_pushSource->open(QIODevice::WriteOnly|QIODevice::Unbuffered);
    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer);
}

QWindowsAudioSink::~QWindowsAudioSink()
{
    close();
}

qint64 QWindowsAudioSink::remainingPlayTimeUs()
{
    auto framesInUse = audioClientFramesInUse(m_audioClient.get());
    return m_resampler.outputFormat().durationForFrames(framesInUse ? *framesInUse : 0);
}

void QWindowsAudioSink::deviceStateChange(QAudio::State state, QAudio::Error error)
{
    if (state != deviceState) {
        if (state == QAudio::ActiveState) {
            m_audioClient->Start();
            qCDebug(qLcAudioOutput) << "Audio client started";

        } else if (deviceState == QAudio::ActiveState) {
            m_timer.stop();
            m_audioClient->Stop();
            qCDebug(qLcAudioOutput) << "Audio client stopped";
        }

        deviceState = state;
        emit stateChanged(deviceState);
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
    if (deviceState == QAudio::StoppedState)
        m_format = fmt;
}

void QWindowsAudioSink::pullSource()
{
    qCDebug(qLcAudioOutput) << "Pull source";
    if (!m_pullSource)
        return;

    auto bytesAvailable = m_pullSource->isOpen() ? qsizetype(m_pullSource->bytesAvailable()) : 0;
    auto readLen = qMin(bytesFree(), bytesAvailable);
    if (readLen > 0) {
        QByteArray samples = m_pullSource->read(readLen);
        if (samples.size() == 0) {
            deviceStateChange(QAudio::IdleState, QAudio::IOError);
            return;
        } else {
            write(samples.data(), samples.size());
        }
    }

    auto playTimeUs = remainingPlayTimeUs();
    if (playTimeUs == 0) {
        deviceStateChange(QAudio::IdleState, m_pullSource->atEnd() ? QAudio::NoError : QAudio::UnderrunError);
    } else {
        deviceStateChange(QAudio::ActiveState, QAudio::NoError);
        m_timer.start(playTimeUs / 2000);
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
    m_timer.disconnect();
    m_timer.callOnTimeout(this, &QWindowsAudioSink::pullSource);
    pullSource();
}

qint64 QWindowsAudioSink::push(const char *data, qint64 len)
{
    if (deviceState == QAudio::StoppedState)
        return -1;

    qint64 written = write(data, len);
    if (written > 0) {
        deviceStateChange(QAudio::ActiveState, QAudio::NoError);
        m_timer.start(remainingPlayTimeUs() /1000);
    }

    return written;
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

    m_timer.disconnect();
    m_timer.callOnTimeout([&](){
        deviceStateChange(QAudio::IdleState, QAudio::UnderrunError);
    });

    return m_pushSource.get();
}

bool QWindowsAudioSink::open()
{
    HRESULT hr = m_device->Activate(IID_IAudioClient, CLSCTX_INPROC_SERVER,
                            nullptr, (void**)m_audioClient.address());
    if (FAILED(hr)) {
        qCWarning(qLcAudioOutput) << "Failed to activate audio device";
        return false;
    }

    WAVEFORMATEX *pwfx = nullptr;
    hr = m_audioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) {
        qCWarning(qLcAudioOutput) << "Format unsupported" << hr;
        return false;
    }

    if (!m_resampler.setup(m_format, QWindowsAudioUtils::waveFormatExToFormat(*pwfx))) {
        qCWarning(qLcAudioOutput) << "Failed to setup resampler";
        CoTaskMemFree(pwfx);
        return false;
    }

    if (m_bufferSize == 0)
        m_bufferSize = m_format.sampleRate() * m_format.bytesPerFrame() / 2;

    REFERENCE_TIME requestedDuration = m_format.durationForBytes(m_bufferSize) * 10;

    hr = m_audioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            0,
            requestedDuration,
            0,
            pwfx,
            nullptr);

    CoTaskMemFree(pwfx);

    if (FAILED(hr)) {
        qCWarning(qLcAudioOutput) << "Failed to initialize audio client" << hr;
        return false;
    }

    auto framesAllocated = audioClientFramesAllocated(m_audioClient.get());
    if (!framesAllocated) {
        qCWarning(qLcAudioOutput) << "Failed to get audio client buffer size";
        return false;
    }

    m_bufferSize = m_format.bytesForDuration(
            m_resampler.outputFormat().durationForFrames(*framesAllocated));

    hr = m_audioClient->GetService(IID_IAudioRenderClient, (void**)m_renderClient.address());
    if (FAILED(hr)) {
        qCWarning(qLcAudioOutput) << "Failed to obtain audio client rendering service" << hr;
        return false;
    }

    return true;
}

void QWindowsAudioSink::close()
{
    qCDebug(qLcAudioOutput) << "close()";
    if (deviceState == QAudio::StoppedState)
        return;

    deviceStateChange(QAudio::StoppedState, QAudio::NoError);

    if (m_pullSource)
        disconnect(m_pullSource, &QIODevice::readyRead, this, &QWindowsAudioSink::pullSource);
    m_audioClient.reset();
    m_renderClient.reset();
    m_pullSource = nullptr;
}

qsizetype QWindowsAudioSink::bytesFree() const
{
    if (!m_audioClient)
        return 0;

    auto framesAvailable = audioClientFramesAvailable(m_audioClient.get());
    if (framesAvailable)
        return m_resampler.inputBufferSize(*framesAvailable * m_resampler.outputFormat().bytesPerFrame());
    return 0;
}

void QWindowsAudioSink::setBufferSize(qsizetype value)
{
    if (!m_audioClient)
        m_bufferSize = value;
}

qint64 QWindowsAudioSink::processedUSecs() const
{
    if (!m_audioClient)
        return 0;

    return m_format.durationForBytes(m_resampler.totalInputBytes());
}

qint64 QWindowsAudioSink::write(const char *data, qint64 len)
{
    Q_ASSERT(m_audioClient);
    Q_ASSERT(m_renderClient);

    qCDebug(qLcAudioOutput) << "write()" << len;

    auto framesAvailable = audioClientFramesAvailable(m_audioClient.get());
    if (!framesAvailable)
        return -1;

    auto maxBytesCanWrite = m_format.bytesForDuration(
            m_resampler.outputFormat().durationForFrames(*framesAvailable));
    qsizetype writeSize = qMin(maxBytesCanWrite, len);

    QByteArray writeBytes = m_resampler.resample({data, writeSize});
    qint32 writeFramesNum = m_resampler.outputFormat().framesForBytes(writeBytes.size());

    quint8 *buffer = nullptr;
    HRESULT hr = m_renderClient->GetBuffer(writeFramesNum, &buffer);
    if (FAILED(hr)) {
        qCWarning(qLcAudioOutput) << "Failed to get buffer"
                                  << std::system_category().message(hr).c_str();
        return -1;
    }

    if (m_volume < qreal(1.0))
        QAudioHelperInternal::qMultiplySamples(m_volume, m_resampler.outputFormat(), writeBytes.data(), buffer, writeBytes.size());
    else
        std::memcpy(buffer, writeBytes.data(), writeBytes.size());

    DWORD flags = writeBytes.isEmpty() ? AUDCLNT_BUFFERFLAGS_SILENT : 0;
    hr = m_renderClient->ReleaseBuffer(writeFramesNum, flags);
    if (FAILED(hr)) {
        qCWarning(qLcAudioOutput) << "Failed to return buffer"
                                  << std::system_category().message(hr).c_str();
        return -1;
    }

    return writeSize;
}

void QWindowsAudioSink::resume()
{
    qCDebug(qLcAudioOutput) << "resume()";
    if (deviceState == QAudio::SuspendedState) {
        if (m_pullSource) {
            pullSource();
        } else {
            // FIXME: Set IdleState to be consistent with implementations on other platforms
            //        even when playing the audio part that was in the buffer when suspended
            deviceStateChange(QAudio::IdleState, QAudio::NoError);
            if (remainingPlayTimeUs() > 0)
                m_audioClient->Start();
        }
    }
}

void QWindowsAudioSink::suspend()
{
    qCDebug(qLcAudioOutput) << "suspend()";
    if (deviceState == QAudio::ActiveState || deviceState == QAudio::IdleState)
        deviceStateChange(QAudio::SuspendedState, QAudio::NoError);
}

void QWindowsAudioSink::setVolume(qreal v)
{
    if (qFuzzyCompare(m_volume, v))
        return;

    m_volume = qBound(qreal(0), v, qreal(1));
}

void QWindowsAudioSink::reset()
{
    if (m_audioClient) {
        m_audioClient->Stop();
        m_audioClient->Reset();
    }
}

QT_END_NAMESPACE

#include "qwindowsaudiosink.moc"
