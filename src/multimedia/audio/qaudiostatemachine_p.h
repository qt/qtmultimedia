// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAUDIOSTATEMACHINE_P_H
#define QAUDIOSTATEMACHINE_P_H

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

#include <QtMultimedia/qaudio.h>

#include <qmutex.h>
#include <qwaitcondition.h>
#include <qpointer.h>
#include <atomic>
#include <chrono>

QT_BEGIN_NAMESPACE

class QAudioStateChangeNotifier;

/* QAudioStateMachine provides an opportunity to
 * toggle QAudio::State with QAudio::Error in
 * a thread-safe manner.
 * The toggling functions return a notifier,
 * which notifies about the change via
 * QAudioStateChangeNotifier::stateChanged and errorChanged.
 *
 * The state machine is supposed to be used mostly in
 * QAudioSink and QAudioSource implementations.
 */
class Q_MULTIMEDIA_EXPORT QAudioStateMachine
{
public:
    using RawState = int;
    class Notifier
    {
    public:
        void reset()
        {
            if (auto stateMachine = std::exchange(m_stateMachine, nullptr))
                stateMachine->reset(m_state, m_prevState, m_error);
        }

        ~Notifier() { reset(); }

        Notifier(const Notifier &) = delete;
        Notifier(Notifier &&other) noexcept
            : m_stateMachine(std::exchange(other.m_stateMachine, nullptr)),
              m_state(other.m_state),
              m_prevState(other.m_prevState),
              m_error(other.m_error)
        {
        }

        operator bool() const { return m_stateMachine != nullptr; }

        void setError(QAudio::Error error) { m_error = error; }

        // Can be added make state changing more flexible
        // but needs some investigation to ensure state change consistency
        // The method is supposed to be used for sync read/write
        // under "notifier = updateActiveOrIdle(isActive)"
        // void setState(QAudio::State state) { ... }

        bool isStateChanged() const { return m_state != m_prevState; }

        QAudio::State prevState() const { return QAudio::State(m_prevState); }

    private:
        Notifier(QAudioStateMachine *stateMachine = nullptr, RawState state = QAudio::StoppedState,
                 RawState prevState = QAudio::StoppedState, QAudio::Error error = QAudio::NoError)
            : m_stateMachine(stateMachine), m_state(state), m_prevState(prevState), m_error(error)
        {
        }

    private:
        QAudioStateMachine *m_stateMachine;
        RawState m_state;
        const RawState m_prevState;
        QAudio::Error m_error;

        friend class QAudioStateMachine;
    };

    QAudioStateMachine(QAudioStateChangeNotifier &notifier, bool synchronize = true);

    ~QAudioStateMachine();

    QAudio::State state() const;

    QAudio::Error error() const;

    bool isDraining() const;

    bool isActiveOrIdle() const;

    // atomicaly checks if the state is stopped and marked as drained
    std::pair<bool, bool> getDrainedAndStopped() const;

    // waits if the method stop(error, true) has bee called
    void waitForDrained(std::chrono::milliseconds timeout);

    // mark as drained and wake up the method waitForDrained
    void onDrained();

    // Active/Idle/Suspended -> Stopped
    Notifier stop(QAudio::Error error = QAudio::NoError, bool shouldDrain = false,
                  bool forceUpdateError = false);

    // Active/Idle/Suspended -> Stopped
    Notifier stopOrUpdateError(QAudio::Error error = QAudio::NoError)
    {
        return stop(error, false, true);
    }

    // Stopped -> Active/Idle
    Notifier start(bool isActive = true);

    // Active/Idle -> Suspended + saves the exchanged state
    Notifier suspend();

    // Suspended -> saved state (Active/Idle)
    Notifier resume();

    // Idle -> Active
    Notifier activateFromIdle();

    // Active/Idle -> Active/Idle + updateError
    Notifier updateActiveOrIdle(bool isActive, QAudio::Error error = QAudio::NoError);

    // Any -> Any; better use more strict methods
    Notifier forceSetState(QAudio::State state, QAudio::Error error = QAudio::NoError);

    // force set the error
    void setError(QAudio::Error error);

private:
    Notifier changeState(std::pair<RawState, uint32_t> prevStatesSet, RawState state,
                         QAudio::Error error = QAudio::NoError, bool shouldDrain = false);

    void reset(RawState state, RawState prevState, QAudio::Error error);

private:
    QPointer<QAudioStateChangeNotifier> m_notifier;
    std::atomic<RawState> m_state = QAudio::StoppedState;
    std::atomic<QAudio::Error> m_error = QAudio::NoError;
    RawState m_suspendedInState = QAudio::SuspendedState;

    struct Synchronizer;
    std::unique_ptr<Synchronizer> m_sychronizer;
};

QT_END_NAMESPACE

#endif // QAUDIOSTATEMACHINE_P_H
