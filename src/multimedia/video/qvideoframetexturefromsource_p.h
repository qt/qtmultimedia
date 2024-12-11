// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVIDEOFRAMETEXTUREFROMSOURCE_P_H
#define QVIDEOFRAMETEXTUREFROMSOURCE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qhwvideobuffer_p.h"
#include "qvideotexturehelper_p.h"

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

namespace QVideoTextureHelper {

using RhiTextureArray = std::array<std::unique_ptr<QRhiTexture>, TextureDescription::maxPlanes>;

class QVideoFrameTexturesFromRhiTextureArray : public QVideoFrameTextures
{
public:
    QVideoFrameTexturesFromRhiTextureArray(RhiTextureArray &&rhiTextures)
        : m_rhiTextures(std::move(rhiTextures))
    {
    }

    QRhiTexture *texture(uint plane) const override
    {
        return plane < m_rhiTextures.size() ? m_rhiTextures[plane].get() : nullptr;
    }

    RhiTextureArray takeRhiTextures() { return std::move(m_rhiTextures); }

private:
    RhiTextureArray m_rhiTextures;
};

class QVideoFrameTexturesFromMemory : public QVideoFrameTexturesFromRhiTextureArray
{
public:
    QVideoFrameTexturesFromMemory(RhiTextureArray &&rhiTextures, QVideoFrame mappedFrame)
        : QVideoFrameTexturesFromRhiTextureArray(std::move(rhiTextures)),
          m_mappedFrame(std::move(mappedFrame))
    {
        Q_ASSERT(!m_mappedFrame.isValid() || m_mappedFrame.isReadable());
    }

    // We keep the source frame mapped during the target texture lifetime.
    // Alternatively, we may use setting a custom image to QRhiTextureSubresourceUploadDescription,
    // unsig videoFramePlaneAsImage, however, the OpenGL rendering pipeline in QRhi
    // may keep QImage, and consequently the mapped QVideoFrame,
    // even after the target texture is deleted: QTBUG-123174.
    ~QVideoFrameTexturesFromMemory() { m_mappedFrame.unmap(); }

private:
    QVideoFrame m_mappedFrame;
};

class QVideoFrameTexturesFromHandlesSet : public QVideoFrameTexturesFromRhiTextureArray
{
public:
    QVideoFrameTexturesFromHandlesSet(RhiTextureArray &&textures,
                                      QVideoFrameTexturesSetUPtr texturesSet)
        : QVideoFrameTexturesFromRhiTextureArray(std::move(textures)),
          m_texturesSet(std::move(texturesSet))
    {
    }

private:
    QVideoFrameTexturesSetUPtr m_texturesSet;
};

} // namespace QVideoTextureHelper

QT_END_NAMESPACE

#endif // QVIDEOFRAMETEXTUREFROMSOURCE_P_H
