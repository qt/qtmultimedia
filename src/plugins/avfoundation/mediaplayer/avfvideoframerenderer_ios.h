/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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
#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
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
    AVFVideoFrameRenderer(QAbstractVideoSurface *surface, QObject *parent = nullptr);

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
