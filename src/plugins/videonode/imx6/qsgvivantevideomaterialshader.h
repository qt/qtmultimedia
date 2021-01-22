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

#ifndef QSGVIDEOMATERIALSHADER_VIVANTE_H
#define QSGVIDEOMATERIALSHADER_VIVANTE_H

#include <QSGMaterial>

class QSGVivanteVideoMaterialShader : public QSGMaterialShader
{
public:
    QSGVivanteVideoMaterialShader();

    void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial);
    virtual char const *const *attributeNames() const;

    void setUVScale(float uScale, float vScale);

protected:
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;
    virtual void initialize();

private:
    int mIdMatrix;
    int mIdTexture;
    int mIdOpacity;
    int mIdUVScale;

    float mUScale;
    float mVScale;
    bool mNewUVScale;
};

#endif // QSGVIDEOMATERIALSHADER_VIVANTE_H
