/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qsgvideonode_p.h"
#include <QtQuick/qsgmaterial.h>
#include "qsgvideotexture_p.h"
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <qmutex.h>

QT_BEGIN_NAMESPACE

/* Helpers */
static inline void qSetGeom(QSGGeometry::TexturedPoint2D *v, const QPointF &p)
{
    v->x = p.x();
    v->y = p.y();
}

static inline void qSetTex(QSGGeometry::TexturedPoint2D *v, const QPointF &p)
{
    v->tx = p.x();
    v->ty = p.y();
}

/* Update the vertices and texture coordinates.  Orientation must be in {0,90,180,270} */
void QSGVideoNode::setTexturedRectGeometry(const QRectF &rect, const QRectF &textureRect, int orientation)
{
    if (rect == m_rect && textureRect == m_textureRect && orientation == m_orientation)
        return;

    m_rect = rect;
    m_textureRect = textureRect;
    m_orientation = orientation;

    QSGGeometry *g = geometry();

    if (g == nullptr)
        g = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);

    QSGGeometry::TexturedPoint2D *v = g->vertexDataAsTexturedPoint2D();

    // Set geometry first
    qSetGeom(v + 0, rect.topLeft());
    qSetGeom(v + 1, rect.bottomLeft());
    qSetGeom(v + 2, rect.topRight());
    qSetGeom(v + 3, rect.bottomRight());

    // and then texture coordinates
    switch (orientation) {
        default:
            // tl, bl, tr, br
            qSetTex(v + 0, textureRect.topLeft());
            qSetTex(v + 1, textureRect.bottomLeft());
            qSetTex(v + 2, textureRect.topRight());
            qSetTex(v + 3, textureRect.bottomRight());
            break;

        case 90:
            // tr, tl, br, bl
            qSetTex(v + 0, textureRect.topRight());
            qSetTex(v + 1, textureRect.topLeft());
            qSetTex(v + 2, textureRect.bottomRight());
            qSetTex(v + 3, textureRect.bottomLeft());
            break;

        case 180:
            // br, tr, bl, tl
            qSetTex(v + 0, textureRect.bottomRight());
            qSetTex(v + 1, textureRect.topRight());
            qSetTex(v + 2, textureRect.bottomLeft());
            qSetTex(v + 3, textureRect.topLeft());
            break;

        case 270:
            // bl, br, tl, tr
            qSetTex(v + 0, textureRect.bottomLeft());
            qSetTex(v + 1, textureRect.bottomRight());
            qSetTex(v + 2, textureRect.topLeft());
            qSetTex(v + 3, textureRect.topRight());
            break;
    }

    if (!geometry())
        setGeometry(g);

    markDirty(DirtyGeometry);
}

class QSGVideoMaterial;

class QSGVideoMaterialRhiShader : public QSGMaterialShader
{
public:
    QSGVideoMaterialRhiShader(const QSGVideoMaterial *material, const QVideoFrameFormat &format)
        : m_material(material),
        m_format(format)
    {
        setShaderFileName(VertexStage, m_format.vertexShaderFileName());
        setShaderFileName(FragmentStage, m_format.fragmentShaderFileName());
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                           QSGMaterial *oldMaterial) override;

    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

protected:
    const QSGVideoMaterial *m_material = nullptr;
    QVideoFrameFormat m_format;
    float m_planeWidth[3] = {0, 0, 0};
    QMatrix4x4 m_colorMatrix;
};

class QSGVideoMaterial : public QSGMaterial
{
public:
    QSGVideoMaterial(const QVideoFrameFormat &format);

    [[nodiscard]] QSGMaterialType *type() const override {
        static QSGMaterialType type[QVideoFrameFormat::NPixelFormats];
        return &type[m_format.pixelFormat()];
    }

    [[nodiscard]] QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override {
        return new QSGVideoMaterialRhiShader(this, m_format);
    }

    int compare(const QSGMaterial *other) const override {
        const QSGVideoMaterial *m = static_cast<const QSGVideoMaterial *>(other);

        qint64 diff = m_textures[0]->comparisonKey() - m->m_textures[0]->comparisonKey();
        if (!diff)
            diff = m_textures[1]->comparisonKey() - m->m_textures[1]->comparisonKey();
        if (!diff)
            diff = m_textures[2]->comparisonKey() - m->m_textures[2]->comparisonKey();

        return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
    }

    void updateBlending() {
        // ### respect video formats with Alpha
        setFlag(Blending, !qFuzzyCompare(m_opacity, float(1.0)));
    }

    void setCurrentFrame(const QVideoFrame &frame) {
        QMutexLocker lock(&m_frameMutex);
        m_frame = frame;
        m_texturesDirty = true;
    }

    void updateTextures(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates);

    QVideoFrameFormat m_format;
    float m_planeWidth[3];
    float m_opacity;

    QMutex m_frameMutex;
    bool m_texturesDirty = false;
    QVideoFrame m_frame;

    QScopedPointer<QSGVideoTexture> m_textures[3];
};

void QSGVideoMaterial::updateTextures(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    QMutexLocker locker(&m_frameMutex);
    if (!m_texturesDirty)
        return;

    // update and upload all textures
    QRhiTexture *textures[3] = {};
    for (int i = 0; i < 3; ++i) {
        if (m_textures[i].data())
            textures[i] = m_textures[i].data()->rhiTexture();
        else
            textures[i] = nullptr;
    }

    QVideoTextureHelper::updateRhiTextures(m_frame, rhi, resourceUpdates, textures);

    for (int i = 0; i < 3; ++i) {
        if (m_textures[i].data())
            m_textures[i].data()->setRhiTexture(textures[i]);
    }
}


bool QSGVideoMaterialRhiShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                                                      QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    auto m = static_cast<QSGVideoMaterial *>(newMaterial);

    if (!state.isMatrixDirty() && !state.isOpacityDirty())
        return false;

    if (state.isOpacityDirty()) {
        m->m_opacity = state.opacity();
        m->updateBlending();
    }

    QByteArray *buf = state.uniformData();
    *buf = m_format.uniformData(m_material->m_frame, state.combinedMatrix(), state.opacity());

    return true;
}

void QSGVideoMaterialRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                       QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);
    if (binding < 1 || binding > 3)
        return;

    auto m = static_cast<QSGVideoMaterial *>(newMaterial);

    m->updateTextures(state.rhi(), state.resourceUpdateBatch());

    *texture = m->m_textures[binding - 1].data();
}

QSGVideoMaterial::QSGVideoMaterial(const QVideoFrameFormat &format) :
    m_format(format),
    m_opacity(1.0)
{
    m_textures[0].reset(new QSGVideoTexture);
    m_textures[1].reset(new QSGVideoTexture);
    m_textures[2].reset(new QSGVideoTexture);

    setFlag(Blending, false);
}

QSGVideoNode::QSGVideoNode(const QVideoFrameFormat &format)
    : m_orientation(-1),
    m_format(format)
{
    setFlag(QSGNode::OwnsMaterial);
    m_material = new QSGVideoMaterial(format);
    setMaterial(m_material);
}

void QSGVideoNode::setCurrentFrame(const QVideoFrame &frame)
{
    m_material->setCurrentFrame(frame);
    markDirty(DirtyMaterial);
}

QT_END_NAMESPACE
