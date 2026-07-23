// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSCREENCAPTURE_P_H
#define QQUICKSCREENCAPTURE_P_H

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

#include <QtCore/private/qglobal_p.h>

#include <QtMultimedia/qscreencapture.h>

#include <QtMultimediaQuick/qtmultimediaquickexports.h>

#include <QtQml/qqml.h>

#include <QtQuick/private/qquickscreen_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIAQUICK_EXPORT QQuickScreenCapture : public QScreenCapture
{
    Q_OBJECT
    Q_PROPERTY(QQuickScreenInfo *screen READ ensureQmlScreen WRITE qmlSetScreen NOTIFY qmlScreenChanged)
    Q_PROPERTY(qreal maximumFrameRate READ qmlMaximumFrameRate WRITE qmlSetMaximumFrameRate RESET
                       qmlResetMaximumFrameRate NOTIFY qmlMaximumFrameRateChanged REVISION(6, 12))
    QML_NAMED_ELEMENT(ScreenCapture)

public:
    QQuickScreenCapture(QObject *parent = nullptr);

    void qmlSetScreen(QQuickScreenInfo *newQmlScreen);

    QQuickScreenInfo *ensureQmlScreen();

    void qmlSetMaximumFrameRate(qreal);
    qreal qmlMaximumFrameRate() const;
    void qmlResetMaximumFrameRate();

Q_SIGNALS:
    void qmlScreenChanged(QQuickScreenInfo *);
    void qmlMaximumFrameRateChanged(qreal);

private:
    QPointer<QQuickScreenInfo> m_qmlScreen;
    std::unique_ptr<QQuickScreenInfo> m_ownQmlScreen;
};

QT_END_NAMESPACE

#endif // QQUICKSCREENCAPTURE_P_H
