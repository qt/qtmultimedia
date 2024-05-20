// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGENCODERTHREAD_P_H
#define QFFMPEGENCODERTHREAD_P_H

#include "qffmpegthread_p.h"
#include "qpointer.h"

#include "private/qmediainputencoderinterface_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class RecordingEngine;

class EncoderThread : public ConsumerThread, public QMediaInputEncoderInterface
{
    Q_OBJECT
public:
    EncoderThread(RecordingEngine &recordingEngine);

    void setPaused(bool paused);

    void setSource(QObject *source) { m_source = source; }

    QObject *source() const { return m_source; }

    bool canPushFrame() const override { return m_canPushFrame.load(std::memory_order_relaxed); }

protected:
    void updateCanPushFrame();

    virtual bool checkIfCanPushFrame() const = 0;

    auto lockLoopData()
    {
        return QScopeGuard([this, locker = ConsumerThread::lockLoopData()]() mutable {
            const bool canPush = !m_paused && checkIfCanPushFrame();
            locker.unlock();
            if (m_canPushFrame.exchange(canPush, std::memory_order_relaxed) != canPush)
                emit canPushFrameChanged();
        });
    }

Q_SIGNALS:
    void canPushFrameChanged();

protected:
    bool m_paused = false;
    std::atomic_bool m_canPushFrame = false;
    RecordingEngine &m_recordingEngine;
    QPointer<QObject> m_source;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
