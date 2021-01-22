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

@class CARenderer;
@class AVPlayerLayer;

QT_BEGIN_NAMESPACE

class QOpenGLFramebufferObject;
class QWindow;
class QOpenGLContext;
class QAbstractVideoSurface;

class AVFVideoFrameRenderer : public QObject
{
public:
    AVFVideoFrameRenderer(QAbstractVideoSurface *surface, QObject *parent = nullptr);

    virtual ~AVFVideoFrameRenderer();

    GLuint renderLayerToTexture(AVPlayerLayer *layer);
    QImage renderLayerToImage(AVPlayerLayer *layer);

private:
    QOpenGLFramebufferObject* initRenderer(AVPlayerLayer *layer);
    void renderLayerToFBO(AVPlayerLayer *layer, QOpenGLFramebufferObject *fbo);

    CARenderer *m_videoLayerRenderer;
    QAbstractVideoSurface *m_surface;
    QOpenGLFramebufferObject *m_fbo[2];
    QWindow *m_offscreenSurface;
    QOpenGLContext *m_glContext;
    QSize m_targetSize;

    uint m_currentBuffer;
    bool m_isContextShared;
};

QT_END_NAMESPACE

#endif // AVFVIDEOFRAMERENDERER_H
