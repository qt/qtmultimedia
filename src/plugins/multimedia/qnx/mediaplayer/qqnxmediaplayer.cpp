// Copyright (C) 2016 Research In Motion
// Copyright (C) 2021 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqnxmediaplayer_p.h"
#include "qqnxvideosink_p.h"
#include "qqnxmediautil_p.h"
#include "qqnxmediaeventthread_p.h"
#include "qqnxwindowgrabber_p.h"

#include <private/qabstractvideobuffer_p.h>

#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/quuid.h>
#include <mm/renderer.h>
#include <qmediaplayer.h>
#include <qqnxaudiooutput_p.h>
#include <qaudiooutput.h>

#include <errno.h>
#include <sys/strm.h>
#include <sys/stat.h>

#include <algorithm>
#include <tuple>

static constexpr int rateToSpeed(qreal rate)
{
    return std::floor(rate * 1000);
}

static constexpr qreal speedToRate(int speed)
{
    return std::floor(speed / 1000.0);
}

static constexpr int normalizeVolume(float volume)
{
    return std::clamp<int>(std::floor(volume * 100.0), 0, 100);
}

static std::tuple<int, int, bool> parseBufferLevel(const QString &value)
{
    if (value.isEmpty())
        return {};

    const int slashPos = value.indexOf('/');
    if (slashPos <= 0)
        return {};

    bool ok = false;
    const int level = value.left(slashPos).toInt(&ok);
    if (!ok || level < 0)
        return  {};

    const int capacity = value.mid(slashPos + 1).toInt(&ok);
    if (!ok || capacity < 0)
        return {};

    return { level, capacity, true };
}

class QnxTextureBuffer : public QAbstractVideoBuffer
{
public:
    QnxTextureBuffer(QQnxWindowGrabber *QQnxWindowGrabber)
        : QAbstractVideoBuffer(QVideoFrame::RhiTextureHandle)
    {
        m_windowGrabber = QQnxWindowGrabber;
        m_handle = 0;
    }

    QVideoFrame::MapMode mapMode() const override
    {
        return QVideoFrame::ReadWrite;
    }

    void unmap() override {}

    MapData map(QVideoFrame::MapMode /*mode*/) override
    {
        return {};
    }

    quint64 textureHandle(int plane) const override
    {
        if (plane != 0)
            return 0;
        if (!m_handle) {
            const_cast<QnxTextureBuffer*>(this)->m_handle = m_windowGrabber->getNextTextureId();
        }
        return m_handle;
    }

private:
    QQnxWindowGrabber *m_windowGrabber;
    quint64 m_handle;
};

class QnxRasterBuffer : public QAbstractVideoBuffer
{
public:
    QnxRasterBuffer(QQnxWindowGrabber *windowGrabber)
        : QAbstractVideoBuffer(QVideoFrame::NoHandle)
    {
        m_windowGrabber = windowGrabber;
    }

    QVideoFrame::MapMode mapMode() const override
    {
        return QVideoFrame::ReadOnly;
    }

    MapData map(QVideoFrame::MapMode /*mode*/) override
    {
        if (buffer.data) {
            qWarning("QnxRasterBuffer: need to unmap before mapping");
            return {};
        }

        buffer = m_windowGrabber->getNextBuffer();

        return {
            .nPlanes = 1,
            .bytesPerLine = { buffer.stride },
            .data = { buffer.data },
            .size = { buffer.width * buffer.height * buffer.pixelSize }
        };
    }

    void unmap() override
    {
        buffer = {};
    }

private:
    QQnxWindowGrabber *m_windowGrabber;
    QQnxWindowGrabber::BufferView buffer;
};

QT_BEGIN_NAMESPACE

QQnxMediaPlayer::QQnxMediaPlayer(QMediaPlayer *parent)
    : QObject(parent)
    , QPlatformMediaPlayer(parent)
    , m_windowGrabber(new QQnxWindowGrabber(this))
{
    m_flushPositionTimer.setSingleShot(true);
    m_flushPositionTimer.setInterval(100);

    connect(&m_flushPositionTimer, &QTimer::timeout, this, &QQnxMediaPlayer::flushPosition);

    connect(m_windowGrabber, &QQnxWindowGrabber::updateScene, this, &QQnxMediaPlayer::updateScene);

    openConnection();
}

