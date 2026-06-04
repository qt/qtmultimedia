// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKWINDOWCAPTURE_H
#define QQUICKWINDOWCAPTURE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qwindowcapture.h>
#include <QtMultimediaQuick/qtmultimediaquickexports.h>

#include <QtQml/qqmlregistration.h>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIAQUICK_EXPORT QQuickWindowCapture : public QWindowCapture
{
    Q_OBJECT
    Q_PROPERTY(qreal frameRate READ qmlFrameRate WRITE qmlSetFrameRate RESET resetFrameRate NOTIFY frameRateChanged)
    QML_NAMED_ELEMENT(WindowCapture)

public:
    QQuickWindowCapture(QObject *parent = nullptr);

    void qmlSetFrameRate(qreal);
    qreal qmlFrameRate() const;
};

QT_END_NAMESPACE

#endif
