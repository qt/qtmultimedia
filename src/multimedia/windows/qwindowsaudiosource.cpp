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


#include "qwindowsaudiosource_p.h"
#include "qwindowsmultimediautils_p.h"
#include "qcomtaskresource_p.h"

#include <QtCore/QDataStream>
#include <QtCore/qtimer.h>

#include <private/qaudiohelpers_p.h>

#include <qloggingcategory.h>
#include <qdebug.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcAudioSource, "qt.multimedia.audiosource")

using namespace QWindowsMultimediaUtils;

class OurSink : public QIODevice
{
public:
    OurSink(QWindowsAudioSource& source) : QIODevice(&source), m_audioSource(source) {}

    qint64 bytesAvailable() const override { return m_audioSource.bytesReady(); }
    qint64 readData(char* data, qint64 len) override { return m_audioSource.read(data, len); }
    qint64 writeData(const char*, qint64) override { return 0; }

private:
    QWindowsAudioSource &m_audioSource;
};

QWindowsAudioSource::QWindowsAudioSource(ComPtr<IMMDevice> device, QObject *parent)
    : QPlatformAudioSource(parent),
      m_timer(new QTimer(this)),
      m_device(std::move(device)),
      m_ourSink(new OurSink(*this))
{
    m_ourSink->open(QIODevice::ReadOnly|QIODevice::Unbuffered);
    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setSingleShot(true);
    m_timer->callOnTimeout(this, &QWindowsAudioSource::pullCaptureClient);
}

void QWindowsAudioSource::setVolume(qreal volume)
{
    m_volume = qBound(qreal(0), volume, qreal(1));
}

qreal QWindowsAudioSource::volume() const
{
    return m_volume;
}

QWindowsAudioSource::~QWindowsAudioSource()
{
    stop();
}

QAudio::Error QWindowsAudioSource::error() const
{
    return m_errorState;
}

QAudio::State QWindowsAudioSource::state() const
{
    return m_deviceState;
}

void QWindowsAudioSource::setFormat(const QAudioFormat& fmt)
{
    if (m_deviceState == QAudio::StoppedState) {
        m_format = fmt;
    } else {
        if (m_format != fmt) {
            qWarning() << "Cannot set a new audio format, in the current state ("
                       << m_deviceState << ")";
        }
    }
}

QAudioFormat QWindowsAudioSource::format() const
{
    return m_format;
}

void QWindowsAudioSource::deviceStateChange(QAudio::State state, QAudio::Error error)
{
    if (state != m_deviceState) {
        bool wasActive = m_deviceState == QAudio::ActiveState || m_deviceState == QAudio::IdleState;
        bool isActive = state == QAudio::ActiveState || state == QAudio::IdleState;

        if (isActive && !wasActive) {
            m_audioClient->Start();
            qCDebug(qLcAudioSource) << "Audio client started";

        } else if (wasActive && !isActive) {
            m_timer->stop();
            m_audioClient->Stop();
            qCDebug(qLcAudioSource) << "Audio client stopped";
        }

        m_deviceState = state;
        emit stateChanged(m_deviceState);
    }

    if (error != m_errorState) {
        m_errorState = error;
        emit errorChanged(error);
    }
}

QByteArray QWindowsAudioSource::readCaptureClientBuffer()
{
    UINT32 actualFrames = 0;
    BYTE *data = nullptr;
    DWORD flags = 0;
    HRESULT hr = m_captureClient->GetBuffer(&data, &actualFrames, &flags, nullptr, nullptr);
    if (FAILED(hr)) {
        qWarning() << "IAudioCaptureClient::GetBuffer failed" << errorString(hr);
        deviceStateChange(QAudio::IdleState, QAudio::IOError);
        return {};
    }

    if (actualFrames == 0)
        return {};

    QByteArray out;
    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
        out.resize(m_resampler.outputFormat().bytesForDuration(
                           m_resampler.inputFormat().framesForDuration(actualFrames)),
                   0);
    } else {
        out = m_resampler.resample(
                { data, m_resampler.inputFormat().bytesForFrames(actualFrames) });
        QAudioHelperInternal::qMultiplySamples(m_volume, m_resampler.outputFormat(), out.data(), out.data(), out.size());
    }

    hr = m_captureClient->ReleaseBuffer(actualFrames);
    if (FAILED(hr)) {
        qWarning() << "IAudioCaptureClient::ReleaseBuffer failed" << errorString(hr);
        deviceStateChange(QAudio::IdleState, QAudio::IOError);
        return {};
    }

    return out;
}

