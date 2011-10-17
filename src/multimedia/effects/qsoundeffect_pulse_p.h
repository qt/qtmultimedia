/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QSOUNDEFFECT_PULSE_H
#define QSOUNDEFFECT_PULSE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//


#include "qsoundeffect.h"

#include <QtCore/qobject.h>
#include <QtCore/qdatetime.h>
#include <qmediaplayer.h>
#include <pulse/pulseaudio.h>
#include "qsamplecache_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Multimedia)


class QSoundEffectPrivate : public QObject
{
    Q_OBJECT
public:
    explicit QSoundEffectPrivate(QObject* parent);
    ~QSoundEffectPrivate();

    static QStringList supportedMimeTypes();

    QUrl source() const;
    void setSource(const QUrl &url);
    int loopCount() const;
    void setLoopCount(int loopCount);
    int volume() const;
    void setVolume(int volume);
    bool isMuted() const;
    void setMuted(bool muted);
    bool isLoaded() const;
    bool isPlaying() const;
    QSoundEffect::Status status() const;

public Q_SLOTS:
    void play();
    void stop();

Q_SIGNALS:
    void volumeChanged();
    void mutedChanged();
    void loadedChanged();
    void playingChanged();
    void statusChanged();

private Q_SLOTS:
    void decoderError();
    void sampleReady();
    void uploadSample();
    void contextReady();
    void underRun();
    void prepare();
    void streamReady();
    void emptyComplete();
    void updateVolume();
    void updateMuted();

private:
    void playSample();

    void emptyStream();
    void createPulseStream();
    void unloadPulseStream();

    void setPlaying(bool playing);
    void setStatus(QSoundEffect::Status status);

    static void stream_write_callback(pa_stream *s, size_t length, void *userdata);
    static void stream_state_callback(pa_stream *s, void *userdata);
    static void stream_underrun_callback(pa_stream *s, void *userdata);
    static void stream_cork_callback(pa_stream *s, int success, void *userdata);
    static void stream_flush_callback(pa_stream *s, int success, void *userdata);
    static void stream_write_done_callback(void *p);
    static void stream_adjust_prebuffer_callback(pa_stream *s, int success, void *userdata);
    static void stream_reset_buffer_callback(pa_stream *s, int success, void *userdata);
    static void setvolume_callback(pa_context *c, int success, void *userdata);
    static void setmuted_callback(pa_context *c, int success, void *userdata);

    pa_stream *m_pulseStream;
    int        m_sinkInputId;
    pa_sample_spec m_pulseSpec;
    int        m_pulseBufferSize;

    bool    m_emptying;
    bool    m_sampleReady;
    bool    m_playing;
    QSoundEffect::Status  m_status;
    bool    m_muted;
    bool    m_playQueued;
    bool    m_stopping;
    int     m_volume;
    int     m_loopCount;
    int     m_runningCount;
    QUrl    m_source;
    QByteArray m_name;

    QSample *m_sample;
    int m_position;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSOUNDEFFECT_PULSE_H
