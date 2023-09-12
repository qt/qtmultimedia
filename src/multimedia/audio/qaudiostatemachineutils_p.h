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
constexpr RawState DrainingFlag = 1 << 16;
constexpr RawState InProgressFlag = 1 << 17;
constexpr RawState WaitingFlags = DrainingFlag | InProgressFlag;

constexpr bool isWaitingState(RawState state)
{
    return (state & WaitingFlags) != 0;
}

constexpr bool isDrainingState(RawState state)
{
    return (state & DrainingFlag) != 0;
}

constexpr RawState fromWaitingState(RawState state)
{
    return state & ~WaitingFlags;
}

constexpr QAudio::State toAudioState(RawState state)
{
    return QAudio::State(fromWaitingState(state));
}

template <typename... States>
constexpr std::pair<RawState, uint32_t> makeStatesSet(QAudio::State first, States... others)
{
    return { first, ((1 << first) | ... | (1 << others)) };
}

// ensures compareExchange (testAndSet) operation with opportunity
// to check several states, can be considered as atomic
template <typename T, typename Predicate>
bool multipleCompareExchange(std::atomic<T> &target, T &prevValue, T newValue, Predicate predicate)
{
    Q_ASSERT(predicate(prevValue));
    do {
        if (target.compare_exchange_strong(prevValue, newValue))
            return true;
    } while (predicate(prevValue));

    return false;
}
} // namespace AudioStateMachineUtils

QT_END_NAMESPACE

#endif // QAUDIOSTATEMACHINEUTILS_P_H
