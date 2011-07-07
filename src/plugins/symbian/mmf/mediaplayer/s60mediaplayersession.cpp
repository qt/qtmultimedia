/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "DebugMacros.h"

#include "s60mediaplayersession.h"

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qvariant.h>
#include <QtCore/qtimer.h>
#include <mmf/common/mmferrors.h>
#include <qmediatimerange.h>

/*!
    Construct a media playersession with the given \a parent.
*/

S60MediaPlayerSession::S60MediaPlayerSession(QObject *parent)
    : QObject(parent)
    , m_stream(false)
    , m_playbackRate(0)
    , m_muted(false)
    , m_volume(0)
    , m_state(QMediaPlayer::StoppedState)
    , m_mediaStatus(QMediaPlayer::NoMedia)
    , m_progressTimer(new QTimer(this))
    , m_stalledTimer(new QTimer(this))
    , m_error(KErrNone)
    , m_play_requested(false)
    , m_seekable(true)
{
    DP0("S60MediaPlayerSession::S60MediaPlayerSession +++");

    connect(m_progressTimer, SIGNAL(timeout()), this, SLOT(tick()));
    connect(m_stalledTimer, SIGNAL(timeout()), this, SLOT(stalled()));

    DP0("S60MediaPlayerSession::S60MediaPlayerSession ---");
}

/*!
    Destroys a media playersession.
*/

S60MediaPlayerSession::~S60MediaPlayerSession()
{
    DP0("S60MediaPlayerSession::~S60MediaPlayerSession +++");
    DP0("S60MediaPlayerSession::~S60MediaPlayerSession ---");
}

/*!
 * \return the audio volume of a player session.
*/
int S60MediaPlayerSession::volume() const
{
    DP1("S60MediaPlayerSession::volume", m_volume);

    return m_volume;
}

/*!
    Sets the audio \a volume of a player session.
*/

void S60MediaPlayerSession::setVolume(int volume)
{
    DP0("S60MediaPlayerSession::setVolume +++");

    DP1("S60MediaPlayerSession::setVolume - ", volume);

    if (m_volume == volume)
        return;
    
    m_volume = volume;
    emit volumeChanged(m_volume);

    // Dont set symbian players volume until media loaded.
    // Leaves with KerrNotReady although documentation says otherwise.
    if (!m_muted && 
        (  mediaStatus() == QMediaPlayer::LoadedMedia 
        || (mediaStatus() == QMediaPlayer::StalledMedia && state() != QMediaPlayer::StoppedState)
        || mediaStatus() == QMediaPlayer::BufferingMedia
        || mediaStatus() == QMediaPlayer::BufferedMedia
        || mediaStatus() == QMediaPlayer::EndOfMedia)) {
        TRAPD(err, doSetVolumeL(m_volume));
        setError(err);
    }
    DP0("S60MediaPlayerSession::setVolume ---");
}

/*!
    \return the mute state of a player session.
*/

bool S60MediaPlayerSession::isMuted() const
{
    DP1("S60MediaPlayerSession::isMuted", m_muted);

    return m_muted;
}

/*!
    Identifies if the current media is seekable.

    \return true if it possible to seek within the current media, and false otherwise.
*/

bool S60MediaPlayerSession::isSeekable() const
{
    DP1("S60MediaPlayerSession::isSeekable", m_seekable);

    return m_seekable;
}

/*!
    Sets the \a status of the current media.
*/

void S60MediaPlayerSession::setMediaStatus(QMediaPlayer::MediaStatus status)
{
    DP0("S60MediaPlayerSession::setMediaStatus +++");

    if (m_mediaStatus == status)
        return;
    
    m_mediaStatus = status;
    
    emit mediaStatusChanged(m_mediaStatus);
    
    if (m_play_requested && m_mediaStatus == QMediaPlayer::LoadedMedia)
        play();

    DP0("S60MediaPlayerSession::setMediaStatus ---");
}

/*!
    Sets the \a state on media player.
*/

void S60MediaPlayerSession::setState(QMediaPlayer::State state)
{
    DP0("S60MediaPlayerSession::setState +++");

    if (m_state == state)
        return;
    
    m_state = state;
    emit stateChanged(m_state);

    DP0("S60MediaPlayerSession::setState ---");
}

/*!
    \return the state of a player session.
*/

