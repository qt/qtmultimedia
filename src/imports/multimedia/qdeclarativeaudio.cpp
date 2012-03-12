/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtDeclarative/qdeclarativeinfo.h>

#include "qdeclarativeaudio_p.h"

#include <qmediaplayercontrol.h>
#include <qmediaavailabilitycontrol.h>

#include <qmediaservice.h>
#include <private/qmediaserviceprovider_p.h>
#include <qmetadatareadercontrol.h>
#include <qmediaavailabilitycontrol.h>

#include "qdeclarativemediametadata_p.h"

#include <QTimerEvent>

QT_BEGIN_NAMESPACE


class QDeclarativeMediaBaseObject : public QMediaObject
{
public:
    QDeclarativeMediaBaseObject(QMediaService *service)
        : QMediaObject(0, service)
    {
    }
};

class QDeclarativeMediaBasePlayerControl : public QMediaPlayerControl
{
public:
    QDeclarativeMediaBasePlayerControl(QObject *parent)
        : QMediaPlayerControl(parent)
    {
    }

    QMediaPlayer::State state() const { return QMediaPlayer::StoppedState; }
    QMediaPlayer::MediaStatus mediaStatus() const { return QMediaPlayer::NoMedia; }

    qint64 duration() const { return 0; }
    qint64 position() const { return 0; }
    void setPosition(qint64) {}
    int volume() const { return 0; }
    void setVolume(int) {}
    bool isMuted() const { return false; }
    void setMuted(bool) {}
    int bufferStatus() const { return 0; }
    bool isAudioAvailable() const { return false; }
    bool isVideoAvailable() const { return false; }
    bool isSeekable() const { return false; }
    QMediaTimeRange availablePlaybackRanges() const { return QMediaTimeRange(); }
    qreal playbackRate() const { return 1; }
    void setPlaybackRate(qreal) {}
    QMediaContent media() const { return QMediaContent(); }
    const QIODevice *mediaStream() const { return 0; }
    void setMedia(const QMediaContent &, QIODevice *) {}

    void play() {}
    void pause() {}
    void stop() {}
};

class QDeclarativeMediaBaseAvailabilityControl : public QMediaAvailabilityControl
{
public:
    QDeclarativeMediaBaseAvailabilityControl(bool available)
        : m_available(available)
    {
    }

    QtMultimedia::AvailabilityError availability() const
    {
        return m_available ? QtMultimedia::NoError : QtMultimedia::ServiceMissingError;
    }

private:
    bool m_available;
};

class QDeclarativeMediaBaseMetaDataControl : public QMetaDataReaderControl
{
public:
    QDeclarativeMediaBaseMetaDataControl(QObject *parent)
        : QMetaDataReaderControl(parent)
    {
    }

    bool isMetaDataAvailable() const { return false; }

    QVariant metaData(const QString &) const { return QVariant(); }
    QStringList availableMetaData() const { return QStringList(); }
};

class QDeclarativeMediaBaseAnimation : public QObject
{
public:
    QDeclarativeMediaBaseAnimation(QDeclarativeAudio *media)
        : m_media(media)
    {
    }

    void start() { if (!m_timer.isActive()) m_timer.start(500, this); }
    void stop() { m_timer.stop(); }

protected:
    void timerEvent(QTimerEvent *event)
    {
        if (event->timerId() == m_timer.timerId()) {
            event->accept();

            if (m_media->m_playbackState == QMediaPlayer::PlayingState)
                emit m_media->positionChanged();
            if (m_media->m_status == QMediaPlayer::BufferingMedia || QMediaPlayer::StalledMedia)
                emit m_media->bufferProgressChanged();
        } else {
            QObject::timerEvent(event);
        }
    }

private:
    QDeclarativeAudio *m_media;
    QBasicTimer m_timer;
};


