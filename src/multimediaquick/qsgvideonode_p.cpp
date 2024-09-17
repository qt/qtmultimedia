// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgvideonode_p.h"
#include <QtQuick/qsgmaterial.h>
#include "qsgvideotexture_p.h"
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <private/qsginternaltextnode_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickvideooutput_p.h>
#include <private/qhwvideobuffer_p.h>

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

static inline void qSwapTex(QSGGeometry::TexturedPoint2D *v0, QSGGeometry::TexturedPoint2D *v1)
{
    auto tvx = v0->tx;
    auto tvy = v0->ty;
    v0->tx = v1->tx;
    v0->ty = v1->ty;
    v1->tx = tvx;
    v1->ty = tvy;
}

class QSGVideoMaterial;

class QSGVideoMaterialRhiShader : public QSGMaterialShader
{
public:
    QSGVideoMaterialRhiShader(const QVideoFrameFormat &videoFormat,
                              const QRhiSwapChain::Format surfaceFormat,
                              const QRhiSwapChainHdrInfo &hdrInfo)
        : m_videoFormat(videoFormat)
        , m_surfaceFormat(surfaceFormat)
        , m_hdrInfo(hdrInfo)
    {
        setShaderFileName(VertexStage, QVideoTextureHelper::vertexShaderFileName(m_videoFormat));
        setShaderFileName(
                FragmentStage,
                QVideoTextureHelper::fragmentShaderFileName(m_videoFormat, m_surfaceFormat));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                           QSGMaterial *oldMaterial) override;

    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

protected:
    QVideoFrameFormat m_videoFormat;
    QRhiSwapChain::Format m_surfaceFormat;
    QRhiSwapChainHdrInfo m_hdrInfo;
};

class QSGVideoMaterial : public QSGMaterial
{
public:
    QSGVideoMaterial(const QVideoFrameFormat &videoFormat);

    [[nodiscard]] QSGMaterialType *type() const override {
        static constexpr int NFormats = QRhiSwapChain::HDRExtendedDisplayP3Linear + 1;
        static QSGMaterialType type[QVideoFrameFormat::NPixelFormats][NFormats];
        return &type[m_videoFormat.pixelFormat()][m_surfaceFormat];
    }

    [[nodiscard]] QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override {
        return new QSGVideoMaterialRhiShader(m_videoFormat, m_surfaceFormat, m_hdrInfo);
    }

    int compare(const QSGMaterial *other) const override {
        const QSGVideoMaterial *m = static_cast<const QSGVideoMaterial *>(other);

        qint64 diff = m_textures[0].comparisonKey() - m->m_textures[0].comparisonKey();
        if (!diff)
            diff = m_textures[1].comparisonKey() - m->m_textures[1].comparisonKey();
        if (!diff)
            diff = m_textures[2].comparisonKey() - m->m_textures[2].comparisonKey();

        return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
    }

    void updateBlending() {
        // ### respect video formats with Alpha
        setFlag(Blending, !qFuzzyCompare(m_opacity, float(1.0)));
    }

    void setCurrentFrame(const QVideoFrame &frame) {
        m_currentFrame = frame;
        m_texturesDirty = true;
    }

    void setSurfaceFormat(const QRhiSwapChain::Format surfaceFormat)
    {
        m_surfaceFormat = surfaceFormat;
    }

    void setHdrInfo(const QRhiSwapChainHdrInfo &hdrInfo)
    {
        m_hdrInfo = hdrInfo;
    }

    void updateTextures(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates);

    QVideoFrameFormat m_videoFormat;
    QRhiSwapChain::Format m_surfaceFormat = QRhiSwapChain::SDR;
    float m_opacity = 1.0f;
    QRhiSwapChainHdrInfo m_hdrInfo;

    bool m_texturesDirty = false;
    QVideoFrame m_currentFrame;

    enum { NVideoFrameSlots = 4 };
    QVideoFrame m_videoFrameSlots[NVideoFrameSlots];
    std::array<QSGVideoTexture, 3> m_textures;
    std::unique_ptr<QVideoFrameTextures> m_videoFrameTextures;
};

