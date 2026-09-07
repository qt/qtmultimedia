// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVAUDIOSESSIONRECOVERY_P_H
#define QAVAUDIOSESSIONRECOVERY_P_H

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

#include <QtMultimedia/private/qavaudiosessionmanager_p.h>
#include <QtMultimedia/qaudio.h>

#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

template <typename EndpointType>
class QAVAudioSessionRecovery
{
public:
    explicit QAVAudioSessionRecovery(EndpointType &endpoint) : m_endpoint(endpoint)
    {
        QAVAudioSessionManager *monitor = QAVAudioSessionManager::instance();

        QObject::connect(monitor, &QAVAudioSessionManager::interruptionBegan, &m_endpoint,
                         [this] {
            onInterruptionBegan();
        });
        QObject::connect(monitor, &QAVAudioSessionManager::interruptionEnded, &m_endpoint,
                         [this](bool shouldResume) {
            onInterruptionEnded(shouldResume);
        });
        QObject::connect(monitor, &QAVAudioSessionManager::mediaServicesWereLost, &m_endpoint,
                         [this] {
            onMediaServicesInvalidated();
        });
        QObject::connect(monitor, &QAVAudioSessionManager::mediaServicesWereReset, &m_endpoint,
                         [this] {
            onMediaServicesInvalidated();
        });

        if (qGuiApp)
            QObject::connect(qGuiApp, &QGuiApplication::applicationStateChanged, &m_endpoint,
                             [this](Qt::ApplicationState state) {
                if (state == Qt::ApplicationState::ApplicationActive)
                    onApplicationActivated();
            });
    }

    Q_DISABLE_COPY_MOVE(QAVAudioSessionRecovery)

    void userResumeRequested()
    {
        if (m_interrupted)
            activateSession();
        m_interrupted = false;
    }
    void streamStopped() { m_interrupted = false; }

private:
    bool isStreamActive() const
    {
        if (!m_endpoint.currentStream())
            return false;

        const QAudio::State state = m_endpoint.state();
        return state == QAudio::ActiveState || state == QAudio::IdleState;
    }

    void onInterruptionBegan()
    {
        if (!isStreamActive())
            return;

        m_interrupted = true;
        m_endpoint.updateStreamState(QAudio::SuspendedState);
    }

    void onInterruptionEnded(bool shouldResume)
    {
        if (!m_interrupted)
            return;

        if (!shouldResume)
            return;

        recover();
    }

    void onMediaServicesInvalidated()
    {
        auto *stream = m_endpoint.currentStream();
        if (!stream)
            return;

        m_interrupted = false;
        stream->invalidateAudioUnit();
        stream->reportIOError();
    }

    void onApplicationActivated()
    {
        if (m_interrupted)
            recover();
        else
            m_endpoint.resumeStreamIfNecessary();
    }

    void recover()
    {
        m_interrupted = false;
        auto *stream = m_endpoint.currentStream();
        if (!stream) {
            return;
        }

        const bool resumed = activateSession() && stream->resume();
        if (resumed)
            m_endpoint.updateStreamState(QAudio::ActiveState);
        else
            stream->reportIOError();
    }

    static bool activateSession()
    {
        QAVAudioSessionManager *monitor = QAVAudioSessionManager::instance();
        return monitor && monitor->reassertActivation();
    }

    EndpointType& m_endpoint;
    bool m_interrupted = false;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QAVAUDIOSESSIONRECOVERY_P_H