void QWindowsAudioSource::schedulePull()
{
    auto allocated = QWindowsAudioUtils::audioClientFramesAllocated(m_audioClient.Get());
    auto inUse = QWindowsAudioUtils::audioClientFramesInUse(m_audioClient.Get());

    if (!allocated || !inUse) {
        deviceStateChange(QAudio::IdleState, QAudio::IOError);
    } else {
        // Schedule the next audio pull immediately if the audio buffer is more
        // than half-full or wait until the audio source fills at least half of it.
        if (*inUse > *allocated / 2) {
            m_timer->start(0);
        } else {
            auto timeToHalfBuffer = m_resampler.inputFormat().durationForFrames(*allocated / 2 - *inUse);
            m_timer->start(timeToHalfBuffer / 1000);
        }
    }
}

void QWindowsAudioSource::pullCaptureClient()
{
    qCDebug(qLcAudioSource) << "Pull captureClient";
    while (true) {
        auto out = readCaptureClientBuffer();
        if (out.isEmpty())
            break;

        if (m_clientSink) {
            qint64 written = m_clientSink->write(out.data(), out.size());
            if (written != out.size())
                qCDebug(qLcAudioSource) << "Did not write all data to the output";

        } else {
            m_clientBufferResidue += out;
            emit m_ourSink->readyRead();
        }
    }

    schedulePull();
}

void QWindowsAudioSource::start(QIODevice* device)
{
    qCDebug(qLcAudioSource) << "start(ioDevice)";
    if (m_deviceState != QAudio::StoppedState)
        close();

    if (device == nullptr)
        return;

    if (!open()) {
        m_errorState = QAudio::OpenError;
        emit errorChanged(QAudio::OpenError);
        return;
    }

    m_clientSink = device;
    schedulePull();
    deviceStateChange(QAudio::ActiveState, QAudio::NoError);
}

QIODevice* QWindowsAudioSource::start()
{
    qCDebug(qLcAudioSource) << "start()";
    if (m_deviceState != QAudio::StoppedState)
        close();

    if (!open()) {
        m_errorState = QAudio::OpenError;
        emit errorChanged(QAudio::OpenError);
        return nullptr;
    }

    schedulePull();
    deviceStateChange(QAudio::IdleState, QAudio::NoError);
    return m_ourSink;
}

void QWindowsAudioSource::stop()
{
    if (m_deviceState == QAudio::StoppedState)
        return;

    close();
}

bool QWindowsAudioSource::open()
{
    HRESULT hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER,
                                    nullptr, (void**)m_audioClient.GetAddressOf());
    if (FAILED(hr)) {
        qCWarning(qLcAudioSource) << "Failed to activate audio device" << errorString(hr);
        return false;
    }

    QComTaskResource<WAVEFORMATEX> pwfx;
    hr = m_audioClient->GetMixFormat(pwfx.address());
    if (FAILED(hr)) {
        qCWarning(qLcAudioSource) << "Format unsupported" << errorString(hr);
        return false;
    }

    if (!m_resampler.setup(QWindowsAudioUtils::waveFormatExToFormat(*pwfx), m_format)) {
        qCWarning(qLcAudioSource) << "Failed to set up resampler";
        return false;
    }

    if (m_bufferSize == 0)
        m_bufferSize = m_format.sampleRate() * m_format.bytesPerFrame() / 5; // 200ms

    REFERENCE_TIME requestedDuration = m_format.durationForBytes(m_bufferSize);

    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, requestedDuration, 0, pwfx.get(),
                                   nullptr);

    if (FAILED(hr)) {
        qCWarning(qLcAudioSource) << "Failed to initialize audio client" << errorString(hr);
        return false;
    }

    auto framesAllocated = QWindowsAudioUtils::audioClientFramesAllocated(m_audioClient.Get());
    if (!framesAllocated) {
        qCWarning(qLcAudioSource) << "Failed to get audio client buffer size";
        return false;
    }

    m_bufferSize = m_format.bytesForDuration(
            m_resampler.inputFormat().durationForFrames(*framesAllocated));

    hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)m_captureClient.GetAddressOf());
    if (FAILED(hr)) {
        qCWarning(qLcAudioSource) << "Failed to obtain audio client rendering service" << errorString(hr);
        return false;
    }

    return true;
}