QMediaPlayer::State S60MediaPlayerSession::state() const
{
    DP1("S60MediaPlayerSession::state", m_state);

    return m_state;
}

/*!
    \return the status of the current media.
*/

QMediaPlayer::MediaStatus S60MediaPlayerSession::mediaStatus() const
{
    DP1("S60MediaPlayerSession::mediaStatus", m_mediaStatus);

    return m_mediaStatus;
}

/*!
 *  Loads the \a url for playback.
 *  If \a url is local file then it loads audio playersesion if its audio file.
 *  If it is a local video file then loads the video playersession.
*/

void S60MediaPlayerSession::load(const QMediaContent source)
{
    DP0("S60MediaPlayerSession::load +++");

    m_source = source;
    setMediaStatus(QMediaPlayer::LoadingMedia);
    startStalledTimer();
    m_stream = (source.canonicalUrl().scheme() == "file")?false:true;
    m_UrlPath = source.canonicalUrl();
    TRAPD(err,
        if (m_stream)
            doLoadUrlL(QString2TPtrC(source.canonicalUrl().toString()));
        else
            doLoadL(QString2TPtrC(QDir::toNativeSeparators(source.canonicalUrl().toLocalFile()))));
    setError(err);

    DP0("S60MediaPlayerSession::load ---");
}

TBool S60MediaPlayerSession::isStreaming()
{
    return m_stream;
}

/*!
    Start or resume playing the current source.
*/
void S60MediaPlayerSession::play()
{
    DP0("S60MediaPlayerSession::play +++");

    if ( (state() == QMediaPlayer::PlayingState && m_play_requested == false)
        || mediaStatus() == QMediaPlayer::UnknownMediaStatus
        || mediaStatus() == QMediaPlayer::NoMedia
        || mediaStatus() == QMediaPlayer::InvalidMedia)
        return;

    setState(QMediaPlayer::PlayingState);

    if (mediaStatus() == QMediaPlayer::LoadingMedia ||
       (mediaStatus() == QMediaPlayer::StalledMedia &&
        state() == QMediaPlayer::StoppedState))
   {
        m_play_requested = true;
        return;
    }

    m_play_requested = false;
    m_duration = duration();
    setVolume(m_volume);
    setMuted(m_muted);
    startProgressTimer();
    doPlay();

    DP0("S60MediaPlayerSession::play ---");
}

/*!
    Pause playing the current source.
*/

void S60MediaPlayerSession::pause()
{
    DP0("S60MediaPlayerSession::pause +++");

    if (state() != QMediaPlayer::PlayingState)
        return;

    if (mediaStatus() == QMediaPlayer::NoMedia ||
        mediaStatus() == QMediaPlayer::InvalidMedia)
        return;

    setState(QMediaPlayer::PausedState);
    stopProgressTimer();
    TRAP_IGNORE(doPauseL());
    m_play_requested = false;

    DP0("S60MediaPlayerSession::pause ---");
}

/*!
    Stop playing, and reset the play position to the beginning.
*/

void S60MediaPlayerSession::stop()
{
    DP0("S60MediaPlayerSession::stop +++");

    if (state() == QMediaPlayer::StoppedState)
        return;

    m_play_requested = false;
    m_state = QMediaPlayer::StoppedState;
    if (mediaStatus() == QMediaPlayer::BufferingMedia ||
        mediaStatus() == QMediaPlayer::BufferedMedia ||
        mediaStatus() == QMediaPlayer::StalledMedia) 
        setMediaStatus(QMediaPlayer::LoadedMedia);
    if (mediaStatus() == QMediaPlayer::LoadingMedia)
        setMediaStatus(QMediaPlayer::UnknownMediaStatus);    
    stopProgressTimer();
    stopStalledTimer();
    doStop();
    emit positionChanged(0);
    emit stateChanged(m_state);

    DP0("S60MediaPlayerSession::stop ---");
}

/*!
 * Stops the playback and closes the controllers.
 * And resets all the flags and status, state to default values.
*/

void S60MediaPlayerSession::reset()
{
    DP0("S60MediaPlayerSession::reset +++");

    m_play_requested = false;
    setError(KErrNone, QString(), true);
    stopProgressTimer();
    stopStalledTimer();
    doStop();
    doClose();
    setState(QMediaPlayer::StoppedState);
    setMediaStatus(QMediaPlayer::UnknownMediaStatus);
    setPosition(0);

    DP0("S60MediaPlayerSession::reset ---");
}

