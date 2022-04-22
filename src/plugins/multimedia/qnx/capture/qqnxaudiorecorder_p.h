/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
#include <QtCore/qtnamespacemacros.h>

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
