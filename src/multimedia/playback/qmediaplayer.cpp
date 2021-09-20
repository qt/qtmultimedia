/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmediaplayer_p.h"

#include <private/qplatformmediaintegration_p.h>
#include <qvideosink.h>
#include <qaudiooutput.h>

#include <QtCore/qcoreevent.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qpointer.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qtemporaryfile.h>
#include <QtCore/qdir.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMediaPlayer
    \brief The QMediaPlayer class allows the playing of a media files.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_playback
    \ingroup multimedia_video

    The QMediaPlayer class is a high level media playback class. It can be used
    to playback audio of video media files. The content
    to playback is specified as a QUrl object.

    \snippet multimedia-snippets/media.cpp Player

    QVideoWidget can be used with QMediaPlayer for video rendering.

    \sa QVideoWidget
*/

void QMediaPlayerPrivate::setState(QMediaPlayer::PlaybackState ps)
{
    Q_Q(QMediaPlayer);

    if (ps != state) {
        state = ps;
        emit q->playbackStateChanged(ps);
    }
}

void QMediaPlayerPrivate::setStatus(QMediaPlayer::MediaStatus s)
{
    Q_Q(QMediaPlayer);

    emit q->mediaStatusChanged(s);
}

void QMediaPlayerPrivate::setError(int error, const QString &errorString)
{
    Q_Q(QMediaPlayer);

    this->error = QMediaPlayer::Error(error);
    this->errorString = errorString;
    emit q->errorChanged();
    emit q->errorOccurred(this->error, errorString);
}

void QMediaPlayerPrivate::setMedia(const QUrl &media, QIODevice *stream)
{
    if (!control)
        return;

    QScopedPointer<QFile> file;

    // Back ends can't play qrc files directly.
    // If the back end supports StreamPlayback, we pass a QFile for that resource.
    // If it doesn't, we copy the data to a temporary file and pass its path.
    if (!media.isEmpty() && !stream && media.scheme() == QLatin1String("qrc")) {
        qrcMedia = media;

        file.reset(new QFile(QLatin1Char(':') + media.path()));
        if (!file->open(QFile::ReadOnly)) {
            file.reset();
            control->setMedia(QUrl(), nullptr);
            control->mediaStatusChanged(QMediaPlayer::InvalidMedia);
            control->error(QMediaPlayer::ResourceError, QMediaPlayer::tr("Attempting to play invalid Qt resource"));

        } else if (control->streamPlaybackSupported()) {
            control->setMedia(media, file.data());
        } else {
#if QT_CONFIG(temporaryfile)
#if defined(Q_OS_ANDROID)
            QString tempFileName = QDir::tempPath() + media.path();
            QDir().mkpath(QFileInfo(tempFileName).path());
            QTemporaryFile *tempFile = QTemporaryFile::createNativeFile(*file);
            if (!tempFile->rename(tempFileName))
                qWarning() << "Could not rename temporary file to:" << tempFileName;
#else
            QTemporaryFile *tempFile = new QTemporaryFile;

            // Preserve original file extension, some back ends might not load the file if it doesn't
            // have an extension.
            const QString suffix = QFileInfo(*file).suffix();
            if (!suffix.isEmpty())
                tempFile->setFileTemplate(tempFile->fileTemplate() + QLatin1Char('.') + suffix);

            // Copy the qrc data into the temporary file
            tempFile->open();
            char buffer[4096];
            while (true) {
                qint64 len = file->read(buffer, sizeof(buffer));
                if (len < 1)
                    break;
                tempFile->write(buffer, len);
            }
            tempFile->close();
#endif
            file.reset(tempFile);
            control->setMedia(QUrl(QUrl::fromLocalFile(file->fileName())), nullptr);
#else
            qWarning("Qt was built with -no-feature-temporaryfile: playback from resource file is not supported!");
#endif
        }
    } else {
        qrcMedia = QUrl();
        control->setMedia(media, stream);
    }

    qrcFile.swap(file); // Cleans up any previous file
}