/*!
 * Sets \a renderer as video renderer.
*/

void S60MediaPlayerSession::setVideoRenderer(QObject *renderer)
{
    DP0("S60MediaPlayerSession::setVideoRenderer +++");

    Q_UNUSED(renderer);

    DP0("S60MediaPlayerSession::setVideoRenderer ---");
}

/*!
 *   the percentage of the temporary buffer filled before playback begins.

    When the player object is buffering; this property holds the percentage of
    the temporary buffer that is filled. The buffer will need to reach 100%
    filled before playback can resume, at which time the MediaStatus will be
    BufferedMedia.

    \sa mediaStatus()
*/

int S60MediaPlayerSession::bufferStatus()
{
    DP0("S60MediaPlayerSession::bufferStatus");

    if (state() ==QMediaPlayer::StoppedState)
        return 0;

    if(   mediaStatus() == QMediaPlayer::LoadingMedia
       || mediaStatus() == QMediaPlayer::UnknownMediaStatus
       || mediaStatus() == QMediaPlayer::NoMedia
       || mediaStatus() == QMediaPlayer::InvalidMedia)
        return 0;

    int progress = 0;
    TRAPD(err, progress = doGetBufferStatusL());
    // If buffer status query not supported by codec return 100
    // do not set error
    if(err == KErrNotSupported)
        return 100;

    setError(err);
    return progress;
}

/*!
 * return TRUE if Meta data is available in current media source.
*/

bool S60MediaPlayerSession::isMetadataAvailable() const
{
    DP0("S60MediaPlayerSession::isMetadataAvailable");

    return !m_metaDataMap.isEmpty();
}

/*!
 * \return the \a key meta data.
*/
QVariant S60MediaPlayerSession::metaData(const QString &key) const
{
    DP0("S60MediaPlayerSession::metaData (const QString &key) const");

    return m_metaDataMap.value(key);
}

/*!
 * \return the \a key meta data as QString.
*/

QVariant S60MediaPlayerSession::metaData(QtMultimediaKit::MetaData key) const
{
    DP0("S60MediaPlayerSession::metaData (QtMultimediaKit::MetaData key) const");

    return metaData(metaDataKeyAsString(key));
}

/*!
 * \return List of all available meta data from current media source.
*/

QList<QtMultimediaKit::MetaData> S60MediaPlayerSession::availableMetaData() const
{
    DP0("S60MediaPlayerSession::availableMetaData +++");

    QList<QtMultimediaKit::MetaData> metaDataTags;
    if (isMetadataAvailable()) {
        for (int i = QtMultimediaKit::Title; i <= QtMultimediaKit::ThumbnailImage; i++) {
            QString metaDataItem = metaDataKeyAsString((QtMultimediaKit::MetaData)i);
            if (!metaDataItem.isEmpty()) {
                if (!metaData(metaDataItem).isNull()) {
                    metaDataTags.append((QtMultimediaKit::MetaData)i);
                }
            }
        }
    }

    DP0("S60MediaPlayerSession::availableMetaData ---");

    return metaDataTags;
}

/*!
 * \return all available extended meta data of current media source.
*/

QStringList S60MediaPlayerSession::availableExtendedMetaData() const
{
    DP0("S60MediaPlayerSession::availableExtendedMetaData");

    return m_metaDataMap.keys();
}

/*!
 * \return meta data \a key as QString.
*/

