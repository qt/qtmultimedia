// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QWINDOWSD3D11TEXTUREREADBACK_P_H
#define QWINDOWSD3D11TEXTUREREADBACK_P_H

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
#include <QtMultimedia/qabstractvideobuffer.h>

#include <QtCore/qsize.h>
#include <QtCore/private/qcomptr_p.h>

#include <d3d11.h>
#include <d3d11_1.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

/*! \internal
    Copies a D3D11 texture into a CPU-accessible staging texture so that its pixels can be
    read back, e.g. to implement QAbstractVideoBuffer::map() as a fallback when GPU import
    into the sink's QRhi is unavailable.
*/
class Q_MULTIMEDIA_EXPORT QWindowsD3D11TextureReadback
{
public:
    /** Maps 'texture', owned by 'device', for CPU read access. 'frameSize' is the region of
     *  the texture that holds valid frame data. Returns an empty MapData on failure. Must be
     *  paired with a call to unmap(). Only one mapping may be active at a time. */
    QAbstractVideoBuffer::MapData map(const ComPtr<ID3D11Device> &, const ComPtr<ID3D11Texture2D> &,
                                      const QSize &frameSize);
    void unmap();

private:
    ComPtr<ID3D11Texture2D> m_stagingTex;
    ComPtr<ID3D11DeviceContext> m_stagingCtx;
    bool m_mapped = false;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QWINDOWSD3D11TEXTUREREADBACK_P_H
