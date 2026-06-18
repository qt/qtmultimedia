// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGDARWINHWFRAMEHELPERS_P_H
#define QFFMPEGDARWINHWFRAMEHELPERS_P_H

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

#include <QtCore/private/qexpected_p.h>

#define AVMediaType XAVMediaType
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#undef AVMediaType

#include <QtMultimedia/private/qavfhelpers_p.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

[[nodiscard]] q23::expected<QVideoFrame, QString> qVideoFrameFromCvPixelBuffer(
    const QFFmpeg::HWAccel &hwAccel,
    std::chrono::microseconds presentationTimeStamp,
    const QAVFHelpers::QSharedCVPixelBuffer &imageBuffer,
    QVideoFrameFormat format);

[[nodiscard]] q23::expected<QAVFHelpers::QSharedCVPixelBuffer, QString> deepCopyCvPixelBuffer(
    CVPixelBufferRef);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGDARWINHWFRAMEHELPERS_P_H
