// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGHWACCEL_VIDEOTOOLBOX_P_H
#define QFFMPEGHWACCEL_VIDEOTOOLBOX_P_H

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

#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>

#ifdef Q_OS_DARWIN

#include <QtCore/private/qcore_mac_p.h>

#include <CoreVideo/CVBase.h>

#include <CoreVideo/CVPixelBuffer.h>
#include <CoreVideo/CVImageBuffer.h>
#include <CoreVideo/CVMetalTexture.h>
#include <CoreVideo/CVMetalTextureCache.h>
#if defined(Q_OS_MACOS)
#include <CoreVideo/CVOpenGLTextureCache.h>
#elif defined(Q_OS_IOS)
#include <CoreVideo/CVOpenGLESTextureCache.h>
#endif

// forward-declare CVMetalTextureCacheRef for non-Objective C code
#if !defined(__OBJC__)
typedef struct CV_BRIDGED_TYPE(id)
        __CVMetalTextureCache *CVMetalTextureCacheRef CV_SWIFT_NONSENDABLE;
#endif

QT_BEGIN_NAMESPACE

class QRhi;

namespace QFFmpeg {

class VideoToolBoxTextureConverter : public TextureConverterBackend
{
public:
    VideoToolBoxTextureConverter(QRhi *rhi);
    ~VideoToolBoxTextureConverter();
    QVideoFrameTexturesHandlesUPtr
    createTextureHandles(AVFrame *frame, QVideoFrameTexturesHandlesUPtr oldHandles) override;

private:
    void freeTextureCaches();

    QCFType<CVMetalTextureCacheRef> cvMetalTextureCache;
#if defined(Q_OS_MACOS)
    QCFType<CVOpenGLTextureCacheRef> cvOpenGLTextureCache;
#elif defined(Q_OS_IOS)
    QCFType<CVOpenGLESTextureCacheRef> cvOpenGLESTextureCache;
#endif
};

}

QT_END_NAMESPACE

#endif

#endif
