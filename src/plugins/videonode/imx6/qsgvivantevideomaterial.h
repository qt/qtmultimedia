/****************************************************************************
**
** Copyright (C) 2014 Pelagicore AG
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
    QVideoFrame mCurrentFrame, mNextFrame;
    GLuint mCurrentTexture;
    bool mMappable;

    QMutex mFrameMutex;

    GLuint mTexDirectTexture;
    GLvoid *mTexDirectPlanes[3];

    QSGVivanteVideoMaterialShader *mShader;
};

#endif // QSGVIDEOMATERIAL_VIVMAP_H
