/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT7PLAYERSESSION_H
#define QT7PLAYERSESSION_H

#include <QtCore/qobject.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qset.h>
#include <QtCore/qresource.h>

#include <qmediaplayercontrol.h>
#include <qmediaplayer.h>

QT_BEGIN_NAMESPACE

class QT7PlayerControl;
class QMediaPlaylist;
class QMediaPlaylistNavigator;
class QT7VideoOutput;
class QT7PlayerSession;
class QT7PlayerService;


class QT7PlayerSession : public QObject
{
    Q_OBJECT
public:
    QT7PlayerSession(QObject *parent = 0);
    ~QT7PlayerSession();

    void *movie() const;

    void setControl(QT7PlayerControl *control);

    void setVideoOutput(QT7VideoOutput *output);

    QMediaPlayer::State state() const;
    QMediaPlayer::MediaStatus mediaStatus() const;

    QMediaContent media() const;
    const QIODevice *mediaStream() const;
    void setMedia(const QMediaContent &content, QIODevice *stream);

    qint64 position() const;
    qint64 duration() const;

    int bufferStatus() const;

    int volume() const;
    bool isMuted() const;

    bool isAudioAvailable() const;
    bool isVideoAvailable() const;

    bool isSeekable() const;
    QMediaTimeRange availablePlaybackRanges() const;

    qreal playbackRate() const;

public Q_SLOTS:
    void setPlaybackRate(qreal rate);

    void setPosition(qint64 pos);

    void play();
    void pause();
    void stop();

    void setVolume(int volume);
    void setMuted(bool muted);

    void processEOS();
    void processLoadStateChange();
    void processVolumeChange();
    void processNaturalSizeChange();
    void processPositionChange();

Q_SIGNALS:
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void stateChanged(QMediaPlayer::State newState);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void volumeChanged(int volume);
    void mutedChanged(bool muted);
    void audioAvailableChanged(bool audioAvailable);
    void videoAvailableChanged(bool videoAvailable);
    void error(int error, const QString &errorString);

private:
    class ResourceHandler {
    public:
        ResourceHandler():resource(0) {}
        ~ResourceHandler() { clear(); }
        void setResourceFile(const QString &file) {
            if (resource) {
                if (resource->fileName() == file)
                    return;
                delete resource;
                rawData.clear();
            }
            resource = new QResource(file);
        }
        bool isValid() const { return resource && resource->isValid() && resource->data() != 0; }
        const uchar *data() {
            if (!isValid())
                return 0;
            if (resource->isCompressed()) {
                if (rawData.size() == 0)
                    rawData = qUncompress(resource->data(), resource->size());
                return (const uchar *)rawData.constData();
            }
            return resource->data();
        }
        qint64 size() {
            if (data() == 0)
                return 0;
            return resource->isCompressed() ? rawData.size() : resource->size();
        }
        void clear() {
            delete resource;
            rawData.clear();
        }
        QResource *resource;
        QByteArray rawData;
    };

    void openMovie(bool tryAsync);

    void *m_QTMovie;
    void *m_movieObserver;

    QMediaPlayer::State m_state;
    QMediaPlayer::MediaStatus m_mediaStatus;
    QIODevice *m_mediaStream;
    QMediaContent m_resources;
    ResourceHandler m_resourceHandler;

    QT7VideoOutput * m_videoOutput;

    bool m_muted;
    bool m_tryingAsync;
    int m_volume;
    qreal m_rate;

    qint64 m_duration;
    bool m_videoAvailable;
    bool m_audioAvailable;
};

QT_END_NAMESPACE

#endif
