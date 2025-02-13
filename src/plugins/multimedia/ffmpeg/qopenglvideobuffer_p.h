// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGLVIDEOBUFFER_P_H
#define QOPENGLVIDEOBUFFER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qhwvideobuffer_p.h>

QT_BEGIN_NAMESPACE

class QImageVideoBuffer;
class QOpenGLFramebufferObject;

class QOpenGLVideoBuffer : public QHwVideoBuffer
{
public:
    QOpenGLVideoBuffer(std::unique_ptr<QOpenGLFramebufferObject> fbo);
    ~QOpenGLVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;
    quint64 textureHandle(QRhi &, int plane) override;

    QImageVideoBuffer &ensureImageBuffer();

private:
    std::unique_ptr<QOpenGLFramebufferObject> m_fbo;
    std::unique_ptr<QImageVideoBuffer> m_imageBuffer;
};

QT_END_NAMESPACE

#endif // QOPENGLVIDEOBUFFER_P_H
