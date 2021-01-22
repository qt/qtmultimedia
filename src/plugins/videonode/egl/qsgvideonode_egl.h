/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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

#ifndef EGLVIDEONODE_H
#define EGLVIDEONODE_H

#include <private/qsgvideonode_p.h>

#include <QSGOpaqueTextureMaterial>
#include <QSGTexture>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifdef Bool
#  undef Bool
#endif
#ifdef None
#  undef None
#endif

QT_BEGIN_NAMESPACE

class QSGVideoMaterial_EGL : public QSGMaterial
{
public:
    QSGVideoMaterial_EGL();
    ~QSGVideoMaterial_EGL();

    QSGMaterialShader *createShader() const;
    QSGMaterialType *type() const;
    int compare(const QSGMaterial *other) const;

    void setImage(EGLImageKHR image);

private:
    friend class QSGVideoMaterial_EGLShader;

    QRectF m_subrect;
    EGLImageKHR m_image;
    GLuint m_textureId;
};

class QSGVideoNode_EGL : public QSGVideoNode
{
public:
    QSGVideoNode_EGL(const QVideoSurfaceFormat &format);
    ~QSGVideoNode_EGL();

    void setCurrentFrame(const QVideoFrame &frame, FrameFlags flags);
    QVideoFrame::PixelFormat pixelFormat() const;
    QAbstractVideoBuffer::HandleType handleType() const;

private:
    QSGVideoMaterial_EGL m_material;
    QVideoFrame::PixelFormat m_pixelFormat;
};

class QSGVideoNodeFactory_EGL : public QSGVideoNodeFactoryPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.sgvideonodefactory/5.2" FILE "egl.json")
public:
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handleType) const;
    QSGVideoNode *createNode(const QVideoSurfaceFormat &format);
};

QT_END_NAMESPACE

#endif