QQnxMediaPlayer::~QQnxMediaPlayer()
{
    stop();
    detach();
    closeConnection();
}

void QQnxMediaPlayer::openConnection()
{
    static int idCounter = 0;

    m_connection = mmr_connect(nullptr);
    if (!m_connection) {
        emitPError(QString::fromLatin1("Unable to connect to the multimedia renderer"));
        return;
    }

    m_id = idCounter++;
    m_contextName = QString::fromLatin1("QQnxMediaPlayer_%1_%2").arg(m_id)
                                                         .arg(QCoreApplication::applicationPid());
    m_context = mmr_context_create(m_connection, m_contextName.toLatin1(),
                                   0, S_IRWXU|S_IRWXG|S_IRWXO);
    if (!m_context) {
        emitPError(QString::fromLatin1("Unable to create context"));
        closeConnection();
        return;
    }

    startMonitoring();
}

void QQnxMediaPlayer::handleMmEventState(const mmr_event_t *event)
{
    if (!event || event->type != MMR_EVENT_STATE)
        return;

    switch (event->state) {
    case MMR_STATE_DESTROYED:
        break;
    case MMR_STATE_IDLE:
        mediaStatusChanged(QMediaPlayer::NoMedia);
        stateChanged(QMediaPlayer::StoppedState);
        detachVideoOutput();
        detachInput();
        break;
    case MMR_STATE_STOPPED:
        stateChanged(QMediaPlayer::StoppedState);
        m_windowGrabber->stop();

        if (m_platformVideoSink)
            m_platformVideoSink->setVideoFrame({});
        break;
    case MMR_STATE_PLAYING:
        if (event->speed == 0) {
            stateChanged(QMediaPlayer::PausedState);
            m_windowGrabber->pause();
        } else if (state() == QMediaPlayer::PausedState) {
            m_windowGrabber->resume();
            stateChanged(QMediaPlayer::PlayingState);
        } else {
            m_windowGrabber->start();
            stateChanged(QMediaPlayer::PlayingState);
        }

        if (event->speed != m_speed) {
            m_speed = event->speed;

            if (state() != QMediaPlayer::PausedState)
                m_configuredSpeed = m_speed;

            playbackRateChanged(::speedToRate(m_speed));
        }
        break;
    }
}

void QQnxMediaPlayer::handleMmEventStatus(const mmr_event_t *event)
{
    if (!event || event->type != MMR_EVENT_STATUS)
        return;

    if (event->data)
        handleMmEventStatusData(event->data);

    // update pos
    if (!event->pos_str || isPendingPositionFlush())
        return;

    const QByteArray valueBa(event->pos_str);

    bool ok;
    const qint64 position = valueBa.toLongLong(&ok);

    if (!ok)
        qCritical("Could not parse position from '%s'", valueBa.constData());
    else
        handleMmPositionChanged(position);
}

void QQnxMediaPlayer::handleMmEventStatusData(const strm_dict_t *data)
{
    if (!data)
        return;

    const auto getValue = [data](const char *key) -> QString {
        const strm_string_t *value = strm_dict_find_rstr(data, key);

        if (!value)
            return {};

        return QString::fromUtf8(strm_string_get(value));
    };

    // update bufferProgress
    const QString bufferLevel = getValue("bufferlevel");

    if (!bufferLevel.isEmpty()) {
        const auto & [level, capacity, ok] = ::parseBufferLevel(bufferLevel);

        if (ok)
            updateBufferLevel(level, capacity);
        else
            qCritical("Could not parse buffer capacity from '%s'", qUtf8Printable(bufferLevel));
    }

    // update MediaStatus
    const QString bufferStatus = getValue("bufferstatus");
    const QString suspended = getValue("suspended");

    if (suspended == QStringLiteral("yes"))
        mediaStatusChanged(QMediaPlayer::StalledMedia);
    else if (bufferStatus == QStringLiteral("buffering"))
        mediaStatusChanged(QMediaPlayer::BufferingMedia);
    else if (bufferStatus == QStringLiteral("playing"))
        mediaStatusChanged(QMediaPlayer::BufferedMedia);
}

