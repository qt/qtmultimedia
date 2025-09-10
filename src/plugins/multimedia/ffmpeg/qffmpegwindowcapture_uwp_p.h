// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGWINDOWCAPTURE_UWP_P_H
#define QFFMPEGWINDOWCAPTURE_UWP_P_H

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
#include <QtMultimedia/private/qplatformsurfacecapture_p.h>
#include "qvideoframeformat.h"
#include <memory>

QT_BEGIN_NAMESPACE

class QFFmpegWindowCaptureUwp : public QPlatformSurfaceCapture
{
public:
    QFFmpegWindowCaptureUwp();
    ~QFFmpegWindowCaptureUwp() override;

    QVideoFrameFormat frameFormat() const override;

    static bool isSupported();

private:
    class Grabber;

    bool setActiveInternal(bool active) override;

private:
    std::unique_ptr<Grabber> m_grabber;
};

QT_END_NAMESPACE

#endif // QFFMPEGWINDOWCAPTURE_UWP_P_H
