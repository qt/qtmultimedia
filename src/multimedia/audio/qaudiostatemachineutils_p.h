// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAUDIOSTATEMACHINEUTILS_P_H
#define QAUDIOSTATEMACHINEUTILS_P_H

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

#include "qaudio.h"

QT_BEGIN_NAMESPACE

namespace AudioStateMachineUtils {

using RawState = int;

constexpr uint32_t AudioStateBitsCount = 8;
constexpr RawState AudioStateMask = 0xFF;
constexpr RawState AudioErrorMask = 0xFF00;
constexpr RawState DrainingFlag = 0x10000;

static_assert(!(AudioStateMask & DrainingFlag) && !(AudioStateMask & AudioErrorMask)
                      && !(AudioErrorMask & DrainingFlag),
              "Invalid masks");

constexpr bool isDrainingState(RawState state)
{
    return (state & DrainingFlag) != 0;
}

constexpr RawState addDrainingFlag(RawState state)
{
    return state | DrainingFlag;
}

constexpr RawState removeDrainingFlag(RawState state)
{
    return state & ~DrainingFlag;
}

constexpr QAudio::State toAudioState(RawState state)
{
    return QAudio::State(state & AudioStateMask);
}

constexpr QAudio::Error toAudioError(RawState state)
{
    return QAudio::Error((state & AudioErrorMask) >> AudioStateBitsCount);
}

constexpr RawState toRawState(QAudio::State state, QAudio::Error error = QAudio::NoError)
{
    return state | (error << AudioStateBitsCount);
}

constexpr RawState setStateError(RawState state, QAudio::Error error)
{
    return (error << AudioStateBitsCount) | (state & ~AudioErrorMask);
}

template <typename... States>
constexpr auto makeStatesChecker(States... states)
{
    return [=](RawState state) {
        state &= (AudioStateMask | DrainingFlag);
        return (... || (state == states));
    };
}

// ensures compareExchange (testAndSet) operation with opportunity
// to check several states, can be considered as atomic
template <typename T, typename Predicate, typename NewValueGetter>
bool multipleCompareExchange(std::atomic<T> &target, T &prevValue, Predicate predicate,
                             NewValueGetter newValueGetter)
{
    while (predicate(prevValue))
        if (target.compare_exchange_strong(prevValue, newValueGetter(prevValue),
                                           std::memory_order_acq_rel))
            return true;

    return false;
}
} // namespace AudioStateMachineUtils

QT_END_NAMESPACE

#endif // QAUDIOSTATEMACHINEUTILS_P_H
