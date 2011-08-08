/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativeaudio_p.h"

#include <qmediaplayercontrol.h>

QT_BEGIN_NAMESPACE


/*!
    \qmlclass Audio QDeclarativeAudio
    \brief The Audio element allows you to add audio playback to a scene.

    \ingroup qml-multimedia

    This element is part of the \bold{QtMultimediaKit 1.1} module.

    \qml
    import Qt 4.7
    import QtMultimediaKit 1.1

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


QDeclarativeAudio::QDeclarativeAudio(QObject *parent)
    : QObject(parent)
{
}

QDeclarativeAudio::~QDeclarativeAudio()
{
    shutdown();
}

/*!
    \qmlmethod Audio::play()

    Starts playback of the media.

    Sets the \l playing property to true, and the \l paused property to false.
*/

void QDeclarativeAudio::play()
{
    if (!m_complete)
        return;

    setPaused(false);
    setPlaying(true);
}

/*!
    \qmlmethod Audio::pause()

    Pauses playback of the media.

    Sets the \l playing and \l paused properties to true.
*/

void QDeclarativeAudio::pause()
{
    if (!m_complete)
        return;

    setPaused(true);
    setPlaying(true);
}

/*!
    \qmlmethod Audio::stop()

    Stops playback of the media.

    Sets the \l playing and \l paused properties to false.
*/

void QDeclarativeAudio::stop()
{
    if (!m_complete)
        return;

    setPlaying(false);
    setPaused(false);
}

/*!
    \qmlproperty url Audio::source

    This property holds the source URL of the media.
*/

/*!
    \qmlproperty url Audio::autoLoad

    This property indicates if loading of media should begin immediately.

    Defaults to true, if false media will not be loaded until playback is started.
*/

/*!
    \qmlproperty bool Audio::playing

    This property holds whether the media is playing.

    Defaults to false, and can be set to true to start playback.
*/

/*!
    \qmlproperty bool Audio::paused

    This property holds whether the media is paused.

    Defaults to false, and can be set to true to pause playback.
*/

/*!
    \qmlsignal Audio::onStarted()

    This handler is called when playback is started.
*/

/*!
    \qmlsignal Audio::onResumed()

    This handler is called when playback is resumed from the paused state.
*/

/*!
    \qmlsignal Audio::onPaused()

    This handler is called when playback is paused.
*/

/*!
    \qmlsignal Audio::onStopped()

    This handler is called when playback is stopped.
*/

/*!
    \qmlproperty enumeration Audio::status

    This property holds the status of media loading. It can be one of:

    \list
    \o NoMedia - no media has been set.
    \o Loading - the media is currently being loaded.
    \o Loaded - the media has been loaded.
    \o Buffering - the media is buffering data.
    \o Stalled - playback has been interrupted while the media is buffering data.
    \o Buffered - the media has buffered data.
    \o EndOfMedia - the media has played to the end.
    \o InvalidMedia - the media cannot be played.
    \o UnknownStatus - the status of the media is unknown.
    \endlist
*/

QDeclarativeAudio::Status QDeclarativeAudio::status() const
{
    return Status(m_status);
}

/*!
    \qmlproperty int Audio::duration

    This property holds the duration of the media in milliseconds.

    If the media doesn't have a fixed duration (a live stream for example) this will be 0.
*/

/*!
    \qmlproperty int Audio::position

    This property holds the current playback position in milliseconds.

    If the \l seekable property is true, this property can be set to seek to a new position.
*/

/*!
    \qmlproperty real Audio::volume

    This property holds the volume of the audio output, from 0.0 (silent) to 1.0 (maximum volume).
*/

/*!
    \qmlproperty bool Audio::muted

    This property holds whether the audio output is muted.
*/

/*!
    \qmlproperty real Audio::bufferProgress

    This property holds how much of the data buffer is currently filled, from 0.0 (empty) to 1.0
    (full).
*/

/*!
    \qmlproperty bool Audio::seekable

    This property holds whether position of the audio can be changed.

    If true; setting a \l position value will cause playback to seek to the new position.
*/

/*!
    \qmlproperty real Audio::playbackRate

    This property holds the rate at which audio is played at as a multiple of the normal rate.
*/

