// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGVIDEOSINK_H
#define QFFMPEGVIDEOSINK_H

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

#include <private/qplatformvideosink_p.h>
#include <qffmpeghwaccel_p.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class QFFmpegVideoSink : public QPlatformVideoSink
{
    Q_OBJECT

public:
    QFFmpegVideoSink(QVideoSink *sink);
    void setRhi(QRhi *rhi) override;

    void setVideoFrame(const QVideoFrame &frame) override;

private:
    QFFmpeg::TextureConverter textureConverter;
    QRhi *m_rhi = nullptr;
};

QT_END_NAMESPACE


#endif
