// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFSCREENCAPTURE_H
#define QAVFSCREENCAPTURE_H

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

#include "private/qplatformsurfacecapture_p.h"
#include <qmutex.h>
#include <qwaitcondition.h>

QT_BEGIN_NAMESPACE

class QFFmpegVideoSink;

class QAVFScreenCapture : public QPlatformSurfaceCapture
{
    Q_OBJECT

    class Grabber;

public:
    explicit QAVFScreenCapture();
    ~QAVFScreenCapture() override;

    QVideoFrameFormat frameFormat() const override;

    std::optional<int> ffmpegHWPixelFormat() const override;

protected:
    bool setActiveInternal(bool active) override;

private:
    void onNewFrame(const QVideoFrame &frame);

    bool initScreenCapture(QScreen *screen);

    void resetCapture();

private:
    std::optional<QVideoFrameFormat> m_format;
    mutable QMutex m_formatMutex;
    mutable QWaitCondition m_waitForFormat;

    std::unique_ptr<Grabber> m_grabber;
};

QT_END_NAMESPACE

#endif // QAVFSCREENCAPTURE_H
