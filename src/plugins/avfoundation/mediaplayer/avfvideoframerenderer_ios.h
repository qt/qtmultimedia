/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef AVFVIDEOFRAMERENDERER_H
#define AVFVIDEOFRAMERENDERER_H

#include <QtCore/QObject>
#include <QtGui/QImage>
#include <QtGui/QOpenGLContext>
#include <QtCore/QSize>

@class AVPlayerLayer;
@class AVPlayerItemVideoOutput;

QT_BEGIN_NAMESPACE

class QOpenGLContext;
class QOpenGLFramebufferObject;
class QOpenGLShaderProgram;
class QOffscreenSurface;
class QAbstractVideoSurface;

typedef struct __CVBuffer *CVBufferRef;
typedef CVBufferRef CVImageBufferRef;
typedef CVImageBufferRef CVPixelBufferRef;
#if defined(Q_OS_IOS)
typedef struct __CVOpenGLESTextureCache *CVOpenGLESTextureCacheRef;
typedef CVImageBufferRef CVOpenGLESTextureRef;
// helpers to avoid boring if def
typedef CVOpenGLESTextureCacheRef CVOGLTextureCacheRef;
typedef CVOpenGLESTextureRef CVOGLTextureRef;
#define CVOGLTextureGetTarget CVOpenGLESTextureGetTarget
#define CVOGLTextureGetName CVOpenGLESTextureGetName
#define CVOGLTextureCacheCreate CVOpenGLESTextureCacheCreate
#define CVOGLTextureCacheCreateTextureFromImage CVOpenGLESTextureCacheCreateTextureFromImage
#define CVOGLTextureCacheFlush CVOpenGLESTextureCacheFlush
#else
typedef struct __CVOpenGLTextureCache *CVOpenGLTextureCacheRef;
typedef CVImageBufferRef CVOpenGLTextureRef;
// helpers to avoid boring if def
typedef CVOpenGLTextureCacheRef CVOGLTextureCacheRef;
typedef CVOpenGLTextureRef CVOGLTextureRef;
#define CVOGLTextureGetTarget CVOpenGLTextureGetTarget
#define CVOGLTextureGetName CVOpenGLTextureGetName
#define CVOGLTextureCacheCreate CVOpenGLTextureCacheCreate
#define CVOGLTextureCacheCreateTextureFromImage CVOpenGLTextureCacheCreateTextureFromImage
#define CVOGLTextureCacheFlush CVOpenGLTextureCacheFlush
#endif

class AVFVideoFrameRenderer : public QObject
{
public:
    AVFVideoFrameRenderer(QAbstractVideoSurface *surface, QObject *parent = 0);

    virtual ~AVFVideoFrameRenderer();

    void setPlayerLayer(AVPlayerLayer *layer);

    CVOGLTextureRef renderLayerToTexture(AVPlayerLayer *layer);
    QImage renderLayerToImage(AVPlayerLayer *layer);

private:
    void initRenderer();
    CVPixelBufferRef copyPixelBufferFromLayer(AVPlayerLayer *layer, size_t& width, size_t& height);
    CVOGLTextureRef createCacheTextureFromLayer(AVPlayerLayer *layer, size_t& width, size_t& height);

    QOpenGLContext *m_glContext;
    QOffscreenSurface *m_offscreenSurface;
    QAbstractVideoSurface *m_surface;
    CVOGLTextureCacheRef m_textureCache;
    AVPlayerItemVideoOutput* m_videoOutput;
    bool m_isContextShared;
};

QT_END_NAMESPACE

#endif // AVFVIDEOFRAMERENDERER_H
