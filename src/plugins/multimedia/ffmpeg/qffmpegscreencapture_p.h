// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGSCREENCAPTURE_P_H
#define QFFMPEGSCREENCAPTURE_P_H

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

#include "qffmpegscreencapturebase_p.h"
#include "qvideoframeformat.h"

#include <memory>

QT_BEGIN_NAMESPACE

class QFFmpegScreenCapture : public QFFmpegScreenCaptureBase
{
    class Grabber;

public:
    explicit QFFmpegScreenCapture(QScreenCapture *screenCapture);
    ~QFFmpegScreenCapture() override;

    QVideoFrameFormat format() const override;

protected:
    bool setActiveInternal(bool active) override;

    void updateError(QScreenCapture::Error error, const QString &description);

private:
    void resetGrabber();

private:
    std::unique_ptr<Grabber> m_grabber;
};

QT_END_NAMESPACE

#endif // QFFMPEGSCREENCAPTURE_P_H
