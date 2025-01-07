/****************************************************************************
**
** Copyright (C) 2023 The Qt Company Ltd.
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
******************************************************************************/

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
