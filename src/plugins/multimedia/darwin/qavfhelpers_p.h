// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QAVFHELPERS_H
#define QAVFHELPERS_H

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

#include <QtMultimedia/qvideoframe.h>
#include <qvideoframeformat.h>

#include <CoreVideo/CVBase.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <CoreVideo/CVImageBuffer.h>

QT_BEGIN_NAMESPACE

using CvPixelFormat = unsigned;
constexpr CvPixelFormat CvPixelFormatInvalid = 0;

namespace QAVFHelpers
{
QVideoFrameFormat::ColorRange colorRangeForCVPixelFormat(CvPixelFormat cvPixelFormat);
QVideoFrameFormat::PixelFormat fromCVPixelFormat(CvPixelFormat cvPixelFormat);
CvPixelFormat toCVPixelFormat(QVideoFrameFormat::PixelFormat pixFmt,
                              QVideoFrameFormat::ColorRange colorRange);

QVideoFrameFormat videoFormatForImageBuffer(CVImageBufferRef buffer, bool openGL = false);
};

QT_END_NAMESPACE

#endif