void QQnxMediaPlayer::handleMmEventError(const mmr_event_t *event)
{
    if (!event)
        return;

    // When playback is explicitly stopped using mmr_stop(), mm-renderer
    // generates a STATE event. When the end of media is reached, an ERROR
    // event is generated and the error code contained in the event information
    // is set to MMR_ERROR_NONE. When an error causes playback to stop,
    // the error code is set to something else.
    if (event->details.error.info.error_code == MMR_ERROR_NONE) {
        mediaStatusChanged(QMediaPlayer::EndOfMedia);
        stateChanged(QMediaPlayer::StoppedState);
    }
}

void QQnxMediaPlayer::closeConnection()
{
    stopMonitoring();

    if (m_context) {
        mmr_context_destroy(m_context);
        m_context = nullptr;
        m_contextName.clear();
    }

    if (m_connection) {
        mmr_disconnect(m_connection);
        m_connection = nullptr;
    }
}

QByteArray QQnxMediaPlayer::resourcePathForUrl(const QUrl &url)
{
    // If this is a local file, mmrenderer expects the file:// prefix and an absolute path.
    // We treat URLs without scheme as local files, most likely someone just forgot to set the
    // file:// prefix when constructing the URL.
    if (url.isLocalFile() || url.scheme().isEmpty()) {
        const QString relativeFilePath = url.scheme().isEmpty() ? url.path() : url.toLocalFile();
        const QFileInfo fileInfo(relativeFilePath);
        return QFile::encodeName(QStringLiteral("file://") + fileInfo.absoluteFilePath());

    // HTTP or similar URL
    } else {
        return url.toEncoded();
    }
}

void QQnxMediaPlayer::attach()
{
    // Should only be called in detached state
    if (isInputAttached())
        return;

    if (!m_media.isValid() || !m_context) {
        mediaStatusChanged(QMediaPlayer::NoMedia);
        return;
    }

    resetMonitoring();

    if (!(attachVideoOutput() && attachAudioOutput() && attachInput())) {
        detach();
        return;
    }

    mediaStatusChanged(QMediaPlayer::LoadedMedia);
}

bool QQnxMediaPlayer::attachVideoOutput()
{
    if (isVideoOutputAttached()) {
        qWarning() << "QQnxMediaPlayer: Video output already attached!";
        return true;
    }

    if (!m_context) {
        qWarning() << "QQnxMediaPlayer: No media player context!";
        return false;
    }

    const QByteArray windowGroupId = m_windowGrabber->windowGroupId();
    if (windowGroupId.isEmpty()) {
        qWarning() << "QQnxMediaPlayer: Unable to find window group";
        return false;
    }

    static int winIdCounter = 0;

    const QString windowName = QStringLiteral("QQnxVideoSink_%1_%2")
                                             .arg(winIdCounter++)
                                             .arg(QCoreApplication::applicationPid());

    m_windowGrabber->setWindowId(windowName.toLatin1());

    if (m_platformVideoSink)
        m_windowGrabber->setRhi(m_platformVideoSink->rhi());

    // Start with an invisible window, because we just want to grab the frames from it.
    const QString videoDeviceUrl = QStringLiteral("screen:?winid=%1&wingrp=%2&initflags=invisible&nodstviewport=1")
        .arg(windowName, QString::fromLatin1(windowGroupId));

    m_videoId = mmr_output_attach(m_context, videoDeviceUrl.toLatin1(), "video");

    if (m_videoId == -1) {
        qWarning() << "mmr_output_attach() for video failed";
        return false;
    }

    return true;
}

bool QQnxMediaPlayer::attachAudioOutput()
{
    if (isAudioOutputAttached()) {
        qWarning() << "QQnxMediaPlayer: Audio output already attached!";
        return true;
    }

    const QByteArray defaultAudioDevice = qgetenv("QQNX_RENDERER_DEFAULT_AUDIO_SINK");

    m_audioId = mmr_output_attach(m_context,
            defaultAudioDevice.isEmpty() ? "snd:" : defaultAudioDevice.constData(), "audio");

    if (m_audioId == -1) {
        emitMmError("mmr_output_attach() for audio failed");

        return false;
    }

    return true;
}

