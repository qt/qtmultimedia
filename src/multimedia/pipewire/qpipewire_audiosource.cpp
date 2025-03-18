// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiosource_p.h"

#include "qpipewire_audiocontextmanager_p.h"
#include "qpipewire_audiodevice_p.h"
#include "qpipewire_audiostream_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qpointer.h>
#include <QtCore/qloggingcategory.h>
#include <QtMultimedia/private/qaudio_qiodevice_support_p.h>
#include <QtMultimedia/private/qaudio_rtsan_support_p.h>
#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using QtMultimediaPrivate::QPlatformAudioSourceStream;

// LATER:
// ideally the ringbuffer should fill a buffer that can grow via a worker thread on which we can
// allocate.
struct QPipewireAudioSourceStream final : std::enable_shared_from_this<QPipewireAudioSourceStream>,
                                          QPipewireAudioStream,
                                          QPlatformAudioSourceStream
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

    using QPlatformAudioSourceStream::bytesReady;
    using QPlatformAudioSourceStream::processedDuration;
    using QPlatformAudioSourceStream::setVolume;

    void updateStreamIdle(bool idle) override;

private:
    using QPlatformAudioSourceStream::m_format;

    void process() QT_MM_NONBLOCKING override;
    void handleDeviceRemoved() override;

    void stateChanged(pw_stream_state old, pw_stream_state state, const char *error) override;
    void disconnectStream();

    std::shared_ptr<QPipewireAudioSourceStream> m_self;

    // xrun detection
    void xrunOccurred(int /*xrunCount*/) override { m_xrunOccurred.set(); }
    QtPrivate::QAutoResetEvent m_xrunOccurred;
    QMetaObject::Connection m_xrunNotification;

    QPipewireAudioSource *m_parent;
};

QPipewireAudioSourceStream::QPipewireAudioSourceStream(QPipewireAudioSource *parent,
                                                       const QAudioFormat &format,
                                                       std::optional<qsizetype> ringbufferSize,
                                                       std::optional<qsizetype> hardwareBufferSize):
    QPipewireAudioStream {
        format,
    },
    QPlatformAudioSourceStream {
        {},
        format,
        ringbufferSize,
    },
    m_parent {
        parent,
    }
{
    auto extraProperties = std::array{
        spa_dict_item{ PW_KEY_MEDIA_CATEGORY, "Capture" },
        spa_dict_item{ PW_KEY_MEDIA_ROLE, "Music" },
    };
    createStream(extraProperties, hardwareBufferSize, "QPipewireAudioSource");

    m_xrunNotification =
            QObject::connect(&m_xrunOccurred, &QAutoResetEvent::activated, &m_xrunOccurred, [this] {
        if (isStopRequested())
            return;
        m_parent->reportXRuns(m_xrunCount.exchange(0));
    });
}

QPipewireAudioSourceStream::~QPipewireAudioSourceStream() = default;

qsizetype QPipewireAudioSourceStream::bytesReady() const
{
    return visitRingbuffer([&](auto &ringbuffer) {
        using SampleType = typename std::decay_t<decltype(ringbuffer)>::ValueType;
        return ringbuffer.used() * sizeof(SampleType);
    });
}

bool QPipewireAudioSourceStream::start(QIODevice *device, ObjectSerial sourceNodeSerial)
{
    setQIODevice(device);

    assert(hasStream());
    bool connected = connectStream(sourceNodeSerial, SPA_DIRECTION_INPUT);
    if (!connected)
        return false;

    createQIODeviceConnections(device);

    // keep instance alive until PW_STREAM_STATE_UNCONNECTED
    m_self = shared_from_this();

    return connected;
}

QIODevice *QPipewireAudioSourceStream::start(ObjectSerial sourceNodeSerial)
{
    QIODevice *device = createRingbufferReaderDevice();

    bool started = start(device, sourceNodeSerial);
    if (!started)
        return nullptr;

    return device;
}

void QPipewireAudioSourceStream::stop()
{
    // TODO: fix stopping behavior
    requestStop();

    // disconnect immediately
    disconnectStream();

    unregisterDeviceObserver();
}

void QPipewireAudioSourceStream::updateStreamIdle(bool idle)
{
    if (m_parent)
        m_parent->updateStreamIdle(idle);
}

void QPipewireAudioSourceStream::disconnectStream()
{
    auto self = shared_from_this(); // extend lifetime until this function returns;

    QPipewireAudioStream::disconnectStream();

    QObject::disconnect(m_xrunNotification);
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

    int numberOfFrames = m_format.framesForBytes(buffer.size());

    performXRunDetection(numberOfFrames);

    uint64_t framesWritten = QPlatformAudioSourceStream::process(buffer, numberOfFrames);
    addFramesHandled(framesWritten);
    pw_stream_queue_buffer(m_stream.get(), b);
}

void QPipewireAudioSourceStream::handleDeviceRemoved()
{
    if (!isStopRequested())
        m_parent->setError(QAudio::Error::IOError);
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

QPipewireAudioSource::QPipewireAudioSource(QAudioDevice device, const QAudioFormat &format,
                                           QObject *parent)
    : BaseClass(std::move(device), format, parent)
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
        setError(QtAudio::Error::OpenError);
        return;
    }

    std::optional<ObjectSerial> deviceSerial = findSourceNodeSerial();
    if (!deviceSerial) {
        qWarning() << "Cannot find device: " << privateDevice()->nodeName();
        setError(QtAudio::Error::OpenError);
        return;
    }

    m_stream = std::make_shared<QPipewireAudioSourceStream>(this, format(), m_bufferSize,
                                                            m_hardwareBufferSize);
    if (!m_stream->hasStream()) {
        setError(QtAudio::Error::OpenError);
        return;
    }

    m_stream->setVolume(m_volume);

    bool started = starter(m_stream, *deviceSerial);
    if (started)
        updateStreamState(QtAudio::State::ActiveState);
    else
        setError(QtAudio::Error::OpenError);
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
        updateStreamIdle(true, EmitStateSignal::False);
        return bool(deviceToReturn);
    });

    return deviceToReturn;
}

void QPipewireAudioSource::stop()
{
    if (!m_stream)
        return;

    m_stream->stop(/*ShutdownPolicy::DrainRingbuffer*/);
    updateStreamState(QtAudio::State::StoppedState);
    m_stream.reset();
}

void QPipewireAudioSource::reset()
{
    if (!m_stream)
        return;

    m_stream->stop(/*ShutdownPolicy::DiscardRingbuffer*/);
    updateStreamState(QtAudio::State::StoppedState);
    m_stream.reset();
}

void QPipewireAudioSource::suspend()
{
    if (!m_stream)
        return;

    m_stream->suspend();

    updateStreamState(QtAudio::State::SuspendedState);
}

void QPipewireAudioSource::resume()
{
    if (!m_stream)
        return;

    if (state() == QtAudio::State::ActiveState)
        return;

    m_stream->resume();

    updateStreamState(QtAudio::State::ActiveState);
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
