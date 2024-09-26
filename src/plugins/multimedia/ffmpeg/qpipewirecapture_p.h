// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef PIPEWIREAPTURE_P_H
#define PIPEWIREAPTURE_P_H

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

QT_BEGIN_NAMESPACE

class QPipeWireCaptureHelper;

class QPipeWireCapture : public QPlatformSurfaceCapture
{
    Q_OBJECT
public:
    explicit QPipeWireCapture(Source initialSource);
    ~QPipeWireCapture() override;

    QVideoFrameFormat frameFormat() const override;

    static bool isSupported();

protected:
    bool setActiveInternal(bool active) override;

private:
    std::unique_ptr<QPipeWireCaptureHelper> m_helper;
};

QT_END_NAMESPACE

#endif // PIPEWIREAPTURE_P_H