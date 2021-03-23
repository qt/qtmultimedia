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
#include "qsgvideonode_yuv_p.h"
#include "qsgvideotexture_p.h"
#include <QtCore/qmutex.h>
#include <QtQuick/qsgmaterial.h>

QT_BEGIN_NAMESPACE

QList<QVideoFrame::PixelFormat> QSGVideoNodeFactory_YUV::supportedPixelFormats(
                                        QVideoFrame::HandleType) const
{
    QList<QVideoFrame::PixelFormat> formats;

    formats << QVideoFrame::Format_YUV420P << QVideoFrame::Format_YV12 << QVideoFrame::Format_YUV422P
            << QVideoFrame::Format_NV12 << QVideoFrame::Format_NV21
            << QVideoFrame::Format_UYVY << QVideoFrame::Format_YUYV;

    return formats;
}

QSGVideoNode *QSGVideoNodeFactory_YUV::createNode(const QVideoSurfaceFormat &format)
{
    if (supportedPixelFormats(QVideoFrame::NoHandle).contains(format.pixelFormat()))
        return new QSGVideoNode_YUV(format);

    return nullptr;
}

class QSGVideoMaterialRhiShader_YUV : public QSGMaterialShader
{
public:
    QSGVideoMaterialRhiShader_YUV()
    {
        setShaderFileName(VertexStage, QStringLiteral(":/qtmultimediaquicktools/shaders/yuv.vert.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                           QSGMaterial *oldMaterial) override;

    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

    virtual void mapFrame(QSGVideoMaterial_YUV *) = 0;

protected:
    float m_planeWidth[3] = {0, 0, 0};
    QMatrix4x4 m_colorMatrix;
};

class QSGVideoMaterialRhiShader_UYVY : public QSGVideoMaterialRhiShader_YUV
{
public:
    QSGVideoMaterialRhiShader_UYVY()
    {
        setShaderFileName(FragmentStage, QStringLiteral(":/qtmultimediaquicktools/shaders/uyvy.frag.qsb"));
    }

    void mapFrame(QSGVideoMaterial_YUV *m) override;
};

class QSGVideoMaterialRhiShader_YUYV : public QSGVideoMaterialRhiShader_UYVY
{
public:
    QSGVideoMaterialRhiShader_YUYV()
    {
        setShaderFileName(FragmentStage, QStringLiteral(":/qtmultimediaquicktools/shaders/yuyv.frag.qsb"));
    }
};

class QSGVideoMaterialRhiShader_YUV_YV : public QSGVideoMaterialRhiShader_YUV
{
public:
    QSGVideoMaterialRhiShader_YUV_YV()
    {
        setShaderFileName(FragmentStage, QStringLiteral(":/qtmultimediaquicktools/shaders/yuv_yv.frag.qsb"));
    }

    void mapFrame(QSGVideoMaterial_YUV *m) override;
};

class QSGVideoMaterialRhiShader_NV12 : public QSGVideoMaterialRhiShader_YUV
{
public:
    QSGVideoMaterialRhiShader_NV12()
    {
        setShaderFileName(FragmentStage, QStringLiteral(":/qtmultimediaquicktools/shaders/nv12.frag.qsb"));
    }

    void mapFrame(QSGVideoMaterial_YUV *m) override;
};

class QSGVideoMaterialRhiShader_NV21 : public QSGVideoMaterialRhiShader_NV12
{
public:
    QSGVideoMaterialRhiShader_NV21()
    {
        setShaderFileName(FragmentStage, QStringLiteral(":/qtmultimediaquicktools/shaders/nv21.frag.qsb"));
    }
};

class QSGVideoMaterial_YUV : public QSGMaterial
{
public:
    QSGVideoMaterial_YUV(const QVideoSurfaceFormat &format);

    [[nodiscard]] QSGMaterialType *type() const override {
        static QSGMaterialType biPlanarType, biPlanarSwizzleType, triPlanarType, uyvyType, yuyvType;

        switch (m_format.pixelFormat()) {
        case QVideoFrame::Format_NV12:
            return &biPlanarType;
        case QVideoFrame::Format_NV21:
            return &biPlanarSwizzleType;
        case QVideoFrame::Format_UYVY:
            return &uyvyType;
        case QVideoFrame::Format_YUYV:
            return &yuyvType;
        default: // Currently: YUV420P, YUV422P and YV12
            return &triPlanarType;
        }
    }

    [[nodiscard]] QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override {
        switch (m_format.pixelFormat()) {
        case QVideoFrame::Format_NV12:
            return new QSGVideoMaterialRhiShader_NV12;
        case QVideoFrame::Format_NV21:
            return new QSGVideoMaterialRhiShader_NV21;
        case QVideoFrame::Format_UYVY:
            return new QSGVideoMaterialRhiShader_UYVY;
        case QVideoFrame::Format_YUYV:
            return new QSGVideoMaterialRhiShader_YUYV;
        default: // Currently: YUV420P, YUV422P and YV12
            return new QSGVideoMaterialRhiShader_YUV_YV;
        }
    }

    int compare(const QSGMaterial *other) const override {
        const QSGVideoMaterial_YUV *m = static_cast<const QSGVideoMaterial_YUV *>(other);

        qint64 diff = m_textures[0]->comparisonKey() - m->m_textures[0]->comparisonKey();
        if (!diff)
            diff = m_textures[1]->comparisonKey() - m->m_textures[1]->comparisonKey();
        if (!diff)
            diff = m_textures[2]->comparisonKey() - m->m_textures[2]->comparisonKey();

        return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
    }

    void updateBlending() {
        setFlag(Blending, !qFuzzyCompare(m_opacity, float(1.0)));
    }

    void setCurrentFrame(const QVideoFrame &frame) {
        QMutexLocker lock(&m_frameMutex);
        m_frame = frame;
    }

    QVideoSurfaceFormat m_format;
    float m_planeWidth[3];
    float m_opacity;
    QMatrix4x4 m_colorMatrix;
    QVideoFrame m_frame;
    QMutex m_frameMutex;
    QScopedPointer<QSGVideoTexture> m_textures[3];
};

bool QSGVideoMaterialRhiShader_YUV::updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                                                      QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    auto m = static_cast<QSGVideoMaterial_YUV *>(newMaterial);
    bool changed = false;
    QByteArray *buf = state.uniformData();

    if (state.isMatrixDirty()) {
        memcpy(buf->data(), state.combinedMatrix().constData(), 64);
        changed = true;
    }

    if (m->m_colorMatrix != m_colorMatrix) {
        memcpy(buf->data() + 64, m->m_colorMatrix.constData(), 64);
        changed = true;
    }
    m_colorMatrix = m->m_colorMatrix;

    if (state.isOpacityDirty()) {
        m->m_opacity = state.opacity();
        m->updateBlending();
        memcpy(buf->data() + 64 + 64, &m->m_opacity, 4);
        changed = true;
    }

    m->m_frameMutex.lock();
    mapFrame(m);
    m->m_frameMutex.unlock();

    if (!qFuzzyCompare(m->m_planeWidth[0], m_planeWidth[0])
        || !qFuzzyCompare(m->m_planeWidth[1], m_planeWidth[1])
        || !qFuzzyCompare(m->m_planeWidth[2], m_planeWidth[2]))
    {
        memcpy(buf->data() + 64 + 64 + 4, &m->m_planeWidth[0], 4);
        memcpy(buf->data() + 64 + 64 + 4 + 4, &m->m_planeWidth[1], 4);
        memcpy(buf->data() + 64 + 64 + 4 + 4 + 4, &m->m_planeWidth[2], 4);
        changed = true;
    }
    m_planeWidth[0] = m->m_planeWidth[0];
    m_planeWidth[1] = m->m_planeWidth[1];
    m_planeWidth[2] = m->m_planeWidth[2];

    return changed;
}

void QSGVideoMaterialRhiShader_YUV::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                       QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);
    if (binding < 1 || binding > 3)
        return;

