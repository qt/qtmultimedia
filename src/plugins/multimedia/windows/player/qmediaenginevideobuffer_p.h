// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEDIAENGINEVIDEOBUFFER_P_H
#define QMEDIAENGINEVIDEOBUFFER_P_H

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

#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qwindowsd3d11rhitextureimporter_p.h>
#include <QtMultimedia/private/qwindowsd3d11texturereadback_p.h>
#include <QtCore/qsize.h>
#include <QtCore/private/qcomptr_p.h>

#include <d3d11.h>
#include <d3d11_1.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QMediaEngineVideoBuffer : public QHwVideoBuffer
{
public:
    // 'importer' is created once by the player and shared with every video buffer, so that
    // the interop resources it caches (shared texture bridge, D3D11on12 wrapper device, WGL
    // interop device handle, etc.) survive across frames.
    QMediaEngineVideoBuffer(
            ComPtr<ID3D11Texture2D> texture, ComPtr<ID3D11Device> engineDevice, QSize frameSize,
            std::shared_ptr<QtMultimediaPrivate::QWindowsD3D11RhiTextureImporter> importer);
    ~QMediaEngineVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &oldTextures) override;

private:
    ComPtr<ID3D11Texture2D> m_texture;
    ComPtr<ID3D11Device> m_engineDevice;
    QSize m_frameSize;

    QtMultimediaPrivate::QWindowsD3D11TextureReadback m_readback;
    std::shared_ptr<QtMultimediaPrivate::QWindowsD3D11RhiTextureImporter> m_importer;
};

QT_END_NAMESPACE

#endif // QMEDIAENGINEVIDEOBUFFER_P_H
