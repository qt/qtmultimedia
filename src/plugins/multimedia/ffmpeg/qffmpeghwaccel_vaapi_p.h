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

#include "qffmpeghwaccel_p.h"

#if QT_CONFIG(vaapi)

#include <qshareddata.h>

QT_BEGIN_NAMESPACE

class QRhi;
class QOpenGLContext;

namespace QFFmpeg {

class VAAPITextureConverter : public TextureConverterBackend
{
public:
    VAAPITextureConverter(QRhi *rhi);
    ~VAAPITextureConverter();

    TextureSet *getTextures(AVFrame *frame) override;

    Qt::HANDLE eglDisplay = nullptr;
    QOpenGLContext *glContext = nullptr;
    QFunctionPointer eglImageTargetTexture2D = nullptr;
};
}

QT_END_NAMESPACE

#endif

#endif
