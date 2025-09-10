// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef X11SURFACECAPTURE_P_H
#define X11SURFACECAPTURE_P_H

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

#include <QtMultimedia/private/qplatformsurfacecapture_p.h>

QT_BEGIN_NAMESPACE

class QX11SurfaceCapture : public QPlatformSurfaceCapture
{
    class Grabber;

public:
    explicit QX11SurfaceCapture(Source initialSource);
    ~QX11SurfaceCapture() override;

    QVideoFrameFormat frameFormat() const override;

    static bool isSupported();

protected:
    bool setActiveInternal(bool active) override;

private:
    void activate(ScreenSource);

    void activate(WindowSource);

private:
    std::unique_ptr<Grabber> m_grabber;
};

QT_END_NAMESPACE

#endif // X11SURFACECAPTURE_P_H
