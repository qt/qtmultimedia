// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiosource_p.h"

#include "qpipewire_audiocontextmanager_p.h"
#include "qpipewire_audiodevice_p.h"
#include "qpipewire_support_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using ShutdownPolicy = QPlatformAudioIOStream::ShutdownPolicy;
using namespace std::chrono_literals;
using namespace Qt::Literals;

QPipewireAudioSourceStream::QPipewireAudioSourceStream(QAudioDevice device, const QAudioFormat &format,
                                                       std::optional<qsizetype> ringbufferSize,
                                                       QPipewireAudioSource *parent,
                                                       float volume,
                                                       std::optional<int32_t> hardwareBufferFrames
                                                       ):
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
    m_xrunNotification = m_xrunOccurred.callOnActivated([this] {
        if (isStopRequested())
            return;
        m_parent->reportXRuns(m_xrunCount.exchange(0));
    });
}

QPipewireAudioSourceStream::~QPipewireAudioSourceStream() = default;

bool QPipewireAudioSourceStream::start(QIODevice *device)
{
    createStream(StreamType::Ringbuffer);

    Q_ASSERT(hasStream());
    auto sourceNodeSerial = findSourceNodeSerial();
    if (!sourceNodeSerial) {
        requestStop();
        return false;
    }

    setQIODevice(device);

    bool connected = connectStream(*sourceNodeSerial, SPA_DIRECTION_INPUT);
    if (!connected) {
        requestStop();
        return false;
    }

    createQIODeviceConnections(device);

    // keep instance alive until PW_STREAM_STATE_UNCONNECTED
    m_self = shared_from_this();
    QAudioContextManager::instance()->registerStreamReference(m_self);

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

bool QPipewireAudioSourceStream::start(AudioCallback &&audioCallback)
{
    createStream(StreamType::Callback);

    Q_ASSERT(hasStream());
    auto sourceNodeSerial = findSourceNodeSerial();
    if (!sourceNodeSerial) {
        requestStop();
        return false;
    }

    m_audioCallback = std::move(audioCallback);

    bool connected = connectStream(*sourceNodeSerial, SPA_DIRECTION_INPUT);
    if (!connected) {
        requestStop();
        return false;
    }

    // keep instance alive until PW_STREAM_STATE_UNCONNECTED
    m_self = shared_from_this();
    return true;
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

void QPipewireAudioSourceStream::createStream(StreamType streamType)
{
    auto extraProperties = std::array{
        spa_dict_item{ PW_KEY_MEDIA_CATEGORY, "Capture" },
        spa_dict_item{ PW_KEY_MEDIA_ROLE, "Music" },
    };

    QString applicationName = qApp->applicationName();
    if (applicationName.isNull())
        applicationName = u"QPipewireAudioSource"_s;

    QPipewireAudioStream::createStream(extraProperties, m_hardwareBufferFrames,
                                       applicationName.toUtf8().constData(), streamType);
}

std::optional<ObjectSerial> QPipewireAudioSourceStream::findSourceNodeSerial()
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

void QPipewireAudioSourceStream::processCallback() noexcept QT_MM_NONBLOCKING
{
    using namespace QtMultimediaPrivate;
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

    runAudioCallback(*m_audioCallback, buffer, m_format, volume());

    addFramesHandled(numberOfFrames);
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
        QAudioContextManager::instance()->unregisterStreamReference(m_self);
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

void QPipewireAudioSource::reportXRuns(int numberOfXruns)
{
    qDebug() << "XRuns occurred:" << numberOfXruns;
}

} // namespace QtPipeWire

QT_END_NAMESPACE
