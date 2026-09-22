// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QWINDOWSD3D11VIDEOPROCESSOR_P_H
#define QWINDOWSD3D11VIDEOPROCESSOR_P_H

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

#include <QtMultimedia/qtmultimediaexports.h>
#include <QtMultimedia/qvideoframeformat.h>

#include <QtCore/qsize.h>
#include <QtCore/private/qcomptr_p.h>

#include <d3d11.h>
#include <d3d11_1.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

/*! \internal
    Converts an NV12 or P010 D3D11 texture into a BGRA8 texture on the same D3D11 device, using
    the fixed-function ID3D11VideoProcessor. Single-plane output is required for import into
    non-D3D11 QRhi backends (see QWindowsD3D11RhiTextureImporter). HDR input is tone-mapped to
    SDR by the processor; the output is always 8-bit BGRA8.
*/
class Q_MULTIMEDIA_EXPORT QWindowsD3D11VideoProcessor
{
public:
    QWindowsD3D11VideoProcessor();
    ~QWindowsD3D11VideoProcessor();

    Q_DISABLE_COPY_MOVE(QWindowsD3D11VideoProcessor)

    static bool isPixelFormatSupported(QVideoFrameFormat::PixelFormat);

    // Converts slice 'subresourceIndex' of 'texture' to BGRA8 on the same device.
    ComPtr<ID3D11Texture2D> convertToBGRA(ID3D11Device *, ID3D11DeviceContext *context,
                                          const ComPtr<ID3D11Texture2D> &texture,
                                          const QSize &frameSize,
                                          QVideoFrameFormat::ColorSpace colorSpace,
                                          QVideoFrameFormat::ColorRange colorRange,
                                          QVideoFrameFormat::ColorTransfer colorTransfer,
                                          UINT subresourceIndex = 0);

private:
    bool ensureVideoProcessor(ID3D11Device *, const QSize &frameSize);
    bool ensureOutputTexture(ID3D11Device *, const QSize &frameSize);

    ComPtr<ID3D11VideoDevice> m_videoDevice;
    ComPtr<ID3D11VideoContext1> m_videoContext;
    ComPtr<ID3D11VideoProcessorEnumerator> m_enumerator;
    ComPtr<ID3D11VideoProcessor> m_videoProcessor;
    QSize m_processorSize;

    ComPtr<ID3D11Texture2D> m_outputTex;
    ComPtr<ID3D11VideoProcessorOutputView> m_outputView;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QWINDOWSD3D11VIDEOPROCESSOR_P_H