void QSGVideoMaterial::updateTextures(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    if (!m_texturesDirty)
        return;

    // keep the video frames alive until we know that they are not needed anymore
    Q_ASSERT(NVideoFrameSlots >= rhi->resourceLimit(QRhi::FramesInFlight));
    m_videoFrameSlots[rhi->currentFrameSlot()] = m_currentFrame;

    // update and upload all textures
    m_videoFrameTextures = QVideoTextureHelper::createTextures(m_currentFrame, rhi, resourceUpdates, std::move(m_videoFrameTextures));
    if (!m_videoFrameTextures)
        return;

    for (int plane = 0; plane < 3; ++plane)
        m_textures[plane].setRhiTexture(m_videoFrameTextures->texture(plane));
    m_texturesDirty = false;
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

    // Do this here, not in updateSampledImage. First, with multiple textures we want to
    // do this once. More importantly, on some platforms (Android) the externalMatrix is
    // updated by this function and we need that already in updateUniformData.
    m->updateTextures(state.rhi(), state.resourceUpdateBatch());

    float maxNits = 100; // Default to de-facto SDR nits
    if (m_surfaceFormat == QRhiSwapChain::HDRExtendedSrgbLinear) {
        if (m_hdrInfo.limitsType == QRhiSwapChainHdrInfo::ColorComponentValue)
            maxNits = 100 * m_hdrInfo.limits.colorComponentValue.maxColorComponentValue;
        else
            maxNits = m_hdrInfo.limits.luminanceInNits.maxLuminance;
    }

    QVideoTextureHelper::updateUniformData(state.uniformData(), m_videoFormat,
        m->m_currentFrame, state.combinedMatrix(), state.opacity(), maxNits);

    return true;
}

void QSGVideoMaterialRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                       QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(state);
    Q_UNUSED(oldMaterial);
    if (binding < 1 || binding > 3)
        return;

    auto m = static_cast<QSGVideoMaterial *>(newMaterial);
    *texture = &m->m_textures[binding - 1];
}

QSGVideoMaterial::QSGVideoMaterial(const QVideoFrameFormat &videoFormat)
    : m_videoFormat(videoFormat)
{
    setFlag(Blending, false);
}

QSGVideoNode::QSGVideoNode(QQuickVideoOutput *parent, const QVideoFrameFormat &videoFormat)
    : m_parent(parent), m_videoFormat(videoFormat)
{
    setFlag(QSGNode::OwnsMaterial);
    setFlag(QSGNode::OwnsGeometry);
    m_material = new QSGVideoMaterial(videoFormat);
    setMaterial(m_material);
}

QSGVideoNode::~QSGVideoNode()
{
    delete m_subtitleTextNode;
}

void QSGVideoNode::setCurrentFrame(const QVideoFrame &frame)
{
    m_material->setCurrentFrame(frame);
    markDirty(DirtyMaterial);
    updateSubtitle(frame);
}

void QSGVideoNode::setSurfaceFormat(const QRhiSwapChain::Format surfaceFormat)
{
    m_material->setSurfaceFormat(surfaceFormat);
    markDirty(DirtyMaterial);
}

void QSGVideoNode::setHdrInfo(const QRhiSwapChainHdrInfo &hdrInfo)
{
    m_material->setHdrInfo(hdrInfo);
    markDirty(DirtyMaterial);
}

void QSGVideoNode::updateSubtitle(const QVideoFrame &frame)
{
    QSize subtitleFrameSize = m_rect.size().toSize();
    if (subtitleFrameSize.isEmpty())
        return;

    subtitleFrameSize = qRotatedFrameSize(subtitleFrameSize, m_orientation);

    if (!m_subtitleLayout.update(subtitleFrameSize, frame.subtitleText()))
        return;

    delete m_subtitleTextNode;
    m_subtitleTextNode = nullptr;
    if (frame.subtitleText().isEmpty())
        return;

    QQuickItemPrivate *parent_d = QQuickItemPrivate::get(m_parent);

    m_subtitleTextNode = parent_d->sceneGraphContext()->createInternalTextNode(parent_d->sceneGraphRenderContext());
    m_subtitleTextNode->setColor(Qt::white);
    QColor bgColor = Qt::black;
    bgColor.setAlpha(128);
    m_subtitleTextNode->addRectangleNode(m_subtitleLayout.bounds, bgColor);
    m_subtitleTextNode->addTextLayout(m_subtitleLayout.layout.position(), &m_subtitleLayout.layout);
    appendChildNode(m_subtitleTextNode);
    setSubtitleGeometry();
}