/*!
    \qmlproperty enumeration Audio::error

    This property holds the error state of the audio.  It can be one of:

    \list
    \o NoError - there is no current error.
    \o ResourceError - the audio cannot be played due to a problem allocating resources.
    \o FormatError - the audio format is not supported.
    \o NetworkError - the audio cannot be played due to network issues.
    \o AccessDenied - the audio cannot be played due to insufficient permissions.
    \o ServiceMissing -  the audio cannot be played because the media service could not be
    instantiated.
    \endlist
*/

QDeclarativeAudio::Error QDeclarativeAudio::error() const
{
    return Error(m_error);
}

void QDeclarativeAudio::classBegin()
{
    setObject(this);
    emit mediaObjectChanged();
}

void QDeclarativeAudio::componentComplete()
{
    QDeclarativeMediaBase::componentComplete();
}


/*!
    \qmlproperty string Audio::errorString

    This property holds a string describing the current error condition in more detail.
*/

/*!
    \qmlsignal Audio::onError(error, errorString)

    This handler is called when an \l {QMediaPlayer::Error}{error} has
    occurred.  The errorString parameter may contain more detailed
    information about the error.
*/

/*!
    \qmlproperty variant Audio::metaData.title

    This property holds the tile of the media.

    \sa {QtMultimediaKit::Title}
*/

/*!
    \qmlproperty variant Audio::metaData.subTitle

    This property holds the sub-title of the media.

    \sa {QtMultimediaKit::SubTitle}
*/

/*!
    \qmlproperty variant Audio::metaData.author

    This property holds the author of the media.

    \sa {QtMultimediaKit::Author}
*/

/*!
    \qmlproperty variant Audio::metaData.comment

    This property holds a user comment about the media.

    \sa {QtMultimediaKit::Comment}
*/

/*!
    \qmlproperty variant Audio::metaData.description

    This property holds a description of the media.

    \sa {QtMultimediaKit::Description}
*/

/*!
    \qmlproperty variant Audio::metaData.category

    This property holds the category of the media

    \sa {QtMultimediaKit::Category}
*/

/*!
    \qmlproperty variant Audio::metaData.genre

    This property holds the genre of the media.

    \sa {QtMultimediaKit::Genre}
*/

/*!
    \qmlproperty variant Audio::metaData.year

    This property holds the year of release of the media.

    \sa {QtMultimediaKit::Year}
*/

/*!
    \qmlproperty variant Audio::metaData.date

    This property holds the date of the media.

    \sa {QtMultimediaKit::Date}
*/

/*!
    \qmlproperty variant Audio::metaData.userRating

    This property holds a user rating of the media in the range of 0 to 100.

    \sa {QtMultimediaKit::UserRating}
*/

/*!
    \qmlproperty variant Audio::metaData.keywords

    This property holds a list of keywords describing the media.

    \sa {QtMultimediaKit::Keywords}
*/

/*!
    \qmlproperty variant Audio::metaData.language

    This property holds the language of the media, as an ISO 639-2 code.

    \sa {QtMultimediaKit::Language}
*/

/*!
    \qmlproperty variant Audio::metaData.publisher

    This property holds the publisher of the media.

    \sa {QtMultimediaKit::Publisher}
*/

/*!
    \qmlproperty variant Audio::metaData.copyright

    This property holds the media's copyright notice.

    \sa {QtMultimediaKit::Copyright}
*/

/*!
    \qmlproperty variant Audio::metaData.parentalRating

    This property holds the parental rating of the media.

    \sa {QtMultimediaKit::ParentalRating}
*/

/*!
    \qmlproperty variant Audio::metaData.ratingOrganisation

    This property holds the name of the rating organisation responsible for the
    parental rating of the media.

    \sa {QtMultimediaKit::RatingOrganisation}
*/

/*!
    \qmlproperty variant Audio::metaData.size

    This property property holds the size of the media in bytes.

    \sa {QtMultimediaKit::Size}
*/

/*!
    \qmlproperty variant Audio::metaData.mediaType

    This property holds the type of the media.

    \sa {QtMultimediaKit::MediaType}
*/

/*!
    \qmlproperty variant Audio::metaData.audioBitRate

    This property holds the bit rate of the media's audio stream ni bits per
    second.

    \sa {QtMultimediaKit::AudioBitRate}
*/

/*!
    \qmlproperty variant Audio::metaData.audioCodec

    This property holds the encoding of the media audio stream.

    \sa {QtMultimediaKit::AudioCodec}
*/

/*!
    \qmlproperty variant Audio::metaData.averageLevel

    This property holds the average volume level of the media.

    \sa {QtMultimediaKit::AverageLevel}
*/