bool QQnxMediaPlayer::attachInput()
{
    if (isInputAttached())
        return true;

    const QByteArray resourcePath = resourcePathForUrl(m_media);

    if (resourcePath.isEmpty())
        return false;

    if (mmr_input_attach(m_context, resourcePath.constData(), "track") != 0) {
        emitMmError(QStringLiteral("mmr_input_attach() failed for ")
                + QString::fromUtf8(resourcePath));

        mediaStatusChanged(QMediaPlayer::InvalidMedia);

        return false;
    }

    m_inputAttached = true;

    return true;
}

void QQnxMediaPlayer::detach()
{
    if (!m_context)
        return;

    if (isVideoOutputAttached())
        detachVideoOutput();

    if (isAudioOutputAttached())
        detachAudioOutput();

    if (isInputAttached())
        detachInput();

    resetMonitoring();
}

void QQnxMediaPlayer::detachVideoOutput()
{
    m_windowGrabber->stop();

    if (m_platformVideoSink)
        m_platformVideoSink->setVideoFrame({});

    if (isVideoOutputAttached())
        mmr_output_detach(m_context, m_videoId);

    m_videoId = -1;
}

void QQnxMediaPlayer::detachAudioOutput()
{
    if (isAudioOutputAttached())
        mmr_output_detach(m_context, m_audioId);

    m_audioId = -1;
}

void QQnxMediaPlayer::detachInput()
{
    if (isInputAttached())
        mmr_input_detach(m_context);

    m_inputAttached = false;
}

bool QQnxMediaPlayer::isVideoOutputAttached() const
{
    return m_videoId != -1;
}

bool QQnxMediaPlayer::isAudioOutputAttached() const
{
    return m_audioId != -1;
}

bool QQnxMediaPlayer::isInputAttached() const
{
    return m_inputAttached;
}

void QQnxMediaPlayer::updateScene(const QSize &size)
{
    if (!m_platformVideoSink)
        return;

    auto *buffer = m_windowGrabber->isEglImageSupported()
        ? static_cast<QAbstractVideoBuffer*>(new QnxTextureBuffer(m_windowGrabber))
        : static_cast<QAbstractVideoBuffer*>(new QnxRasterBuffer(m_windowGrabber));

    const QVideoFrame actualFrame(buffer,
            QVideoFrameFormat(size, QVideoFrameFormat::Format_BGRX8888));

    m_platformVideoSink->setVideoFrame(actualFrame);
}

qint64 QQnxMediaPlayer::duration() const
{
    return m_metaData.duration();
}

qint64 QQnxMediaPlayer::position() const
{
    return m_position;
}

void QQnxMediaPlayer::setPosition(qint64 position)
{
    if (m_position == position)
        return;

    m_pendingPosition = position;
    m_flushPositionTimer.start();
}

void QQnxMediaPlayer::setPositionInternal(qint64 position)
{
    if (!m_context || !m_metaData.isSeekable() || mediaStatus() == QMediaPlayer::NoMedia)
        return;

    if (mmr_seek(m_context, QString::number(position).toLatin1()) != 0)
        emitMmError("Seeking failed");
}

void QQnxMediaPlayer::flushPosition()
{
    setPositionInternal(m_pendingPosition);
}

bool QQnxMediaPlayer::isPendingPositionFlush() const
{
    return m_flushPositionTimer.isActive();
}

void QQnxMediaPlayer::setDeferredSpeedEnabled(bool enabled)
{
    m_deferredSpeedEnabled = enabled;
}

bool QQnxMediaPlayer::isDeferredSpeedEnabled() const
{
    return m_deferredSpeedEnabled;
}

void QQnxMediaPlayer::setVolume(float volume)
{
    const int normalizedVolume = ::normalizeVolume(volume);

    if (m_volume == normalizedVolume)
        return;

    m_volume = normalizedVolume;

    if (!m_muted)
        updateVolume();
}

void QQnxMediaPlayer::setMuted(bool muted)
{
    if (m_muted == muted)
        return;

    m_muted = muted;

    updateVolume();
}

