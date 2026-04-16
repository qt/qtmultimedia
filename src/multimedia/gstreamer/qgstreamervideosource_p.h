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
#include <QtMultimedia/qgstreamervideosource.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QPlatformCamera;
class QGStreamerVideoSource;

class QGStreamerVideoSourcePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGStreamerVideoSource)
public:
    QString gstBinDescription;
    QPlatformCamera *control = nullptr;
};

QT_END_NAMESPACE

#endif // QGSTREAMERVIDEOSOURCE_P_H
