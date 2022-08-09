// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQNXAUDIORECORDER_H
#define QQNXAUDIORECORDER_H

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

#include "mmrenderertypes.h"

#include <QByteArray>
#include <QUrl>

#include <QtCore/qobject.h>
#include <QtCore/qtconfigmacros.h>

#include <QtMultimedia/qmediarecorder.h>

#include <private/qplatformmediarecorder_p.h>

#include <mm/renderer.h>
#include <mm/renderer/types.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQnxMediaEventThread;

class QQnxAudioRecorder : public QObject
{
    Q_OBJECT

public:
    explicit QQnxAudioRecorder(QObject *parent = nullptr);
    ~QQnxAudioRecorder();

    void setInputDeviceId(const QByteArray &id);
    void setOutputUrl(const QUrl &url);
    void setMediaEncoderSettings(const QMediaEncoderSettings &settings);

    void record();
    void stop();

Q_SIGNALS:
    void stateChanged(QMediaRecorder::RecorderState state);
    void durationChanged(qint64 durationMs);
    void actualLocationChanged(const QUrl &location);

private:
    void openConnection();
    void closeConnection();
    void attach();
    void detach();
    void configureOutputBitRate();
    void startMonitoring();
    void stopMonitoring();
    void readEvents();
    void handleMmEventStatus(const mmr_event_t *event);
    void handleMmEventState(const mmr_event_t *event);
    void handleMmEventError(const mmr_event_t *event);

    bool isAttached() const;

    struct ContextDeleter
    {
        void operator()(mmr_context_t *ctx) { if (ctx) mmr_context_destroy(ctx); }
    };

    struct ConnectionDeleter
    {
        void operator()(mmr_connection_t *conn) { if (conn) mmr_disconnect(conn); }
    };

    using ContextUniquePtr = std::unique_ptr<mmr_context_t, ContextDeleter>;
    ContextUniquePtr m_context;

    using ConnectionUniquePtr = std::unique_ptr<mmr_connection_t, ConnectionDeleter>;
    ConnectionUniquePtr m_connection;

    int m_id = -1;
    int m_audioId = -1;

    QByteArray m_inputDeviceId;

    QUrl m_outputUrl;

    QMediaEncoderSettings m_encoderSettings;

    std::unique_ptr<QQnxMediaEventThread> m_eventThread;
};

QT_END_NAMESPACE

#endif
