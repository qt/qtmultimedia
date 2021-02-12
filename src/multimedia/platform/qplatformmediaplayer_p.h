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

#ifndef QMEDIAPLAYERCONTROL_H
#define QMEDIAPLAYERCONTROL_H

#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/qmediatimerange.h>
#include <QtMultimedia/qaudiodeviceinfo.h>
#include <QtMultimedia/qmediametadata.h>

#include <QtCore/qpair.h>

QT_BEGIN_NAMESPACE

class QMediaStreamsControl;

class Q_MULTIMEDIA_EXPORT QPlatformMediaPlayer : public QObject
{
    Q_OBJECT

public:
    virtual QMediaPlayer::State state() const = 0;

    virtual QMediaPlayer::MediaStatus mediaStatus() const = 0;

    virtual qint64 duration() const = 0;

    virtual qint64 position() const = 0;
    virtual void setPosition(qint64 position) = 0;

    virtual int volume() const = 0;
    virtual void setVolume(int volume) = 0;

    virtual bool isMuted() const = 0;
    virtual void setMuted(bool mute) = 0;

    virtual int bufferStatus() const = 0;

    virtual bool isAudioAvailable() const = 0;
    virtual bool isVideoAvailable() const = 0;

    virtual bool isSeekable() const = 0;

    virtual QMediaTimeRange availablePlaybackRanges() const = 0;

    virtual qreal playbackRate() const = 0;
    virtual void setPlaybackRate(qreal rate) = 0;

    virtual QUrl media() const = 0;
    virtual const QIODevice *mediaStream() const = 0;
    virtual void setMedia(const QUrl &media, QIODevice *stream) = 0;

    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;

    virtual void setAudioRole(QAudio::Role /*role*/) {}
    virtual QList<QAudio::Role> supportedAudioRoles() const { return {}; }

    virtual void setCustomAudioRole(const QString &/*role*/) {}
    virtual QStringList supportedCustomAudioRoles() const { return {}; }

    virtual bool streamPlaybackSupported() const { return false; }

    virtual bool setAudioOutput(const QAudioDeviceInfo &) { return false; }
    virtual QAudioDeviceInfo audioOutput() const { return QAudioDeviceInfo(); }

    virtual QMediaMetaData metaData() const { return {}; }

    virtual void setVideoSurface(QAbstractVideoSurface *surface) = 0;

    virtual QMediaStreamsControl *mediaStreams() { return nullptr; }

Q_SIGNALS:
    void audioRoleChanged(QAudio::Role role);
    void customAudioRoleChanged(const QString &role);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void stateChanged(QMediaPlayer::State newState);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void volumeChanged(int volume);
    void mutedChanged(bool mute);
    void audioAvailableChanged(bool audioAvailable);
    void videoAvailableChanged(bool videoAvailable);
    void bufferStatusChanged(int percentFilled);
    void seekableChanged(bool seekable);
    void availablePlaybackRangesChanged(const QMediaTimeRange &ranges);
    void playbackRateChanged(qreal rate);
    void error(int error, const QString &errorString);
    void metaDataChanged();

protected:
    explicit QPlatformMediaPlayer(QObject *parent = nullptr);
};

QT_END_NAMESPACE


#endif  // QMEDIAPLAYERCONTROL_H

