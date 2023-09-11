// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiostatemachine_p.h"
#include "qaudiosystem_p.h"
#include <qpointer.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

using Notifier = QAudioStateMachine::Notifier;
using namespace AudioStateMachineUtils;

QAudioStateMachine::QAudioStateMachine(QAudioStateChangeNotifier &notifier) : m_notifier(&notifier)
{
}

QAudioStateMachine::~QAudioStateMachine() = default;

QAudio::State QAudioStateMachine::state() const
{
    return toAudioState(m_state.load(std::memory_order_acquire));
}

QAudio::Error QAudioStateMachine::error() const
{
    return toAudioError(m_state.load(std::memory_order_acquire));
}

template <typename StatesChecker, typename NewState>
Notifier QAudioStateMachine::changeState(const StatesChecker &checker, const NewState &newState)
{
    if constexpr (std::is_same_v<RawState, NewState>)
        return changeState(checker, [newState](RawState) { return newState; });
    else {
        RawState prevState = m_state.load(std::memory_order_relaxed);
        const auto exchanged = multipleCompareExchange(m_state, prevState, checker, newState);

        if (Q_LIKELY(exchanged))
            return { this, newState(prevState), prevState };

        return {};
    }
}

Notifier QAudioStateMachine::stop(QAudio::Error error, bool shouldDrain, bool forceUpdateError)
{
    auto statesChecker =
            makeStatesChecker(QAudio::ActiveState, QAudio::IdleState, QAudio::SuspendedState,
                              forceUpdateError ? QAudio::StoppedState : QAudio::ActiveState);

    const auto state = toRawState(QAudio::StoppedState, error);
    auto getNewState = [&](RawState prevState) {
        const bool shouldAddFlag = shouldDrain && toAudioState(prevState) == QAudio::ActiveState;
        return shouldAddFlag ? addDrainingFlag(state) : state;
    };

    return changeState(statesChecker, getNewState);
}

Notifier QAudioStateMachine::start(bool active)
{
    return changeState(makeStatesChecker(QAudio::StoppedState),
                       toRawState(active ? QAudio::ActiveState : QAudio::IdleState));
}

bool QAudioStateMachine::isActiveOrIdle() const
{
    const auto state = this->state();
    return state == QAudio::ActiveState || state == QAudio::IdleState;
}

bool QAudioStateMachine::onDrained()
{
    return changeState(isDrainingState, removeDrainingFlag);
}

bool QAudioStateMachine::isDraining() const
{
    return isDrainingState(m_state.load(std::memory_order_acquire));
}

std::pair<bool, bool> QAudioStateMachine::getDrainedAndStopped() const
{
    const auto state = m_state.load(std::memory_order_acquire);
    return { !isDrainingState(state), toAudioState(state) == QAudio::StoppedState };
}

Notifier QAudioStateMachine::suspend()
{
    // Due to the current documentation, we set QAudio::NoError.
    // TBD: leave the previous error should be more reasonable (IgnoreError)
    const auto error = QAudio::NoError;
    auto result = changeState(makeStatesChecker(QAudio::ActiveState, QAudio::IdleState),
                              toRawState(QAudio::SuspendedState, error));

    if (result)
        m_suspendedInState = result.prevAudioState();

    return result;
}

Notifier QAudioStateMachine::resume()
{
    // Due to the current documentation, we set QAudio::NoError.
    // TBD: leave the previous error should be more reasonable (IgnoreError)
    const auto error = QAudio::NoError;
    return changeState(makeStatesChecker(QAudio::SuspendedState),
                       toRawState(m_suspendedInState, error));
}

Notifier QAudioStateMachine::activateFromIdle()
{
    return changeState(makeStatesChecker(QAudio::IdleState), toRawState(QAudio::ActiveState));
}

Notifier QAudioStateMachine::updateActiveOrIdle(bool isActive, QAudio::Error error)
{
    const auto state = isActive ? QAudio::ActiveState : QAudio::IdleState;
    return changeState(makeStatesChecker(QAudio::ActiveState, QAudio::IdleState),
                       toRawState(state, error));
}

Notifier QAudioStateMachine::setError(QAudio::Error error)
{
    auto fixState = [error](RawState prevState) { return setStateError(prevState, error); };
    return changeState([](RawState) { return true; }, fixState);
}

Notifier QAudioStateMachine::forceSetState(QAudio::State state, QAudio::Error error)
{
    return changeState([](RawState) { return true; }, toRawState(state, error));
}

void QAudioStateMachine::reset(RawState state, RawState prevState)
{
    auto notifier = m_notifier;

    const auto audioState = toAudioState(state);
    const auto audioError = toAudioError(state);

    if (toAudioState(prevState) != audioState && notifier)
        emit notifier->stateChanged(audioState);

    // check the notifier in case the object was deleted in
    if (toAudioError(prevState) != audioError && notifier)
        emit notifier->errorChanged(audioError);
}

QT_END_NAMESPACE
