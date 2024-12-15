// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideoframetexturefromsource_p.h"

QT_BEGIN_NAMESPACE

using namespace QVideoTextureHelper;

QVideoFrameTexturesFromRhiTextureArray::QVideoFrameTexturesFromRhiTextureArray(RhiTextureArray &&rhiTextures)
    : m_rhiTextures(std::move(rhiTextures))
{
}

// out-of-line destructor to get arount potential RTTI problems
QVideoFrameTexturesFromRhiTextureArray::~QVideoFrameTexturesFromRhiTextureArray() = default;

QRhiTexture *QVideoFrameTexturesFromRhiTextureArray::texture(uint plane) const
{
    return plane < m_rhiTextures.size() ? m_rhiTextures[plane].get() : nullptr;
}

void QVideoFrameTexturesFromMemory::setMappedFrame(QVideoFrame mappedFrame) {
    Q_ASSERT(!mappedFrame.isValid() || mappedFrame.isReadable());
    m_mappedFrame.unmap();
    m_mappedFrame = std::move(mappedFrame);
}

// We keep the source frame mapped until QRhi::endFrame is invoked.
// QRhi::endFrame ensures that the mapped frame's memory has been loaded into the texture.
// See QTBUG-123174 for bug's details.
QVideoFrameTexturesFromMemory::~QVideoFrameTexturesFromMemory()
{
    m_mappedFrame.unmap();
}

void QVideoFrameTexturesFromMemory::onFrameEndInvoked()
{
    // After invoking QRhi::endFrame, the texture is loaded, and we don't need to
    // to store the source mapped frame anymore
    setMappedFrame({});
    setSourceFrame({});
}

QVideoFrameTexturesFromHandlesSet::QVideoFrameTexturesFromHandlesSet(
        RhiTextureArray &&rhiTextures, QVideoFrameTexturesHandlesUPtr handles)
    : QVideoFrameTexturesFromRhiTextureArray(std::move(rhiTextures)),
      m_textureHandles(std::move(handles))
{
}

// out-of-line destructor to get arount potential RTTI problems
QVideoFrameTexturesFromHandlesSet::~QVideoFrameTexturesFromHandlesSet() = default;

QT_END_NAMESPACE