QList<QMediaMetaData> QMediaPlayerPrivate::trackMetaData(QPlatformMediaPlayer::TrackType s) const
{
    QList<QMediaMetaData> tracks;
    if (control) {
        int count = control->trackCount(s);
        for (int i = 0; i < count; ++i) {
            tracks.append(control->trackMetaData(s, i));
        }
    }
    return tracks;
}

/*!
    Constructs a QMediaPlayer instance as a child of \a{parent}.
*/

QMediaPlayer::QMediaPlayer(QObject *parent)
    : QObject(*new QMediaPlayerPrivate, parent)
{
    Q_D(QMediaPlayer);

    d->control = QPlatformMediaIntegration::instance()->createPlayer(this);
    if (!d->control) { // ### Should this be an assertion?
        d->setError(QMediaPlayer::ResourceError, QMediaPlayer::tr("Platform does not support media playback."));
        return;
    }
    Q_ASSERT(d->control);

    d->state = d->control->state();
}


/*!
    Destroys the player object.
*/

QMediaPlayer::~QMediaPlayer()
{
    Q_D(QMediaPlayer);

    // Disconnect everything to prevent notifying
    // when a receiver is already destroyed.
    disconnect();

    d->setVideoSink(nullptr);
    delete d->control;
}

QUrl QMediaPlayer::source() const
{
    Q_D(const QMediaPlayer);

    return d->source;
}

/*!
    Returns the stream source of media data.

    This is only valid if a stream was passed to setSource().

    \sa setSource()
*/

const QIODevice *QMediaPlayer::sourceDevice() const
{
    Q_D(const QMediaPlayer);

    return d->stream;
}

QMediaPlayer::PlaybackState QMediaPlayer::playbackState() const
{
    Q_D(const QMediaPlayer);

    // In case if EndOfMedia status is already received
    // but state is not.
    if (d->control != nullptr
        && d->control->mediaStatus() == QMediaPlayer::EndOfMedia
        && d->state != d->control->state()) {
        return d->control->state();
    }

    return d->state;
}

QMediaPlayer::MediaStatus QMediaPlayer::mediaStatus() const
{
    Q_D(const QMediaPlayer);
    return d->control ? d->control->mediaStatus() : NoMedia;
}

/*!
    Returns the duration of the current media in ms.

    Returns 0 if the media player doesn't have a valid media file or stream.
    For live streams, the duration usually changes during playback as more
    data becomes available.
*/
qint64 QMediaPlayer::duration() const
{
    Q_D(const QMediaPlayer);

    if (d->control != nullptr)
        return d->control->duration();

    return 0;
}

/*!
    Returns the current position inside the media being played back in ms.

    Returns 0 if the media player doesn't have a valid media file or stream.
    For live streams, the duration usually changes during playback as more
    data becomes available.
*/
qint64 QMediaPlayer::position() const
{
    Q_D(const QMediaPlayer);

    if (d->control != nullptr)
        return d->control->position();

    return 0;
}

/*!
    Returns a number betwee 0 and 1 when buffering data.

    0 means that there is no buffered data available, playback is usually
    stalled in this case. Playback will resume once the buffer reaches 1,
    meaning enough data has been buffered to be able to resume playback.

    bufferProgress() will always return 1 for local files.
*/
float QMediaPlayer::bufferProgress() const
{
    Q_D(const QMediaPlayer);

    if (d->control != nullptr)
        return d->control->bufferProgress();

    return 0.;
}

/*!
    Returns a QMediaTimeRange describing the currently buffered data.

    When streaming media from a remote source, different parts of the media
    file can be available locally. The returned QMediaTimeRange object describes
    the time ranges that are buffered and available for immediate playback.

    \sa QMediaTimeRange
*/
QMediaTimeRange QMediaPlayer::bufferedTimeRange() const
{
    Q_D(const QMediaPlayer);

    if (d->control)
        return d->control->availablePlaybackRanges();

    return {};
}

