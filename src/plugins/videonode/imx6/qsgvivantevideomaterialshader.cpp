// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
