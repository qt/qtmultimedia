/****************************************************************************
**
** Copyright (C) 2016 Pelagicore AG
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

#ifndef QSGVIDEOMATERIAL_VIVMAP_H
#define QSGVIDEOMATERIAL_VIVMAP_H

#include <QList>
#include <QPair>

#include <QSGMaterial>
#include <QVideoFrame>
#include <QMutex>

#include <private/qsgvideonode_p.h>

class QSGVivanteVideoMaterialShader;

class QSGVivanteVideoMaterial : public QSGMaterial
{
public:
    QSGVivanteVideoMaterial();
    ~QSGVivanteVideoMaterial();

    virtual QSGMaterialType *type() const;
    virtual QSGMaterialShader *createShader() const;
    virtual int compare(const QSGMaterial *other) const;
    void updateBlending();
    void setCurrentFrame(const QVideoFrame &frame, QSGVideoNode::FrameFlags flags);

    void bind();
    GLuint vivanteMapping(QVideoFrame texIdVideoFramePair);

    void setOpacity(float o) { mOpacity = o; }

private:
    void clearTextures();

    qreal mOpacity;

    int mWidth;
    int mHeight;
    QVideoFrame::PixelFormat mFormat;

    QMap<const uchar*, GLuint> mBitsToTextureMap;
    QVideoFrame mCurrentFrame;
    GLuint mCurrentTexture;
    bool mMappable;
    GLenum mMapError = GL_NO_ERROR;

    QMutex mFrameMutex;

    GLuint mTexDirectTexture;
    GLvoid *mTexDirectPlanes[3];

    QSGVivanteVideoMaterialShader *mShader;
};

#endif // QSGVIDEOMATERIAL_VIVMAP_H
