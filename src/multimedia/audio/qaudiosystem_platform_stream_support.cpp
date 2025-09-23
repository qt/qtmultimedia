// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiosystem_platform_stream_support_p.h"

#include <QtCore/qdebug.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>
#include <QtMultimedia/private/qaudio_qiodevice_support_p.h>
#include <QtMultimedia/private/qmultimedia_assume_p.h>

#include <stdlib.h>
#if __has_include(<alloca.h>)
#  include <alloca.h>
#endif
#if __has_include(<malloc.h>)
#  include <malloc.h>
#endif

#ifdef Q_CC_MSVC
#  define alloca _alloca
#endif

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

using namespace std::chrono_literals;

QPlatformAudioIOStream::QPlatformAudioIOStream(QAudioDevice m_audioDevice, QAudioFormat m_format,
                                               std::optional<int> ringbufferSize,
                                               std::optional<int32_t> hardwareBufferFrames,
                                               float volume)
    : m_audioDevice{
          std::move(m_audioDevice),
      },
      m_format{
          m_format,
      },
      m_hardwareBufferFrames{
          hardwareBufferFrames,
      },
      m_volume{
          volume,
      }
{
    prepareRingbuffer(ringbufferSize);
}

QPlatformAudioIOStream::~QPlatformAudioIOStream()
{
    Q_ASSERT(m_stopRequested);
}

void QPlatformAudioIOStream::setVolume(float volume)
{
    m_volume.store(volume, std::memory_order_relaxed);
}

void QPlatformAudioIOStream::prepareRingbuffer(std::optional<int> ringbufferSize)
{
    using SampleFormat = QAudioFormat::SampleFormat;

    // Warning: QAudioSink::setBufferSize is measured in *bytes* not in *frames*
    int ringbufferElements = inferRingbufferFrames(ringbufferSize, m_hardwareBufferFrames, m_format)
            * m_format.channelCount();

    switch (m_format.sampleFormat()) {
    case SampleFormat::Float:
        m_ringbuffer.emplace<QAudioRingBuffer<float>>(ringbufferElements);
        break;
    case SampleFormat::Int16:
        m_ringbuffer.emplace<QAudioRingBuffer<int16_t>>(ringbufferElements);
        break;
    case SampleFormat::Int32:
        m_ringbuffer.emplace<QAudioRingBuffer<int32_t>>(ringbufferElements);
        break;
    case SampleFormat::UInt8:
        m_ringbuffer.emplace<QAudioRingBuffer<uint8_t>>(ringbufferElements);
        break;

    default:
        qCritical() << "invalid sample format";
        Q_UNREACHABLE_RETURN();
    }
}

void QPlatformAudioIOStream::requestStop()
{
    m_stopRequested.store(true, std::memory_order_release);
}

qsizetype
QPlatformAudioIOStream::inferRingbufferFrames(const std::optional<int> &ringbufferSize,
                                              const std::optional<int32_t> &hardwareBufferFrames,
                                              const QAudioFormat &format)
{
    int bytesPerFrame = format.bytesPerFrame();
    QT_MM_ASSUME(bytesPerFrame > 0);

    return inferRingbufferBytes(ringbufferSize, hardwareBufferFrames, format) / bytesPerFrame;
}

qsizetype
QPlatformAudioIOStream::inferRingbufferBytes(const std::optional<int> &ringbufferSize,
                                             const std::optional<int32_t> &hardwareBufferFrames,
                                             const QAudioFormat &format)
{
    // ensure to a sane minimum ringbuffer size of twice the hw buffer size or 32 frames
    const int minimumRingbufferFrames = hardwareBufferFrames ? *hardwareBufferFrames * 2 : 32;
    const int minimumRingbufferBytes = format.bytesForFrames(minimumRingbufferFrames);
    if (ringbufferSize)
        return ringbufferSize >= minimumRingbufferBytes ? *ringbufferSize : minimumRingbufferBytes;

    using namespace std::chrono;
    static constexpr auto defaultBufferDuration = 250ms;

    return format.bytesForDuration(microseconds(defaultBufferDuration).count());
}

