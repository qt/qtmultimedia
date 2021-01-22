/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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
#include <QtCore/qmutex.h>
#include <qmediaplayer.h>
#include <pulse/pulseaudio.h>
#include "qsamplecache_p.h"

#include <private/qmediaresourcepolicy_p.h>
#include <private/qmediaresourceset_p.h>

QT_BEGIN_NAMESPACE

class QSoundEffectRef;

class QSoundEffectPrivate : public QObject
{
    Q_OBJECT
public:
    explicit QSoundEffectPrivate(QObject* parent);
    explicit QSoundEffectPrivate(const QAudioDeviceInfo &audioDevice, QObject *parent);
    ~QSoundEffectPrivate();

    static QStringList supportedMimeTypes();

    QUrl source() const;
    void setSource(const QUrl &url);
    int loopCount() const;
    int loopsRemaining() const;
    void setLoopCount(int loopCount);
    qreal volume() const;
    void setVolume(qreal volume);
    bool isMuted() const;
    void setMuted(bool muted);
    bool isLoaded() const;
    bool isPlaying() const;
    QSoundEffect::Status status() const;

    void release();

    QString category() const;
    void setCategory(const QString &category);

public Q_SLOTS:
    void play();
    void stop();

Q_SIGNALS:
    void loopsRemainingChanged();
    void volumeChanged();
    void mutedChanged();
    void loadedChanged();
    void playingChanged();
    void statusChanged();
    void categoryChanged();

private Q_SLOTS:
    void decoderError();
    void sampleReady();
    void uploadSample();
    void contextReady();
    void contextFailed();
    void underRun();
    void prepare();
    void streamReady();
    void emptyComplete(void *stream, bool reload);

    void handleAvailabilityChanged(bool available);

private:
    void playAvailable();
    void playSample();

    enum EmptyStreamOption {
        ReloadSampleWhenDone = 0x1
    };
    Q_DECLARE_FLAGS(EmptyStreamOptions, EmptyStreamOption)
    void emptyStream(EmptyStreamOptions options = EmptyStreamOptions());

    void createPulseStream();
    void unloadPulseStream();

    int writeToStream(const void *data, int size);

    void setPlaying(bool playing);
    void setStatus(QSoundEffect::Status status);
    void setLoopsRemaining(int loopsRemaining);

    static void stream_write_callback(pa_stream *s, size_t length, void *userdata);
    static void stream_state_callback(pa_stream *s, void *userdata);
    static void stream_underrun_callback(pa_stream *s, void *userdata);
    static void stream_cork_callback(pa_stream *s, int success, void *userdata);
    static void stream_flush_callback(pa_stream *s, int success, void *userdata);
    static void stream_flush_reload_callback(pa_stream *s, int success, void *userdata);
    static void stream_write_done_callback(void *p);
    static void stream_adjust_prebuffer_callback(pa_stream *s, int success, void *userdata);

    pa_stream *m_pulseStream = nullptr;
    QString m_sinkName;
    int m_sinkInputId = -1;
    pa_sample_spec m_pulseSpec;
    int m_pulseBufferSize = 0;

    bool m_emptying = false;
    bool m_sampleReady = false;
    bool m_playing = false;
    QSoundEffect::Status m_status = QSoundEffect::Null;
    bool m_muted = false;
    bool m_playQueued = false;
    bool m_stopping = false;
    qreal m_volume = 1.0;
    int m_loopCount = 1;
    int m_runningCount = 0;
    QUrl m_source;
    QByteArray m_name;
    QString m_category;
    bool m_reloadCategory = false;

    QSample *m_sample = nullptr;
    int m_position = 0;
    QSoundEffectRef *m_ref = nullptr;

    bool m_resourcesAvailable = false;

    // Protects volume while PuseAudio is accessing it
    mutable QMutex m_volumeLock;

    QMediaPlayerResourceSetInterface *m_resources = nullptr;
};

QT_END_NAMESPACE

#endif // QSOUNDEFFECT_PULSE_H