bool QMediaPlayer::hasAudio() const
{
    Q_D(const QMediaPlayer);

    if (d->control != nullptr)
        return d->control->isAudioAvailable();

    return false;
}

bool QMediaPlayer::hasVideo() const
{
    Q_D(const QMediaPlayer);

    if (d->control != nullptr)
        return d->control->isVideoAvailable();

    return false;
}

/*!
    Returns true if the media is seekable. Most file based media files are seekable,
    but live streams usually are not.

    \sa position
*/
bool QMediaPlayer::isSeekable() const
{
    Q_D(const QMediaPlayer);

    if (d->control != nullptr)
        return d->control->isSeekable();

    return false;
}

/*!
    Returns the current playback rate.
*/
qreal QMediaPlayer::playbackRate() const
{
    Q_D(const QMediaPlayer);

    if (d->control != nullptr)
        return d->control->playbackRate();

    return 0.0;
}

/*!
    Returns the current error state.
*/
QMediaPlayer::Error QMediaPlayer::error() const
{
    return d_func()->error;
}

QString QMediaPlayer::errorString() const
{
    return d_func()->errorString;
}

/*!
    Start or resume playing the current source.
*/

void QMediaPlayer::play()
{
    Q_D(QMediaPlayer);

    if (!d->control)
        return;

    // Reset error conditions
    d->error = NoError;
    d->errorString = QString();

    d->control->play();
}

/*!
    Pause playing the current source.
*/

void QMediaPlayer::pause()
{
    Q_D(QMediaPlayer);

    if (d->control != nullptr)
        d->control->pause();
}

/*!
    Stop playing, and reset the play position to the beginning.
*/

void QMediaPlayer::stop()
{
    Q_D(QMediaPlayer);

    if (d->control != nullptr)
        d->control->stop();
}

void QMediaPlayer::setPosition(qint64 position)
{
    Q_D(QMediaPlayer);

    if (d->control == nullptr)
        return;

    d->control->setPosition(qMax(position, 0ll));
}

void QMediaPlayer::setPlaybackRate(qreal rate)
{
    Q_D(QMediaPlayer);

    if (d->control != nullptr)
        d->control->setPlaybackRate(rate);
}

/*!
    Sets the current \a source.

    Setting the media to a null QUrl will cause the player to discard all
    information relating to the current media source and to cease all I/O operations related
    to that media.

    \note This function returns immediately after recording the specified source of the media.
    It does not wait for the media to finish loading and does not check for errors. Listen for
    the mediaStatusChanged() and error() signals to be notified when the media is loaded and
    when an error occurs during loading.
*/

void QMediaPlayer::setSource(const QUrl &source)
{
    Q_D(QMediaPlayer);
    stop();

    if (d->source == source && d->stream == nullptr)
        return;

    d->source = source;
    d->stream = nullptr;

    d->setMedia(source, nullptr);
    emit sourceChanged(d->source);
}

/*!
    Sets the current source \a device.

    The media data will be read from \a device. The \a sourceUrl can be provided
    to resolve additional information about the media, mime type etc. The
    \a device must be open and readable.

    For macOS the \a device should also be seek-able.

    \note This function returns immediately after recording the specified source
    of the media. It does not wait for the media to finish loading and does not
    check for errors. Listen for the mediaStatusChanged() and error() signals to
    be notified when the media is loaded, and if an error occurs during loading.
*/
void QMediaPlayer::setSourceDevice(QIODevice *device, const QUrl &sourceUrl)
{
    Q_D(QMediaPlayer);
    stop();

    if (d->source == sourceUrl && d->stream == device)
        return;

    d->source = sourceUrl;
    d->stream = device;

    d->setMedia(d->source, device);
    emit sourceChanged(d->source);
}

/*!
    Sets the audio \a output device.

    If \a output is \nullptr, sets the output to the system default output
    device.

    Listen for the audioOutputChanged() signal to be notified if the output is
    successfully changed.
*/
void QMediaPlayer::setAudioOutput(QAudioOutput *output)
{
    Q_D(QMediaPlayer);
    if (d->audioOutput == output)
        return;
    d->audioOutput = output;
    d->control->setAudioOutput(output ? output->handle() : nullptr);
    emit audioOutputChanged();
}