/*!
    \qmlclass MediaPlayer
    \brief The MediaPlayer element allows you to add media playback to a scene.

    \inqmlmodule QtMultimedia 5
    \ingroup multimedia_qml

    This element is part of the \b{QtMultimedia 5.0} module.

    \qml
    import QtQuick 2.0
    import QtMultimedia 5.0

    Text {
        text: "Click Me!";
        font.pointSize: 24;
        width: 150; height: 50;

        MediaPlayer {
            id: playMusic
            source: "music.wav"
        }
        MouseArea {
            id: playArea
            anchors.fill: parent
            onPressed:  { playMusic.play() }
        }
    }
    \endqml

    You can use MediaPlayer by itself to play audio content (like the \l Audio element),
    or you can use it in conjunction with a \l VideoOutput element for rendering video.

    \qml
    import QtQuick 2.0
    import QtMultimedia 5.0

    Item {
        MediaPlayer {
            id: mediaplayer
            source: "groovy_video.mp4"
        }

        VideoOutput {
            anchors: parent.fill
            source: mediaplayer
        }

        MouseArea {
            id: playArea
            anchors.fill: parent
            onPressed: mediaplayer.play();
        }
    }
    \endqml

    \sa VideoOutput
*/

/*!
    \qmlclass Audio QDeclarativeAudio
    \brief The Audio element allows you to add audio playback to a scene.

    \inqmlmodule QtMultimedia 5
    \ingroup multimedia_qml

    This element is part of the \b{QtMultimedia 5.0} module.

    \qml
    import QtQuick 2.0
    import QtMultimedia 5.0

    Text {
        text: "Click Me!";
        font.pointSize: 24;
        width: 150; height: 50;

        Audio {
            id: playMusic
            source: "music.wav"
        }
        MouseArea {
            id: playArea
            anchors.fill: parent
            onPressed:  { playMusic.play() }
        }
    }
    \endqml

    \sa Video
*/

/*!
    \internal
    \class QDeclarativeAudio
    \brief The QDeclarativeAudio class provides an audio item that you can add to a QDeclarativeView.
*/

void QDeclarativeAudio::_q_error(int errorCode, const QString &errorString)
{
    m_error = QMediaPlayer::Error(errorCode);
    m_errorString = errorString;

    emit error(Error(errorCode), errorString);
    emit errorChanged();
}

void QDeclarativeAudio::_q_availabilityChanged(QtMultimedia::AvailabilityError)
{
    emit availabilityChanged(availability());
}


QDeclarativeAudio::QDeclarativeAudio(QObject *parent)
    : QObject(parent)
    , m_autoPlay(false)
    , m_autoLoad(true)
    , m_loaded(false)
    , m_muted(false)
    , m_complete(false)
    , m_loopCount(1)
    , m_runningCount(0)
    , m_position(0)
    , m_vol(1.0)
    , m_playbackRate(1.0)
    , m_mediaService(0)
    , m_playerControl(0)
    , m_mediaObject(0)
    , m_mediaProvider(0)
    , m_metaDataControl(0)
    , m_animation(0)
    , m_availabilityControl(0)
    , m_playbackState(QMediaPlayer::StoppedState)
    , m_status(QMediaPlayer::NoMedia)
    , m_error(QMediaPlayer::ServiceMissingError)
{
}

QDeclarativeAudio::~QDeclarativeAudio()
{
    delete m_mediaObject;
    m_metaData.reset();

    if (m_mediaProvider)
        m_mediaProvider->releaseService(m_mediaService);

    delete m_animation;
}

/*!
    \qmlproperty enumeration QtMultimedia5::Audio::availability

    Returns the availability state of the media player.

    This is one of:
    \table
    \header \li Value \li Description
    \row \li Available
        \li The media player is available to use.
    \row \li Busy
        \li The media player is usually available, but some other
           process is utilizing the hardware necessary to play media.
    \row \li Unavailable
        \li There are no supported media playback facilities.
    \row \li ResourceMissing
        \li There is one or more resources missing, so the media player cannot
           be used.  It may be possible to try again at a later time.
    \endtable
 */
QDeclarativeAudio::Availability QDeclarativeAudio::availability() const
{
    if (!m_playerControl)
        return Unavailable;
    if (m_availabilityControl)
        return Availability(m_availabilityControl->availability());
    return Available;
}

QUrl QDeclarativeAudio::source() const
{
    return m_source;
}

bool QDeclarativeAudio::autoPlay() const
{
    return m_autoPlay;
}

void QDeclarativeAudio::setAutoPlay(bool autoplay)
{
    if (m_autoPlay == autoplay)
        return;

    m_autoPlay = autoplay;
    emit autoPlayChanged();
}


