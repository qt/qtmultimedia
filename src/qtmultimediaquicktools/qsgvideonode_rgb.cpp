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
#include "qsgvideonode_rgb_p.h"
#include "qsgvideotexture_p.h"
#include <QtQuick/qsgmaterial.h>
#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

QList<QVideoFrame::PixelFormat> QSGVideoNodeFactory_RGB::supportedPixelFormats(
                                        QVideoFrame::HandleType handleType) const
{
    QList<QVideoFrame::PixelFormat> pixelFormats;

    if (handleType == QVideoFrame::NoHandle) {
        pixelFormats.append(QVideoFrame::Format_RGB32);
        pixelFormats.append(QVideoFrame::Format_ARGB32);
        pixelFormats.append(QVideoFrame::Format_ARGB32_Premultiplied);
        pixelFormats.append(QVideoFrame::Format_BGR32);
        pixelFormats.append(QVideoFrame::Format_BGRA32);
        pixelFormats.append(QVideoFrame::Format_RGB565);
    }

    return pixelFormats;
}

QSGVideoNode *QSGVideoNodeFactory_RGB::createNode(const QVideoSurfaceFormat &format)
{
    if (supportedPixelFormats(QVideoFrame::NoHandle).contains(format.pixelFormat()))
        return new QSGVideoNode_RGB(format);

    return nullptr;
}

class QSGVideoMaterialRhiShader_RGB : public QSGMaterialShader
{
public:
    QSGVideoMaterialRhiShader_RGB()
    {
        setShaderFileName(VertexStage, QStringLiteral(":/qtmultimediaquicktools/shaders/rgba.vert.qsb"));
        setShaderFileName(FragmentStage, QStringLiteral(":/qtmultimediaquicktools/shaders/rgba.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                           QSGMaterial *oldMaterial) override;

    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

class QSGVideoMaterial_RGB : public QSGMaterial
{
public:
    QSGVideoMaterial_RGB(const QVideoSurfaceFormat &format) :
        m_format(format),
        m_opacity(1.0)
    {
        setFlag(Blending, false);
        m_texture.reset(new QSGVideoTexture);
    }

    [[nodiscard]] QSGMaterialType *type() const override {
        static QSGMaterialType normalType;
        return &normalType;
    }

    [[nodiscard]] QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override {
        return new QSGVideoMaterialRhiShader_RGB;
    }

    int compare(const QSGMaterial *other) const override {
        const QSGVideoMaterial_RGB *m = static_cast<const QSGVideoMaterial_RGB *>(other);

        const qint64 diff = m_texture->comparisonKey() - m->m_texture->comparisonKey();
        return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
    }

    void updateBlending() {
        setFlag(Blending, !qFuzzyCompare(m_opacity, float(1.0)));
    }

    void setVideoFrame(const QVideoFrame &frame) {
        QMutexLocker lock(&m_frameMutex);
        m_frame = frame;
    }

    QVideoFrame m_frame;
    QMutex m_frameMutex;
    QSize m_textureSize;
    QVideoSurfaceFormat m_format;
    float m_opacity;
    QScopedPointer<QSGVideoTexture> m_texture;
};

bool QSGVideoMaterialRhiShader_RGB::updateUniformData(RenderState &state, QSGMaterial *newMaterial,
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
        auto m = static_cast<QSGVideoMaterial_RGB *>(newMaterial);
        m->m_opacity = state.opacity();
        m->updateBlending();
        memcpy(buf->data() + 64, &m->m_opacity, 4);
        changed = true;
    }

    return changed;
}

void QSGVideoMaterialRhiShader_RGB::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                       QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    if (binding < 1)
        return;

    auto m = static_cast<QSGVideoMaterial_RGB *>(newMaterial);

    m->m_frameMutex.lock();
    auto frame = m->m_frame;

    if (frame.pixelFormat() == QVideoFrame::Format_RGB565) // Format_RGB565 requires GL_UNSIGNED_SHORT_5_6_5
        frame = frame.image().convertToFormat(QImage::Format_RGBA8888_Premultiplied);

    auto format = QRhiTexture::RGBA8;
    if (frame.pixelFormat() == QVideoFrame::Format_RGB32
        || frame.pixelFormat() == QVideoFrame::Format_ARGB32
        || frame.pixelFormat() == QVideoFrame::Format_ARGB32_Premultiplied)
    {
        format = QRhiTexture::BGRA8;
    }

    if (frame.isValid() && frame.map(QVideoFrame::ReadOnly)) {
        m->m_texture->setData(format, frame.size(), frame.bits(), frame.bytesPerLine() * frame.height());
        frame.unmap();
    }
    m->m_frameMutex.unlock();

    m->m_texture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
    *texture = m->m_texture.data();
}

QSGVideoNode_RGB::QSGVideoNode_RGB(const QVideoSurfaceFormat &format) :
    m_format(format)
{
    setFlag(QSGNode::OwnsMaterial);
    m_material = new QSGVideoMaterial_RGB(format);
    setMaterial(m_material);
}

QSGVideoNode_RGB::~QSGVideoNode_RGB() = default;

void QSGVideoNode_RGB::setCurrentFrame(const QVideoFrame &frame, FrameFlags)
{
    m_material->setVideoFrame(frame);
    markDirty(DirtyMaterial);
}

QT_END_NAMESPACE
