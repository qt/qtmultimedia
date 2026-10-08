// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideoframetexturepool_p.h"
#include "qvideotexturehelper_p.h"

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

void QVideoFrameTexturePool::setCurrentFrame(QVideoFrame frame) {
    m_texturesDirty = true;
    m_currentFrame = std::move(frame);
}

QVideoFrameTextures* QVideoFrameTexturePool::updateTextures(QRhi &rhi, QRhiResourceUpdateBatch &rub) {
    const int currentSlot = rhi.currentFrameSlot();
    Q_ASSERT(size_t(currentSlot) < MaxSlotsCount);

    m_texturesDirty = false;
    QVideoFrameTexturesUPtr &textures = m_textureSlots[currentSlot];
    Q_ASSERT(!m_oldTextures);
    m_oldTextures = std::move(textures);
    textures = QVideoTextureHelper::createTextures(m_currentFrame, rhi, rub, m_oldTextures);

    m_currentSlot = textures ? currentSlot : std::optional<int>{};

    return textures.get();
}

void QVideoFrameTexturePool::onFrameEndInvoked()
{
    m_oldTextures.reset();
    if (m_currentSlot && m_textureSlots[*m_currentSlot])
        m_textureSlots[*m_currentSlot]->onFrameEndInvoked();
}

void QVideoFrameTexturePool::clearTextures()
{
    std::fill(std::begin(m_textureSlots), std::end(m_textureSlots), nullptr);
    m_currentSlot.reset();
    m_texturesDirty = m_currentFrame.isValid();
}

QT_END_NAMESPACE