void QDeclarativeAudio::setSource(const QUrl &url)
{
    if (url == m_source)
        return;

    m_source = url;
    m_loaded = false;
    if (m_complete && (m_autoLoad || url.isEmpty() || m_autoPlay)) {
        if (m_error != QMediaPlayer::ServiceMissingError && m_error != QMediaPlayer::NoError) {
            m_error = QMediaPlayer::NoError;
            m_errorString = QString();

            emit errorChanged();
        }

        m_playerControl->setMedia(m_source, 0);
        m_loaded = true;
    }
    else
        emit sourceChanged();

    if (m_autoPlay) m_playerControl->play();
}

bool QDeclarativeAudio::isAutoLoad() const
{
    return m_autoLoad;
}

void QDeclarativeAudio::setAutoLoad(bool autoLoad)
{
    if (m_autoLoad == autoLoad)
        return;

    m_autoLoad = autoLoad;
    emit autoLoadChanged();
}

int QDeclarativeAudio::loopCount() const
{
    return m_loopCount;
}

void QDeclarativeAudio::setLoopCount(int loopCount)
{
    if (loopCount == 0)
        loopCount = 1;
    else if (loopCount < -1)
        loopCount = -1;

    if (m_loopCount == loopCount) {
        return;
    }
    m_loopCount = loopCount;
    m_runningCount = loopCount - 1;
    emit loopCountChanged();
}

void QDeclarativeAudio::setPlaybackState(QMediaPlayer::State playbackState)
{
    if (m_playbackState == playbackState)
        return;

    if (m_complete) {
        switch (playbackState){
        case (QMediaPlayer::PlayingState):
            if (!m_loaded) {
                m_playerControl->setMedia(m_source, 0);
                m_playerControl->setPosition(m_position);
                m_loaded = true;
            }
            m_playerControl->play();
            break;

        case (QMediaPlayer::PausedState):
            if (!m_loaded) {
                m_playerControl->setMedia(m_source, 0);
                m_playerControl->setPosition(m_position);
                m_loaded = true;
            }
            m_playerControl->pause();
            break;

        case (QMediaPlayer::StoppedState):
            m_playerControl->stop();
        }
    }
}

int QDeclarativeAudio::duration() const
{
    return !m_complete ? 0 : m_playerControl->duration();
}

int QDeclarativeAudio::position() const
{
    return !m_complete ? m_position : m_playerControl->position();
}

void QDeclarativeAudio::setPosition(int position)
{
    if (this->position() == position)
        return;

    m_position = position;
    if (m_complete)
        m_playerControl->setPosition(m_position);
    else
        emit positionChanged();
}

qreal QDeclarativeAudio::volume() const
{
    return !m_complete ? m_vol : qreal(m_playerControl->volume()) / 100;
}

void QDeclarativeAudio::setVolume(qreal volume)
{
    if (volume < 0 || volume > 1) {
        qmlInfo(this) << tr("volume should be between 0.0 and 1.0");
        return;
    }

    if (m_vol == volume)
        return;

    m_vol = volume;

    if (m_complete)
        m_playerControl->setVolume(qRound(volume * 100));
    else
        emit volumeChanged();
}

bool QDeclarativeAudio::isMuted() const
{
    return !m_complete ? m_muted : m_playerControl->isMuted();
}

void QDeclarativeAudio::setMuted(bool muted)
{
    if (m_muted == muted)
        return;

    m_muted = muted;

    if (m_complete)
        m_playerControl->setMuted(muted);
    else
        emit mutedChanged();
}

qreal QDeclarativeAudio::bufferProgress() const
{
    return !m_complete ? 0 : qreal(m_playerControl->bufferStatus()) / 100;
}

bool QDeclarativeAudio::isSeekable() const
{
    return !m_complete ? false : m_playerControl->isSeekable();
}

qreal QDeclarativeAudio::playbackRate() const
{
    return m_playbackRate;
}

void QDeclarativeAudio::setPlaybackRate(qreal rate)
{
    if (m_playbackRate == rate)
        return;

    m_playbackRate = rate;

    if (m_complete)
        m_playerControl->setPlaybackRate(m_playbackRate);
    else
        emit playbackRateChanged();
}

QString QDeclarativeAudio::errorString() const
{
    return m_errorString;
}

QDeclarativeMediaMetaData *QDeclarativeAudio::metaData() const
{
    return m_metaData.data();
}


