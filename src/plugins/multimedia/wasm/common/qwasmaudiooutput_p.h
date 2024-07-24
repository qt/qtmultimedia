// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMAUDIOOUTPUT_H
#define QWASMAUDIOOUTPUT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformaudiooutput_p.h>
#include "qwasmmediaplayer_p.h"

#include <emscripten/val.h>
#include <private/qstdweb_p.h>
#include <private/qwasmaudiosink_p.h>
#include <QIODevice>
#include <QObject>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QWasmAudioOutput : public QObject, public QPlatformAudioOutput
{
    Q_OBJECT

public:
    QWasmAudioOutput(QAudioOutput *qq);
    ~QWasmAudioOutput();

    enum AudioElementError {
        MEDIA_ERR_ABORTED = 1,
        MEDIA_ERR_NETWORK,
        MEDIA_ERR_DECODE,
        MEDIA_ERR_SRC_NOT_SUPPORTED
    };

    void setAudioDevice(const QAudioDevice &device) final;
    void setMuted(bool muted) override;
    void setVolume(float volume) override;

    void start();
    void stop();
    void pause();

    void setSource(const QUrl &url);
    void setSource(QIODevice *stream);
    void setVideoElement(emscripten::val videoElement);

Q_SIGNALS:
    void readyChanged(bool);
    void bufferingChanged(qint32 percent);
    void errorOccured(qint32 code, const QString &message);
    void stateChanged(QWasmMediaPlayer::QWasmMediaPlayerState newState);
    void progressChanged(qint32 position);
    void durationChanged(qint64 duration);
    void statusChanged(QMediaPlayer::MediaStatus status);
    void sizeChange(qint32 width, qint32 height);
    void metaDataLoaded();
    void seekableChanged(bool seekable);

private:
    void doElementCallbacks();
    void createAudioElement(const std::string &id);

    emscripten::val videoElement();

    QScopedPointer<QWasmAudioSink> m_sink;
    QScopedPointer<qstdweb::EventCallback> m_playEvent;
    QScopedPointer<qstdweb::EventCallback> m_endedEvent;
    QScopedPointer<qstdweb::EventCallback> m_durationChangeEvent;
    QScopedPointer<qstdweb::EventCallback> m_errorChangeEvent;
    QScopedPointer<qstdweb::EventCallback> m_canPlayChangeEvent;
    QScopedPointer<qstdweb::EventCallback> m_canPlayThroughChangeEvent;

    QScopedPointer<qstdweb::EventCallback> m_playingChangeEvent;
    QScopedPointer<qstdweb::EventCallback> m_progressChangeEvent;
    QScopedPointer<qstdweb::EventCallback> m_pauseChangeEvent;
    QScopedPointer<qstdweb::EventCallback> m_timeUpdateEvent;
    QScopedPointer<qstdweb::EventCallback> m_loadedDataEvent;

    QString m_source;
    QIODevice *m_audioIODevice = nullptr;
    emscripten::val m_audio = emscripten::val::undefined();
    emscripten::val m_videoElement = emscripten::val::undefined();
    QMediaPlayer::MediaStatus m_currentMediaStatus;
    qreal m_currentBufferedValue;
};

QT_END_NAMESPACE

#endif // QWASMAUDIOOUTPUT_H
