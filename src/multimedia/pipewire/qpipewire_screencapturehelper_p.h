// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_SCREENCAPTUREHELPER_P_H
#define QPIPEWIRE_SCREENCAPTUREHELPER_P_H

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

#include "qpipewire_screencapture_p.h"
#include "qpipewire_support_p.h"

#include <QtMultimedia/qvideoframe.h>

#include <pipewire/pipewire.h>
#include <spa/debug/types.h>
#include <spa/utils/dict.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/video/type-info.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QDBusArgument;
class QDBusInterface;

namespace QtPipeWire {

class QPipeWireInstance;

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
    std::shared_ptr<QPipeWireInstance> m_instance;
    QPipeWireCapture &m_capture;

    QVideoFrame m_currentFrame;
    QVideoFrameFormat m_videoFrameFormat;
    QVideoFrameFormat::PixelFormat m_pixelFormat{};
    QSize m_size;

    PwThreadLoopHandle m_threadLoop;
    PwContextHandle m_context;

    PwCoreConnectionHandle m_core;
    spa_hook m_coreListener = {};

    PwRegistryHandle m_registry = nullptr;
    spa_hook m_registryListener = {};

    PwStreamHandle m_stream = nullptr;
    spa_hook m_streamListener = {};

    spa_video_info m_format{};

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
    QString m_sessionHandle;

    struct StreamInfo
    {
        quint32 nodeId;
        quint32 sourceType;
        QRect rect;
    };
    QVector<StreamInfo> m_streams;

    int m_pipewireFd = -1;

    std::unique_ptr<QDBusInterface> m_screenCastInterface;

    enum OperationState { NoOperation, CreateSession, SelectSources, StartStream, OpenPipeWireRemote };
    OperationState m_operationState = NoOperation;

    enum State { NoState, Starting, Streaming, Stopping };
    State m_state = NoState;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_SCREENCAPTUREHELPER_P_H