/*!
    \qmlmethod QtMultimedia5::Audio::play()

    Starts playback of the media.

    Sets the \l playbackState property to PlayingState.
*/

void QDeclarativeAudio::play()
{
    if (!m_complete)
        return;

    setPlaybackState(QMediaPlayer::PlayingState);
}

/*!
    \qmlmethod QtMultimedia5::Audio::pause()

    Pauses playback of the media.

    Sets the \l playbackState property to PausedState.
*/

void QDeclarativeAudio::pause()
{
    if (!m_complete)
        return;

    setPlaybackState(QMediaPlayer::PausedState);
}

/*!
    \qmlmethod QtMultimedia5::Audio::stop()

    Stops playback of the media.

    Sets the \l playbackState property to StoppedState.
*/

void QDeclarativeAudio::stop()
{
    if (!m_complete)
        return;

    setPlaybackState(QMediaPlayer::StoppedState);
}

/*!
    \qmlproperty url QtMultimedia5::Audio::source

    This property holds the source URL of the media.
*/

/*!
    \qmlproperty url QtMultimedia5::Audio::autoLoad

    This property indicates if loading of media should begin immediately.

    Defaults to true, if false media will not be loaded until playback is started.
*/

/*!
    \qmlsignal QtMultimedia5::Audio::playbackStateChanged()

    This handler is called when the \l playbackState property is altered.
*/


/*!
    \qmlsignal QtMultimedia5::Audio::paused()

    This handler is called when playback is paused.
*/

/*!
    \qmlsignal QtMultimedia5::Audio::stopped()

    This handler is called when playback is stopped.
*/

/*!
    \qmlsignal QtMultimedia5::Audio::playing()

    This handler is called when playback is started or resumed.
*/

/*!
    \qmlproperty enumeration QtMultimedia5::Audio::status

    This property holds the status of media loading. It can be one of:

    \list
    \li NoMedia - no media has been set.
    \li Loading - the media is currently being loaded.
    \li Loaded - the media has been loaded.
    \li Buffering - the media is buffering data.
    \li Stalled - playback has been interrupted while the media is buffering data.
    \li Buffered - the media has buffered data.
    \li EndOfMedia - the media has played to the end.
    \li InvalidMedia - the media cannot be played.
    \li UnknownStatus - the status of the media is unknown.
    \endlist
*/

QDeclarativeAudio::Status QDeclarativeAudio::status() const
{
    return Status(m_status);
}


/*!
    \qmlproperty enumeration QtMultimedia5::Audio::playbackState

    This property holds the state of media playback. It can be one of:

    \list
    \li PlayingState - the media is currently playing.
    \li PausedState - playback of the media has been suspended.
    \li StoppedState - playback of the media is yet to begin.
    \endlist
*/

QDeclarativeAudio::PlaybackState QDeclarativeAudio::playbackState() const
{
    return PlaybackState(m_playbackState);
}

/*!
    \qmlproperty int QtMultimedia5::Audio::autoPlay

    This property controls whether the media will begin to play on start up.

    Defaults to false, if set true the value of autoLoad will be overwritten to true.
*/

/*!
    \qmlproperty int QtMultimedia5::Audio::duration

    This property holds the duration of the media in milliseconds.

    If the media doesn't have a fixed duration (a live stream for example) this will be 0.
*/

/*!
    \qmlproperty int QtMultimedia5::Audio::position

    This property holds the current playback position in milliseconds.

    If the \l seekable property is true, this property can be set to seek to a new position.
*/

/*!
    \qmlproperty real QtMultimedia5::Audio::volume

    This property holds the volume of the audio output, from 0.0 (silent) to 1.0 (maximum volume).
*/

/*!
    \qmlproperty bool QtMultimedia5::Audio::muted

    This property holds whether the audio output is muted.
*/

/*!
    \qmlproperty bool QtMultimedia5::Audio::hasAudio

    This property holds whether the media contains audio.
*/

bool QDeclarativeAudio::hasAudio() const
{
    return !m_complete ? false : m_playerControl->isAudioAvailable();
}

/*!
    \qmlproperty bool QtMultimedia5::Audio::hasVideo

    This property holds whether the media contains video.
*/

bool QDeclarativeAudio::hasVideo() const
{
    return !m_complete ? false : m_playerControl->isVideoAvailable();
}

