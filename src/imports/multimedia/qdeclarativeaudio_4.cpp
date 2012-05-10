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

#include "qdeclarativeaudio_p_4.h"

#include <qmediaplayercontrol.h>

QT_BEGIN_NAMESPACE


/*
    \qmlclass Audio QDeclarativeAudio
    \brief The Audio element allows you to add audio playback to a scene.

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

/*
    \internal
    \class QDeclarativeAudio
    \brief The QDeclarativeAudio class provides an audio item that you can add to a QQuickView.
*/

void QDeclarativeAudio_4::_q_error(int errorCode, const QString &errorString)
{
    m_error = QMediaPlayer::Error(errorCode);
    m_errorString = errorString;

    emit error(Error(errorCode), errorString);
    emit errorChanged();
}


QDeclarativeAudio_4::QDeclarativeAudio_4(QObject *parent)
    : QObject(parent)
{
}

QDeclarativeAudio_4::~QDeclarativeAudio_4()
{
    shutdown();
}

/*
    \qmlmethod Audio::play()

    Starts playback of the media.

    Sets the \l playing property to true, and the \l paused property to false.
*/

void QDeclarativeAudio_4::play()
{
    if (!m_complete)
        return;

    setPaused(false);
    setPlaying(true);
}

/*
    \qmlmethod Audio::pause()

    Pauses playback of the media.

    Sets the \l playing and \l paused properties to true.
*/

void QDeclarativeAudio_4::pause()
{
    if (!m_complete)
        return;

    setPaused(true);
    setPlaying(true);
}

/*
    \qmlmethod Audio::stop()

    Stops playback of the media.

    Sets the \l playing and \l paused properties to false.
*/

void QDeclarativeAudio_4::stop()
{
    if (!m_complete)
        return;

    setPlaying(false);
    setPaused(false);
}

/*
    \qmlproperty url Audio::source

    This property holds the source URL of the media.
*/

/*
    \qmlproperty url Audio::autoLoad

    This property indicates if loading of media should begin immediately.

    Defaults to true, if false media will not be loaded until playback is started.
*/

/*
    \qmlproperty bool Audio::playing

    This property holds whether the media is playing.

    Defaults to false, and can be set to true to start playback.
*/

/*
    \qmlproperty bool Audio::paused

    This property holds whether the media is paused.

    Defaults to false, and can be set to true to pause playback.
*/

/*
    \qmlsignal Audio::onStarted()

    This handler is called when playback is started.
*/

/*
    \qmlsignal Audio::onResumed()

    This handler is called when playback is resumed from the paused state.
*/

/*
    \qmlsignal Audio::onPaused()

    This handler is called when playback is paused.
*/

/*
    \qmlsignal Audio::onStopped()

    This handler is called when playback is stopped.
*/

/*
    \qmlproperty enumeration Audio::status

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

QDeclarativeAudio_4::Status QDeclarativeAudio_4::status() const
{
    return Status(m_status);
}

/*
    \qmlproperty int Audio::duration

    This property holds the duration of the media in milliseconds.

    If the media doesn't have a fixed duration (a live stream for example) this will be 0.
*/

/*
    \qmlproperty int Audio::position

    This property holds the current playback position in milliseconds.

    If the \l seekable property is true, this property can be set to seek to a new position.
*/

/*
    \qmlproperty real Audio::volume

    This property holds the volume of the audio output, from 0.0 (silent) to 1.0 (maximum volume).
*/

/*
    \qmlproperty bool Audio::muted

    This property holds whether the audio output is muted.
*/

/*
    \qmlproperty bool Audio::hasAudio

    This property holds whether the media contains audio.
*/

bool QDeclarativeAudio_4::hasAudio() const
{
    return !m_complete ? false : m_playerControl->isAudioAvailable();
}

/*
    \qmlproperty bool Audio::hasVideo

    This property holds whether the media contains video.
*/

bool QDeclarativeAudio_4::hasVideo() const
{
    return !m_complete ? false : m_playerControl->isVideoAvailable();
}

/*
    \qmlproperty real Audio::bufferProgress

    This property holds how much of the data buffer is currently filled, from 0.0 (empty) to 1.0
    (full).
*/

/*
    \qmlproperty bool Audio::seekable

    This property holds whether position of the audio can be changed.

    If true; setting a \l position value will cause playback to seek to the new position.
*/

/*
    \qmlproperty real Audio::playbackRate

    This property holds the rate at which audio is played at as a multiple of the normal rate.
*/

