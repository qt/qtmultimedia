/****************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "bbmediaplayercontrol.h"
#include "bbvideowindowcontrol.h"
#include "bbutil.h"
#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qtimer.h>
#include <QtCore/quuid.h>
#include <mm/renderer.h>
#include <bps/mmrenderer.h>
#include <bps/screen.h>
#include <errno.h>
#include <sys/strm.h>
#include <sys/stat.h>

//#define QBBMEDIA_DEBUG

#ifdef QBBMEDIA_DEBUG
#define qBbMediaDebug qDebug
#else
#define qBbMediaDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

static int idCounter = 0;

static bool s_eventFilterInstalled = 0;
static QAbstractEventDispatcher::EventFilter s_previousEventFilter = 0;
static QHash< int, BbMediaPlayerControl* > s_idToPlayerMap;

static bool s_eventFilter(void *message)
{
    bps_event_t * const event = static_cast<bps_event_t *>(message);

    if (event &&
        (bps_event_get_domain(event) == mmrenderer_get_domain() ||
         bps_event_get_domain(event) == screen_get_domain() )) {
        const int id = mmrenderer_event_get_userdata(event);
        BbMediaPlayerControl * const control = s_idToPlayerMap.value(id);
        if (control)
            control->bpsEventHandler(event);
    }

    if (s_previousEventFilter)
        return s_previousEventFilter(message);
    else
        return false;
}

BbMediaPlayerControl::BbMediaPlayerControl(QObject *parent)
    : QMediaPlayerControl(parent),
      m_connection(0),
      m_context(0),
      m_audioId(-1),
      m_state(QMediaPlayer::StoppedState),
      m_volume(100),
      m_muted(false),
      m_rate(1),
      m_id(-1),
      m_eventMonitor(0),
      m_position(0),
      m_mediaStatus(QMediaPlayer::NoMedia),
      m_playAfterMediaLoaded(false),
      m_inputAttached(false),
      m_stopEventsToIgnore(0),
      m_bufferStatus(0)
{
    if (!s_eventFilterInstalled) {
        s_eventFilterInstalled = true;
        s_previousEventFilter =
                QCoreApplication::eventDispatcher()->setEventFilter(s_eventFilter);
    }

    openConnection();
}

BbMediaPlayerControl::~BbMediaPlayerControl()
{
    stop();
    detach();
    closeConnection();
}

void BbMediaPlayerControl::openConnection()
{
    m_connection = mmr_connect(NULL);
    if (!m_connection) {
        emitPError("Unable to connect to the multimedia renderer");
        return;
    }

    m_id = idCounter++;
    m_contextName = QString("BbMediaPlayerControl_%1_%2").arg(m_id)
                                                         .arg(QCoreApplication::applicationPid());
    m_context = mmr_context_create(m_connection, m_contextName.toLatin1(),
                                   0, S_IRWXU|S_IRWXG|S_IRWXO);
    if (!m_context) {
        emitPError("Unable to create context");
        closeConnection();
        return;
    }

    s_idToPlayerMap.insert(m_id, this);
    m_eventMonitor = mmrenderer_request_events(m_contextName.toLatin1(), 0, m_id);
    if (!m_eventMonitor) {
        qBbMediaDebug() << "Unable to request multimedia events";
        emit error(0, "Unable to request multimedia events");
    }
}

void BbMediaPlayerControl::closeConnection()
{
    s_idToPlayerMap.remove(m_id);
    if (m_eventMonitor) {
        mmrenderer_stop_events(m_eventMonitor);
        m_eventMonitor = 0;
    }

    if (m_context) {
        mmr_context_destroy(m_context);
        m_context = 0;
        m_contextName.clear();
    }

    if (m_connection) {
        mmr_disconnect(m_connection);
        m_connection = 0;
    }
}

void BbMediaPlayerControl::attach()
{
    if (m_media.isNull() || !m_context) {
        setMediaStatus(QMediaPlayer::NoMedia);
        return;
    }

    if (m_videoControl)
        m_videoControl->attachDisplay(m_context);

    m_audioId = mmr_output_attach(m_context, "audio:default", "audio");
    if (m_audioId == -1) {
        emitMmError("mmr_output_attach() for audio failed");
        return;
    }

    // If this is a local file, use the full path as the resource identifier
    QString mediaFile = m_media.canonicalUrl().toString();

    // The mmrenderer does not support playback from resource files, so copy it to a temporary
    // file
    const QString resourcePrefix("qrc:");
    if (mediaFile.startsWith(resourcePrefix)) {
        mediaFile.remove(0, resourcePrefix.length() - 1);
        const QFileInfo resourceFileInfo(mediaFile);
        m_tempMediaFileName = QDir::tempPath() + "/qtmedia_" + QUuid::createUuid().toString() + "." +
                              resourceFileInfo.suffix();
        if (!QFile::copy(mediaFile, m_tempMediaFileName)) {
            const QString errorMsg =
                QString("Failed to copy resource file to temporary file %1 for playback").arg(m_tempMediaFileName);
            qBbMediaDebug() << errorMsg;
            emit error(0, errorMsg);
            detach();
            return;
        }
        mediaFile = m_tempMediaFileName;
    }

    if (m_media.canonicalUrl().scheme().isEmpty()) {
        const QFileInfo fileInfo(mediaFile);
        mediaFile = "file://" + fileInfo.absoluteFilePath();
    }

    if (mmr_input_attach(m_context, QFile::encodeName(mediaFile), "track") != 0) {
        emitMmError(QString("mmr_input_attach() for %1 failed").arg(mediaFile));
        setMediaStatus(QMediaPlayer::InvalidMedia);
        detach();
        return;
    }

    // For whatever reason, the mmrenderer sends out a MMR_STOPPED event when calling
    // mmr_input_attach() above. Ignore it, as otherwise we'll trigger stopping right after we
    // started.
    m_stopEventsToIgnore++;

    m_inputAttached = true;
    setMediaStatus(QMediaPlayer::LoadedMedia);
    m_bufferStatus = 0;
    emit bufferStatusChanged(m_bufferStatus);
}

void BbMediaPlayerControl::detach()
{
    if (m_context) {
        if (m_inputAttached) {
            mmr_input_detach(m_context);
            m_inputAttached = false;
        }
        if (m_videoControl)
            m_videoControl->detachDisplay();
        if (m_audioId != -1 && m_context) {
            mmr_output_detach(m_context, m_audioId);
            m_audioId = -1;
        }
    }

    if (!m_tempMediaFileName.isEmpty()) {
        QFile::remove(m_tempMediaFileName);
        m_tempMediaFileName.clear();
    }
}

QMediaPlayer::State BbMediaPlayerControl::state() const
{
    return m_state;
}

QMediaPlayer::MediaStatus BbMediaPlayerControl::mediaStatus() const
{
    return m_mediaStatus;
}

qint64 BbMediaPlayerControl::duration() const
{
    return m_metaData.duration();
}

qint64 BbMediaPlayerControl::position() const
{
    return m_position;
}

void BbMediaPlayerControl::setPosition(qint64 position)
{
    if (m_position != position) {
        m_position = position;

        // Don't update in stopped state, it would not have any effect. Instead, the position is
        // updated in play().
        if (m_state != QMediaPlayer::StoppedState)
            setPositionInternal(m_position);

        emit positionChanged(m_position);
    }
}

int BbMediaPlayerControl::volume() const
{
    return m_volume;
}

void BbMediaPlayerControl::setVolumeInternal(int newVolume)
{
    if (!m_context)
        return;

    newVolume = qBound(0, newVolume, 100);
    if (m_audioId != -1) {
        strm_dict_t * dict = strm_dict_new();
        dict = strm_dict_set(dict, "volume", QString::number(newVolume).toLatin1());
        if (mmr_output_parameters(m_context, m_audioId, dict) != 0)
            emitMmError("mmr_output_parameters: Setting volume failed");
    }
}

void BbMediaPlayerControl::setPlaybackRateInternal(qreal rate)
{
    if (!m_context)
        return;

    const int mmRate = rate * 1000;
    if (mmr_speed_set(m_context, mmRate) != 0)
        emitMmError("mmr_speed_set failed");
}

void BbMediaPlayerControl::setPositionInternal(qint64 position)
{
    if (!m_context)
        return;

    if (mmr_seek(m_context, QString::number(position).toLatin1()) != 0)
        emitMmError("Seeking failed");
}

void BbMediaPlayerControl::setMediaStatus(QMediaPlayer::MediaStatus status)
{
    if (m_mediaStatus != status) {
        m_mediaStatus = status;
        emit mediaStatusChanged(m_mediaStatus);
    }
}

void BbMediaPlayerControl::setState(QMediaPlayer::State state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(m_state);
    }
}

void BbMediaPlayerControl::stopInternal(StopCommand stopCommand)
{
    if (m_state != QMediaPlayer::StoppedState) {

        if (stopCommand == StopMmRenderer) {
            ++m_stopEventsToIgnore;
            mmr_stop(m_context);
        }

        setState(QMediaPlayer::StoppedState);
    }

    if (m_position != 0) {
        m_position = 0;
        emit positionChanged(0);
    }
}

void BbMediaPlayerControl::setVolume(int volume)
{
    const int newVolume = qBound(0, volume, 100);
    if (m_volume != newVolume) {
        m_volume = newVolume;
        if (!m_muted)
            setVolumeInternal(m_volume);
        emit volumeChanged(m_volume);
    }
}

bool BbMediaPlayerControl::isMuted() const
{
    return m_muted;
}

void BbMediaPlayerControl::setMuted(bool muted)
{
    if (m_muted != muted) {
        m_muted = muted;
        setVolumeInternal(muted ? 0 : m_volume);
        emit mutedChanged(muted);
    }
}

int BbMediaPlayerControl::bufferStatus() const
{
    return m_bufferStatus;
}

bool BbMediaPlayerControl::isAudioAvailable() const
{
    return m_metaData.hasAudio();
}

bool BbMediaPlayerControl::isVideoAvailable() const
{
    return m_metaData.hasVideo();
}

bool BbMediaPlayerControl::isSeekable() const
{
    // We can currently not get that information from the mmrenderer API. Just pretend we can seek,
    // it will fail at runtime if we can not.
    return true;
}

QMediaTimeRange BbMediaPlayerControl::availablePlaybackRanges() const
{
    // We can't get this information from the mmrenderer API yet, so pretend we can seek everywhere
    return QMediaTimeRange(0, m_metaData.duration());
}

qreal BbMediaPlayerControl::playbackRate() const
{
    return m_rate;
}

void BbMediaPlayerControl::setPlaybackRate(qreal rate)
{
    if (m_rate != rate) {
        m_rate = rate;
        setPlaybackRateInternal(m_rate);
        emit playbackRateChanged(m_rate);
    }
}

QMediaContent BbMediaPlayerControl::media() const
{
    return m_media;
}

const QIODevice *BbMediaPlayerControl::mediaStream() const
{
    // Always 0, we don't support QIODevice streams
    return 0;
}

void BbMediaPlayerControl::setMedia(const QMediaContent &media, QIODevice *stream)
{
    Q_UNUSED(stream); // not supported

    stop();
    detach();

    m_media = media;
    emit mediaChanged(m_media);

    // Slight hack: With MediaPlayer QtQuick elements that have autoPlay set to true, playback
    // would start before the QtQuick canvas is propagated to all elements, and therefore our
    // video output would not work. Therefore, delay actually playing the media a bit so that the
    // canvas is ready.
    // The mmrenderer doesn't allow to attach video outputs after playing has started, otherwise
    // this would be unnecessary.
    setMediaStatus(QMediaPlayer::LoadingMedia);
    QTimer::singleShot(0, this, SLOT(continueLoadMedia()));
}

void BbMediaPlayerControl::continueLoadMedia()
{
    attach();
    updateMetaData();
    if (m_playAfterMediaLoaded)
        play();
}

void BbMediaPlayerControl::play()
{
    if (m_playAfterMediaLoaded)
        m_playAfterMediaLoaded = false;

    // No-op if we are already playing, except if we were called from continueLoadMedia(), in which
    // case m_playAfterMediaLoaded is true (hence the 'else').
    else if (m_state == QMediaPlayer::PlayingState)
        return;

    if (m_mediaStatus == QMediaPlayer::LoadingMedia) {

        // State changes are supposed to be synchronous
        setState(QMediaPlayer::PlayingState);

        // Defer playing to later, when the timer triggers continueLoadMedia()
        m_playAfterMediaLoaded = true;
        return;
    }

    // Un-pause the state when it is paused
    if (m_state == QMediaPlayer::PausedState) {
        setPlaybackRateInternal(m_rate);
        setState(QMediaPlayer::PlayingState);
        return;
    }

    if (m_media.isNull() || !m_connection || !m_context || m_audioId == -1) {
        setState(QMediaPlayer::StoppedState);
        return;
    }

    setPositionInternal(m_position);
    setVolumeInternal(m_volume);
    setPlaybackRateInternal(m_rate);

    if (mmr_play(m_context) != 0) {
        setState(QMediaPlayer::StoppedState);
        emitMmError("mmr_play() failed");
        return;
    }

    setState( QMediaPlayer::PlayingState);
}

void BbMediaPlayerControl::pause()
{
    if (m_state == QMediaPlayer::PlayingState) {
        setPlaybackRateInternal(0);
        setState(QMediaPlayer::PausedState);
    }
}

void BbMediaPlayerControl::stop()
{
    stopInternal(StopMmRenderer);
}

void BbMediaPlayerControl::setVideoControl(BbVideoWindowControl *videoControl)
{
    m_videoControl = videoControl;
}

void BbMediaPlayerControl::bpsEventHandler(bps_event_t *event)
{
    if (m_videoControl)
        m_videoControl->bpsEventHandler(event);

    if (bps_event_get_domain(event) != mmrenderer_get_domain())
        return;

    if (bps_event_get_code(event) == MMRENDERER_STATE_CHANGE) {
        const mmrenderer_state_t newState = mmrenderer_event_get_state(event);
        if (newState == MMR_STOPPED) {

            // Only react to stop events that happen when the end of the stream is reached and
            // playback is stopped because of this.
            // Ignore other stop event sources, souch as calling mmr_stop() ourselves and
            // mmr_input_attach().
            if (m_stopEventsToIgnore > 0)
                --m_stopEventsToIgnore;
            else
                stopInternal(IgnoreMmRenderer);
            return;
        }
    }

    if (bps_event_get_code(event) == MMRENDERER_STATUS_UPDATE) {

        // Prevent spurious position change events from overriding our own position, for example
        // when setting the position to 0 in stop().
        if (m_state != QMediaPlayer::PlayingState)
            return;

        const qint64 newPosition = QString::fromLatin1(mmrenderer_event_get_position(event)).toLongLong();
        if (newPosition != 0 && newPosition != m_position) {
            m_position = newPosition;
            emit positionChanged(m_position);
        }

        const QString bufferStatus = QString::fromLatin1(mmrenderer_event_get_bufferlevel(event));
        const int slashPos = bufferStatus.indexOf('/');
        if (slashPos != -1) {
            const int fill = bufferStatus.left(slashPos).toInt();
            const int capacity = bufferStatus.mid(slashPos + 1).toInt();
            if (capacity != 0) {
                m_bufferStatus = fill / static_cast<float>(capacity) * 100.0f;
                emit bufferStatusChanged(m_bufferStatus);
            }
        }
    }
}

void BbMediaPlayerControl::updateMetaData()
{
    if (m_mediaStatus == QMediaPlayer::LoadedMedia)
        m_metaData.parse(m_contextName);
    else
        m_metaData.clear();

    if (m_videoControl)
        m_videoControl->setMetaData(m_metaData);

    emit durationChanged(m_metaData.duration());
    emit audioAvailableChanged(m_metaData.hasAudio());
    emit videoAvailableChanged(m_metaData.hasVideo());
    emit availablePlaybackRangesChanged(availablePlaybackRanges());
}

void BbMediaPlayerControl::emitMmError(const QString &msg)
{
    int errorCode = MMR_ERROR_NONE;
    const QString errorMessage = mmErrorMessage(msg, m_context, &errorCode);
    qBbMediaDebug() << errorMessage;
    emit error(errorCode, errorMessage);
}

void BbMediaPlayerControl::emitPError(const QString &msg)
{
    const QString errorMessage = QString("%1: %2").arg(msg).arg(strerror(errno));
    qBbMediaDebug() << errorMessage;
    emit error(errno, errorMessage);
}

QT_END_NAMESPACE
