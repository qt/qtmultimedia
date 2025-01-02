// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef qffmpegpositionwithoffset_p_H
#define qffmpegpositionwithoffset_p_H

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

#include <qtypes.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

struct LoopOffset
{
    qint64 pos = 0;
    int index = 0;
};

struct PositionWithOffset
{
    qint64 pos = 0;
    LoopOffset offset;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // qffmpegpositionwithoffset_p_H