QAudioOutput *QMediaPlayer::audioOutput() const
{
    Q_D(const QMediaPlayer);
    return d->audioOutput;
}

/*!
    Lists the set of available audio tracks inside the media.

    The QMediaMetaData returned describes the properties of individual
    tracks.

    Different audio tracks can for example contain audio in different languages.
*/
QList<QMediaMetaData> QMediaPlayer::audioTracks() const
{
    Q_D(const QMediaPlayer);
    return d->trackMetaData(QPlatformMediaPlayer::AudioStream);
}

/*!
    Lists the set of available video tracks inside the media.

    The QMediaMetaData returned describes the properties of individual
    tracks.
*/
QList<QMediaMetaData> QMediaPlayer::videoTracks() const
{
    Q_D(const QMediaPlayer);
    return d->trackMetaData(QPlatformMediaPlayer::VideoStream);
}

/*!
    Lists the set of available subtitle tracks inside the media.

    The QMediaMetaData returned describes the properties of individual
    tracks.
*/
QList<QMediaMetaData> QMediaPlayer::subtitleTracks() const
{
    Q_D(const QMediaPlayer);
    return d->trackMetaData(QPlatformMediaPlayer::SubtitleStream);
}

/*!
    Returns the currently active audio track.
*/
int QMediaPlayer::activeAudioTrack() const
{
    Q_D(const QMediaPlayer);
    if (d->control)
        return d->control->activeTrack(QPlatformMediaPlayer::AudioStream);
    return 0;
}

/*!
    Returns the currently active video track.
*/
int QMediaPlayer::activeVideoTrack() const
{
    Q_D(const QMediaPlayer);
    if (d->control)
        return d->control->activeTrack(QPlatformMediaPlayer::VideoStream);
    return -1;
}

/*!
    Returns the currently active subtitle track.
*/
int QMediaPlayer::activeSubtitleTrack() const
{
    Q_D(const QMediaPlayer);
    if (d->control)
        return d->control->activeTrack(QPlatformMediaPlayer::SubtitleStream);
    return -1;
}

/*!
    Sets the currently active audio track to one at the given \a index.

    By default, the first available audio track will be chosen.

    Set \a index to \c -1 to disable all audio tracks.
*/
void QMediaPlayer::setActiveAudioTrack(int index)
{
    Q_D(QMediaPlayer);
    if (!d->control)
        return;

    if (activeAudioTrack() == index)
        return;
    d->control->setActiveTrack(QPlatformMediaPlayer::AudioStream, index);
}

/*!
    Sets the currently active video track to one at the given \a index.

    By default, the first available video track will be chosen.
*/
void QMediaPlayer::setActiveVideoTrack(int index)
{
    Q_D(QMediaPlayer);
    if (!d->control)
        return;

    if (activeVideoTrack() == index)
        return;
    d->control->setActiveTrack(QPlatformMediaPlayer::VideoStream, index);
}

/*!
    Sets the currently active subtitle track to one at the given \a index.

    Set \a index to \c -1 to disable subtitles.

    Subtitles are disabled by default.
*/
void QMediaPlayer::setActiveSubtitleTrack(int index)
{
    Q_D(QMediaPlayer);
    if (!d->control)
        return;

    if (activeSubtitleTrack() == index)
        return;
    d->control->setActiveTrack(QPlatformMediaPlayer::SubtitleStream, index);
}

QObject *QMediaPlayer::videoOutput() const
{
    Q_D(const QMediaPlayer);
    return d->videoOutput;
}

/*!
    Attach a video \a output to the media player.

    If the media player has already video output attached,
    it will be replaced with a new one.
*/
void QMediaPlayer::setVideoOutput(QObject *output)
{
    Q_D(QMediaPlayer);
    if (!d->control)
        return;
    if (d->videoOutput == output)
        return;

    QVideoSink *sink = qobject_cast<QVideoSink *>(output);
    if (!sink && output) {
        auto *mo = output->metaObject();
        if (output)
            mo->invokeMethod(output, "videoSink", Q_RETURN_ARG(QVideoSink *, sink));
    }
    d->videoOutput = output;
    d->setVideoSink(sink);
}

