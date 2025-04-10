// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiosource_p.h"

#include "qpipewire_audiocontextmanager_p.h"
#include "qpipewire_audiodevice_p.h"
#include "qpipewire_audiostream_p.h"
#include "qpipewire_support_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qpointer.h>
#include <QtCore/qsemaphore.h>
#include <QtCore/qloggingcategory.h>
#include <QtMultimedia/private/qaudio_qiodevice_support_p.h>
#include <QtMultimedia/private/qaudio_rtsan_support_p.h>
#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using QtMultimediaPrivate::QPlatformAudioSourceStream;
using ShutdownPolicy = QtMultimediaPrivate::QPlatformAudioIOStream::ShutdownPolicy;
using namespace std::chrono_literals;
using namespace Qt::Literals;

// LATER:
// ideally the ringbuffer should fill a buffer that can grow via a worker thread on which we can
// allocate.
struct QPipewireAudioSourceStream final : std::enable_shared_from_this<QPipewireAudioSourceStream>,
                                          QPipewireAudioStream,
                                          QPlatformAudioSourceStream
{
    using SampleFormat = QAudioFormat::SampleFormat;

    QPipewireAudioSourceStream(QAudioDevice, QPipewireAudioSource *parent,
                               const QAudioFormat &format, std::optional<qsizetype> ringbufferSize,
                               std::optional<int32_t> hardwareBufferSize, float volume);
    ~QPipewireAudioSourceStream();

    Q_DISABLE_COPY_MOVE(QPipewireAudioSourceStream)

    bool start(QIODevice *device);
    QIODevice *start();
    void stop(ShutdownPolicy);

    using QPlatformAudioSourceStream::bytesReady;
    using QPlatformAudioSourceStream::deviceIsRingbufferReader;
    using QPlatformAudioSourceStream::inferRingbufferBytes;
    using QPlatformAudioSourceStream::processedDuration;
    using QPlatformAudioSourceStream::ringbufferSizeInBytes;
    using QPlatformAudioSourceStream::setVolume;

    void updateStreamIdle(bool idle) override;

private:
    std::optional<ObjectSerial> findSourceNodeSerial()
    {
        const QPipewireAudioDevicePrivate *device =
                QAudioDevicePrivate::handle<QPipewireAudioDevicePrivate>(m_audioDevice);

        QByteArray nodeName = device->nodeName();
        auto ret = QAudioContextManager::deviceMonitor().findSourceNodeSerial(std::string_view{
                nodeName.data(),
                size_t(nodeName.size()),
        });

        if (!ret)
            qWarning() << "Cannot find device: " << nodeName;
        return ret;
    }

    using QPlatformAudioSourceStream::m_format;

    void processRingbuffer() noexcept QT_MM_NONBLOCKING override;
    void processCallback() noexcept QT_MM_NONBLOCKING override { Q_ASSERT(false); }
    void handleDeviceRemoved() override;

    void stateChanged(pw_stream_state old, pw_stream_state state, const char *error) override;
    void disconnectStream();

    std::shared_ptr<QPipewireAudioSourceStream> m_self;
    QSemaphore m_streamDisconnected;

    // xrun detection
    void xrunOccurred(int /*xrunCount*/) override { m_xrunOccurred.set(); }
    QtPrivate::QAutoResetEvent m_xrunOccurred;
    QMetaObject::Connection m_xrunNotification;

    QPipewireAudioSource *m_parent;
};

QPipewireAudioSourceStream::QPipewireAudioSourceStream(QAudioDevice device, QPipewireAudioSource *parent,
                                                       const QAudioFormat &format,
                                                       std::optional<qsizetype> ringbufferSize,
                                                       std::optional<int32_t> hardwareBufferFrames,
                                                       float volume):
    QPipewireAudioStream {
        format,
    },
    QPlatformAudioSourceStream {
        std::move(device),
        format,
        ringbufferSize,
        hardwareBufferFrames,
        volume,
    },
    m_parent {
        parent,
    }
{
    auto extraProperties = std::array{
        spa_dict_item{ PW_KEY_MEDIA_CATEGORY, "Capture" },
        spa_dict_item{ PW_KEY_MEDIA_ROLE, "Music" },
    };

    QString applicationName = qApp->applicationName();
    if (applicationName.isNull())
        applicationName = u"QPipewireAudioSource"_s;

    createStream(extraProperties, hardwareBufferFrames, applicationName.toUtf8().constData());

    m_xrunNotification =
            QObject::connect(&m_xrunOccurred, &QAutoResetEvent::activated, &m_xrunOccurred, [this] {
        if (isStopRequested())
            return;
        m_parent->reportXRuns(m_xrunCount.exchange(0));
    });
}

