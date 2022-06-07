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

#include <QtMultimedia/qvideoframe.h>
#include <private/qabstractvideobuffer_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>
#include <avfvideosink_p.h>

#include <CoreVideo/CVImageBuffer.h>

#import "Metal/Metal.h"
#import "MetalKit/MetalKit.h"

QT_BEGIN_NAMESPACE

struct AVFMetalTexture;
class AVFVideoBuffer : public QAbstractVideoBuffer
{
public:
    AVFVideoBuffer(AVFVideoSinkInterface *sink, CVImageBufferRef buffer);
    ~AVFVideoBuffer();

    QVideoFrame::MapMode mapMode() const { return m_mode; }
    MapData map(QVideoFrame::MapMode mode);
    void unmap();

    virtual quint64 textureHandle(int plane) const;

    QVideoFrameFormat videoFormat() const { return m_format; }

private:
    AVFVideoSinkInterface *sink = nullptr;

    mutable CVMetalTextureRef cvMetalTexture[3] = {};

#if defined(Q_OS_MACOS)
    mutable CVOpenGLTextureRef cvOpenGLTexture = nullptr;
#elif defined(Q_OS_IOS)
    mutable CVOpenGLESTextureRef cvOpenGLESTexture = nullptr;
#endif

    CVImageBufferRef m_buffer = nullptr;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
    QVideoFrameFormat m_format;
};

QT_END_NAMESPACE

#endif
