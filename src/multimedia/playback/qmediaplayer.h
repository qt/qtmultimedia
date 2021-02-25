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
#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmediaenumdebug.h>
#include <QtMultimedia/qaudio.h>

QT_BEGIN_NAMESPACE


class QAbstractVideoSurface;
class QAudioDeviceInfo;
class QMediaMetaData;

class QMediaPlayerPrivate;
class Q_MULTIMEDIA_EXPORT QMediaPlayer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int notifyInterval READ notifyInterval WRITE setNotifyInterval NOTIFY notifyIntervalChanged)
    Q_PROPERTY(QUrl media READ media WRITE setMedia NOTIFY mediaChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(int bufferStatus READ bufferStatus NOTIFY bufferStatusChanged)
    Q_PROPERTY(bool audioAvailable READ isAudioAvailable NOTIFY audioAvailableChanged)
    Q_PROPERTY(bool videoAvailable READ isVideoAvailable NOTIFY videoAvailableChanged)
    Q_PROPERTY(bool seekable READ isSeekable NOTIFY seekableChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(MediaStatus mediaStatus READ mediaStatus NOTIFY mediaStatusChanged)
    Q_PROPERTY(QAudio::Role audioRole READ audioRole WRITE setAudioRole NOTIFY audioRoleChanged)
    Q_PROPERTY(QString customAudioRole READ customAudioRole WRITE setCustomAudioRole NOTIFY customAudioRoleChanged)
    Q_PROPERTY(QMediaMetaData metaData READ metaData NOTIFY metaDataChanged)
    Q_PROPERTY(QString error READ errorString)
    Q_ENUMS(State)
    Q_ENUMS(MediaStatus)
    Q_ENUMS(Error)

public:
    enum State
    {
        StoppedState,
        PlayingState,
        PausedState
    };

    enum MediaStatus
    {
        UnknownMediaStatus,
        NoMedia,
        LoadingMedia,
        LoadedMedia,
        StalledMedia,
        BufferingMedia,
        BufferedMedia,
        EndOfMedia,
        InvalidMedia
    };

    enum Error
    {
        NoError,
        ResourceError,
        FormatError,
        NetworkError,
        AccessDeniedError,
        ServiceMissingError
    };

    explicit QMediaPlayer(QObject *parent = nullptr);
    ~QMediaPlayer();

    // new API
//    bool enableLowLatencyPlayback(bool tryEnable);
//    bool isLowLatencyPlaybackEnabled() const;

    int notifyInterval() const;
    void setNotifyInterval(int milliSeconds);
    void addPropertyWatch(QByteArray const &name);
    void removePropertyWatch(QByteArray const &name);

    bool setAudioOutput(const QAudioDeviceInfo &device);
    QAudioDeviceInfo audioOutput() const;

    QList<QMediaMetaData> audioTracks() const;
    QList<QMediaMetaData> videoTracks() const;
    QList<QMediaMetaData> subtitleTracks() const;

    int activeAudioTrack() const;
    int activeVideoTrack() const;
    int activeSubtitleTrack() const;

    void setActiveAudioTrack(int index);
    void setActiveVideoTrack(int index);
    void setActiveSubtitleTrack(int index);

    void setVideoOutput(QObject *);
    void setVideoOutput(QAbstractVideoSurface *surface);
    void setVideoOutput(const QList<QAbstractVideoSurface *> &surfaces);

    QUrl media() const;
    const QIODevice *mediaStream() const;

    State state() const;
    MediaStatus mediaStatus() const;

    qint64 duration() const;
    qint64 position() const;

    int volume() const;
    bool isMuted() const;
    bool isAudioAvailable() const;
    bool isVideoAvailable() const;

    int bufferStatus() const;

    bool isSeekable() const;
    qreal playbackRate() const;

    Error error() const;
    QString errorString() const;

    bool isAvailable() const;
    QMediaMetaData metaData() const;

    QAudio::Role audioRole() const;
    void setAudioRole(QAudio::Role audioRole);
    QList<QAudio::Role> supportedAudioRoles() const;
    QString customAudioRole() const;
    void setCustomAudioRole(const QString &audioRole);
    QStringList supportedCustomAudioRoles() const;

public Q_SLOTS:
    void play();
    void pause();
    void stop();

    void setPosition(qint64 position);
    void setVolume(int volume);
    void setMuted(bool muted);

    void setPlaybackRate(qreal rate);

    void setMedia(const QUrl &media, QIODevice *stream = nullptr);

Q_SIGNALS:
    void notifyIntervalChanged(int milliSeconds);

    void mediaChanged(const QUrl &media);

    void stateChanged(QMediaPlayer::State newState);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);

    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);

    void volumeChanged(int volume);
    void mutedChanged(bool muted);
    void audioAvailableChanged(bool available);
    void videoAvailableChanged(bool videoAvailable);

    void bufferStatusChanged(int percentFilled);

    void seekableChanged(bool seekable);
    void playbackRateChanged(qreal rate);

    void audioRoleChanged(QAudio::Role role);
    void customAudioRoleChanged(const QString &role);

    void metaDataChanged();

    void tracksChanged();
    void activeTracksChanged();

    void error(QMediaPlayer::Error error);

private:
    Q_DISABLE_COPY(QMediaPlayer)
    Q_DECLARE_PRIVATE(QMediaPlayer)
    friend class QPlatformMediaPlayer;
    Q_PRIVATE_SLOT(d_func(), void _q_notify())
};

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(QMediaPlayer, State)
Q_MEDIA_ENUM_DEBUG(QMediaPlayer, MediaStatus)
Q_MEDIA_ENUM_DEBUG(QMediaPlayer, Error)

#endif  // QMEDIAPLAYER_H
