/****************************************************************************
**
** Copyright (C) 2016 Pelagicore AG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgvivantevideomaterialshader.h"
#include "qsgvivantevideonode.h"
#include "qsgvivantevideomaterial.h"
#include "private/qsgvideotexture_p.h"

QSGVivanteVideoMaterialShader::QSGVivanteVideoMaterialShader() :
    mUScale(1),
    mVScale(1),
    mNewUVScale(true)
{
    setShaderFileName(VertexStage, QStringLiteral(":/imx6/shaders/rgba.vert.qsb"));
    setShaderFileName(FragmentStage, QStringLiteral(":/imx6/shaders/rgba.frag.qsb"));
}

bool QSGVivanteVideoMaterialShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                                                      QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    bool changed = false;
    QByteArray *buf = state.uniformData();

    if (state.isMatrixDirty()) {
        memcpy(buf->data(), state.combinedMatrix().constData(), 64);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        auto m = static_cast<QSGVivanteVideoMaterial *>(newMaterial);
        m->mOpacity = state.opacity();
        memcpy(buf->data() + 64, &m->mOpacity, 4);
        changed = true;
    }

    if (mNewUVScale) {
        memcpy(buf->data() + 64 + 4, &mUScale, 4);
        memcpy(buf->data() + 64 + 4 + 4, &mVScale, 4);
        changed = true;
        mNewUVScale = false;
    }

    return changed;
}

void QSGVivanteVideoMaterialShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                       QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    if (binding < 1)
        return;

    auto m = static_cast<QSGVivanteVideoMaterial *>(newMaterial);
    if (!m->mTexture)
        m->mTexture.reset(new QSGVideoTexture);

    m->bind();
    m->mTexture->setNativeObject(m->mCurrentTexture, m->mCurrentFrame.size());
    m->mTexture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
    *texture = m->mTexture.data();
}

void QSGVivanteVideoMaterialShader::setUVScale(float uScale, float vScale)
{
    mUScale = uScale;
    mVScale = vScale;
    mNewUVScale = true;
}
