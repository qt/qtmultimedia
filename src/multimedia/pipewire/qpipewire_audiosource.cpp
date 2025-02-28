// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiosource_p.h"

#include "qpipewire_audiocontextmanager_p.h"
#include "qpipewire_audiodevice_p.h"
#include "qpipewire_audiostream_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qpointer.h>
#include <QtCore/qloggingcategory.h>
#include <QtMultimedia/private/qaudioringbuffer_p.h>
#include <QtMultimedia/private/qautoresetevent_p.h>
#include <QtMultimedia/private/qaudio_qiodevice_support_p.h>
#include <QtMultimedia/private/qaudio_rtsan_support_p.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using QtPrivate::QAudioRingBuffer;
using QtPrivate::QAutoResetEvent;
using QtPrivate::QIODeviceRingBufferReader;

// LATER:
// ideally the ringbuffer should fill a buffer that can grow via a worker thread on which we can
// allocate.
struct QPipewireAudioSourceStream final : std::enable_shared_from_this<QPipewireAudioSourceStream>,
                                          QPipewireAudioStream
{
    using SampleFormat = QAudioFormat::SampleFormat;

    QPipewireAudioSourceStream(QPipewireAudioSource *parent, const QAudioFormat &format,
                               std::optional<qsizetype> ringbufferSize,
                               std::optional<qsizetype> hardwareBufferSize = std::nullopt);
    ~QPipewireAudioSourceStream();

    qsizetype bytesReady() const;
    bool start(QIODevice *device, ObjectSerial sourceNodeSerial);
    QIODevice *start(ObjectSerial sourceNodeSerial);
    void stop();

    void setVolume(qreal);

    std::chrono::microseconds processedDuration();

private:
    void process() QT_MM_NONBLOCKING override;
    void handleDeviceRemoved() override;

    void stateChanged(pw_stream_state old, pw_stream_state state, const char *error) override;
    void prepareFormat(const QAudioFormat &format, std::optional<qsizetype> ringbufferSize);
    void disconnectStream();

    void pushToIODevice();

    std::atomic<qreal> m_volume{ 1.f };

    using Ringbuffer = std::variant<QAudioRingBuffer<float>, QAudioRingBuffer<int32_t>,
                                    QAudioRingBuffer<int16_t>, QAudioRingBuffer<uint8_t>>;

    Ringbuffer m_ringbuffer{
        std::in_place_type_t<QAudioRingBuffer<float>>{},
        0,
    };

    template <typename Functor>
    auto visitRingbuffer(Functor &&f)
    {
        return std::visit(f, m_ringbuffer);
    }

    template <typename Functor>
    auto visitRingbuffer(Functor &&f) const
    {
        return std::visit(f, m_ringbuffer);
    }

    QAutoResetEvent m_ringbufferHasData;
    QAutoResetEvent m_ringbufferIsFull;

    std::atomic_bool m_stopRequested{ false };
    std::shared_ptr<QPipewireAudioSourceStream> m_self;
    std::unique_ptr<QIODevice> m_ringbufferReaderDevice;

    std::atomic<uint64_t> m_totalNumberOfFramesPushedToRingbuffer;

    // xrun detection
    void xrunOccurred(int /*xrunCount*/) override { m_xrunOccurred.set(); }
    QtPrivate::QAutoResetEvent m_xrunOccurred;
    QMetaObject::Connection m_xrunNotification;

    QPointer<QIODevice> m_device;

    QPipewireAudioSource *m_parent;
};

QPipewireAudioSourceStream::QPipewireAudioSourceStream(QPipewireAudioSource *parent,
                                                       const QAudioFormat &format,
                                                       std::optional<qsizetype> ringbufferSize,
                                                       std::optional<qsizetype> hardwareBufferSize):
    QPipewireAudioStream {
        format,
    }, m_parent {
        parent,
    }
{
    prepareFormat(format, ringbufferSize);

    auto extraProperties = std::array{
        spa_dict_item{ PW_KEY_MEDIA_CATEGORY, "Capture" },
        spa_dict_item{ PW_KEY_MEDIA_ROLE, "Music" },
    };
    createStream(extraProperties, hardwareBufferSize, "QPipewireAudioSource");

    m_xrunNotification = QObject::connect(&m_xrunOccurred, &QAutoResetEvent::activated,
                                          &m_xrunOccurred, [this, parent] {
                                              if (m_stopRequested)
                                                  return;
                                              parent->reportXRuns(m_xrunCount.exchange(0));
                                          });
}