int QPlatformAudioIOStream::ringbufferSizeInBytes()
{
    return visitRingbuffer([](auto &ringbuffer) {
        using SampleType = typename std::decay_t<decltype(ringbuffer)>::ValueType;
        return ringbuffer.size() * sizeof(SampleType);
    });
}

////////////////////////////////////////////////////////////////////////////////////////////////////

QPlatformAudioSinkStream::QPlatformAudioSinkStream(QAudioDevice audioDevice,
                                                   const QAudioFormat &format,
                                                   std::optional<int> ringbufferSize,
                                                   std::optional<int32_t> hardwareBufferFrames,
                                                   float volume)
    : QPlatformAudioIOStream{
          std::move(audioDevice), format, ringbufferSize, hardwareBufferFrames, volume,
      }
{
    m_streamIdleDetectionConnection = m_streamIdleDetectionNotifier.callOnActivated([this] {
        if (isStopRequested())
            return;

        bool sinkIsIdle = m_streamIsIdle.load();

        if (sinkIsIdle) {
            // data has been pushed to the ringbuffer, while the stream is
            // still idle, this will change during the next audio callback
            bool ringbufferIsEmpty = visitRingbuffer([&](auto &ringbuffer) {
                return ringbuffer.free() == ringbuffer.size();
            });

            sinkIsIdle = ringbufferIsEmpty;
        }

        updateStreamIdle(sinkIsIdle);
    });
}

QPlatformAudioSinkStream::~QPlatformAudioSinkStream() = default;

uint64_t
QPlatformAudioSinkStream::process(QSpan<std::byte> hostBuffer, qsizetype totalNumberOfFrames,
                                  std::optional<NativeSampleFormat> nativeFormat) noexcept QT_MM_NONBLOCKING
{
    qsizetype totalNumberOfSamples = totalNumberOfFrames * m_format.channelCount();

    const float vol = volume();

    int samplesConsumedFromRingbuffer = visitRingbuffer([&](auto &ringbuffer) {
        return ringbuffer.consume(totalNumberOfSamples, [&](auto ringbufferRange) {
            if (nativeFormat) {
                // Amount of bytes in output range differ from ringbuffer range
                const qsizetype samplesInChunk = ringbufferRange.size();
                const qsizetype bytesInChunk = samplesInChunk * bytesPerSample(*nativeFormat);

                QSpan<std::byte> outputByteRange = take(hostBuffer, bytesInChunk);
                hostBuffer = drop(hostBuffer, bytesInChunk);
                convertToNative(as_bytes(ringbufferRange), outputByteRange, vol, *nativeFormat);
            } else {
                QSpan<std::byte> outputByteRange = take(hostBuffer, ringbufferRange.size_bytes());
                hostBuffer = drop(hostBuffer, ringbufferRange.size_bytes());
                QAudioHelperInternal::applyVolume(vol, m_format, as_bytes(ringbufferRange),
                                                  outputByteRange);
            }
        });
    });

    if (m_ringbufferWriterDevice) {
        qint64 bytes = samplesConsumedFromRingbuffer * m_format.bytesPerSample();
        m_ringbufferWriterDevice->bytesConsumedFromRingbuffer(bytes);
    }

    if (!isStopRequested()) {
        if (notificationThresholdBytes == 0 || bytesFree() > notificationThresholdBytes)
            m_ringbufferHasSpace.set();

        bool streamIsIdle = m_streamIsIdle.load(std::memory_order_relaxed);
        if (streamIsIdle && samplesConsumedFromRingbuffer) {
            m_streamIsIdle.store(false);
            m_streamIdleDetectionNotifier.set();
        } else if (!streamIsIdle && !samplesConsumedFromRingbuffer) {
            m_streamIsIdle.store(true);
            m_streamIdleDetectionNotifier.set();
        }
    }
    if (!hostBuffer.empty())
        std::fill_n(hostBuffer.data(), hostBuffer.size(), std::byte{});

    uint64_t consumedFrames = samplesConsumedFromRingbuffer / m_format.channelCount();
    m_processedFrameCount += consumedFrames;
    m_totalFrameCount += totalNumberOfFrames;

    return consumedFrames;
}

