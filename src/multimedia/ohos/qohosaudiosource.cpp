// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosaudiosource_p.h"

#include <QtMultimedia/private/qaudiohelpers_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qpermissions.h>

QT_BEGIN_NAMESPACE

namespace QtOHAudio {

Q_STATIC_LOGGING_CATEGORY(qLcOhosAudioSource, "qt.multimedia.ohos.audiosource")

QOhosAudioSourceStream::QOhosAudioSourceStream(QAudioDevice device, const QAudioFormat &format,
                                               std::optional<int> ringbufferSize,
                                               QOhosAudioSource *parent, float volume,
                                               std::optional<QtMultimediaPrivate::NativePeriodFrames> hardwareBufferFrames)
    : QtMultimediaPrivate::QPlatformAudioSourceStream(std::move(device), format, ringbufferSize,
                                                      hardwareBufferFrames, volume),
      m_parent(parent)
{
    QAudioFormat builderFormat = format;
    const QAudioFormat::SampleFormat preferredFormat =
            QtOHAudio::preferredCompatibleSampleFormat(format.sampleFormat());
    if (preferredFormat != format.sampleFormat()) {
        m_hostFormat = format;
        builderFormat.setSampleFormat(preferredFormat);
    }

    QtOHAudio::StreamBuilder builder(builderFormat, AUDIOSTREAM_TYPE_CAPTURER);

    qCDebug(qLcOhosAudioSource) << "Creating source for device id:" << m_audioDevice.id()
                                << "description:" << m_audioDevice.description();

    builder.params.inputSourceType = AUDIOSTREAM_SOURCE_TYPE_MIC;
    builder.userData = this;
    builder.readCallback = [](OH_AudioCapturer *, void *userData, void *audioData,
                              int32_t audioDataSize) -> int32_t {
        auto *stream = reinterpret_cast<QOhosAudioSourceStream *>(userData);
        Q_ASSERT(stream);
        auto audioSpan = stream->getHostSpan(audioData, audioDataSize);
        return stream->m_audioCallback ? stream->processCallback(audioSpan)
                                       : stream->processRingbuffer(audioSpan, audioDataSize);
    };

    builder.setupBuilder();
    m_stream = std::make_unique<QtOHAudio::Stream>(builder);
}

bool QOhosAudioSourceStream::open()
{
    QMicrophonePermission permission;
    const bool permitted = qApp->checkPermission(permission) == Qt::PermissionStatus::Granted;
    if (!permitted) {
        qCWarning(qLcOhosAudioSource) << "Missing microphone permission";
        requestStop();
        return false;
    }

    if (!m_stream || !m_stream->isOpen()) {
        qCWarning(qLcOhosAudioSource) << "Stream creation failed";
        requestStop();
        return false;
    }
    return true;
}

bool QOhosAudioSourceStream::start(QIODevice *device)
{
    Q_ASSERT(thread()->isCurrentThread());
    setQIODevice(device);
    createQIODeviceConnections(device);

    if (!m_stream->start()) {
        requestStop();
        return false;
    }
    return true;
}

QIODevice *QOhosAudioSourceStream::start()
{
    auto *device = createRingbufferReaderDevice();
    return start(device) ? device : nullptr;
}

bool QOhosAudioSourceStream::start(AudioCallback &&callback)
{
    Q_ASSERT(thread()->isCurrentThread());
    m_audioCallback = std::move(callback);

    if (!m_stream->start()) {
        requestStop();
        return false;
    }
    return true;
}

void QOhosAudioSourceStream::suspend()
{
    Q_ASSERT(thread()->isCurrentThread());
    m_stream->pause();
}

void QOhosAudioSourceStream::resume()
{
    Q_ASSERT(thread()->isCurrentThread());
    m_stream->start();
}

void QOhosAudioSourceStream::stop(ShutdownPolicy policy)
{
    Q_ASSERT(thread()->isCurrentThread());
    requestStop();

    m_stream->stop();

    disconnectQIODeviceConnections();
    finalizeQIODevice(policy);

    if (policy == ShutdownPolicy::DiscardRingbuffer)
        emptyRingbuffer();
}

void QOhosAudioSourceStream::updateStreamIdle(bool idle)
{
    if (m_parent)
        m_parent->updateStreamIdle(idle);
}

QSpan<const std::byte>
QOhosAudioSourceStream::getHostSpan(void *audioData,
                                    int32_t numBytes) const noexcept QT_MM_NONBLOCKING
{
    return QSpan<const std::byte>{ reinterpret_cast<const std::byte *>(audioData),
                                   static_cast<qsizetype>(numBytes) };
}

int32_t
QOhosAudioSourceStream::processRingbuffer(QSpan<const std::byte> audioSpan,
                                          int32_t numBytes) noexcept QT_MM_NONBLOCKING
{
    const QAudioFormat &format = m_hostFormat.value_or(m_format);
    const int32_t numFrames = numBytes / format.bytesPerFrame();

    auto framesWritten = m_hostFormat
            ? QPlatformAudioSourceStream::process(
                      audioSpan, numFrames,
                      QAudioHelperInternal::toNativeSampleFormat(m_hostFormat->sampleFormat()))
            : QPlatformAudioSourceStream::process(audioSpan, numFrames);

    Q_UNUSED(framesWritten)
    return 0;
}

int32_t
QOhosAudioSourceStream::processCallback(QSpan<const std::byte> audioSpan) noexcept QT_MM_NONBLOCKING
{
    if (isStopRequested())
        return 0;

    if (m_hostFormat)
        QtMultimediaPrivate::runAudioCallback(*m_audioCallback, audioSpan, m_format, volume(),
                                              *m_hostFormat);
    else
        QtMultimediaPrivate::runAudioCallback(*m_audioCallback, audioSpan, m_format, volume());

    return 0;
}

QOhosAudioSource::QOhosAudioSource(QAudioDevice device, const QAudioFormat &format, QObject *parent)
    : BaseClass(std::move(device), format, parent)
{
}

QOhosAudioSource::~QOhosAudioSource() = default;

} // namespace QtOHAudio

QT_END_NAMESPACE