void QSGVideoNode::setSubtitleGeometry()
{
    if (!m_subtitleTextNode)
        return;

    if (m_material)
        updateSubtitle(m_material->m_currentFrame);

    float rotate = -1.f * m_orientation;
    float yTranslate = 0;
    float xTranslate = 0;
    if (m_orientation == 90) {
        yTranslate = m_rect.height();
    } else if (m_orientation == 180) {
        yTranslate = m_rect.height();
        xTranslate = m_rect.width();
    } else if (m_orientation == 270) {
        xTranslate = m_rect.width();
    }

    QMatrix4x4 transform;
    transform.translate(m_rect.x() + xTranslate, m_rect.y() + yTranslate);
    transform.rotate(rotate, 0, 0, 1);

    m_subtitleTextNode->setMatrix(transform);
    m_subtitleTextNode->markDirty(DirtyGeometry);
}

/* Update the vertices and texture coordinates.  Orientation must be in {0,90,180,270} */
void QSGVideoNode::setTexturedRectGeometry(const QRectF &rect, const QRectF &textureRect, int orientation)
{
    const NormalizedVideoTransformation currentFrameTransformation =
            qNormalizedFrameTransformation(m_material ? m_material->m_currentFrame : QVideoFrame{});

    if (rect == m_rect && textureRect == m_textureRect && orientation == m_orientation
        && currentFrameTransformation == m_frameTransformation)
        return;

    m_rect = rect;
    m_textureRect = textureRect;
    m_orientation = orientation;
    m_frameTransformation = currentFrameTransformation;

    QSGGeometry *g = geometry();

    if (g == nullptr)
        g = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);

    QSGGeometry::TexturedPoint2D *v = g->vertexDataAsTexturedPoint2D();

    // Vertexes:
    // 0   2
    //
    // 1   3

    // Set geometry first
    qSetGeom(v + 0, rect.topLeft());
    qSetGeom(v + 1, rect.bottomLeft());
    qSetGeom(v + 2, rect.topRight());
    qSetGeom(v + 3, rect.bottomRight());

    // and then texture coordinates
    switch (currentFrameTransformation.rotation) {
    default:
        // tl, bl, tr, br
        qSetTex(v + 0, textureRect.topLeft());
        qSetTex(v + 1, textureRect.bottomLeft());
        qSetTex(v + 2, textureRect.topRight());
        qSetTex(v + 3, textureRect.bottomRight());
        break;

    case QtVideo::Rotation::Clockwise90:
        // bl, br, tl, tr
        qSetTex(v + 0, textureRect.bottomLeft());
        qSetTex(v + 1, textureRect.bottomRight());
        qSetTex(v + 2, textureRect.topLeft());
        qSetTex(v + 3, textureRect.topRight());
        break;

    case QtVideo::Rotation::Clockwise180:
        // br, tr, bl, tl
        qSetTex(v + 0, textureRect.bottomRight());
        qSetTex(v + 1, textureRect.topRight());
        qSetTex(v + 2, textureRect.bottomLeft());
        qSetTex(v + 3, textureRect.topLeft());
        break;

    case QtVideo::Rotation::Clockwise270:
        // tr, tl, br, bl
        qSetTex(v + 0, textureRect.topRight());
        qSetTex(v + 1, textureRect.topLeft());
        qSetTex(v + 2, textureRect.bottomRight());
        qSetTex(v + 3, textureRect.bottomLeft());
        break;
    }

    if (m_frameTransformation.xMirrorredAfterRotation) {
        qSwapTex(v + 0, v + 2);
        qSwapTex(v + 1, v + 3);
    }

    if (!geometry())
        setGeometry(g);

    markDirty(DirtyGeometry);

    setSubtitleGeometry();
}

QT_END_NAMESPACE
