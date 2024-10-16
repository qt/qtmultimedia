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

#include "qaudiostatemachineutils_p.h"

#include <qpointer.h>
#include <atomic>

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
    using RawState = AudioStateMachineUtils::RawState;
    class Notifier
    {
    public:
        void reset()
        {
            if (auto stateMachine = std::exchange(m_stateMachine, nullptr))
                stateMachine->reset(m_state, m_prevState);
        }

        ~Notifier() { reset(); }

        Notifier(const Notifier &) = delete;
        Notifier(Notifier &&other) noexcept
            : m_stateMachine(std::exchange(other.m_stateMachine, nullptr)),
              m_state(other.m_state),
              m_prevState(other.m_prevState)
        {
        }

        operator bool() const { return m_stateMachine != nullptr; }

        QAudio::State prevAudioState() const { return AudioStateMachineUtils::toAudioState(m_prevState); }

        QAudio::State audioState() const { return AudioStateMachineUtils::toAudioState(m_state); }

        bool isDraining() const { return AudioStateMachineUtils::isDrainingState(m_state); }

        bool isStateChanged() const { return prevAudioState() != audioState(); }

    private:
        Notifier(QAudioStateMachine *stateMachine = nullptr, RawState state = QAudio::StoppedState,
                 RawState prevState = QAudio::StoppedState)
            : m_stateMachine(stateMachine), m_state(state), m_prevState(prevState)
        {
        }

    private:
        QAudioStateMachine *m_stateMachine;
        RawState m_state;
        const RawState m_prevState;

        friend class QAudioStateMachine;
    };

    enum class RunningState {
        Active = QtAudio::State::ActiveState,
        Idle = QtAudio::State::IdleState
    };

    QAudioStateMachine(QAudioStateChangeNotifier &notifier);

    ~QAudioStateMachine();

    QAudio::State state() const;

    QAudio::Error error() const;

    bool isActiveOrIdle() const;

    bool isDraining() const;

    // atomicaly checks if the state is stopped and marked as drained
    std::pair<bool, bool> getDrainedAndStopped() const;

    // Stopped[draining] -> Stopped
    bool onDrained();

    // Active/Idle/Suspended -> Stopped
    // or Active -> Stopped[draining] for shouldDrain = true
    Notifier stop(QAudio::Error error = QAudio::NoError, bool shouldDrain = false,
                  bool forceUpdateError = false);

    // Active/Idle/Suspended -> Stopped
    Notifier stopOrUpdateError(QAudio::Error error = QAudio::NoError)
    {
        return stop(error, false, true);
    }

    // Stopped -> Active/Idle
    Notifier start(RunningState activeOrIdle = RunningState::Active);

    // Active/Idle -> Suspended + saves the exchanged state
    Notifier suspend();

    // Suspended -> saved state (Active/Idle)
    Notifier resume();

    // Idle -> Active
    Notifier activateFromIdle();

    // Active/Idle -> Active/Idle + updateError
    Notifier updateActiveOrIdle(RunningState activeOrIdle, QAudio::Error error = QAudio::NoError);

    // Any -> Any; better use more strict methods
    Notifier forceSetState(QAudio::State state, QAudio::Error error = QAudio::NoError);

    // force set the error
    Notifier setError(QAudio::Error error);

private:
    template <typename StatesChecker, typename NewState>
    Notifier changeState(const StatesChecker &statesChecker, const NewState &newState);

    void reset(RawState state, RawState prevState);

private:
    QPointer<QAudioStateChangeNotifier> m_notifier;
    std::atomic<RawState> m_state = QAudio::StoppedState;
    QAudio::State m_suspendedInState = QAudio::SuspendedState;
};

QT_END_NAMESPACE

#endif // QAUDIOSTATEMACHINE_P_H
