// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef PIPEWIREAPTUREHELPER_P_H
#define PIPEWIREAPTUREHELPER_P_H

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

#include "qpipewirecapture_p.h"

#include <qvideoframe.h>

#include <spa/debug/types.h>
#include <spa/utils/dict.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/video/type-info.h>

#include <pipewire/pipewire.h>

#include <mutex>
#include <memory>

QT_BEGIN_NAMESPACE

class QDBusArgument;
class QDBusInterface;
namespace QtPipeWire {
    class Pipewire;
}

class QPipeWireCaptureHelper : public QObject
{
    Q_OBJECT
public:
    explicit QPipeWireCaptureHelper(QPipeWireCapture &capture);
    ~QPipeWireCaptureHelper() override;

    bool open(int fd);

    static bool isSupported();

    QVideoFrameFormat frameFormat() const;

    bool setActiveInternal(bool active);

    void initPipeWire();
    void deinitPipeWire();

protected:
    void updateError(QPlatformSurfaceCapture::Error error, const QString &description = {});

private:
    void destroy();

    void onCoreEventDone(uint32_t id, int seq);
    void onRegistryEventGlobal(uint32_t id, uint32_t permissions, const char *type, uint32_t version, const spa_dict *props);
    void onStateChanged(pw_stream_state old, pw_stream_state state, const char *error);
    void onProcess();
    void onParamChanged(uint32_t id, const struct spa_pod *param);

    void updateCoreInitSeq();

    void recreateStream();
    void destroyStream(bool forceDrain);

    void signalLoop(bool onProcessDone, bool err);

    static QVideoFrameFormat::PixelFormat toQtPixelFormat(spa_video_format spaVideoFormat);
    static spa_video_format toSpaVideoFormat(QVideoFrameFormat::PixelFormat pixelFormat);

    QString getRequestToken();
    int generateRequestToken();
    void createInterface();
    void createSession();
    void selectSources(const QString &sessionHandle);
    void startStream();
    void updateStreams(const QDBusArgument &streamsInfo);
    void openPipeWireRemote();

private Q_SLOTS:
    void gotRequestResponse(uint result, const QVariantMap &map);

private:
    QPipeWireCapture &m_capture;
    std::shared_ptr<QtPipeWire::Pipewire> m_pipewire;

    QVideoFrame m_currentFrame;
    QVideoFrameFormat m_videoFrameFormat;
    QVideoFrameFormat::PixelFormat m_pixelFormat;
    QSize m_size;

    pw_thread_loop *m_threadLoop = nullptr;
    pw_context *m_context = nullptr;

    pw_core *m_core = nullptr;
    spa_hook m_coreListener = {};

    pw_registry *m_registry = nullptr;
    spa_hook m_registryListener = {};

    pw_stream *m_stream = nullptr;
    spa_hook m_streamListener = {};

    spa_video_info m_format;

    bool m_err = false;
    bool m_hasSource = false;
    bool m_initDone = false;
    bool m_ignoreStateChange = false;
    bool m_streamPaused = false;
    bool m_silence = false;
    bool m_processed = false;

    int m_coreInitSeq = 0;

    int m_requestToken = -1;
    QString m_requestTokenPrefix;
    QString m_sessionHandle = {};

    struct StreamInfo
    {
        quint32 nodeId;
        quint32 sourceType;
        QRect rect;
    };
    QVector<StreamInfo> m_streams = {};

    int m_pipewireFd = -1;

    std::unique_ptr<QDBusInterface> m_screenCastInterface;

    enum OperationState { NoOperation, CreateSession, SelectSources, StartStream, OpenPipeWireRemote };
    OperationState m_operationState = NoOperation;

    enum State { NoState, Starting, Streaming, Stopping };
    State m_state = NoState;
};

QT_END_NAMESPACE

#endif // PIPEWIREAPTUREHELPER_P_H