QString S60MediaPlayerSession::metaDataKeyAsString(QtMultimediaKit::MetaData key) const
{
    DP1("S60MediaPlayerSession::metaDataKeyAsString", key);

    switch(key) {
    case QtMultimediaKit::Title: return "title";
    case QtMultimediaKit::AlbumArtist: return "artist";
    case QtMultimediaKit::Comment: return "comment";
    case QtMultimediaKit::Genre: return "genre";
    case QtMultimediaKit::Year: return "year";
    case QtMultimediaKit::Copyright: return "copyright";
    case QtMultimediaKit::AlbumTitle: return "album";
    case QtMultimediaKit::Composer: return "composer";
    case QtMultimediaKit::TrackNumber: return "albumtrack";
    case QtMultimediaKit::AudioBitRate: return "audiobitrate";
    case QtMultimediaKit::VideoBitRate: return "videobitrate";
    case QtMultimediaKit::Duration: return "duration";
    case QtMultimediaKit::MediaType: return "contenttype";
    case QtMultimediaKit::CoverArtImage: return "attachedpicture";
    case QtMultimediaKit::SubTitle: // TODO: Find the matching metadata keys
    case QtMultimediaKit::Description:
    case QtMultimediaKit::Category:
    case QtMultimediaKit::Date:
    case QtMultimediaKit::UserRating:
    case QtMultimediaKit::Keywords:
    case QtMultimediaKit::Language:
    case QtMultimediaKit::Publisher:
    case QtMultimediaKit::ParentalRating:
    case QtMultimediaKit::RatingOrganisation:
    case QtMultimediaKit::Size:
    case QtMultimediaKit::AudioCodec:
    case QtMultimediaKit::AverageLevel:
    case QtMultimediaKit::ChannelCount:
    case QtMultimediaKit::PeakValue:
    case QtMultimediaKit::SampleRate:
    case QtMultimediaKit::Author:
    case QtMultimediaKit::ContributingArtist:
    case QtMultimediaKit::Conductor:
    case QtMultimediaKit::Lyrics:
    case QtMultimediaKit::Mood:
    case QtMultimediaKit::TrackCount:
    case QtMultimediaKit::CoverArtUrlSmall:
    case QtMultimediaKit::CoverArtUrlLarge:
    case QtMultimediaKit::Resolution:
    case QtMultimediaKit::PixelAspectRatio:
    case QtMultimediaKit::VideoFrameRate:
    case QtMultimediaKit::VideoCodec:
    case QtMultimediaKit::PosterUrl:
    case QtMultimediaKit::ChapterNumber:
    case QtMultimediaKit::Director:
    case QtMultimediaKit::LeadPerformer:
    case QtMultimediaKit::Writer:
    case QtMultimediaKit::CameraManufacturer:
    case QtMultimediaKit::CameraModel:
    case QtMultimediaKit::Event:
    case QtMultimediaKit::Subject:
    default:
        break;
    }

    return QString();
}

/*!
    Sets the \a muted state of a player session.
*/

void S60MediaPlayerSession::setMuted(bool muted)
{
    DP0("S60MediaPlayerSession::setMuted +++");
    DP1("S60MediaPlayerSession::setMuted - ", muted);

    m_muted = muted;
    emit mutedChanged(m_muted);

    if(   m_mediaStatus == QMediaPlayer::LoadedMedia 
       || (m_mediaStatus == QMediaPlayer::StalledMedia && state() != QMediaPlayer::StoppedState)
       || m_mediaStatus == QMediaPlayer::BufferingMedia
       || m_mediaStatus == QMediaPlayer::BufferedMedia
       || m_mediaStatus == QMediaPlayer::EndOfMedia) {
        TRAPD(err, doSetVolumeL((m_muted)?0:m_volume));
        setError(err);
    }
    DP0("S60MediaPlayerSession::setMuted ---");
}

/*!
   \return the duration of the current media in milliseconds.
*/

qint64 S60MediaPlayerSession::duration() const
{
  //  DP0("S60MediaPlayerSession::duration");

    if(   mediaStatus() == QMediaPlayer::LoadingMedia
       || mediaStatus() == QMediaPlayer::UnknownMediaStatus
       || mediaStatus() == QMediaPlayer::NoMedia
       || (mediaStatus() == QMediaPlayer::StalledMedia && state() == QMediaPlayer::StoppedState)
       || mediaStatus() == QMediaPlayer::InvalidMedia)
        return -1;

    qint64 pos = 0;
    TRAP_IGNORE(pos = doGetDurationL());
    return pos;
}

/*!
    \return the current playback position in milliseconds.
*/

qint64 S60MediaPlayerSession::position() const
{
 //   DP0("S60MediaPlayerSession::position");

    if(   mediaStatus() == QMediaPlayer::LoadingMedia
       || mediaStatus() == QMediaPlayer::UnknownMediaStatus
       || mediaStatus() == QMediaPlayer::NoMedia
       || (mediaStatus() == QMediaPlayer::StalledMedia && state() == QMediaPlayer::StoppedState)
       || mediaStatus() == QMediaPlayer::InvalidMedia)
        return 0;
    
    qint64 pos = 0;
    TRAP_IGNORE(pos = doGetPositionL());
    if (!m_play_requested && pos ==0
        && mediaStatus() != QMediaPlayer::LoadedMedia)
        return m_duration;
    return pos;
}

