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

#include "qffmpeghwaccel_p.h"

#if QT_CONFIG(wmf)

QT_BEGIN_NAMESPACE

class QRhi;

namespace QFFmpeg {

class D3D11TextureConverter : public TextureConverterBackend
{
public:
    D3D11TextureConverter(QRhi *rhi);

    TextureSet *getTextures(AVFrame *frame) override;

    static void SetupDecoderTextures(AVCodecContext *s);
};

}

QT_END_NAMESPACE

#endif

#endif