/*!
    \qmlproperty real QtMultimedia5::Audio::bufferProgress

    This property holds how much of the data buffer is currently filled, from 0.0 (empty) to 1.0
    (full).
*/

/*!
    \qmlproperty bool QtMultimedia5::Audio::seekable

    This property holds whether position of the audio can be changed.

    If true; setting a \l position value will cause playback to seek to the new position.
*/

/*!
    \qmlproperty real QtMultimedia5::Audio::playbackRate

    This property holds the rate at which audio is played at as a multiple of the normal rate.
*/

/*!
    \qmlproperty enumeration QtMultimedia5::Audio::error

    This property holds the error state of the audio.  It can be one of:

    \list
    \li NoError - there is no current error.
    \li ResourceError - the audio cannot be played due to a problem allocating resources.
    \li FormatError - the audio format is not supported.
    \li NetworkError - the audio cannot be played due to network issues.
    \li AccessDenied - the audio cannot be played due to insufficient permissions.
    \li ServiceMissing -  the audio cannot be played because the media service could not be
    instantiated.
    \endlist
*/

QDeclarativeAudio::Error QDeclarativeAudio::error() const
{
    return Error(m_error);
}

void QDeclarativeAudio::classBegin()
{
    if ((m_mediaProvider = QMediaServiceProvider::defaultServiceProvider()) != 0) {
        m_mediaService = m_mediaProvider->requestService(Q_MEDIASERVICE_MEDIAPLAYER);
        if (m_mediaService != 0) {
            m_playerControl = qobject_cast<QMediaPlayerControl *>(
                    m_mediaService->requestControl(QMediaPlayerControl_iid));
            m_metaDataControl = qobject_cast<QMetaDataReaderControl *>(
                    m_mediaService->requestControl(QMetaDataReaderControl_iid));
            m_mediaObject = new QDeclarativeMediaBaseObject(m_mediaService);
            m_availabilityControl = m_mediaService->requestControl<QMediaAvailabilityControl*>();
        }
    }

    bool realPlayer = m_playerControl;

    if (m_playerControl) {
        QObject::connect(m_playerControl, SIGNAL(stateChanged(QMediaPlayer::State)),
                this, SLOT(_q_statusChanged()));
        QObject::connect(m_playerControl, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
                this, SLOT(_q_statusChanged()));
        QObject::connect(m_playerControl, SIGNAL(mediaChanged(QMediaContent)),
                this, SIGNAL(sourceChanged()));
        QObject::connect(m_playerControl, SIGNAL(durationChanged(qint64)),
                this, SIGNAL(durationChanged()));
        QObject::connect(m_playerControl, SIGNAL(positionChanged(qint64)),
                this, SIGNAL(positionChanged()));
        QObject::connect(m_playerControl, SIGNAL(volumeChanged(int)),
                this, SIGNAL(volumeChanged()));
        QObject::connect(m_playerControl, SIGNAL(mutedChanged(bool)),
                this, SIGNAL(mutedChanged()));
        QObject::connect(m_playerControl, SIGNAL(bufferStatusChanged(int)),
                this, SIGNAL(bufferProgressChanged()));
        QObject::connect(m_playerControl, SIGNAL(seekableChanged(bool)),
                this, SIGNAL(seekableChanged()));
        QObject::connect(m_playerControl, SIGNAL(playbackRateChanged(qreal)),
                this, SIGNAL(playbackRateChanged()));
        QObject::connect(m_playerControl, SIGNAL(error(int,QString)),
                this, SLOT(_q_error(int,QString)));

        m_animation = new QDeclarativeMediaBaseAnimation(this);
        m_error = QMediaPlayer::NoError;
    } else {
        m_playerControl = new QDeclarativeMediaBasePlayerControl(this);
    }

    if (!m_metaDataControl)
        m_metaDataControl = new QDeclarativeMediaBaseMetaDataControl(this);

    if (!m_availabilityControl)
        m_availabilityControl = new QDeclarativeMediaBaseAvailabilityControl(realPlayer);

    QObject::connect(m_availabilityControl, SIGNAL(availabilityChanged(QtMultimedia::AvailabilityError)),
                     this, SLOT(_q_availabilityChanged(QtMultimedia::AvailabilityError)));

    m_metaData.reset(new QDeclarativeMediaMetaData(m_metaDataControl));

    QObject::connect(m_metaDataControl, SIGNAL(metaDataChanged()),
            m_metaData.data(), SIGNAL(metaDataChanged()));

    if (m_mediaService) {
        connect(m_playerControl, SIGNAL(audioAvailableChanged(bool)),
                this, SIGNAL(hasAudioChanged()));
        connect(m_playerControl, SIGNAL(videoAvailableChanged(bool)),
                this, SIGNAL(hasVideoChanged()));
    }

    emit mediaObjectChanged();
}

