// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERVIDEOSOURCE_P_H
#define QGSTREAMERVIDEOSOURCE_P_H

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

#include "private/qobject_p.h"
#include <QtMultimedia/spi/qgstreamervideosource.h>
#include <QtCore/qstring.h>

#include <variant>

QT_BEGIN_NAMESPACE

class QPlatformCamera;
class QGStreamerVideoSource;
class QMediaCaptureSession;

using GstElementOrDescription = std::variant<QString, GstElement *>;

class QGStreamerVideoSourcePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGStreamerVideoSource)
public:
    QString gstBinDescription;
    QPlatformCamera *platformCamera = nullptr;
    QMediaCaptureSession *captureSession = nullptr;

    static QGStreamerVideoSourcePrivate *get(QGStreamerVideoSource *source)
    {
        return static_cast<QGStreamerVideoSourcePrivate *>(QObjectPrivate::get(source));
    }

    // export to use in QQuickGStreamerVideoSource
    Q_MULTIMEDIA_EXPORT void createPlatformCamera(QGStreamerVideoSource *source,
                                                  GstElementOrDescription elementOrDesc,
                                                  bool active = false);
};

QT_END_NAMESPACE

#endif // QGSTREAMERVIDEOSOURCE_P_H
