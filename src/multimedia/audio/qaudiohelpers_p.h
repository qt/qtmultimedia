// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAUDIOHELPERS_H
#define QAUDIOHELPERS_H

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

#include <qaudioformat.h>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QAudioHelperInternal
{
Q_MULTIMEDIA_EXPORT void qMultiplySamples(qreal factor, const QAudioFormat& format, const void *src, void* dest, int len);
}

QT_END_NAMESPACE

#endif
