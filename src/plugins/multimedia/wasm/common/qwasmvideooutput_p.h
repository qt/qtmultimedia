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
#include <emscripten/html5.h>

#include <QMediaPlayer>
#include <QVideoFrame>
#include <QtMultimedia/qtvideo.h>

#include "qwasmmediaplayer_p.h"
#include "private/qwasmmediadevices_p.h"
#include "qwasmvideoframegrabber_p.h"

#include <private/qwasmjs_p.h>

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
    enum WasmVideoMode { VideoDisplay, Camera, SurfaceCapture };
    enum WasmCameraMode { Front, Back, Undefined = -1 };
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

    void createVideoElement(const std::string &id);
    void doElementCallbacks();
    void updateVideoElementGeometry(const QRect &windowGeometry);
    void updateVideoElementSource(const QString &src);
    void addCameraSourceElement(const std::string &id);
    void setVideoMode(QWasmVideoOutput::WasmVideoMode mode);
    void setVideoConstraints(QSize resolution, float minFrameRate, float maxFrameRate);

    void setHasAudio(bool needsAudio) { m_hasAudio = needsAudio; }

    emscripten::val getDeviceCapabilities();
    bool setDeviceSetting(const std::string &key, emscripten::val value);
    bool isCameraReady() { return m_cameraIsReady; }

    // mediacapturesession has the videosink
    QVideoSink *m_wasmSink = nullptr;

    emscripten::val currentVideoElement() { return (
        m_video.isNull() || m_video.isUndefined() ? emscripten::val::null()
                                                  : m_video) ; }

    std::string m_videoSurfaceId;

    void removeCurrentVideoElement();

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
    void orientationChanged(int rotationIndex);

private:
    bool isPlatformiOs();
    void detectIosCameraRotation();

    emscripten::val m_video = emscripten::val::undefined();

    QString m_source;
    float m_requestedPosition = 0.0;
    WasmCameraMode m_cameraMode = WasmCameraMode::Undefined;
    QtVideo::Rotation m_rotateBy = QtVideo::Rotation::None;
    int getCurrentOrientationIndex();


    bool m_isStopped = false;
    bool m_toBePaused = false;
    bool m_isSeeking = false;
    bool m_hasAudio = false;
    bool m_cameraIsReady = false;
    bool m_shouldBeStarted = false;
    bool m_streamStarted = false;
    bool m_isSeekable = false;
    bool m_useCameraRotation = false;

    QSize m_pendingVideoSize;
    QWasmVideoOutput::WasmVideoMode m_currentVideoMode = QWasmVideoOutput::VideoDisplay;
    QMediaPlayer::MediaStatus m_currentMediaStatus;
    qreal m_currentBufferedValue;
    JsMediaInputStream *m_mediaInputStream = nullptr;

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

    std::string m_cameraId;
    QMetaObject::Connection m_connection;
    QSize m_videoResolution;
    float m_minFrameRate = 0;
    float m_maxFrameRate = 0;
    float m_streamFrameRate = 0;
    static bool orientationchangeCallback(int eventType, const EmscriptenOrientationChangeEvent *orientationChangeEvent, void *userData);

    QWasmVideoFrameGrabber m_frameGrabber;
    friend class QWasmVideoFrameGrabber;
};

QT_END_NAMESPACE
#endif // QWASMVIDEOOUTPUT_H
