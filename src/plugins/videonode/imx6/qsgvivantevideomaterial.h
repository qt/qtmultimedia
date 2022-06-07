// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGVIDEOMATERIAL_VIVMAP_H
#define QSGVIDEOMATERIAL_VIVMAP_H

#include <QList>
#include <QPair>

#include <QSGMaterial>
#include <QVideoFrame>
#include <QMutex>

#include <private/qsgvideonode_p.h>

class QSGVivanteVideoMaterialShader;
class QSGVideoTexture;
class QSGVivanteVideoMaterial : public QSGMaterial
{
public:
    QSGVivanteVideoMaterial();
    ~QSGVivanteVideoMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override;
    int compare(const QSGMaterial *other) const override;
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
    QVideoFrameFormat::PixelFormat mFormat;

    QMap<const uchar*, GLuint> mBitsToTextureMap;
    QVideoFrame mCurrentFrame;
    GLuint mCurrentTexture;
    bool mMappable;
    QScopedPointer<QSGVideoTexture> mTexture;
    GLenum mMapError = GL_NO_ERROR;

    QMutex mFrameMutex;

    GLuint mTexDirectTexture;
    GLvoid *mTexDirectPlanes[3];

    QSGVivanteVideoMaterialShader *mShader;
    friend class QSGVivanteVideoMaterialShader;
};

#endif // QSGVIDEOMATERIAL_VIVMAP_H
