// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSCREENCAPTURE_P_H
#define QSCREENCAPTURE_P_H

#include <QtCore/private/qobject_p.h>

#include <QtMultimedia/qscreencapture.h>

#include <memory>

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

QT_BEGIN_NAMESPACE

class QMediaCaptureSession;
class QPlatformSurfaceCapture;

class Q_MULTIMEDIA_EXPORT QScreenCapturePrivate : public QObjectPrivate
{
public:
    [[nodiscard]] static QScreenCapturePrivate *get(QScreenCapture &capture)
    {
        return capture.d_func();
    }

    QMediaCaptureSession *captureSession = nullptr;
    std::unique_ptr<QPlatformSurfaceCapture> platformScreenCapture;

    // Only applied when next stream starts.
    void setIgnoreCursor(bool);
};

QT_END_NAMESPACE

#endif // QSCREENCAPTURE_P_H