void QDeclarativeAudio::componentComplete()
{
    if (!qFuzzyCompare(m_vol, qreal(1.0)))
        m_playerControl->setVolume(m_vol * 100);
    if (m_muted)
        m_playerControl->setMuted(m_muted);
    if (!qFuzzyCompare(m_playbackRate, qreal(1.0)))
        m_playerControl->setPlaybackRate(m_playbackRate);

    if (!m_source.isEmpty() && (m_autoLoad || m_autoPlay)) {
        m_playerControl->setMedia(m_source, 0);
        m_loaded = true;
        if (m_position > 0)
            m_playerControl->setPosition(m_position);
    }

    m_complete = true;

    if (m_autoPlay) {
        if (m_source.isEmpty()) {
            m_playerControl->stop();
        } else {
            m_playerControl->play();
        }
    }
}

void QDeclarativeAudio::_q_statusChanged()
{
    if (m_playerControl->mediaStatus() == QMediaPlayer::EndOfMedia && m_runningCount != 0) {
        m_runningCount -= 1;
        m_playerControl->play();
    }
    const QMediaPlayer::MediaStatus oldStatus = m_status;
    const QMediaPlayer::State lastPlaybackState = m_playbackState;

    const QMediaPlayer::State state = m_playerControl->state();

    m_playbackState = state;

    m_status = m_playerControl->mediaStatus();

    if (m_status != oldStatus)
        emit statusChanged();

    if (lastPlaybackState != state) {

        if (lastPlaybackState == QMediaPlayer::StoppedState)
            m_runningCount = m_loopCount - 1;

        switch (state) {
        case QMediaPlayer::StoppedState:
            emit stopped();
            break;
        case QMediaPlayer::PausedState:
            emit paused();
            break;
        case QMediaPlayer::PlayingState:
            emit playing();
            break;
        }

        emit playbackStateChanged();
    }

    // Check
    if ((state == QMediaPlayer::PlayingState)
            || m_status == QMediaPlayer::BufferingMedia
            || m_status == QMediaPlayer::StalledMedia) {
        m_animation->start();
    }
    else {
        m_animation->stop();
    }
}

/*!
    \qmlproperty string QtMultimedia5::Audio::errorString

    This property holds a string describing the current error condition in more detail.
*/

