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
    using AVFrameUPtr = QFFmpeg::AVFrameUPtr;

    QFFmpegVideoBuffer(AVFrameUPtr frame, AVRational pixelAspectRatio = { 1, 1 });
    ~QFFmpegVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    virtual std::unique_ptr<QVideoFrameTextures> mapTextures(QRhi *) override;
    virtual quint64 textureHandle(QRhi *rhi, int plane) const override;

    QVideoFrameFormat::PixelFormat pixelFormat() const;
    QSize size() const;

    static QVideoFrameFormat::PixelFormat toQtPixelFormat(AVPixelFormat avPixelFormat, bool *needsConversion = nullptr);
    static AVPixelFormat toAVPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat);

    void convertSWFrame();

    AVFrame *getHWFrame() const { return m_hwFrame.get(); }

    void setTextureConverter(const QFFmpeg::TextureConverter &converter);

    QVideoFrameFormat::ColorSpace colorSpace() const;
    QVideoFrameFormat::ColorTransfer colorTransfer() const;
    QVideoFrameFormat::ColorRange colorRange() const;

    float maxNits();

private:
    QVideoFrameFormat::PixelFormat m_pixelFormat;
    AVFrame *m_frame = nullptr;
    AVFrameUPtr m_hwFrame;
    AVFrameUPtr m_swFrame;
    QSize m_size;
    QFFmpeg::TextureConverter m_textureConverter;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
    std::unique_ptr<QFFmpeg::TextureSet> m_textures;
};

QT_END_NAMESPACE

#endif