QPipewireAudioSourceStream::~QPipewireAudioSourceStream()
{
}

qsizetype QPipewireAudioSourceStream::bytesReady() const
{
    return visitRingbuffer([&](auto &ringbuffer) {
        using SampleType = typename std::decay_t<decltype(ringbuffer)>::ValueType;
        return ringbuffer.used() * sizeof(SampleType);
    });
}

bool QPipewireAudioSourceStream::start(QIODevice *device, ObjectSerial sourceNodeSerial)
{
    m_device = device;

    assert(hasStream());
    bool connected = connectStream(sourceNodeSerial, SPA_DIRECTION_INPUT);
    if (!connected)
        return false;

    if (device->isWritable()) {
        QObject::connect(&m_ringbufferHasData, &QAutoResetEvent::activated, device, [this] {
            if (!m_stopRequested && m_parent)
                m_parent->streamIdle(false);
            pushToIODevice();
        });
        QObject::connect(device, &QIODevice::bytesWritten, device, [this] {
            pushToIODevice();
        });
    }

    // keep instance alive until PW_STREAM_STATE_UNCONNECTED
    m_self = shared_from_this();

    return connected;
}

QIODevice *QPipewireAudioSourceStream::start(ObjectSerial sourceNodeSerial)
{
    visitRingbuffer([&](auto &rb) {
        using SampleType = typename std::decay_t<decltype(rb)>::ValueType;
        m_ringbufferReaderDevice = std::make_unique<QIODeviceRingBufferReader<SampleType>>(&rb);
    });

    m_ringbufferReaderDevice->open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    QObject::connect(&m_ringbufferHasData, &QAutoResetEvent::activated,
                     m_ringbufferReaderDevice.get(), [this] {
        emit m_ringbufferReaderDevice->readyRead();

        if (!m_stopRequested && m_parent)
            m_parent->streamIdle(false);
    });

    QObject::connect(&m_ringbufferIsFull, &QAutoResetEvent::activated,
                     m_ringbufferReaderDevice.get(), [this] {
        if (!m_stopRequested && m_parent)
            m_parent->streamIdle(true);
    });

    bool started = start(m_ringbufferReaderDevice.get(), sourceNodeSerial);
    if (!started)
        return nullptr;

    return m_ringbufferReaderDevice.get();
}

void QPipewireAudioSourceStream::stop()
{
    // TODO: fix stopping behavior

    m_stopRequested = true;

    // disconnect immediately
    disconnectStream();

    unregisterDeviceObserver();
}

void QPipewireAudioSourceStream::setVolume(qreal volume)
{
    m_volume.store(volume, std::memory_order_relaxed);
}

std::chrono::microseconds QPipewireAudioSourceStream::processedDuration()
{
    return std::chrono::microseconds{
        m_format.durationForFrames(
                m_totalNumberOfFramesPushedToRingbuffer.load(std::memory_order_relaxed)),
    };
}

