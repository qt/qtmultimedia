// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLFSSCREENCAPTURE_H
#define QEGLFSSCREENCAPTURE_H

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

#include <QtMultimedia/private/qplatformsurfacecapture_p.h>
#include <memory>

QT_BEGIN_NAMESPACE

class QEglfsScreenCapture : public QPlatformSurfaceCapture
{
public:
    QEglfsScreenCapture();

    ~QEglfsScreenCapture() override;

    QVideoFrameFormat frameFormat() const override;

    static bool isSupported();

private:
    bool setActiveInternal(bool active) override;

private:
    class Grabber;
    class QuickGrabber;

    std::unique_ptr<Grabber> createGrabber();

    std::unique_ptr<Grabber> m_grabber;
};

QT_END_NAMESPACE

#endif // QEGLFSSCREENCAPTURE_H
