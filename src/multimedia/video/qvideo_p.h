// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVIDEO_H
#define QVIDEO_H

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

#include <qvideoframe.h>

// The header ensures the code compatibility of qt versions >= 6.7 with 6.6 and 6.5

QT_BEGIN_NAMESPACE

namespace QVideo
{
using RotationAngle = QVideoFrame::RotationAngle;
constexpr RotationAngle Rotation0 = RotationAngle::Rotation0;
constexpr RotationAngle Rotation90 = RotationAngle::Rotation90;
constexpr RotationAngle Rotation180 = RotationAngle::Rotation180;
constexpr RotationAngle Rotation270 = RotationAngle::Rotation270;
}

QT_END_NAMESPACE

#endif // QVIDEO_H
