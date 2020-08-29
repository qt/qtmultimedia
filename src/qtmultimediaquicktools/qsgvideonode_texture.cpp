/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include "qsgvideonode_texture_p.h"
#include "qsgvideotexture_p.h"
#include <private/qsgrhisupport_p.h>
#include <QtQuick/qsgmaterial.h>
#include <QtCore/qmutex.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

QT_BEGIN_NAMESPACE

QList<QVideoFrame::PixelFormat> QSGVideoNodeFactory_Texture::supportedPixelFormats(
                                        QAbstractVideoBuffer::HandleType handleType) const
{
    QList<QVideoFrame::PixelFormat> pixelFormats;

    QList<QAbstractVideoBuffer::HandleType> types;

    auto rhi = QSGRhiSupport::instance();
    auto metalEnabled = rhi->isRhiEnabled() && rhi->rhiBackend() == QRhi::Metal;
    if (metalEnabled)
        types.append(QAbstractVideoBuffer::MTLTextureHandle);

#if QT_CONFIG(opengl)
    types.append(QAbstractVideoBuffer::GLTextureHandle);
#endif

    if (types.contains(handleType)) {
        pixelFormats.append(QVideoFrame::Format_RGB565);
        pixelFormats.append(QVideoFrame::Format_RGB32);
        pixelFormats.append(QVideoFrame::Format_ARGB32);
        pixelFormats.append(QVideoFrame::Format_BGR32);
        pixelFormats.append(QVideoFrame::Format_BGRA32);
#if !QT_CONFIG(gpu_vivante)
        pixelFormats.append(QVideoFrame::Format_ABGR32);
#endif
    }

    return pixelFormats;
}

QSGVideoNode *QSGVideoNodeFactory_Texture::createNode(const QVideoSurfaceFormat &format)
{
    if (supportedPixelFormats(format.handleType()).contains(format.pixelFormat()))
        return new QSGVideoNode_Texture(format);

    return 0;
}

class QSGVideoMaterialRhiShader_Texture : public QSGMaterialShader
{
public:
    QSGVideoMaterialRhiShader_Texture()
    {
        setShaderFileName(VertexStage, QStringLiteral(":/qtmultimediaquicktools/shaders/rgba.vert.qsb"));
        setShaderFileName(FragmentStage, QStringLiteral(":/qtmultimediaquicktools/shaders/rgba.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                           QSGMaterial *oldMaterial) override;

    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

class QSGVideoMaterialRhiShader_Texture_swizzle : public QSGVideoMaterialRhiShader_Texture
{
public:
    QSGVideoMaterialRhiShader_Texture_swizzle()
    {
        setShaderFileName(FragmentStage, QStringLiteral(":/qtmultimediaquicktools/shaders/bgra.frag.qsb"));
    }
};

class QSGVideoMaterial_Texture : public QSGMaterial
{
public:
    QSGVideoMaterial_Texture(const QVideoSurfaceFormat &format) :
        m_format(format),
        m_opacity(1.0)
    {
        setFlag(Blending, false);
        m_texture.reset(new QSGVideoTexture);
    }

    ~QSGVideoMaterial_Texture()
    {
    }

    QSGMaterialType *type() const override {
        static QSGMaterialType normalType, swizzleType;
        return needsSwizzling() ? &swizzleType : &normalType;
    }

    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override {
        return needsSwizzling() ? new QSGVideoMaterialRhiShader_Texture_swizzle
                                : new QSGVideoMaterialRhiShader_Texture;
    }

    int compare(const QSGMaterial *other) const override {
        const QSGVideoMaterial_Texture *m = static_cast<const QSGVideoMaterial_Texture *>(other);

        const qint64 diff = m_texture->comparisonKey() - m->m_texture->comparisonKey();
        return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
    }

    void updateBlending() {
        setFlag(Blending, qFuzzyCompare(m_opacity, float(1.0)) ? false : true);
    }

    void setVideoFrame(const QVideoFrame &frame) {
        QMutexLocker lock(&m_frameMutex);
        m_frame = frame;
    }

    QVideoFrame m_frame;
    QMutex m_frameMutex;
    QVideoSurfaceFormat m_format;
    quint64 m_textureId;
    float m_opacity;
    QScopedPointer<QSGVideoTexture> m_texture;

private:
    bool needsSwizzling() const {
        return m_format.pixelFormat() == QVideoFrame::Format_RGB32
                || m_format.pixelFormat() == QVideoFrame::Format_ARGB32;
    }
};

bool QSGVideoMaterialRhiShader_Texture::updateUniformData(RenderState &state, QSGMaterial *newMaterial,
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
        auto m = static_cast<QSGVideoMaterial_Texture *>(newMaterial);
        m->m_opacity = state.opacity();
        m->updateBlending();
        memcpy(buf->data() + 64, &m->m_opacity, 4);
        changed = true;
    }

    return changed;
}

void QSGVideoMaterialRhiShader_Texture::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    if (binding < 1)
        return;

    auto m = static_cast<QSGVideoMaterial_Texture *>(newMaterial);

    m->m_frameMutex.lock();
    auto size = m->m_frame.size();
    if (m->m_frame.isValid())
        m->m_textureId = m->m_frame.handle().toULongLong();
    m->m_frameMutex.unlock();

    m->m_texture->setNativeObject(m->m_textureId, size);
    m->m_texture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
    *texture = m->m_texture.data();
}

QSGVideoNode_Texture::QSGVideoNode_Texture(const QVideoSurfaceFormat &format) :
    m_format(format)
{
    setFlag(QSGNode::OwnsMaterial);
    m_material = new QSGVideoMaterial_Texture(format);
    setMaterial(m_material);
}

QSGVideoNode_Texture::~QSGVideoNode_Texture()
{
}

void QSGVideoNode_Texture::setCurrentFrame(const QVideoFrame &frame, FrameFlags)
{
    m_material->setVideoFrame(frame);
    markDirty(DirtyMaterial);
}

QT_END_NAMESPACE
