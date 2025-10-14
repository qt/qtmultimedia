// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGPLAYBACKENGINEOBJECT_P_H
#define QFFMPEGPLAYBACKENGINEOBJECT_P_H

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

#include <QtCore/qatomic.h>
#include <QtCore/qthread.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qcoreapplication.h>
#include <QtMultimedia/qmediaplayer.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegplaybackenginedefs_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegplaybackutils_p.h>

#include <chrono>
#include <optional>

QT_BEGIN_NAMESPACE

class QChronoTimer;

namespace QFFmpeg {

class PlaybackEngineObject : public QObject
{
    Q_OBJECT

    static constexpr QEvent::Type FuncEventType = QEvent::User;
    class FuncEvent : public QEvent
    {
    public:
        FuncEvent() : QEvent(FuncEventType) { }
        virtual void invoke() = 0;
    };

    template <typename F>
    class FuncEventImpl final : public FuncEvent
    {
    public:
        explicit FuncEventImpl(F &&f) : m_func(std::forward<F>(f)) { }
        void invoke() override { m_func(); }

    private:
        std::decay_t<F> m_func;
    };

public:
    using TimePoint = std::chrono::steady_clock::time_point;
    using TimePointOpt = std::optional<TimePoint>;

    explicit PlaybackEngineObject(const PlaybackEngineObjectID &id);

    ~PlaybackEngineObject() override;

    bool isPaused() const;

    bool isAtEnd() const;

    void kill();

    void setPaused(bool isPaused);

    quint64 objectID() const { return m_id.objectID; }

signals:
    void atEnd(PlaybackEngineObjectID id);

    void error(QMediaPlayer::Error, const QString &errorString);

protected:
    bool event(QEvent *e) override;

    bool checkSessionID(quint64 sessionID) const { return sessionID == m_id.sessionID; }

    bool checkID(const PlaybackEngineObjectID &id) const
    {
        return checkSessionID(id.sessionID) && id.objectID == objectID();
    }

    const PlaybackEngineObjectID &id() const
    {
        Q_ASSERT(thread()->isCurrentThread());
        return m_id;
    }

    template <typename F>
    void invokePriorityMethod(F &&f)
    {
        Q_ASSERT(!thread()->isCurrentThread());
        // Note, that the event loop takes ownership of the event
        QCoreApplication::postEvent(this, new FuncEventImpl<F>(std::forward<F>(f)),
                                    Qt::HighEventPriority);
    }

    QChronoTimer &timer();

    void scheduleNextStep();

    virtual void onPauseChanged();

    virtual bool canDoNextStep() const;

    virtual TimePoint nextTimePoint() const;

    void setAtEnd(bool isAtEnd);

    virtual void doNextStep() { }

private slots:
    void onTimeout();

private:
    enum class StepType : uint8_t { None, Timeout, Immediate };

    void doNextStep(StepType type);

    bool isValid() const { return m_invalidateCounter.load(std::memory_order_relaxed) == 0; }

    std::unique_ptr<QChronoTimer> m_timer;

    QAtomicInteger<bool> m_paused = true;
    QAtomicInteger<bool> m_atEnd = false;
    std::atomic_int m_invalidateCounter = 0;
    PlaybackEngineObjectID m_id;

    TimePointOpt m_nextTimePoint;
    TimePointOpt m_timePoint;
    StepType m_stepType = StepType::None;
};
} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGPLAYBACKENGINEOBJECT_P_H