void QPipewireAudioSourceStream::prepareFormat(const QAudioFormat &format,
                                               std::optional<qsizetype> ringbufferSize)
{
    using namespace std::chrono;
    using namespace std::chrono_literals;
    static constexpr auto defaultBufferDuration = 250ms;
    // prepare ringbuffer
    int defaultBufferSize = m_format.framesForDuration(microseconds(defaultBufferDuration).count())
            * m_format.channelCount();

    int bufferSize = ringbufferSize.value_or(defaultBufferSize);

    switch (format.sampleFormat()) {
    case SampleFormat::Float:
        m_ringbuffer.emplace<QAudioRingBuffer<float>>(bufferSize);
        break;
    case SampleFormat::Int16:
        m_ringbuffer.emplace<QAudioRingBuffer<int16_t>>(bufferSize);
        break;
    case SampleFormat::Int32:
        m_ringbuffer.emplace<QAudioRingBuffer<int32_t>>(bufferSize);
        break;
    case SampleFormat::UInt8:
        m_ringbuffer.emplace<QAudioRingBuffer<uint8_t>>(bufferSize);
        break;

    default:
        Q_UNREACHABLE();
    }
}

void QPipewireAudioSourceStream::disconnectStream()
{
    auto self = shared_from_this(); // extend lifetime until this function returns;

    QPipewireAudioStream::disconnectStream();

    QObject::disconnect(m_xrunNotification);
}

void QPipewireAudioSourceStream::pushToIODevice()
{
    using namespace QtMultimediaPrivate;
    using namespace QtPrivate;

    visitRingbuffer([&](auto &ringbuffer) {
        using SampleType = typename std::decay_t<decltype(ringbuffer)>::ValueType;

        for (;;) {
            auto ringbufferRegion = ringbuffer.acquireReadRegion(ringbuffer.size());
            if (ringbufferRegion.empty())
                return;

            QSpan bufferByteRegion = as_bytes(ringbufferRegion);

            int bytesToWrite = alignDown(m_device->bytesToWrite(), sizeof(SampleType));
            bufferByteRegion = take(bufferByteRegion, bytesToWrite);
            int bytesWritten = writeToDevice(*m_device, bufferByteRegion);

            if (bytesWritten < 0) {
                qWarning() << "QPipewireAudioSourceStream::pushToIODevice cannot push data to "
                              "QIODevice";
                return;
            }
            if (bytesWritten == 0)
                return;

            Q_ASSERT(bytesWritten % sizeof(SampleType) == 0);
            int samplesWritten = bytesWritten / sizeof(SampleType);
            ringbuffer.releaseReadRegion(samplesWritten);
        }
    });
}

void QPipewireAudioSourceStream::process() QT_MM_NONBLOCKING
{
    struct pw_buffer *b = pw_stream_dequeue_buffer(m_stream.get());
    if (!b) {
        qCritical() << "pw_stream_dequeue_buffer failed";
        return;
    }

    struct spa_buffer *buf = b->buffer;
    if (buf->datas[0].data == nullptr) {
        qWarning() << "pw_stream_dequeue_buffer received null buffer";
        return;
    }

    QSpan buffer{
        reinterpret_cast<const std::byte *>(buf->datas[0].data),
        qsizetype(buf->datas[0].chunk->size),
    };

    int numberOfSamplesRemaining = buffer.size() / m_format.bytesPerSample();

    performXRunDetection(m_format.framesForBytes(buffer.size()));

    const qreal volume = m_volume.load(std::memory_order_relaxed);

    uint64_t totalBytesWritten = 0;
    visitRingbuffer([&](auto &rb) {
        for (;;) {
            auto region = rb.acquireWriteRegion(numberOfSamplesRemaining);
            if (region.empty())
                break;

            QAudioHelperInternal::applyVolume(volume, m_format, buffer, as_writable_bytes(region));
            rb.releaseWriteRegion(region.size());
            totalBytesWritten += region.size_bytes();
            numberOfSamplesRemaining -= region.size();
        }
    });

    if (totalBytesWritten)
        m_ringbufferHasData.set();

    int framesWritten = m_format.framesForBytes(totalBytesWritten);
    m_totalNumberOfFramesPushedToRingbuffer = m_format.framesForBytes(framesWritten);
    addFramesHandled(m_format.framesForBytes(framesWritten));

    pw_stream_queue_buffer(m_stream.get(), b);
}

void QPipewireAudioSourceStream::handleDeviceRemoved()
{
    if (!m_stopRequested)
        m_parent->updateError(QAudio::Error::IOError);
}