/*
    \qmlproperty enumeration Audio::error

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

QDeclarativeAudio_4::Error QDeclarativeAudio_4::error() const
{
    return Error(m_error);
}

void QDeclarativeAudio_4::classBegin()
{
    setObject(this);

    if (m_mediaService) {
        connect(m_playerControl, SIGNAL(audioAvailableChanged(bool)),
                this, SIGNAL(hasAudioChanged()));
        connect(m_playerControl, SIGNAL(videoAvailableChanged(bool)),
                this, SIGNAL(hasVideoChanged()));
    }

    emit mediaObjectChanged();
}

void QDeclarativeAudio_4::componentComplete()
{
    QDeclarativeMediaBase_4::componentComplete();
}


/*
    \qmlproperty string Audio::errorString

    This property holds a string describing the current error condition in more detail.
*/

/*
    \qmlsignal Audio::onError(error, errorString)

    This handler is called when an \l {QMediaPlayer::Error}{error} has
    occurred.  The errorString parameter may contain more detailed
    information about the error.
*/

/*
    \qmlproperty variant Audio::metaData.title

    This property holds the tile of the media.

    \sa {QtMultimedia::MetaData::Title}
*/

/*
    \qmlproperty variant Audio::metaData.subTitle

    This property holds the sub-title of the media.

    \sa {QtMultimedia::MetaData::SubTitle}
*/

/*
    \qmlproperty variant Audio::metaData.author

    This property holds the author of the media.

    \sa {QtMultimedia::MetaData::Author}
*/

/*
    \qmlproperty variant Audio::metaData.comment

    This property holds a user comment about the media.

    \sa {QtMultimedia::MetaData::Comment}
*/

/*
    \qmlproperty variant Audio::metaData.description

    This property holds a description of the media.

    \sa {QtMultimedia::MetaData::Description}
*/

/*
    \qmlproperty variant Audio::metaData.category

    This property holds the category of the media

    \sa {QtMultimedia::MetaData::Category}
*/

/*
    \qmlproperty variant Audio::metaData.genre

    This property holds the genre of the media.

    \sa {QtMultimedia::MetaData::Genre}
*/

/*
    \qmlproperty variant Audio::metaData.year

    This property holds the year of release of the media.

    \sa {QtMultimedia::MetaData::Year}
*/

/*
    \qmlproperty variant Audio::metaData.date

    This property holds the date of the media.

    \sa {QtMultimedia::MetaData::Date}
*/

/*
    \qmlproperty variant Audio::metaData.userRating

    This property holds a user rating of the media in the range of 0 to 100.

    \sa {QtMultimedia::MetaData::UserRating}
*/

/*
    \qmlproperty variant Audio::metaData.keywords

    This property holds a list of keywords describing the media.

    \sa {QtMultimedia::MetaData::Keywords}
*/

/*
    \qmlproperty variant Audio::metaData.language

    This property holds the language of the media, as an ISO 639-2 code.

    \sa {QtMultimedia::MetaData::Language}
*/

/*
    \qmlproperty variant Audio::metaData.publisher

    This property holds the publisher of the media.

    \sa {QtMultimedia::MetaData::Publisher}
*/

/*
    \qmlproperty variant Audio::metaData.copyright

    This property holds the media's copyright notice.

    \sa {QtMultimedia::MetaData::Copyright}
*/

/*
    \qmlproperty variant Audio::metaData.parentalRating

    This property holds the parental rating of the media.

    \sa {QtMultimedia::MetaData::ParentalRating}
*/

/*
    \qmlproperty variant Audio::metaData.ratingOrganization

    This property holds the name of the rating organization responsible for the
    parental rating of the media.

    \sa {QtMultimedia::MetaData::RatingOrganization}
*/

/*
    \qmlproperty variant Audio::metaData.size

    This property property holds the size of the media in bytes.

    \sa {QtMultimedia::MetaData::Size}
*/

/*
    \qmlproperty variant Audio::metaData.mediaType

    This property holds the type of the media.

    \sa {QtMultimedia::MetaData::MediaType}
*/

/*
    \qmlproperty variant Audio::metaData.audioBitRate

    This property holds the bit rate of the media's audio stream ni bits per
    second.

    \sa {QtMultimedia::MetaData::AudioBitRate}
*/

/*
    \qmlproperty variant Audio::metaData.audioCodec

    This property holds the encoding of the media audio stream.

    \sa {QtMultimedia::MetaData::AudioCodec}
*/

/*
    \qmlproperty variant Audio::metaData.averageLevel

    This property holds the average volume level of the media.

    \sa {QtMultimedia::MetaData::AverageLevel}
*/

