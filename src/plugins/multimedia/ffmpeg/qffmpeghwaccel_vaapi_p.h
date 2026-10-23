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

#include <QtFFmpegMediaPluginImpl/private/qffmpegtextureconverter_p.h>

#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <memory>

static_assert(QT_CONFIG(vaapi));

QT_BEGIN_NAMESPACE

class QRhi;

namespace QFFmpeg {

struct VAAPITextureConverterFactory
{
public:
    static std::shared_ptr<TextureConverterBackend> create(QRhi *rhi);
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