    auto m = static_cast<QSGVideoMaterial_YUV *>(newMaterial);
    *texture = m->m_textures[binding - 1].data();
    (*texture)->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
}

void QSGVideoMaterialRhiShader_UYVY::mapFrame(QSGVideoMaterial_YUV *m)
{
    if (!m->m_frame.isValid() || !m->m_frame.map(QVideoFrame::ReadOnly))
        return;

    int fw = m->m_frame.width();
    int fh = m->m_frame.height();

    m->m_planeWidth[0] = 1;
    m->m_planeWidth[1] = 1;

    // Either r,b (YUYV) or g,a (UYVY) values are used as source of UV.
    // Additionally U and V are set per 2 pixels hence only 1/2 of image width is used.
    m->m_textures[0]->setData(QRhiTexture::RG8, m->m_frame.size(),
        m->m_frame.bits(), m->m_frame.bytesPerLine() * fh);
    m->m_textures[1]->setData(QRhiTexture::RGBA8, QSize(fw / 2, fh),
        m->m_frame.bits(), m->m_frame.bytesPerLine() * fh);

    m->m_frame.unmap();
}

void QSGVideoMaterialRhiShader_YUV_YV::mapFrame(QSGVideoMaterial_YUV *m)
{
    if (!m->m_frame.isValid() || !m->m_frame.map(QVideoFrame::ReadOnly))
        return;

    int y = 0;
    int u = m->m_frame.pixelFormat() == QVideoFrame::Format_YV12 ? 2 : 1;
    int v = m->m_frame.pixelFormat() == QVideoFrame::Format_YV12 ? 1 : 2;
    int fw = m->m_frame.width();
    int fh = m->m_frame.height();
    int uvHeight = m->m_frame.pixelFormat() == QVideoFrame::Format_YUV422P ? fh : fh / 2;

    m->m_planeWidth[0] = float(fw) / m->m_frame.bytesPerLine(y);
    m->m_planeWidth[1] = m->m_planeWidth[2] = float(fw) / (2 * m->m_frame.bytesPerLine(u));

    m->m_textures[0]->setData(QRhiTexture::R8, QSize(m->m_frame.bytesPerLine(y), fh),
        m->m_frame.bits(y), m->m_frame.bytesPerLine(y) * fh);
    m->m_textures[1]->setData(QRhiTexture::R8, QSize(m->m_frame.bytesPerLine(u), uvHeight),
        m->m_frame.bits(u), m->m_frame.bytesPerLine(u) * uvHeight);
    m->m_textures[2]->setData(QRhiTexture::R8, QSize(m->m_frame.bytesPerLine(v), uvHeight),
        m->m_frame.bits(v), m->m_frame.bytesPerLine(v) * uvHeight);

    m->m_frame.unmap();
}