void QMediaPlayer::setVideoSink(QVideoSink *sink)
{
    Q_D(QMediaPlayer);
    if (!d->control)
        return;

    d->videoOutput = nullptr;
    d->setVideoSink(sink);
}

QVideoSink *QMediaPlayer::videoSink() const
{
    Q_D(const QMediaPlayer);
    return d->videoSink;
}


#if 0
/*
    \since 5.15
    Sets multiple video sinks as the video output of a media player.
    This allows the media player to render video frames on several outputs.

    If a video output has already been set on the media player the new surfaces
    will replace it.
*/
void QMediaPlayer::setVideoOutput(const QList<QVideoSink *> &sinks)
{
    // ### IMPLEMENT ME
    Q_UNUSED(sinks);
//    setVideoOutput(!surfaces.empty() ? new QVideoSurfaces(surfaces, this) : nullptr);
}
#endif

/*!
    Returns true if the media player is supported on this platform.
*/
bool QMediaPlayer::isAvailable() const
{
    Q_D(const QMediaPlayer);

    if (!d->control)
        return false;

    return true;
}

/*!
    Returns meta data for the current media used by the media player.

    Meta data can contain information such as the title of the video or it's creation date.
*/
QMediaMetaData QMediaPlayer::metaData() const
{
    Q_D(const QMediaPlayer);
    return d->control->metaData();
}

// Enums
/*!
    \enum QMediaPlayer::PlaybackState

    Defines the current state of a media player.

    \value StoppedState The media player is not playing content, playback will begin from the start
    of the current track.
    \value PlayingState The media player is currently playing content.
    \value PausedState The media player has paused playback, playback of the current track will
    resume from the position the player was paused at.
*/

/*!
    \enum QMediaPlayer::MediaStatus

    Defines the status of a media player's current media.

    \value NoMedia The is no current media.  The player is in the StoppedState.
    \value LoadingMedia The current media is being loaded. The player may be in any state.
    \value LoadedMedia The current media has been loaded. The player is in the StoppedState.
    \value StalledMedia Playback of the current media has stalled due to insufficient buffering or
    some other temporary interruption.  The player is in the PlayingState or PausedState.
    \value BufferingMedia The player is buffering data but has enough data buffered for playback to
    continue for the immediate future.  The player is in the PlayingState or PausedState.
    \value BufferedMedia The player has fully buffered the current media.  The player is in the
    PlayingState or PausedState.
    \value EndOfMedia Playback has reached the end of the current media.  The player is in the
    StoppedState.
    \value InvalidMedia The current media cannot be played.  The player is in the StoppedState.
*/

/*!
    \enum QMediaPlayer::Error

    Defines a media player error condition.

    \value NoError No error has occurred.
    \value ResourceError A media resource couldn't be resolved.
    \value FormatError The format of a media resource isn't (fully) supported.  Playback may still
    be possible, but without an audio or video component.
    \value NetworkError A network error occurred.
    \value AccessDeniedError There are not the appropriate permissions to play a media resource.
*/

// Signals
/*!
    \fn QMediaPlayer::errorOccurred(QMediaPlayer::Error error, const QString &errorString)

    Signals that an \a error condition has occurred, with \a errorString
    containing a description of the error.

    \sa errorString()
*/

/*!
    \fn QMediaPlayer::mediaStatusChanged(QMediaPlayer::MediaStatus status)

    Signals that the \a status of the current media has changed.

    \sa mediaStatus()
*/

/*!
    \fn void QMediaPlayer::sourceChanged(const QUrl &media);

    Signals that the media source has been changed to \a media.
*/

/*!
    \fn void QMediaPlayer::playbackRateChanged(qreal rate);

    Signals the playbackRate has changed to \a rate.
*/

