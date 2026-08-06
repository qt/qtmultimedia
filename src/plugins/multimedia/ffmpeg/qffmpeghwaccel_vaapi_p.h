// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGHWACCEL_VAAPI_P_H
#define QFFMPEGHWACCEL_VAAPI_P_H

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

#include <QtMultimedia/private/qdmabuftextureimporter_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegtextureconverter_p.h>

static_assert(QT_CONFIG(vaapi));

QT_BEGIN_NAMESPACE

class QRhi;

namespace QFFmpeg {

using QtMultimediaPrivate::DmaBufEglContext;
using QtMultimediaPrivate::DmaBufPlane;
using QtMultimediaPrivate::FailureSeverity;
using QtMultimediaPrivate::importDmaBufTextures;

class VAAPITextureConverter : public TextureConverterBackend
{
public:
    VAAPITextureConverter(QRhi *rhi);
    ~VAAPITextureConverter() override;

    QVideoFrameTexturesHandlesUPtr
    createTextureHandles(AVFrame *frame, QVideoFrameTexturesHandlesUPtr oldHandles) override;

    DmaBufEglContext eglContext;
};
} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