void QPipewireAudioSourceStream::stateChanged(pw_stream_state /*oldState*/, pw_stream_state state,
                                              const char * /*error*/)
{
    switch (state) {
    case pw_stream_state::PW_STREAM_STATE_UNCONNECTED:
        m_self.reset();
        break;

    default:
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QPipewireAudioSource

QPipewireAudioSource::QPipewireAudioSource(const QAudioDevice &device, const QAudioFormat &format, QObject *parent)
    : BaseClass(device, format, parent)
{
}

QPipewireAudioSource::~QPipewireAudioSource()
{
    stop();
}

template <typename Functor>
void QPipewireAudioSource::startHelper(Functor &&starter)
{
    if (!m_format.isValid()) {
        qWarning() << "invalid format" << m_format;
        updateError(QtAudio::Error::OpenError);
        return;
    }

    std::optional<ObjectSerial> deviceSerial = findSourceNodeSerial();
    if (!deviceSerial) {
        qWarning() << "Cannot find device: " << privateDevice()->nodeName();
        updateError(QtAudio::Error::OpenError);
        return;
    }

    m_stream = std::make_shared<QPipewireAudioSourceStream>(this, format(), m_bufferSize,
                                                            m_hardwareBufferSize);
    if (!m_stream->hasStream()) {
        updateError(QtAudio::Error::OpenError);
        return;
    }

    m_stream->setVolume(m_volume);

    bool started = starter(m_stream, *deviceSerial);
    if (started)
        setUserOwnedState(QtAudio::State::ActiveState);
    else
        updateError(QtAudio::Error::OpenError);
}

using SharedSourceStream = std::shared_ptr<QPipewireAudioSourceStream>;

void QPipewireAudioSource::start(QIODevice *device)
{
    startHelper([&](const SharedSourceStream &stream, ObjectSerial deviceSerial) {
        return stream->start(device, deviceSerial);
    });
}

QIODevice *QPipewireAudioSource::start()
{
    QIODevice *deviceToReturn{};

    startHelper([&](const SharedSourceStream &stream, ObjectSerial deviceSerial) {
        deviceToReturn = stream->start(deviceSerial);
        // HACK alert: we're "idle" until a consumer starts reading.
        // this is fundamentally broken, and we should fix this behavior
        m_streamIsIdle = true;
        return bool(deviceToReturn);
    });

    return deviceToReturn;
}

void QPipewireAudioSource::stop()
{
    if (!m_stream)
        return;

    m_stream->stop(/*ShutdownPolicy::DrainRingbuffer*/);
    setUserOwnedState(QtAudio::State::StoppedState);
    m_stream.reset();
}

void QPipewireAudioSource::reset()
{
    if (!m_stream)
        return;

    m_stream->stop(/*ShutdownPolicy::DiscardRingbuffer*/);
    setUserOwnedState(QtAudio::State::StoppedState);
    m_stream.reset();
}

void QPipewireAudioSource::suspend()
{
    if (!m_stream)
        return;

    m_stream->suspend();

    setUserOwnedState(QtAudio::State::SuspendedState);
}

void QPipewireAudioSource::resume()
{
    if (!m_stream)
        return;

    if (m_userOwnedState == QtAudio::State::ActiveState)
        return;

    m_stream->resume();

    setUserOwnedState(QtAudio::State::ActiveState);
}

qsizetype QPipewireAudioSource::bytesReady() const
{
    if (m_stream)
        return m_stream->bytesReady();
    return 0;
}

std::optional<ObjectSerial> QPipewireAudioSource::findSourceNodeSerial()
{
    QByteArray nodeName = privateDevice()->nodeName();

    return QAudioContextManager::deviceMonitor().findSourceNodeSerial(std::string_view{
            nodeName.data(),
            size_t(nodeName.size()),
    });
}

void QPipewireAudioSource::reportXRuns(int numberOfXruns)
{
    qDebug() << "XRuns occurred:" << numberOfXruns;
}

} // namespace QtPipeWire

QT_END_NAMESPACE
