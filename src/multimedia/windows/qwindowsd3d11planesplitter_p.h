// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QWINDOWSD3D11PLANESPLITTER_P_H
#define QWINDOWSD3D11PLANESPLITTER_P_H

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

#include <memory>
#include <optional>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

/*! \internal
    Splits one array slice of a multi-planar D3D11 texture (NV12, P010) into one single-plane
    texture per plane, cropped to a given frame size, for import into single-plane-only QRhi
    backends. Planes are copied with a texel-fetch shader because D3D11 rejects direct copies
    between planar and single-channel formats. Instances are bound to one device: create them
    with create(), which returns null if the shaders cannot be created.
*/
class Q_MULTIMEDIA_EXPORT QWindowsD3D11PlaneSplitter
{
public:
    ~QWindowsD3D11PlaneSplitter();

    static std::unique_ptr<QWindowsD3D11PlaneSplitter> create(ID3D11Device *device);

    static bool isPixelFormatSupported(QVideoFrameFormat::PixelFormat);
    Q_DISABLE_COPY_MOVE(QWindowsD3D11PlaneSplitter)

    struct Plane
    {
        ComPtr<ID3D11Texture2D> texture;
        QSize size;
    };

    // Splits slice 'subresourceIndex' of 'texture' into one texture per plane, cropped to 'frameSize'
    std::optional<std::vector<Plane>> split(ID3D11Device *, ID3D11DeviceContext *,
                                            const ComPtr<ID3D11Texture2D> &texture,
                                            UINT subresourceIndex, const QSize &frameSize,
                                            QVideoFrameFormat::PixelFormat pixelFormat);

private:
    QWindowsD3D11PlaneSplitter();

    bool createPipeline(ID3D11Device *device);

    // Returns 'texture' itself when it has shader access, otherwise a cached internal copy.
    std::pair<ComPtr<ID3D11Texture2D>, UINT>
    shaderReadableTexture(ID3D11Device *device, ID3D11DeviceContext *context,
                          const ComPtr<ID3D11Texture2D> &texture, UINT subresourceIndex);

    ComPtr<ID3D11Device> m_shaderDevice; // device the shaders below were created for
    ComPtr<ID3D11VertexShader> m_vs;
    ComPtr<ID3D11PixelShader> m_ps;

    // Reused across frames; recreated when the device, format or size of the source changes.
    ComPtr<ID3D11Texture2D> m_shaderReadableCopy;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QWINDOWSD3D11PLANESPLITTER_P_H
