// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#ifndef QWASMVIDEOOUTPUT_H
#define QWASMVIDEOOUTPUT_H

#include <QObject>

#include <emscripten/val.h>
#include <QMediaPlayer>
#include <QVideoFrame>

#include "qwasmmediaplayer_p.h"
#include <QtCore/qloggingcategory.h>

#include <private/qstdweb_p.h>
#include <private/qwasmsuspendresumecontrol_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qWasmMediaVideoOutput)

class QVideoSink;

class QWasmVideoOutput : public QObject
{
    Q_OBJECT
public:
    using MediaStatus = QMediaPlayer::MediaStatus;
    enum WasmVideoMode { VideoOutput, Camera };
    Q_ENUM(WasmVideoMode)

    explicit QWasmVideoOutput(QObject *parent = nullptr);
    ~QWasmVideoOutput();

    void setVideoSize(const QSize &);
    void start();
    void stop();
    void reset();
    void pause();

    void setSurface(QVideoSink *surface);
    emscripten::val surfaceElement();

    bool isReady() const;

    void setSource(const QUrl &url);
    void setSource(QIODevice *stream);
    void setVolume(qreal volume);
    void setMuted(bool muted);

    qint64 getCurrentPosition();
    void seekTo(qint64 position);
    bool isVideoSeekable();
    void setPlaybackRate(qreal rate);
    qreal playbackRate();

    qint64 getDuration();
    void newFrame(const QVideoFrame &newFrame);

    void createVideoElement(const std::string &id);
    void createOffscreenElement(const QSize &offscreenSize);
    void doElementCallbacks();
    void updateVideoElementGeometry(const QRect &windowGeometry);
    void updateVideoElementSource(const QString &src);
    void addCameraSourceElement(const std::string &id);
    void removeSourceElement();
    void setVideoMode(QWasmVideoOutput::WasmVideoMode mode);

    void setHasAudio(bool needsAudio) { m_hasAudio = needsAudio; }

    bool hasCapability(const QString &cap);
    emscripten::val getDeviceCapabilities();
    bool setDeviceSetting(const std::string &key, emscripten::val value);
    bool isCameraReady() { return m_cameraIsReady; }
    bool m_hasVideoFrame = false;

    void videoFrameCallback(void *context);
    void videoFrameTimerCallback();
    // mediacapturesession has the videosink
    QVideoSink *m_wasmSink = nullptr;

    emscripten::val currentVideoElement() { return (
        m_video.isNull() || m_video.isUndefined() ? emscripten::val::null()
                                                  : m_video) ; }

    std::string m_videoSurfaceId;

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
    void checkNetworkState();
    void videoComputeFrame(void *context);
    void getDeviceSettings();

    static QVideoFrameFormat::PixelFormat fromJsPixelFormat(std::string videoFormat);

    emscripten::val m_video = emscripten::val::undefined();
    emscripten::val m_videoElementSource = emscripten::val::undefined();

    QString m_source;
    float m_requestedPosition = 0.0;
    emscripten::val m_offscreen = emscripten::val::undefined();

    bool m_shouldStop = false;
    bool m_toBePaused = false;
    bool m_isSeeking = false;
    bool m_hasAudio = false;
    bool m_cameraIsReady = false;
    bool m_shouldBeStarted = false;
    bool m_isSeekable = false;

    emscripten::val m_offscreenContext = emscripten::val::undefined();
    QSize m_pendingVideoSize;
    QWasmVideoOutput::WasmVideoMode m_currentVideoMode = QWasmVideoOutput::VideoOutput;
    QMediaPlayer::MediaStatus m_currentMediaStatus;
    qreal m_currentBufferedValue;

    QScopedPointer<QWasmEventHandler> m_timeUpdateEvent;
    QScopedPointer<QWasmEventHandler> m_playEvent;
    QScopedPointer<QWasmEventHandler> m_endedEvent;
    QScopedPointer<QWasmEventHandler> m_durationChangeEvent;
    QScopedPointer<QWasmEventHandler> m_loadedDataEvent;
    QScopedPointer<QWasmEventHandler> m_errorChangeEvent;
    QScopedPointer<QWasmEventHandler> m_resizeChangeEvent;
    QScopedPointer<QWasmEventHandler> m_loadedMetadataChangeEvent;
    QScopedPointer<QWasmEventHandler> m_loadStartChangeEvent;
    QScopedPointer<QWasmEventHandler> m_canPlayChangeEvent;
    QScopedPointer<QWasmEventHandler> m_canPlayThroughChangeEvent;
    QScopedPointer<QWasmEventHandler> m_seekingChangeEvent;
    QScopedPointer<QWasmEventHandler> m_seekedChangeEvent;
    QScopedPointer<QWasmEventHandler> m_emptiedChangeEvent;
    QScopedPointer<QWasmEventHandler> m_stalledChangeEvent;
    QScopedPointer<QWasmEventHandler> m_waitingChangeEvent;
    QScopedPointer<QWasmEventHandler> m_playingChangeEvent;
    QScopedPointer<QWasmEventHandler> m_progressChangeEvent;
    QScopedPointer<QWasmEventHandler> m_pauseChangeEvent;
    QScopedPointer<QWasmEventHandler> m_beforeUnloadEvent;
};

QT_END_NAMESPACE
#endif // QWASMVIDEOOUTPUT_H