void QQnxMediaPlayer::updateVolume()
{
    if (!m_context || m_audioId == -1)
        return;

    const int volume = m_muted ? 0 : m_volume;

    char buf[] = "100";
    std::snprintf(buf, sizeof buf, "%d", volume);

    strm_dict_t * dict = strm_dict_new();
    dict = strm_dict_set(dict, "volume", buf);

    if (mmr_output_parameters(m_context, m_audioId, dict) != 0)
        emitMmError("mmr_output_parameters: Setting volume failed");
}

void QQnxMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    QAudioOutput *out = output ? output->q : nullptr;
    if (m_audioOutput == out)
        return;

    if (m_audioOutput)
        disconnect(m_audioOutput.get());
    m_audioOutput = out;
    if (m_audioOutput) {
        connect(out, &QAudioOutput::volumeChanged, this, &QQnxMediaPlayer::setVolume);
        connect(out, &QAudioOutput::mutedChanged, this, &QQnxMediaPlayer::setMuted);
    }
    setVolume(out ? out->volume() : 1.);
    setMuted(out ? out->isMuted() : true);
}

float QQnxMediaPlayer::bufferProgress() const
{
    // mm-renderer has buffer properties "status" and "level"
    // QMediaPlayer's buffer status maps to mm-renderer's buffer level
    return m_bufferLevel/100.0f;
}

bool QQnxMediaPlayer::isAudioAvailable() const
{
    return m_metaData.hasAudio();
}

bool QQnxMediaPlayer::isVideoAvailable() const
{
    return m_metaData.hasVideo();
}

bool QQnxMediaPlayer::isSeekable() const
{
    return m_metaData.isSeekable();
}

QMediaTimeRange QQnxMediaPlayer::availablePlaybackRanges() const
{
    // We can't get this information from the mmrenderer API yet, so pretend we can seek everywhere
    return QMediaTimeRange(0, m_metaData.duration());
}

qreal QQnxMediaPlayer::playbackRate() const
{
    return ::speedToRate(m_speed);
}

void QQnxMediaPlayer::setPlaybackRate(qreal rate)
{
    if (!m_context)
        return;

    const int speed = ::rateToSpeed(rate);

    if (m_speed == speed)
        return;

    // defer setting the playback speed for when play() is called to prevent
    // mm-renderer from inadvertently transitioning into play state
    if (isDeferredSpeedEnabled() && state() != QMediaPlayer::PlayingState) {
        m_deferredSpeed = speed;
        return;
    }

    if (mmr_speed_set(m_context, speed) != 0)
        emitMmError("mmr_speed_set failed");
}

QUrl QQnxMediaPlayer::media() const
{
    return m_media;
}

const QIODevice *QQnxMediaPlayer::mediaStream() const
{
    // Always 0, we don't support QIODevice streams
    return 0;
}

void QQnxMediaPlayer::setMedia(const QUrl &media, QIODevice *stream)
{
    Q_UNUSED(stream); // not supported

    stop();
    detach();

    stateChanged(QMediaPlayer::StoppedState);
    mediaStatusChanged(QMediaPlayer::LoadingMedia);

    m_media = media;

    updateMetaData(nullptr);
    attach();
}

void QQnxMediaPlayer::play()
{
    if (!m_media.isValid() || !m_connection || !m_context || m_audioId == -1) {
        stateChanged(QMediaPlayer::StoppedState);
        return;
    }

    if (state() == QMediaPlayer::PlayingState)
        return;

    setDeferredSpeedEnabled(false);

    if (m_deferredSpeed) {
        setPlaybackRate(::speedToRate(*m_deferredSpeed));
        m_deferredSpeed = {};
    } else {
        setPlaybackRate(::speedToRate(m_configuredSpeed));
    }

    setDeferredSpeedEnabled(true);

    // Un-pause the state when it is paused
    if (state() == QMediaPlayer::PausedState) {
        return;
    }


    if (mediaStatus() == QMediaPlayer::EndOfMedia)
        setPositionInternal(0);

    resetMonitoring();
    updateVolume();

    if (mmr_play(m_context) != 0) {
        stateChanged(QMediaPlayer::StoppedState);
        emitMmError("mmr_play() failed");
        return;
    }

    stateChanged(QMediaPlayer::PlayingState);
}

