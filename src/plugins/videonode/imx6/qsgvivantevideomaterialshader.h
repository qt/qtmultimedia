// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QSGVIDEOMATERIALSHADER_VIVANTE_H
#define QSGVIDEOMATERIALSHADER_VIVANTE_H

#include <QSGMaterial>

class QSGVivanteVideoMaterialShader : public QSGMaterialShader
{
public:
    QSGVivanteVideoMaterialShader();

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                           QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

    void setUVScale(float uScale, float vScale);

private:
    float mUScale;
    float mVScale;
    bool mNewUVScale;
};

#endif // QSGVIDEOMATERIALSHADER_VIVANTE_H