/*!
    Sets the playback \a pos of the current media.  This will initiate a seek and it may take
    some time for playback to reach the position set.
*/

void S60MediaPlayerSession::setPosition(qint64 pos)
{
    DP0("S60MediaPlayerSession::setPosition +++");

    DP1("S60MediaPlayerSession::setPosition - ", pos);

    if (position() == pos)
        return;

    QMediaPlayer::State originalState = state();

    if (originalState == QMediaPlayer::PlayingState) 
        pause();

    TRAPD(err, doSetPositionL(pos * 1000));
    setError(err);

    if (err == KErrNone) {
        if (mediaStatus() == QMediaPlayer::EndOfMedia)
            setMediaStatus(QMediaPlayer::LoadedMedia);
    }
    else if (err == KErrNotSupported) {
        m_seekable = false;
        emit seekableChanged(m_seekable);
    }

    if (originalState == QMediaPlayer::PlayingState)
        play();

    emit positionChanged(position());

    DP0("S60MediaPlayerSession::setPosition ---");
}

/*!
 * Set the audio endpoint to \a audioEndpoint.
*/

void S60MediaPlayerSession::setAudioEndpoint(const QString& audioEndpoint)
{
    DP0("S60MediaPlayerSession::setAudioEndpoint +++");

    DP1("S60MediaPlayerSession::setAudioEndpoint - ", audioEndpoint);

    doSetAudioEndpoint(audioEndpoint);

    DP0("S60MediaPlayerSession::setAudioEndpoint ---");
}

/*!
 * Loading of media source is completed.
 * And ready for playback. Updates all the media status, state, settings etc.
 * And emits the signals.
*/

void S60MediaPlayerSession::loaded()
{
    DP0("S60MediaPlayerSession::loaded +++");

    stopStalledTimer();
    if (m_error == KErrNone || m_error == KErrMMPartialPlayback) {
        setMediaStatus(QMediaPlayer::LoadedMedia);
        TRAPD(err, updateMetaDataEntriesL());
        setError(err);
        emit durationChanged(duration());
        emit positionChanged(0);
        emit videoAvailableChanged(isVideoAvailable());
        emit audioAvailableChanged(isAudioAvailable());
        emit mediaChanged();

        m_seekable = getIsSeekable();
    }

    DP0("S60MediaPlayerSession::loaded ---");
}

/*!
 * Playback is completed as medai source reached end of media.
*/
void S60MediaPlayerSession::endOfMedia()
{
    DP0("S60MediaPlayerSession::endOfMedia +++");

    m_state = QMediaPlayer::StoppedState;
    setMediaStatus(QMediaPlayer::EndOfMedia);
    //there is a chance that user might have called play from EOF callback
    //if we are already in playing state, do not send state change callback
    if(m_state == QMediaPlayer::StoppedState)
        emit stateChanged(QMediaPlayer::StoppedState);
    emit positionChanged(m_duration);

    DP0("S60MediaPlayerSession::endOfMedia ---");
}

/*!
 *  The percentage of the temporary buffer filling before playback begins.

    When the player object is buffering; this property holds the percentage of
    the temporary buffer that is filled. The buffer will need to reach 100%
    filled before playback can resume, at which time the MediaStatus will be
    BufferedMedia.

    \sa mediaStatus()
*/

void S60MediaPlayerSession::buffering()
{
    DP0("S60MediaPlayerSession::buffering +++");

    startStalledTimer();
    setMediaStatus(QMediaPlayer::BufferingMedia);

//Buffering cannot happen in stopped state. Hence update the state
    if (state() == QMediaPlayer::StoppedState)
        setState(QMediaPlayer::PausedState);

    DP0("S60MediaPlayerSession::buffering ---");
}

/*!
 * Buffer is filled with data and to for continuing/start playback.
*/

void S60MediaPlayerSession::buffered()
{
    DP0("S60MediaPlayerSession::buffered +++");

    stopStalledTimer();
    setMediaStatus(QMediaPlayer::BufferedMedia);

    DP0("S60MediaPlayerSession::buffered ---");
}