void QQnxMediaPlayer::pause()
{
    if (state() != QMediaPlayer::PlayingState)
        return;

    setPlaybackRate(0);
}

void QQnxMediaPlayer::stop()
{
    if (!m_context
            || state() == QMediaPlayer::StoppedState
            || mediaStatus() == QMediaPlayer::NoMedia)
        return;

    // mm-renderer does not rewind by default
    setPositionInternal(0);

    mmr_stop(m_context);
}

void QQnxMediaPlayer::setVideoSink(QVideoSink *videoSink)
{
    m_platformVideoSink = videoSink
        ? static_cast<QQnxVideoSink *>(videoSink->platformVideoSink())
        : nullptr;
}

void QQnxMediaPlayer::startMonitoring()
{
    m_eventThread = new QQnxMediaEventThread(m_context);

    connect(m_eventThread, &QQnxMediaEventThread::eventPending,
            this, &QQnxMediaPlayer::readEvents);

    m_eventThread->setObjectName(QStringLiteral("MmrEventThread-") + QString::number(m_id));
    m_eventThread->start();
}

void QQnxMediaPlayer::stopMonitoring()
{
    delete m_eventThread;
    m_eventThread = nullptr;
}

void QQnxMediaPlayer::resetMonitoring()
{
    m_bufferLevel = 0;
    m_position = 0;
    m_speed = 0;
}

void QQnxMediaPlayer::handleMmPositionChanged(qint64 newPosition)
{
    m_position = newPosition;

    if (state() == QMediaPlayer::PausedState)
        m_windowGrabber->forceUpdate();

    positionChanged(m_position);
}

void QQnxMediaPlayer::updateBufferLevel(int level, int capacity)
{
    m_bufferLevel = capacity == 0 ? 0 : level / static_cast<float>(capacity) * 100.0f;
    m_bufferLevel = qBound(0, m_bufferLevel, 100);
    bufferProgressChanged(m_bufferLevel/100.0f);
}

void QQnxMediaPlayer::updateMetaData(const strm_dict *dict)
{
    m_metaData.update(dict);

    durationChanged(m_metaData.duration());
    audioAvailableChanged(m_metaData.hasAudio());
    videoAvailableChanged(m_metaData.hasVideo());
    seekableChanged(m_metaData.isSeekable());
}

void QQnxMediaPlayer::emitMmError(const char *msg)
{
    emitMmError(QString::fromUtf8(msg));
}

void QQnxMediaPlayer::emitMmError(const QString &msg)
{
    int errorCode = MMR_ERROR_NONE;
    const QString errorMessage = mmErrorMessage(msg, m_context, &errorCode);
    emit error(errorCode, errorMessage);
}

void QQnxMediaPlayer::emitPError(const QString &msg)
{
    const QString errorMessage = QString::fromLatin1("%1: %2").arg(msg).arg(QString::fromUtf8(strerror(errno)));
    emit error(errno, errorMessage);
}


void QQnxMediaPlayer::readEvents()
{
    while (const mmr_event_t *event = mmr_event_get(m_context)) {
        if (event->type == MMR_EVENT_NONE)
            break;

        switch (event->type) {
        case MMR_EVENT_STATUS:
            handleMmEventStatus(event);
            break;
        case MMR_EVENT_STATE:
            handleMmEventState(event);
            break;
        case MMR_EVENT_METADATA:
            updateMetaData(event->data);
            break;
        case MMR_EVENT_ERROR:
            handleMmEventError(event);
            break;
        case MMR_EVENT_NONE:
        case MMR_EVENT_OVERFLOW:
        case MMR_EVENT_WARNING:
        case MMR_EVENT_PLAYLIST:
        case MMR_EVENT_INPUT:
        case MMR_EVENT_OUTPUT:
        case MMR_EVENT_CTXTPAR:
        case MMR_EVENT_TRKPAR:
        case MMR_EVENT_OTHER:
            break;
        }
    }

    if (m_eventThread)
        m_eventThread->signalRead();
}

QT_END_NAMESPACE

#include "moc_qqnxmediaplayer_p.cpp"
