// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFVIDEOBUFFER_H
#define AVFVIDEOBUFFER_H

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

#include <private/qhwvideobuffer_p.h>
#include <private/qcore_mac_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>
#include <avfvideosink_p.h>

#include <CoreVideo/CVImageBuffer.h>

#import "Metal/Metal.h"
#import "MetalKit/MetalKit.h"

QT_BEGIN_NAMESPACE

struct AVFMetalTexture;
class AVFVideoBuffer : public QHwVideoBuffer
{
public:
    AVFVideoBuffer(AVFVideoSinkInterface *sink, QCFType<CVImageBufferRef> buffer);
    ~AVFVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    quint64 textureHandle(QRhi &, int plane) override;

    QVideoFrameFormat videoFormat() const { return m_format; }

private:
    AVFVideoSinkInterface *sink = nullptr;

    QCFType<CVMetalTextureCacheRef> metalCache;
    QCFType<CVImageBufferRef> m_buffer;
    QCFType<CVMetalTextureRef> cvMetalTexture[3];
#if defined(Q_OS_MACOS)
    QCFType<CVOpenGLTextureRef> cvOpenGLTexture;
#elif defined(Q_OS_IOS)
    QCFType<CVOpenGLESTextureRef> cvOpenGLESTexture;
#endif

    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
    QVideoFrameFormat m_format;
};

QT_END_NAMESPACE

#endif
