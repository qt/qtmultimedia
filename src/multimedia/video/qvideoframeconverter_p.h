// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVIDEOFRAMECONVERTER_H
#define QVIDEOFRAMECONVERTER_H

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

QT_BEGIN_NAMESPACE

Q_MULTIMEDIA_EXPORT QImage qImageFromVideoFrame(const QVideoFrame &frame, QVideoFrame::RotationAngle rotation = QVideoFrame::Rotation0, bool mirrorX = false, bool mirrorY = false);

QT_END_NAMESPACE

#endif

