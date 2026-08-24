// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_MULTIMEDIAQUICK_QML_SCREENCAPTUREHELPER_H
#define TST_MULTIMEDIAQUICK_QML_SCREENCAPTUREHELPER_H

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qstring.h>

#include <QtMultimedia/qscreencapture.h>

QT_USE_NAMESPACE

class ScreenCaptureHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString primaryScreenName READ primaryScreenName CONSTANT FINAL)

public:
    static QString primaryScreenName();

    Q_INVOKABLE static bool isScreenCapture(QScreenCapture *capture);
    Q_INVOKABLE static bool isQuickScreenCapture(QScreenCapture *capture);
    Q_INVOKABLE static bool hasScreen(QScreenCapture *capture);
    Q_INVOKABLE static QString screenName(QScreenCapture *capture);
    Q_INVOKABLE static QObject *qmlScreen(QScreenCapture *capture);
    Q_INVOKABLE static bool hasMaximumFrameRate(QScreenCapture *capture);
    Q_INVOKABLE static qreal maximumFrameRate(QScreenCapture *capture);
    Q_INVOKABLE static bool isActive(QScreenCapture *capture);

    Q_INVOKABLE static void setPrimaryScreen(QScreenCapture *capture);
    Q_INVOKABLE static void clearScreen(QScreenCapture *capture);

    Q_INVOKABLE static void setScreenFromTemporaryQmlScreen(QScreenCapture *capture, bool primary);

    Q_INVOKABLE static void setMaximumFrameRate(QScreenCapture *capture, qreal frameRate);
    Q_INVOKABLE static void clearMaximumFrameRate(QScreenCapture *capture);
    Q_INVOKABLE static void setActive(QScreenCapture *capture, bool active);

    Q_INVOKABLE void rememberQmlScreen(QScreenCapture *capture);
    Q_INVOKABLE bool rememberedQmlScreenIsDestroyed() const;

private:
    QPointer<QObject> m_rememberedQmlScreen;
};

#endif // TST_MULTIMEDIAQUICK_QML_SCREENCAPTUREHELPER_H
