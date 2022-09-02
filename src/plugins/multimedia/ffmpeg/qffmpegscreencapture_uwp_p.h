// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGSCREENCAPTURE_UWP_P_H
#define QFFMPEGSCREENCAPTURE_UWP_P_H

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

#include <QtCore/qnamespace.h>
#include "qffmpegscreencapturebase_p.h"
#include "qvideoframeformat.h"
#include <memory>

QT_BEGIN_NAMESPACE

class ScreenGrabberActiveUwp;
class QFFmpegScreenCaptureUwp : public QFFmpegScreenCaptureBase
{
public:
    explicit QFFmpegScreenCaptureUwp(QScreenCapture *screenCapture);
    ~QFFmpegScreenCaptureUwp();

    QVideoFrameFormat format() const override;

    static bool isSupported();

private:
    void resetGrabber();

private:
    friend ScreenGrabberActiveUwp;

    void emitError(QScreenCapture::Error code, const QString &desc);

    bool setActiveInternal(bool active) override;

    std::unique_ptr<ScreenGrabberActiveUwp> m_screenGrabber;
    QVideoFrameFormat m_format;
};

QT_END_NAMESPACE

#endif // QFFMPEGSCREENCAPTURE_UWP_P_H