/*!
 * Sets media status as stalled as waiting for the buffer to be filled to start playback.
*/

void S60MediaPlayerSession::stalled()
{
    DP0("S60MediaPlayerSession::stalled +++");

    setMediaStatus(QMediaPlayer::StalledMedia);

    DP0("S60MediaPlayerSession::stalled ---");
}

/*!
 * \return all the meta data entries in the current media source.
*/

QMap<QString, QVariant>& S60MediaPlayerSession::metaDataEntries()
{
    DP0("S60MediaPlayerSession::metaDataEntries");

    return m_metaDataMap;
}

/*!
 * \return Error by converting Symbian specific error to Multimedia error.
*/

QMediaPlayer::Error S60MediaPlayerSession::fromSymbianErrorToMultimediaError(int error)
{
    DP0("S60MediaPlayerSession::fromSymbianErrorToMultimediaError");

    DP1("S60MediaPlayerSession::fromSymbianErrorToMultimediaError - ", error);

    switch(error) {
        case KErrNoMemory:
        case KErrNotFound:
        case KErrBadHandle:
        case KErrAbort:
        case KErrNotSupported:
        case KErrCorrupt:
        case KErrGeneral:
        case KErrArgument:
        case KErrPathNotFound:
        case KErrDied:
        case KErrServerTerminated:
        case KErrServerBusy:
        case KErrCompletion:  
        case KErrBadPower:    
        case KErrMMInvalidProtocol:
        case KErrMMInvalidURL:
            return QMediaPlayer::ResourceError;
        
        case KErrMMPartialPlayback:   
            return QMediaPlayer::FormatError;

        case KErrMMAudioDevice:
        case KErrMMVideoDevice:
        case KErrMMDecoder:
        case KErrUnknown:    
            return QMediaPlayer::ServiceMissingError;
            
        case KErrMMNotEnoughBandwidth:
        case KErrMMSocketServiceNotFound:
        case KErrMMNetworkRead:
        case KErrMMNetworkWrite:
        case KErrMMServerSocket:
        case KErrMMServerNotSupported:
        case KErrMMUDPReceive:
        case KErrMMMulticast:
        case KErrMMProxyServer:
        case KErrMMProxyServerNotSupported:
        case KErrMMProxyServerConnect:
        case KErrCouldNotConnect:
            return QMediaPlayer::NetworkError;

        case KErrNotReady:
        case KErrInUse:
        case KErrAccessDenied:
        case KErrLocked:
        case KErrMMDRMNotAuthorized:
        case KErrPermissionDenied:
        case KErrCancel:
        case KErrAlreadyExists:
            return QMediaPlayer::AccessDeniedError;

        case KErrNone:
            return QMediaPlayer::NoError;

        default:
            return QMediaPlayer::ResourceError;
    }
}

/*!
 * \return error.
 */

int S60MediaPlayerSession::error() const
{
    DP1("S60MediaPlayerSession::error", m_error);

    return m_error;
}

/*!
 * Sets the error.
 * * If playback complete/prepare complete ..., etc with successful then sets error as ZERO
 *  else Multimedia error.
*/

void S60MediaPlayerSession::setError(int error, const QString &errorString, bool forceReset)
{
    DP0("S60MediaPlayerSession::setError +++");

    DP5("S60MediaPlayerSession::setError - error:", error,"errorString:", errorString, "forceReset:", forceReset);

    if( forceReset ) {
        m_error = KErrNone;
        emit this->error(QMediaPlayer::NoError, QString());
        return;
    }

    // If error does not change and m_error is reseted without forceReset flag
    if (error == m_error || 
        (m_error != KErrNone && error == KErrNone))
        return;

    m_error = error;
    QMediaPlayer::Error mediaError = fromSymbianErrorToMultimediaError(m_error);
    QString symbianError = QString(errorString);

    if (mediaError != QMediaPlayer::NoError) {
        // TODO: fix to user friendly string at some point
        // These error string are only dev usable
        symbianError.append("Symbian:");
        symbianError.append(QString::number(m_error));
    }

    emit this->error(mediaError, symbianError);

    if (m_error == KErrInUse) {
        pause();
    } else if (mediaError != QMediaPlayer::NoError) {
        m_play_requested = false;
        setMediaStatus(QMediaPlayer::InvalidMedia);
        stop();
    }
}