quint64 QPlatformAudioSinkStream::bytesFree() const
{
    return visitRingbuffer([](auto &ringbuffer) {
        using SampleType = typename std::decay_t<decltype(ringbuffer)>::ValueType;
        return ringbuffer.free() * sizeof(SampleType);
    });
}

std::chrono::microseconds QPlatformAudioSinkStream::processedDuration() const
{
    return std::chrono::microseconds{
        m_processedFrameCount * 1'000'000 / m_format.sampleRate(),
    };
}

void QPlatformAudioSinkStream::pullFromQIODevice()
{
    withPullIODeviceReentrancyGuard([this] {
        pullFromQIODeviceImpl();
    });
}

void QPlatformAudioSinkStream::pullFromQIODeviceImpl()
{
    Q_ASSERT(thread()->isCurrentThread());
    Q_ASSERT(m_device);
    Q_ASSERT(m_pullIODeviceReentrancyGuard);

    visitRingbuffer([&](auto &ringbuffer) {
        int elementsPulled = pullFromQIODeviceToRingbuffer(*m_device, ringbuffer);
        if (elementsPulled)
            updateStreamIdle(false);
    });
}

void QPlatformAudioSinkStream::createQIODeviceConnections(QIODevice *device)
{
    // consumed from audio thread
    m_ringbufferHasSpaceConnection = m_ringbufferHasSpace.callOnActivated(device, [this] {
        pullFromQIODevice();
    });

    // data has been pushed to device
    m_iodeviceHasNewDataConnection =
            QObject::connect(device, &QIODevice::readyRead, device, [this] {
        withPullIODeviceReentrancyGuard([this] {
            pullFromQIODeviceImpl();
            updateStreamIdle(false);
        });
    });
}

void QPlatformAudioSinkStream::disconnectQIODeviceConnections()
{
    QObject::disconnect(m_ringbufferHasSpaceConnection);
    QObject::disconnect(m_iodeviceHasNewDataConnection);
}

QIODevice *QPlatformAudioSinkStream::createRingbufferWriterDevice()
{
    m_ringbufferWriterDevice = visitRingbuffer(
            [&](auto &ringbuffer) -> std::unique_ptr<QtPrivate::QIODeviceRingBufferWriterBase> {
        using SampleType = typename std::decay_t<decltype(ringbuffer)>::ValueType;
        return std::make_unique<QtPrivate::QIODeviceRingBufferWriter<SampleType>>(&ringbuffer);
    });

    return m_ringbufferWriterDevice.get();
}

void QPlatformAudioSinkStream::setQIODevice(QIODevice *device)
{
    m_device = device;
}

void QPlatformAudioSinkStream::setIdleState(bool x)
{
    m_streamIsIdle.store(x);
}

void QPlatformAudioSinkStream::stopIdleDetection()
{
    QObject::disconnect(m_streamIdleDetectionConnection);
}

QThread *QPlatformAudioSinkStream::thread() const
{
    // QPlatformAudioSinkStream is not a QObject, but still has a notion of an application thread
    // where it lives on.
    return m_streamIdleDetectionNotifier.thread();
}

// we limit alloca calls to 0.5MB. it's good enough for virtually all use cases (i.e. buffers
// of 4092 frames / 32 channels) and well in the reasonable range of available stack memory on linux
// (8MB)
static constexpr qsizetype scratchpadBufferSizeLimit = 512 * 1024;
static_assert(scratchpadBufferSizeLimit > 4092 * 32 * sizeof(float));

