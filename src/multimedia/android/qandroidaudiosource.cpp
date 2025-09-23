// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidaudiosource_p.h"

#include "qandroidaudioutil_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qpermissions.h>

QT_BEGIN_NAMESPACE

namespace QtAAudio {

Q_STATIC_LOGGING_CATEGORY(qLcAndroidAudioSource, "qt.multimedia.android.audiosource")

QAndroidAudioSourceStream::QAndroidAudioSourceStream(QAudioDevice device,
                                                     const QAudioFormat &format,
                                                     std::optional<int> ringbufferSize,
                                                     QAndroidAudioSource *parent, float volume,
                                                     std::optional<int32_t> hardwareBufferFrames)
    : QtMultimediaPrivate::QPlatformAudioSourceStream(std::move(device), format, ringbufferSize,
                                                      hardwareBufferFrames, volume),
      m_parent(parent)
{
    QtAAudio::StreamBuilder builder(format);

    qCDebug(qLcAndroidAudioSource) << "Creating source for device id:" << m_audioDevice.id()
                                   << ", description:" << m_audioDevice.description();

    // NOTE: Don't set device when creating a stream for the default bluetooth device
    if (!QAndroidAudioUtil::isDefaultBluetoothDevice(m_audioDevice))
        builder.deviceId = m_audioDevice.id().toInt();

    // Set buffer parameters
    builder.bufferCapacity = m_hardwareBufferFrames ? *m_hardwareBufferFrames : 1024;

    // NOTE: AAudio doesn't support UINT8, so convert to INT16 if that's requested
    if (format.sampleFormat() == QAudioFormat::UInt8)
        m_nativeSampleFormat = NativeSampleFormat::int16_t;

    // Set builder parameters for audio source
    builder.params.sharingMode = AAUDIO_SHARING_MODE_SHARED;
    builder.params.direction = AAUDIO_DIRECTION_INPUT;

    // TODO: Set input preset based on device

    builder.userData = this;
    builder.callback = [](AAudioStream *, void *userData, void *audioData,
                          int32_t numFrames) -> int {
        auto *stream = reinterpret_cast<QAndroidAudioSourceStream *>(userData);
        Q_ASSERT(stream);
        return stream->process(audioData, numFrames);
    };
    builder.errorCallback = [](AAudioStream *, void *userData, aaudio_result_t error) -> void {
        auto *stream = reinterpret_cast<QAndroidAudioSourceStream *>(userData);
        Q_ASSERT(stream);
        stream->handleError(error);
    };

    builder.setupBuilder();
    m_stream = std::make_unique<QtAAudio::Stream>(builder);
}

bool QAndroidAudioSourceStream::open()
{
    QMicrophonePermission permission;

    const bool permitted = qApp->checkPermission(permission) == Qt::PermissionStatus::Granted;
    if (!permitted) {
        qWarning("Missing microphone permission!");
        requestStop();
        return false;
    }

    if (!m_stream->isOpen()) {
        qCWarning(qLcAndroidAudioSource) << "Stream null";
        requestStop();
        return false;
    }

    if (!m_stream->areStreamParametersRespected())
        qCWarning(qLcAndroidAudioSource) << "Stream parameters not correct";

    return true;
}

bool QAndroidAudioSourceStream::start(QIODevice *device)
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

QIODevice *QAndroidAudioSourceStream::start()
{
    auto *device = createRingbufferReaderDevice();
    return start(device) ? device : nullptr;
}

void QAndroidAudioSourceStream::suspend()
{
    Q_ASSERT(thread()->isCurrentThread());
    m_stream->stop();
}

void QAndroidAudioSourceStream::resume()
{
    Q_ASSERT(thread()->isCurrentThread());
    m_stream->start();
}

void QAndroidAudioSourceStream::stop(ShutdownPolicy policy)
{
    Q_ASSERT(thread()->isCurrentThread());
    requestStop();

    m_stream->stop();

    disconnectQIODeviceConnections();
    finalizeQIODevice(policy);

    if (policy == ShutdownPolicy::DiscardRingbuffer)
        emptyRingbuffer();
}

void QAndroidAudioSourceStream::updateStreamIdle(bool idle)
{
    if (m_parent)
        m_parent->updateStreamIdle(idle);
}

aaudio_data_callback_result_t
QAndroidAudioSourceStream::process(void *audioData, int numFrames) noexcept QT_MM_NONBLOCKING
{
    qsizetype bytesForFrames = m_nativeSampleFormat
            ? (QAudioHelperInternal::bytesPerSample(*m_nativeSampleFormat) * m_format.channelCount()
               * numFrames)
            : m_format.bytesForFrames(numFrames);
    QSpan<std::byte> audioSpan{ reinterpret_cast<std::byte *>(audioData), bytesForFrames };

    auto framesWritten =
            QPlatformAudioSourceStream::process(audioSpan, numFrames, m_nativeSampleFormat);

    if (framesWritten != static_cast<uint64_t>(numFrames) && isStopRequested())
        return AAUDIO_CALLBACK_RESULT_STOP;

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void QAndroidAudioSourceStream::handleError(aaudio_result_t)
{
    // Handle as IO error which closes the stream
    requestStop();
    invokeOnAppThread([this] {
        // clang-format off
        handleIOError(m_parent);
        // clang-format on
    });
}

QAndroidAudioSource::QAndroidAudioSource(QAudioDevice device, const QAudioFormat &format,
                                         QObject *parent)
    : BaseClass(std::move(device), format, parent)
{
}

} // namespace QtAAudio

QT_END_NAMESPACE