/*!
    \qmlsignal QtMultimedia5::Audio::onError(error, errorString)

    This handler is called when an \l {QMediaPlayer::Error}{error} has
    occurred.  The errorString parameter may contain more detailed
    information about the error.
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.title

    This property holds the tile of the media.

    \sa {QtMultimedia::MetaData::Title}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.subTitle

    This property holds the sub-title of the media.

    \sa {QtMultimedia::MetaData::SubTitle}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.author

    This property holds the author of the media.

    \sa {QtMultimedia::MetaData::Author}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.comment

    This property holds a user comment about the media.

    \sa {QtMultimedia::MetaData::Comment}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.description

    This property holds a description of the media.

    \sa {QtMultimedia::MetaData::Description}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.category

    This property holds the category of the media

    \sa {QtMultimedia::MetaData::Category}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.genre

    This property holds the genre of the media.

    \sa {QtMultimedia::MetaData::Genre}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.year

    This property holds the year of release of the media.

    \sa {QtMultimedia::MetaData::Year}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.date

    This property holds the date of the media.

    \sa {QtMultimedia::MetaData::Date}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.userRating

    This property holds a user rating of the media in the range of 0 to 100.

    \sa {QtMultimedia::MetaData::UserRating}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.keywords

    This property holds a list of keywords describing the media.

    \sa {QtMultimedia::MetaData::Keywords}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.language

    This property holds the language of the media, as an ISO 639-2 code.

    \sa {QtMultimedia::MetaData::Language}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.publisher

    This property holds the publisher of the media.

    \sa {QtMultimedia::MetaData::Publisher}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.copyright

    This property holds the media's copyright notice.

    \sa {QtMultimedia::MetaData::Copyright}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.parentalRating

    This property holds the parental rating of the media.

    \sa {QtMultimedia::MetaData::ParentalRating}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.ratingOrganization

    This property holds the name of the rating organization responsible for the
    parental rating of the media.

    \sa {QtMultimedia::MetaData::RatingOrganization}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.size

    This property property holds the size of the media in bytes.

    \sa {QtMultimedia::MetaData::Size}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.mediaType

    This property holds the type of the media.

    \sa {QtMultimedia::MetaData::MediaType}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.audioBitRate

    This property holds the bit rate of the media's audio stream ni bits per
    second.

    \sa {QtMultimedia::MetaData::AudioBitRate}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.audioCodec

    This property holds the encoding of the media audio stream.

    \sa {QtMultimedia::MetaData::AudioCodec}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.averageLevel

    This property holds the average volume level of the media.

    \sa {QtMultimedia::MetaData::AverageLevel}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.channelCount

    This property holds the number of channels in the media's audio stream.

    \sa {QtMultimedia::MetaData::ChannelCount}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.peakValue

    This property holds the peak volume of media's audio stream.

    \sa {QtMultimedia::MetaData::PeakValue}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.sampleRate

    This property holds the sample rate of the media's audio stream in hertz.

    \sa {QtMultimedia::MetaData::SampleRate}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.albumTitle

    This property holds the title of the album the media belongs to.

    \sa {QtMultimedia::MetaData::AlbumTitle}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.albumArtist

    This property holds the name of the principal artist of the album the media
    belongs to.

    \sa {QtMultimedia::MetaData::AlbumArtist}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.contributingArtist

    This property holds the names of artists contributing to the media.

    \sa {QtMultimedia::MetaData::ContributingArtist}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.composer

    This property holds the composer of the media.

    \sa {QtMultimedia::MetaData::Composer}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.conductor

    This property holds the conductor of the media.

    \sa {QtMultimedia::MetaData::Conductor}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.lyrics

    This property holds the lyrics to the media.

    \sa {QtMultimedia::MetaData::Lyrics}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.mood

    This property holds the mood of the media.

    \sa {QtMultimedia::MetaData::Mood}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.trackNumber

    This property holds the track number of the media.

    \sa {QtMultimedia::MetaData::TrackNumber}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.trackCount

    This property holds the number of track on the album containing the media.

    \sa {QtMultimedia::MetaData::TrackNumber}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.coverArtUrlSmall

    This property holds the URL of a small cover art image.

    \sa {QtMultimedia::MetaData::CoverArtUrlSmall}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.coverArtUrlLarge

    This property holds the URL of a large cover art image.

    \sa {QtMultimedia::MetaData::CoverArtUrlLarge}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.resolution

    This property holds the dimension of an image or video.

    \sa {QtMultimedia::MetaData::Resolution}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.pixelAspectRatio

    This property holds the pixel aspect ratio of an image or video.

    \sa {QtMultimedia::MetaData::PixelAspectRatio}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.videoFrameRate

    This property holds the frame rate of the media's video stream.

    \sa {QtMultimedia::MetaData::VideoFrameRate}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.videoBitRate

    This property holds the bit rate of the media's video stream in bits per
    second.

    \sa {QtMultimedia::MetaData::VideoBitRate}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.videoCodec

    This property holds the encoding of the media's video stream.

    \sa {QtMultimedia::MetaData::VideoCodec}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.posterUrl

    This property holds the URL of a poster image.

    \sa {QtMultimedia::MetaData::PosterUrl}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.chapterNumber

    This property holds the chapter number of the media.

    \sa {QtMultimedia::MetaData::ChapterNumber}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.director

    This property holds the director of the media.

    \sa {QtMultimedia::MetaData::Director}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.leadPerformer

    This property holds the lead performer in the media.

    \sa {QtMultimedia::MetaData::LeadPerformer}
*/

/*!
    \qmlproperty variant QtMultimedia5::Audio::metaData.writer

    This property holds the writer of the media.

    \sa {QtMultimedia::MetaData::Writer}
*/

QT_END_NAMESPACE

#include "moc_qdeclarativeaudio_p.cpp"


