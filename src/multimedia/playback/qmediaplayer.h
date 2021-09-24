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

#ifndef QMEDIAPLAYER_H
#define QMEDIAPLAYER_H

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmediaenumdebug.h>
#include <QtMultimedia/qaudio.h>

QT_BEGIN_NAMESPACE

class QVideoSink;
class QAudioOutput;
class QAudioDevice;
class QMediaMetaData;
class QMediaTimeRange;

class QMediaPlayerPrivate;
class Q_MULTIMEDIA_EXPORT QMediaPlayer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(float bufferProgress READ bufferProgress NOTIFY bufferProgressChanged)
    Q_PROPERTY(bool hasAudio READ hasAudio NOTIFY hasAudioChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(bool seekable READ isSeekable NOTIFY seekableChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(int loops READ loops WRITE setLoops NOTIFY loopsChanged)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(MediaStatus mediaStatus READ mediaStatus NOTIFY mediaStatusChanged)
    Q_PROPERTY(QMediaMetaData metaData READ metaData NOTIFY metaDataChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
    Q_PROPERTY(QObject *videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged)
    Q_PROPERTY(QAudioOutput *audioOutput READ audioOutput WRITE setAudioOutput NOTIFY
                       audioOutputChanged)

    Q_PROPERTY(QList<QMediaMetaData> audioTracks READ audioTracks NOTIFY tracksChanged)
    Q_PROPERTY(QList<QMediaMetaData> videoTracks READ videoTracks NOTIFY tracksChanged)
    Q_PROPERTY(QList<QMediaMetaData> subtitleTracks READ subtitleTracks NOTIFY tracksChanged)

    Q_PROPERTY(int activeAudioTrack READ activeAudioTrack WRITE setActiveAudioTrack NOTIFY
                       activeTracksChanged)
    Q_PROPERTY(int activeVideoTrack READ activeVideoTrack WRITE setActiveVideoTrack NOTIFY
                       activeTracksChanged)
    Q_PROPERTY(int activeSubtitleTrack READ activeSubtitleTrack WRITE setActiveSubtitleTrack NOTIFY
                       activeTracksChanged)

public:
    enum PlaybackState
    {
        StoppedState,
        PlayingState,
        PausedState
    };
    Q_ENUM(PlaybackState)

    enum MediaStatus
    {
        NoMedia,
        LoadingMedia,
        LoadedMedia,
        StalledMedia,
        BufferingMedia,
        BufferedMedia,
        EndOfMedia,
        InvalidMedia
    };
    Q_ENUM(MediaStatus)

    enum Error
    {
        NoError,
        ResourceError,
        FormatError,
        NetworkError,
        AccessDeniedError
    };
    Q_ENUM(Error)

    enum Loops
    {
        Infinite = -1,
        Once = 1
    };
    Q_ENUM(Loops)

    explicit QMediaPlayer(QObject *parent = nullptr);
    ~QMediaPlayer();

    QList<QMediaMetaData> audioTracks() const;
    QList<QMediaMetaData> videoTracks() const;
    QList<QMediaMetaData> subtitleTracks() const;

    int activeAudioTrack() const;
    int activeVideoTrack() const;
    int activeSubtitleTrack() const;

    void setActiveAudioTrack(int index);
    void setActiveVideoTrack(int index);
    void setActiveSubtitleTrack(int index);

    void setAudioOutput(QAudioOutput *output);
    QAudioOutput *audioOutput() const;

    void setVideoOutput(QObject *);
    QObject *videoOutput() const;
#if 0
    void setVideoOutput(const QList<QVideoSink *> &sinks);
#endif

    void setVideoSink(QVideoSink *sink);
    QVideoSink *videoSink() const;

    QUrl source() const;
    const QIODevice *sourceDevice() const;

    PlaybackState playbackState() const;
    MediaStatus mediaStatus() const;

    qint64 duration() const;
    qint64 position() const;

    bool hasAudio() const;
    bool hasVideo() const;

    float bufferProgress() const;
    QMediaTimeRange bufferedTimeRange() const;

    bool isSeekable() const;
    qreal playbackRate() const;

    int loops() const;
    void setLoops(int loops);

    Error error() const;
    QString errorString() const;

    bool isAvailable() const;
    QMediaMetaData metaData() const;

public Q_SLOTS:
    void play();
    void pause();
    void stop();

    void setPosition(qint64 position);

    void setPlaybackRate(qreal rate);

    void setSource(const QUrl &source);
    void setSourceDevice(QIODevice *device, const QUrl &sourceUrl = QUrl());

Q_SIGNALS:
    void sourceChanged(const QUrl &media);
    void playbackStateChanged(QMediaPlayer::PlaybackState newState);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);

    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);

    void hasAudioChanged(bool available);
    void hasVideoChanged(bool videoAvailable);

    void bufferProgressChanged(float progress);

    void seekableChanged(bool seekable);
    void playbackRateChanged(qreal rate);
    void loopsChanged();

    void metaDataChanged();
    void videoOutputChanged();
    void audioOutputChanged();

    void tracksChanged();
    void activeTracksChanged();

    void errorChanged();
    void errorOccurred(QMediaPlayer::Error error, const QString &errorString);

private:
    Q_DISABLE_COPY(QMediaPlayer)
    Q_DECLARE_PRIVATE(QMediaPlayer)
    friend class QPlatformMediaPlayer;
};

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(QMediaPlayer, PlaybackState)
Q_MEDIA_ENUM_DEBUG(QMediaPlayer, MediaStatus)
Q_MEDIA_ENUM_DEBUG(QMediaPlayer, Error)

#endif  // QMEDIAPLAYER_H