/*!
    \qmlproperty variant Audio::metaData.channelCount

    This property holds the number of channels in the media's audio stream.

    \sa {QtMultimediaKit::ChannelCount}
*/

/*!
    \qmlproperty variant Audio::metaData.peakValue

    This property holds the peak volume of media's audio stream.

    \sa {QtMultimediaKit::PeakValue}
*/

/*!
    \qmlproperty variant Audio::metaData.sampleRate

    This property holds the sample rate of the media's audio stream in hertz.

    \sa {QtMultimediaKit::SampleRate}
*/

/*!
    \qmlproperty variant Audio::metaData.albumTitle

    This property holds the title of the album the media belongs to.

    \sa {QtMultimediaKit::AlbumTitle}
*/

/*!
    \qmlproperty variant Audio::metaData.albumArtist

    This property holds the name of the principal artist of the album the media
    belongs to.

    \sa {QtMultimediaKit::AlbumArtist}
*/

/*!
    \qmlproperty variant Audio::metaData.contributingArtist

    This property holds the names of artists contributing to the media.

    \sa {QtMultimediaKit::ContributingArtist}
*/

/*!
    \qmlproperty variant Audio::metaData.composer

    This property holds the composer of the media.

    \sa {QtMultimediaKit::Composer}
*/

/*!
    \qmlproperty variant Audio::metaData.conductor

    This property holds the conductor of the media.

    \sa {QtMultimediaKit::Conductor}
*/

/*!
    \qmlproperty variant Audio::metaData.lyrics

    This property holds the lyrics to the media.

    \sa {QtMultimediaKit::Lyrics}
*/

/*!
    \qmlproperty variant Audio::metaData.mood

    This property holds the mood of the media.

    \sa {QtMultimediaKit::Mood}
*/

/*!
    \qmlproperty variant Audio::metaData.trackNumber

    This property holds the track number of the media.

    \sa {QtMultimediaKit::TrackNumber}
*/

/*!
    \qmlproperty variant Audio::metaData.trackCount

    This property holds the number of track on the album containing the media.

    \sa {QtMultimediaKit::TrackNumber}
*/

/*!
    \qmlproperty variant Audio::metaData.coverArtUrlSmall

    This property holds the URL of a small cover art image.

    \sa {QtMultimediaKit::CoverArtUrlSmall}
*/

/*!
    \qmlproperty variant Audio::metaData.coverArtUrlLarge

    This property holds the URL of a large cover art image.

    \sa {QtMultimediaKit::CoverArtUrlLarge}
*/

/*!
    \qmlproperty variant Audio::metaData.resolution

    This property holds the dimension of an image or video.

    \sa {QtMultimediaKit::Resolution}
*/

/*!
    \qmlproperty variant Audio::metaData.pixelAspectRatio

    This property holds the pixel aspect ratio of an image or video.

    \sa {QtMultimediaKit::PixelAspectRatio}
*/

/*!
    \qmlproperty variant Audio::metaData.videoFrameRate

    This property holds the frame rate of the media's video stream.

    \sa {QtMultimediaKit::VideoFrameRate}
*/

/*!
    \qmlproperty variant Audio::metaData.videoBitRate

    This property holds the bit rate of the media's video stream in bits per
    second.

    \sa {QtMultimediaKit::VideoBitRate}
*/

/*!
    \qmlproperty variant Audio::metaData.videoCodec

    This property holds the encoding of the media's video stream.

    \sa {QtMultimediaKit::VideoCodec}
*/

/*!
    \qmlproperty variant Audio::metaData.posterUrl

    This property holds the URL of a poster image.

    \sa {QtMultimediaKit::PosterUrl}
*/

/*!
    \qmlproperty variant Audio::metaData.chapterNumber

    This property holds the chapter number of the media.

    \sa {QtMultimediaKit::ChapterNumber}
*/

/*!
    \qmlproperty variant Audio::metaData.director

    This property holds the director of the media.

    \sa {QtMultimediaKit::Director}
*/

/*!
    \qmlproperty variant Audio::metaData.leadPerformer

    This property holds the lead performer in the media.

    \sa {QtMultimediaKit::LeadPerformer}
*/

/*!
    \qmlproperty variant Audio::metaData.writer

    This property holds the writer of the media.

    \sa {QtMultimediaKit::Writer}
*/

QT_END_NAMESPACE

#include "moc_qdeclarativeaudio_p.cpp"