void QWindowsAudioSource::close()
{
    qCDebug(qLcAudioSource) << "close()";
    if (m_deviceState == QAudio::StoppedState)
        return;

    deviceStateChange(QAudio::StoppedState, QAudio::NoError);

    m_clientBufferResidue.clear();
    m_captureClient.Reset();
    m_audioClient.Reset();
    m_clientSink = nullptr;
}

qsizetype QWindowsAudioSource::bytesReady() const
{
    if (m_deviceState == QAudio::StoppedState || m_deviceState == QAudio::SuspendedState)
        return 0;

    auto frames = QWindowsAudioUtils::audioClientFramesInUse(m_audioClient.Get());
    if (frames) {
        auto clientBufferSize = m_resampler.outputFormat().bytesForDuration(
                m_resampler.inputFormat().durationForFrames(*frames));

        return clientBufferSize + m_clientBufferResidue.size();

    } else {
        return 0;
    }
}

qint64 QWindowsAudioSource::read(char* data, qint64 len)
{
    deviceStateChange(QAudio::ActiveState, QAudio::NoError);
    schedulePull();

    if (data == nullptr || len < 0)
        return -1;

    auto offset = 0;
    if (!m_clientBufferResidue.isEmpty()) {
        auto copyLen = qMin(m_clientBufferResidue.size(), len);
        memcpy(data, m_clientBufferResidue.data(), copyLen);
        len -= copyLen;
        offset += copyLen;
    }

    m_clientBufferResidue = QByteArray{ m_clientBufferResidue.data() + offset,
                                        m_clientBufferResidue.size() - offset };

    if (len > 0) {
        auto out = readCaptureClientBuffer();
        if (!out.isEmpty()) {
            qsizetype copyLen = qMin(out.size(), len);
            memcpy(data + offset, out.data(), copyLen);
            offset += copyLen;

            m_clientBufferResidue = QByteArray{ out.data() + copyLen, out.size() - copyLen };
        }
    }

    return offset;
}

void QWindowsAudioSource::resume()
{
    qCDebug(qLcAudioSource) << "resume()";
    if (m_deviceState == QAudio::SuspendedState) {
        deviceStateChange(QAudio::ActiveState, QAudio::NoError);
        pullCaptureClient();
    }
}

void QWindowsAudioSource::setBufferSize(qsizetype value)
{
    m_bufferSize = value;
}

qsizetype QWindowsAudioSource::bufferSize() const
{
    return m_bufferSize;
}

qint64 QWindowsAudioSource::processedUSecs() const
{
    if (m_deviceState == QAudio::StoppedState)
        return 0;

    return m_resampler.outputFormat().durationForBytes(m_resampler.totalOutputBytes());
}

void QWindowsAudioSource::suspend()
{
    qCDebug(qLcAudioSource) << "suspend";
    if (m_deviceState == QAudio::ActiveState || m_deviceState == QAudio::IdleState)
        deviceStateChange(QAudio::SuspendedState, QAudio::NoError);
}

void QWindowsAudioSource::reset()
{
    stop();
}

QT_END_NAMESPACE
