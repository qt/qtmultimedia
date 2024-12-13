// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHWVIDEOBUFFER_P_H
#define QHWVIDEOBUFFER_P_H

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

#include "qabstractvideobuffer.h"
#include "qvideoframe.h"

#include <QtGui/qmatrix4x4.h>

QT_BEGIN_NAMESPACE

class QRhi;
class QRhiTexture;
class QVideoFrame;

class Q_MULTIMEDIA_EXPORT QVideoFrameTextures
{
public:
    virtual ~QVideoFrameTextures();
    virtual QRhiTexture *texture(uint plane) const = 0;

    virtual void onFrameEndInvoked() { }

    void setSourceFrame(QVideoFrame sourceFrame) { m_sourceFrame = std::move(sourceFrame); }

private:
    QVideoFrame m_sourceFrame;
};
using QVideoFrameTexturesUPtr = std::unique_ptr<QVideoFrameTextures>;

class Q_MULTIMEDIA_EXPORT QVideoFrameTexturesSet {
public:
    virtual ~QVideoFrameTexturesSet();

    virtual quint64 textureHandle(QRhi &, int /*plane*/) { return 0; };
};
using QVideoFrameTexturesSetUPtr = std::unique_ptr<QVideoFrameTexturesSet>;

class Q_MULTIMEDIA_EXPORT QHwVideoBuffer : public QAbstractVideoBuffer, public QVideoFrameTexturesSet
{
public:
    QHwVideoBuffer(QVideoFrame::HandleType type, QRhi *rhi = nullptr);

    ~QHwVideoBuffer() override;

    QVideoFrame::HandleType handleType() const { return m_type; }
    virtual QRhi *rhi() const { return m_rhi; }

    QVideoFrameFormat format() const override { return {}; }

    virtual QMatrix4x4 externalTextureMatrix() const { return {}; }

    virtual std::unique_ptr<QVideoFrameTextures> mapTextures(QRhi &) { return nullptr; };

    virtual void initTextureConverter(QRhi &) { }

protected:
    QVideoFrame::HandleType m_type;
    QRhi *m_rhi = nullptr;
};

QT_END_NAMESPACE

#endif // QHWVIDEOBUFFER_P_H