void S60MediaPlayerSession::setAndEmitError(int error)
{
    m_error = error;
    QMediaPlayer::Error rateError = fromSymbianErrorToMultimediaError(error);
    QString symbianError;
    symbianError.append("Symbian:");
    symbianError.append(QString::number(error));
    emit this->error(rateError, symbianError);

    DP0("S60MediaPlayerSession::setError ---");
}

/*!
 * emits the signal if there is a changes in position and buffering status.
 */

void S60MediaPlayerSession::tick()
{
    DP0("S60MediaPlayerSession::tick +++");

    emit positionChanged(position());

    if (bufferStatus() < 100)
        emit bufferStatusChanged(bufferStatus());

    DP0("S60MediaPlayerSession::tick ---");
}

/*!
 * Starts the timer once the media source starts buffering.
*/

void S60MediaPlayerSession::startProgressTimer()
{
    DP0("S60MediaPlayerSession::startProgressTimer +++");

    m_progressTimer->start(500);

    DP0("S60MediaPlayerSession::startProgressTimer ---");
}

/*!
 * Stops the timer once the media source finished buffering.
*/

void S60MediaPlayerSession::stopProgressTimer()
{
    DP0("S60MediaPlayerSession::stopProgressTimer +++");

    m_progressTimer->stop();

    DP0("S60MediaPlayerSession::stopProgressTimer ---");
}

/*!
 * Starts the timer while waiting for some events to happen like source buffering or call backs etc.
 * So that if the events doesn't occur before stalled timer stops, it'll set the error/media status etc.
*/

void S60MediaPlayerSession::startStalledTimer()
{
    DP0("S60MediaPlayerSession::startStalledTimer +++");

    m_stalledTimer->start(30000);

    DP0("S60MediaPlayerSession::startStalledTimer ---");
}

/*!
 *  Stops the timer when some events occurred while waiting for them.
 *  media source started buffering or call back is received etc.
*/

void S60MediaPlayerSession::stopStalledTimer()
{
    DP0("S60MediaPlayerSession::stopStalledTimer +++");

    m_stalledTimer->stop();

    DP0("S60MediaPlayerSession::stopStalledTimer ---");
}

/*!
 * \return Converted Symbian specific Descriptor to QString.
*/

QString S60MediaPlayerSession::TDesC2QString(const TDesC& aDescriptor)
{
    DP0("S60MediaPlayerSession::TDesC2QString");

    return QString::fromUtf16(aDescriptor.Ptr(), aDescriptor.Length());
}

/*!
 * \return Converted QString to non-modifiable pointer Descriptor.
*/

TPtrC S60MediaPlayerSession::QString2TPtrC( const QString& string )
{
    DP0("S60MediaPlayerSession::QString2TPtrC");

	// Returned TPtrC is valid as long as the given parameter is valid and unmodified
    return TPtrC16(static_cast<const TUint16*>(string.utf16()), string.length());
}

/*!
 * \return Converted Symbian TRect object to QRect object.
*/

QRect S60MediaPlayerSession::TRect2QRect(const TRect& tr)
{
    DP0("S60MediaPlayerSession::TRect2QRect");

    return QRect(tr.iTl.iX, tr.iTl.iY, tr.Width(), tr.Height());
}

/*!
 * \return converted QRect object to Symbian specific TRec object.
 */

TRect S60MediaPlayerSession::QRect2TRect(const QRect& qr)
{
    DP0("S60MediaPlayerSession::QRect2TRect");

    return TRect(TPoint(qr.left(), qr.top()), TSize(qr.width(), qr.height()));
}

/*!
    \fn bool S60MediaPlayerSession::isVideoAvailable();


    Returns TRUE if Video is available.
*/

/*!
    \fn bool S60MediaPlayerSession::isAudioAvailable();


    Returns TRUE if Audio is available.
*/

/*!
    \fn void S60MediaPlayerSession::setPlaybackRate (qreal rate);


    Sets \a rate play back rate on media source. getIsSeekable
*/

/*!
    \fn bool S60MediaPlayerSession::getIsSeekable () const;


    \return TRUE if Seekable possible on current media source else FALSE.
*/

/*!
    \fn QString S60MediaPlayerSession::activeEndpoint () const;


    \return active end point name..
*/

/*!
    \fn QString S60MediaPlayerSession::defaultEndpoint () const;


    \return default end point name.
*/

