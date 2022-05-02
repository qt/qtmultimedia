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
******************************************************************************/

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
