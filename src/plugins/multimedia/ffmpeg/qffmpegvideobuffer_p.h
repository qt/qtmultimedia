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

#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtCore/qvariant.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>

QT_BEGIN_NAMESPACE

class QFFmpegVideoBuffer : public QHwVideoBuffer
{
public:
    using AVFrameUPtr = QFFmpeg::AVFrameUPtr;

    QFFmpegVideoBuffer(AVFrameUPtr frame, AVRational pixelAspectRatio = { 1, 1 });
    ~QFFmpegVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    QVideoFrameTexturesUPtr mapTextures(QRhi &, QVideoFrameTexturesUPtr& oldTextures) override;

    QVideoFrameFormat::PixelFormat pixelFormat() const;
    QSize size() const;

    /** Overrides the pixel format reported by pixelFormat() and used to interpret the hw frame
     *  when building GPU textures via mapTextures(). Used when the frame will be converted to
     *  a different (single-plane) pixel format before texture import, e.g. an NV12 or P010
     *  D3D11VA frame on a backend that can only import single-plane textures. Must be called
     *  before the buffer is wrapped in a QVideoFrame, since QVideoFrameFormat is fixed at that
     *  point. Does not affect the underlying hw frame or its decoder pool: the CPU (map())
     *  fallback still transfers the real data and then converts it into the overridden pixel
     *  format. Also makes colorSpace()/colorTransfer()/colorRange() report the color properties
     *  of the converted data instead of the original hw frame's, since on Windows this converter
     *  (QWindowsD3D11VideoProcessor) always tone-maps its output to SDR BT.709. */
    void setPixelFormatOverride(QVideoFrameFormat::PixelFormat format)
    {
        m_pixelFormat = format;
        m_isPixelFormatOverridden = true;
    }

    static QVideoFrameFormat::PixelFormat toQtPixelFormat(AVPixelFormat avPixelFormat, bool *needsConversion = nullptr);
    static AVPixelFormat toAVPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat);

    void convertSWFrame();

    AVFrame *getHWFrame() const { return m_hwFrame.get(); }

    void initTextureConverter(QRhi &rhi) override;

    QRhi *associatedCurrentThreadRhi() const override;

    QVideoFrameFormat::ColorSpace colorSpace() const;
    QVideoFrameFormat::ColorTransfer colorTransfer() const;
    QVideoFrameFormat::ColorRange colorRange() const;

    float maxNits();

private:
    // The result texture converter must be accessed from the rhi's thread
    QFFmpeg::TextureConverter &ensureTextureConverter(QRhi &rhi);

    QVideoFrameTexturesUPtr createTexturesFromHwFrame(QRhi &, QVideoFrameTexturesUPtr& oldTextures);

private:
    QVideoFrameFormat::PixelFormat m_pixelFormat;
    bool m_isPixelFormatOverridden = false;
    AVFrame *m_frame = nullptr;
    AVFrameUPtr m_hwFrame;
    AVFrameUPtr m_swFrame;
    QSize m_size;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
};

QT_END_NAMESPACE

#endif