QPipewireAudioSourceStream::~QPipewireAudioSourceStream() = default;

bool QPipewireAudioSourceStream::start(QIODevice *device)
{
    setQIODevice(device);

    assert(hasStream());
    auto sourceNodeSerial = findSourceNodeSerial();
    if (!sourceNodeSerial)
        return false;

    bool connected = connectStream(*sourceNodeSerial, SPA_DIRECTION_INPUT);
    if (!connected)
        return false;

    createQIODeviceConnections(device);

    // keep instance alive until PW_STREAM_STATE_UNCONNECTED
    m_self = shared_from_this();

    return connected;
}

QIODevice *QPipewireAudioSourceStream::start()
{
    QIODevice *device = createRingbufferReaderDevice();

    bool started = start(device);
    if (!started)
        return nullptr;

    return device;
}

void QPipewireAudioSourceStream::stop(ShutdownPolicy shutdownPolicy)
{
    requestStop();

    // disconnect immediately
    disconnectStream();
    unregisterDeviceObserver();
    disconnectQIODeviceConnections();

    finalizeQIODevice(shutdownPolicy);
    if (shutdownPolicy == ShutdownPolicy::DiscardRingbuffer) {
        // Pipewire is asynchronous. So to properly discard the ringbuffer content, we need to wait
        // for the stream to be stopped before we discard the ringbuffer content
        bool streamDisconnected = m_streamDisconnected.try_acquire_for(5s);
        if (!streamDisconnected)
            qWarning() << "QPipewireAudioSourceStream::stop: m_streamDisconnected semaphore "
                          "timeout. This should not happen";
        emptyRingbuffer();
    }
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

void QPipewireAudioSourceStream::processRingbuffer() noexcept QT_MM_NONBLOCKING
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
        QPlatformAudioSourceStream::handleIOError(m_parent);
}

void QPipewireAudioSourceStream::stateChanged(pw_stream_state /*oldState*/, pw_stream_state state,
                                              const char * /*error*/)
{
    switch (state) {
    case pw_stream_state::PW_STREAM_STATE_UNCONNECTED:
        m_streamDisconnected.release();
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
    m_stream = std::make_shared<QPipewireAudioSourceStream>(
            m_audioDevice, this, format(), m_bufferSize, m_hardwareBufferFrames, m_volume);
    if (!m_stream->hasStream()) {
        setError(QtAudio::Error::OpenError);
        m_stream = {};
        return;
    }

    bool started = starter(m_stream);
    if (started) {
        updateStreamState(QtAudio::State::ActiveState);
    } else {
        m_stream = {};
        setError(QtAudio::Error::OpenError);
    }
}

using SharedSourceStream = std::shared_ptr<QPipewireAudioSourceStream>;

void QPipewireAudioSource::start(QIODevice *device)
{
    startHelper([&](const SharedSourceStream &stream) {
        return stream->start(device);
    });
}

QIODevice *QPipewireAudioSource::start()
{
    QIODevice *deviceToReturn{};

    startHelper([&](const SharedSourceStream &stream) {
        deviceToReturn = stream->start();
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

    if (m_stream->deviceIsRingbufferReader())
        // we own the qiodevice, so let's keep it alive to allow users to drain the ringbuffer
        m_retiredStream = m_stream;

    m_stream->stop(ShutdownPolicy::DrainRingbuffer);
    m_stream = {};
    updateStreamState(QtAudio::State::StoppedState);
}

void QPipewireAudioSource::reset()
{
    m_retiredStream = {};

    if (!m_stream)
        return;

    m_stream->stop(ShutdownPolicy::DiscardRingbuffer);
    m_stream = {};
    updateStreamState(QtAudio::State::StoppedState);
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

void QPipewireAudioSource::reportXRuns(int numberOfXruns)
{
    qDebug() << "XRuns occurred:" << numberOfXruns;
}

} // namespace QtPipeWire

QT_END_NAMESPACE