void QPlatformAudioSinkStream::convertToNative(QSpan<const std::byte> internal,
                                               QSpan<std::byte> native, float volume,
                                               NativeSampleFormat nativeFormat) noexcept QT_MM_NONBLOCKING
{
    using namespace QAudioHelperInternal;

    if (volume == 1.f) {
        convertSampleFormat(internal, toNativeSampleFormat(m_format.sampleFormat()), native,
                            nativeFormat);
        return;
    }

    Q_ASSERT(internal.size() <= scratchpadBufferSizeLimit);
    std::byte *scratchpadMemory = reinterpret_cast<std::byte *>(alloca(internal.size()));
    QSpan scratchpadBuffer{ scratchpadMemory, internal.size() };

    applyVolume(volume, m_format, internal, scratchpadBuffer);
    convertSampleFormat(scratchpadBuffer, toNativeSampleFormat(m_format.sampleFormat()), native,
                        nativeFormat);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

QPlatformAudioSourceStream::QPlatformAudioSourceStream(QAudioDevice audioDevice,
                                                       const QAudioFormat &format,
                                                       std::optional<int> ringbufferSize,
                                                       std::optional<int32_t> hardwareBufferFrames,
                                                       float volume)
    : QPlatformAudioIOStream{
          std::move(audioDevice), format, ringbufferSize, hardwareBufferFrames, volume,
      }
{
}

QPlatformAudioSourceStream::~QPlatformAudioSourceStream() = default;

uint64_t QPlatformAudioSourceStream::process(
        QSpan<const std::byte> hostBuffer, qsizetype numberOfFrames,
        std::optional<NativeSampleFormat> nativeFormat) noexcept QT_MM_NONBLOCKING
{
    qsizetype remainingNumberOfSamples = numberOfFrames * m_format.channelCount();

    const float vol = volume();
    using namespace QtMultimediaPrivate;

    uint64_t totalSamplesWritten = visitRingbuffer([&](auto &rb) {
        using SampleType = typename std::decay_t<decltype(rb)>::ValueType;

        // clang-format off
        return rb.produceSome([&](QSpan<SampleType> ringbufferRange) {
            if (nativeFormat) {
                // Amount of bytes in input range differ from ringbuffer range
                const qsizetype samplesInChunk = ringbufferRange.size();
                const qsizetype bytesInChunk = samplesInChunk * bytesPerSample(*nativeFormat);

                QSpan<const std::byte> inputByteRange = take(hostBuffer, bytesInChunk);
                hostBuffer = drop(hostBuffer, bytesInChunk);
                convertFromNative(inputByteRange, as_writable_bytes(ringbufferRange), vol,
                                  *nativeFormat);
            } else {
                QSpan<const std::byte> inputByteRange =
                        take(hostBuffer, ringbufferRange.size_bytes());
                hostBuffer = drop(hostBuffer, ringbufferRange.size_bytes());
                QAudioHelperInternal::applyVolume(vol, m_format, inputByteRange,
                                                  as_writable_bytes(ringbufferRange));
            }
            return ringbufferRange;
        }, remainingNumberOfSamples);
        // clang-format on
    });

    if (totalSamplesWritten)
        m_ringbufferHasData.set();

    uint64_t framesWritten = totalSamplesWritten / m_format.channelCount();
    m_totalNumberOfFramesPushedToRingbuffer += framesWritten;
    return framesWritten;
}

void QPlatformAudioSourceStream::pushToIODevice()
{
    Q_ASSERT(thread()->isCurrentThread());

    qsizetype bytesPushed = visitRingbuffer([&](auto &ringbuffer) {
        return QtPrivate::pushToQIODeviceFromRingbuffer(*m_device, ringbuffer);
    });

    if (bytesPushed)
        Q_EMIT m_device->readyRead();
}

bool QPlatformAudioSourceStream::deviceIsRingbufferReader() const
{
    return m_device == m_ringbufferReaderDevice.get();
}

void QPlatformAudioSourceStream::finalizeQIODevice(ShutdownPolicy shutdownPolicy)
{
    switch (shutdownPolicy) {
    case ShutdownPolicy::DiscardRingbuffer:
        return;
    case ShutdownPolicy::DrainRingbuffer:
        if (!deviceIsRingbufferReader())
            pushToIODevice();
        return;

    default:
        Q_UNREACHABLE_RETURN();
    }
}

void QPlatformAudioSourceStream::emptyRingbuffer()
{
    visitRingbuffer([](auto &ringbuffer) {
        ringbuffer.consumeAll([](auto &) {
        });
    });
}

QThread *QPlatformAudioSourceStream::thread() const
{
    // QPlatformAudioSourceStream is not a QObject, but still has a notion of an application thread
    // where it lives on.
    return m_ringbufferHasData.thread();
}

qsizetype QPlatformAudioSourceStream::bytesReady() const
{
    return visitRingbuffer([](const auto &ringbuffer) {
        return ringbuffer.used() * sizeof(typename std::decay_t<decltype(ringbuffer)>::ValueType);
    });
}

std::chrono::microseconds QPlatformAudioSourceStream::processedDuration() const
{
    return std::chrono::microseconds{
        m_format.durationForFrames(
                m_totalNumberOfFramesPushedToRingbuffer.load(std::memory_order_relaxed)),
    };
}

void QPlatformAudioSourceStream::setQIODevice(QIODevice *device)
{
    m_device = device;
}

void QPlatformAudioSourceStream::createQIODeviceConnections(QIODevice *device)
{
    bool pushToDevice = !deviceIsRingbufferReader();

    if (pushToDevice) {
        m_ringbufferHasDataConnection = m_ringbufferHasData.callOnActivated(device, [this] {
            if (!isStopRequested())
                updateStreamIdle(false);
            pushToIODevice();
        });
    } else {
        m_ringbufferHasDataConnection = m_ringbufferHasData.callOnActivated(device, [this] {
            if (!isStopRequested())
                updateStreamIdle(false);
            Q_EMIT m_device->readyRead();
        });
    }

    m_ringbufferIsFullConnection = m_ringbufferHasData.callOnActivated(device, [this] {
        if (!isStopRequested())
            updateStreamIdle(false);
    });
}

void QPlatformAudioSourceStream::disconnectQIODeviceConnections()
{
    QObject::disconnect(m_ringbufferHasDataConnection);
    QObject::disconnect(m_ringbufferIsFullConnection);
}

QIODevice *QPlatformAudioSourceStream::createRingbufferReaderDevice()
{
    using namespace QtPrivate;

    m_ringbufferReaderDevice = visitRingbuffer([&](auto &rb) -> std::unique_ptr<QIODevice> {
        using SampleType = typename std::decay_t<decltype(rb)>::ValueType;
        return std::make_unique<QIODeviceRingBufferReader<SampleType>>(&rb);
    });

    m_ringbufferReaderDevice->open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    return m_ringbufferReaderDevice.get();
}

void QPlatformAudioSourceStream::convertFromNative(
        QSpan<const std::byte> native, QSpan<std::byte> internal, float volume,
        NativeSampleFormat nativeFormat) noexcept QT_MM_NONBLOCKING
{
    using namespace QAudioHelperInternal;
    if (volume == 1.f) {
        convertSampleFormat(native, nativeFormat, internal,
                            QAudioHelperInternal::toNativeSampleFormat(m_format.sampleFormat()));
        return;
    }

    Q_ASSERT(internal.size() <= scratchpadBufferSizeLimit);
    std::byte *scratchpadMemory = reinterpret_cast<std::byte *>(alloca(internal.size()));
    QSpan scratchpadBuffer{ scratchpadMemory, internal.size() };

    convertSampleFormat(native, nativeFormat, scratchpadBuffer,
                        QAudioHelperInternal::toNativeSampleFormat(m_format.sampleFormat()));

    applyVolume(volume, m_format, scratchpadBuffer, internal);
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#ifdef Q_CC_MSVC
#  undef alloca
#endif
