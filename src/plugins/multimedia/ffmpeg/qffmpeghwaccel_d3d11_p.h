// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGHWACCEL_D3D11_P_H
#define QFFMPEGHWACCEL_D3D11_P_H

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

#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#include <QtMultimedia/private/qwindowsd3d11rhitextureimporter_p.h>
#include <QtCore/private/qcomptr_p.h>

#include <d3d11.h>
#include <d3d11_1.h>

#ifdef Q_OS_WINDOWS

QT_BEGIN_NAMESPACE

class QRhi;

namespace QFFmpeg {

class D3D11TextureConverter : public TextureConverterBackend
{
public:
    static std::shared_ptr<D3D11TextureConverter> create(QRhi *rhi);

    QVideoFrameTexturesHandlesUPtr
    createTextureHandles(AVFrame *frame, QVideoFrameTexturesHandlesUPtr oldHandles) override;

    static void SetupDecoderTextures(AVCodecContext *s);

protected:
    explicit D3D11TextureConverter(QRhi *rhi);

private:
    using QWindowsD3D11RhiTextureImporter = QtMultimediaPrivate::QWindowsD3D11RhiTextureImporter;
    QWindowsD3D11RhiTextureImporter m_importer;
};

AVFrameUPtr copyFromHwPoolD3D11(AVFrameUPtr src);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif

#endif