/*
    \qmlproperty variant Audio::metaData.channelCount

    This property holds the number of channels in the media's audio stream.

    \sa {QtMultimedia::MetaData::ChannelCount}
*/

/*
    \qmlproperty variant Audio::metaData.peakValue

    This property holds the peak volume of media's audio stream.

    \sa {QtMultimedia::MetaData::PeakValue}
*/

/*
    \qmlproperty variant Audio::metaData.sampleRate

    This property holds the sample rate of the media's audio stream in hertz.

    \sa {QtMultimedia::MetaData::SampleRate}
*/

/*
    \qmlproperty variant Audio::metaData.albumTitle

    This property holds the title of the album the media belongs to.

    \sa {QtMultimedia::MetaData::AlbumTitle}
*/

/*
    \qmlproperty variant Audio::metaData.albumArtist

    This property holds the name of the principal artist of the album the media
    belongs to.

    \sa {QtMultimedia::MetaData::AlbumArtist}
*/

/*
    \qmlproperty variant Audio::metaData.contributingArtist

    This property holds the names of artists contributing to the media.

    \sa {QtMultimedia::MetaData::ContributingArtist}
*/

/*
    \qmlproperty variant Audio::metaData.composer

    This property holds the composer of the media.

    \sa {QtMultimedia::MetaData::Composer}
*/

/*
    \qmlproperty variant Audio::metaData.conductor

    This property holds the conductor of the media.

    \sa {QtMultimedia::MetaData::Conductor}
*/

/*
    \qmlproperty variant Audio::metaData.lyrics

    This property holds the lyrics to the media.

    \sa {QtMultimedia::MetaData::Lyrics}
*/

/*
    \qmlproperty variant Audio::metaData.mood

    This property holds the mood of the media.

    \sa {QtMultimedia::MetaData::Mood}
*/

/*
    \qmlproperty variant Audio::metaData.trackNumber

    This property holds the track number of the media.

    \sa {QtMultimedia::MetaData::TrackNumber}
*/

/*
    \qmlproperty variant Audio::metaData.trackCount

    This property holds the number of track on the album containing the media.

    \sa {QtMultimedia::MetaData::TrackNumber}
*/

/*
    \qmlproperty variant Audio::metaData.coverArtUrlSmall

    This property holds the URL of a small cover art image.

    \sa {QtMultimedia::MetaData::CoverArtUrlSmall}
*/

/*
    \qmlproperty variant Audio::metaData.coverArtUrlLarge

    This property holds the URL of a large cover art image.

    \sa {QtMultimedia::MetaData::CoverArtUrlLarge}
*/

/*
    \qmlproperty variant Audio::metaData.resolution

    This property holds the dimension of an image or video.

    \sa {QtMultimedia::MetaData::Resolution}
*/

/*
    \qmlproperty variant Audio::metaData.pixelAspectRatio

    This property holds the pixel aspect ratio of an image or video.

    \sa {QtMultimedia::MetaData::PixelAspectRatio}
*/

/*
    \qmlproperty variant Audio::metaData.videoFrameRate

    This property holds the frame rate of the media's video stream.

    \sa {QtMultimedia::MetaData::VideoFrameRate}
*/

/*
    \qmlproperty variant Audio::metaData.videoBitRate

    This property holds the bit rate of the media's video stream in bits per
    second.

    \sa {QtMultimedia::MetaData::VideoBitRate}
*/

/*
    \qmlproperty variant Audio::metaData.videoCodec

    This property holds the encoding of the media's video stream.

    \sa {QtMultimedia::MetaData::VideoCodec}
*/

/*
    \qmlproperty variant Audio::metaData.posterUrl

    This property holds the URL of a poster image.

    \sa {QtMultimedia::MetaData::PosterUrl}
*/

/*
    \qmlproperty variant Audio::metaData.chapterNumber

    This property holds the chapter number of the media.

    \sa {QtMultimedia::MetaData::ChapterNumber}
*/

/*
    \qmlproperty variant Audio::metaData.director

    This property holds the director of the media.

    \sa {QtMultimedia::MetaData::Director}
*/

/*
    \qmlproperty variant Audio::metaData.leadPerformer

    This property holds the lead performer in the media.

    \sa {QtMultimedia::MetaData::LeadPerformer}
*/

/*
    \qmlproperty variant Audio::metaData.writer

    This property holds the writer of the media.

    \sa {QtMultimedia::MetaData::Writer}
*/

QT_END_NAMESPACE

#include "moc_qdeclarativeaudio_p_4.cpp"