/*!
    \fn void QMediaPlayer::seekableChanged(bool seekable);

    Signals the \a seekable status of the player object has changed.
*/

// Properties
/*!
    \property QMediaPlayer::error
    \brief a string describing the last error condition.

    \sa error()
*/

/*!
    \property QMediaPlayer::source
    \brief the active media source being used by the player object.

    The player object will use the QUrl for selection of the content to
    be played.

    By default this property has a null QUrl.

    Setting this property to a null QUrl will cause the player to discard all
    information relating to the current media source and to cease all I/O operations related
    to that media.

    \sa QUrl
*/

/*!
    \property QMediaPlayer::mediaStatus
    \brief the status of the current media stream.

    The stream status describes how the playback of the current stream is
    progressing.

    By default this property is QMediaPlayer::NoMedia

*/

/*!
    \property QMediaPlayer::duration
    \brief the duration of the current media.

    The value is the total playback time in milliseconds of the current media.
    The value may change across the life time of the QMediaPlayer object and
    may not be available when initial playback begins, connect to the
    durationChanged() signal to receive status notifications.
*/

/*!
    \property QMediaPlayer::position
    \brief the playback position of the current media.

    The value is the current playback position, expressed in milliseconds since
    the beginning of the media. Periodically changes in the position will be
    indicated with the signal positionChanged().
*/

/*!
    \property QMediaPlayer::bufferProgress
    \brief the percentage of the temporary buffer filled before playback begins or resumes, from
    \c 0 (empty) to \c 100 (full).

    When the player object is buffering; this property holds the percentage of
    the temporary buffer that is filled. The buffer will need to reach 100%
    filled before playback can start or resume, at which time mediaStatus() will return
    BufferedMedia or BufferingMedia. If the value is anything lower than \c 100, mediaStatus() will
    return StalledMedia.

    \sa mediaStatus()
*/

/*!
    \property QMediaPlayer::hasAudio
    \brief the audio availabilty status for the current media.

    \note As the life time of QMediaPlayer can be longer than the playback of
    one QUrl, this property may change over time.
*/

/*!
    \property QMediaPlayer::hasVideo
    \brief the video availability status for the current media.

    If available, the QVideoWidget class can be used to view the video.

    \note As the life time of QMediaPlayer can be longer than the playback of
    one QUrl, this property may change over time.

    \sa QVideoWidget, QUrl
*/

/*!
    \property QMediaPlayer::seekable
    \brief the seek-able status of the current media

    If seeking is supported this property will be true; false otherwise. The
    status of this property may change across the life time of the QMediaPlayer
    object, use the seekableChanged signal to monitor changes.
*/

/*!
    \property QMediaPlayer::playbackRate
    \brief the playback rate of the current media.

    This value is a multiplier applied to the media's standard play rate. By
    default this value is 1.0, indicating that the media is playing at the
    standard pace. Values higher than 1.0 will increase the rate of play.
    Values less than zero can be set and indicate the media should rewind at the
    multiplier of the standard pace.

    Not all playback services support change of the playback rate. It is
    framework defined as to the status and quality of audio and video
    while fast forwarding or rewinding.
*/

/*!
    \fn void QMediaPlayer::durationChanged(qint64 duration)

    Signals the duration of the content has changed to \a duration, expressed in milliseconds.
*/

/*!
    \fn void QMediaPlayer::positionChanged(qint64 position)

    Signals the position of the content has changed to \a position, expressed in
    milliseconds.
*/

/*!
    \fn void QMediaPlayer::hasVideoChanged(bool videoAvailable)

    Signals the availability of visual content has changed to \a videoAvailable.
*/

/*!
    \fn void QMediaPlayer::hasAudioChanged(bool available)

    Signals the availability of audio content has changed to \a available.
*/

/*!
    \fn void QMediaPlayer::bufferProgressChanged(float filled)

    Signals the amount of the local buffer \a filled as a number between 0 and 1.
*/

QT_END_NAMESPACE

#include "moc_qmediaplayer.cpp"
