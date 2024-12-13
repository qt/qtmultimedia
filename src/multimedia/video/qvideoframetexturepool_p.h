// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVIDEOFRAMETEXTUREPOOL_P_H
#define QVIDEOFRAMETEXTUREPOOL_P_H

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

#include "qvideoframe.h"
#include "qhwvideobuffer_p.h"

#include <array>

QT_BEGIN_NAMESPACE

class QRhi;
class QRhiResourceUpdateBatch;

class Q_MULTIMEDIA_EXPORT QVideoFrameTexturePool {
    static constexpr size_t MaxSlotsCount = 4;
public:
    bool texturesDirty() const { return m_texturesDirty; }

    const QVideoFrame& currentFrame() const { return m_currentFrame; }

    void setCurrentFrame(QVideoFrame frame);

    QVideoFrameTextures* updateTextures(QRhi &rhi, QRhiResourceUpdateBatch &rub);

private:
    QVideoFrame m_currentFrame;
    bool m_texturesDirty = false;
    std::array<QVideoFrameTexturesUPtr, MaxSlotsCount> m_textureSlots;
};

QT_END_NAMESPACE

#endif // QVIDEOFRAMETEXTUREPOOL_P_H
