// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGCONVERTER_P_H
#define QFFMPEGCONVERTER_P_H

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

#include <QtCore/qtconfigmacros.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

QT_BEGIN_NAMESPACE

class QVideoFrameFormat;
class QVideoFrame;

QVideoFrame convertFrame(QVideoFrame &src, const QVideoFrameFormat &dstFormat);

QT_END_NAMESPACE

#endif
