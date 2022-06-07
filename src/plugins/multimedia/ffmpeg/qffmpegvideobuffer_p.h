// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGVIDEOBUFFER_P_H
#define QFFMPEGVIDEOBUFFER_P_H

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

#include <private/qtmultimediaglobal_p.h>
#include <private/qabstractvideobuffer_p.h>
#include <qvideoframe.h>
#include <QtCore/qvariant.h>

#include "qffmpeg_p.h"
#include "qffmpeghwaccel_p.h"

QT_BEGIN_NAMESPACE

class QFFmpegVideoBuffer : public QAbstractVideoBuffer
{
public:

    QFFmpegVideoBuffer(AVFrame *frame);
    ~QFFmpegVideoBuffer();

    QVideoFrame::MapMode mapMode() const override;
    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    virtual void mapTextures() override;
    virtual quint64 textureHandle(int plane) const override;
    std::unique_ptr<QRhiTexture> texture(int plane) const override;

    QVideoFrameFormat::PixelFormat pixelFormat() const;
    QSize size() const;

    static QVideoFrameFormat::PixelFormat toQtPixelFormat(AVPixelFormat avPixelFormat, bool *needsConversion = nullptr);
    static AVPixelFormat toAVPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat);

    void convertSWFrame();

    AVFrame *getHWFrame() const { return hwFrame; }

    void setTextureConverter(const QFFmpeg::TextureConverter &converter);

    QVideoFrameFormat::ColorSpace colorSpace() const;
    QVideoFrameFormat::ColorTransfer colorTransfer() const;
    QVideoFrameFormat::ColorRange colorRange() const;

    float maxNits();

private:
    QVideoFrameFormat::PixelFormat m_pixelFormat;
    AVFrame *frame = nullptr;
    AVFrame *hwFrame = nullptr;
    AVFrame *swFrame = nullptr;
    QFFmpeg::TextureConverter textureConverter;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
    QFFmpeg::TextureSet *textures = nullptr;
};

QT_END_NAMESPACE

#endif