void QSGVideoMaterialRhiShader_NV12::mapFrame(QSGVideoMaterial_YUV *m)
{
    if (!m->m_frame.isValid())
        return;

    if (m->m_frame.handleType() == QVideoFrame::GLTextureHandle || m->m_frame.handleType() == QVideoFrame::MTLTextureHandle) {
        m->m_planeWidth[0] = m->m_planeWidth[1] = 1;
        auto textures = m->m_frame.handle().toList();
        if (!textures.isEmpty()) {
            auto w = m->m_frame.size().width();
            auto h = m->m_frame.size().height();
            m->m_textures[0]->setNativeObject(textures[0].toULongLong(), {w, h});
            m->m_textures[1]->setNativeObject(textures[1].toULongLong(), {w / 2, h / 2});
        } else {
            qWarning() << "NV12/NV21 requires 2 textures";
        }

        return;
    }

    if (!m->m_frame.map(QVideoFrame::ReadOnly))
        return;

    int y = 0;
    int uv = 1;
    int fw = m->m_frame.width();
    int fh = m->m_frame.height();

    m->m_planeWidth[0] = m->m_planeWidth[1] = qreal(fw) / m->m_frame.bytesPerLine(y);

    m->m_textures[0]->setData(QRhiTexture::R8, m->m_frame.size(),
        m->m_frame.bits(y), m->m_frame.bytesPerLine(y) * fh);
    m->m_textures[1]->setData(QRhiTexture::RG8, QSize(m->m_frame.bytesPerLine(uv) / 2 , fh / 2),
        m->m_frame.bits(uv), m->m_frame.bytesPerLine(uv) * fh / 2);

    m->m_frame.unmap();
}

QSGVideoMaterial_YUV::QSGVideoMaterial_YUV(const QVideoSurfaceFormat &format) :
    m_format(format),
    m_opacity(1.0)
{
    m_textures[0].reset(new QSGVideoTexture);
    m_textures[1].reset(new QSGVideoTexture);
    m_textures[2].reset(new QSGVideoTexture);

    switch (format.yCbCrColorSpace()) {
    case QVideoSurfaceFormat::YCbCr_JPEG:
        m_colorMatrix = QMatrix4x4(
                    1.0f,  0.000f,  1.402f, -0.701f,
                    1.0f, -0.344f, -0.714f,  0.529f,
                    1.0f,  1.772f,  0.000f, -0.886f,
                    0.0f,  0.000f,  0.000f,  1.0000f);
        break;
    case QVideoSurfaceFormat::YCbCr_BT709:
    case QVideoSurfaceFormat::YCbCr_xvYCC709:
        m_colorMatrix = QMatrix4x4(
                    1.164f,  0.000f,  1.793f, -0.5727f,
                    1.164f, -0.534f, -0.213f,  0.3007f,
                    1.164f,  2.115f,  0.000f, -1.1302f,
                    0.0f,    0.000f,  0.000f,  1.0000f);
        break;
    default: //BT 601:
        m_colorMatrix = QMatrix4x4(
                    1.164f,  0.000f,  1.596f, -0.8708f,
                    1.164f, -0.392f, -0.813f,  0.5296f,
                    1.164f,  2.017f,  0.000f, -1.081f,
                    0.0f,    0.000f,  0.000f,  1.0000f);
    }

    setFlag(Blending, false);
}

QSGVideoNode_YUV::QSGVideoNode_YUV(const QVideoSurfaceFormat &format) :
    m_format(format)
{
    setFlag(QSGNode::OwnsMaterial);
    m_material = new QSGVideoMaterial_YUV(format);
    setMaterial(m_material);
}

QSGVideoNode_YUV::~QSGVideoNode_YUV() = default;

void QSGVideoNode_YUV::setCurrentFrame(const QVideoFrame &frame, FrameFlags)
{
    m_material->setCurrentFrame(frame);
    markDirty(DirtyMaterial);
}

QT_END_NAMESPACE
