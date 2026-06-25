// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSCREENCAPTURE_H
#define QSCREENCAPTURE_H

#include <QtCore/qobject.h>
#include <QtCore/qnamespace.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <QtGui/qwindowdefs.h>
#include <QtMultimedia/qtmultimediaglobal.h>

#include <optional>

QT_BEGIN_NAMESPACE

class QMediaCaptureSession;
class QPlatformSurfaceCapture;
class QScreenCapturePrivate;

class Q_MULTIMEDIA_EXPORT QScreenCapture : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QScreen *screen READ screen WRITE setScreen NOTIFY screenChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
    Q_PROPERTY(std::optional<qreal> frameRate READ frameRate WRITE setFrameRate RESET resetFrameRate NOTIFY frameRateChanged)

public:
    enum Error {
        NoError = 0,
        InternalError = 1,
        CapturingNotSupported = 2,
        CaptureFailed = 4,
        NotFound = 5,
    };
    Q_ENUM(Error)

    explicit QScreenCapture(QObject *parent = nullptr);
    ~QScreenCapture() override;

    QMediaCaptureSession *captureSession() const;

    void setScreen(QScreen *screen);
    QScreen *screen() const;

    bool isActive() const;

    Error error() const;
    QString errorString() const;

    void setFrameRate(std::optional<qreal> preferredFrameRate);
    std::optional<qreal> frameRate() const;
    void resetFrameRate();

public Q_SLOTS:
    void setActive(bool active);
    void start() { setActive(true); }
    void stop() { setActive(false); }

Q_SIGNALS:
    void activeChanged(bool);
    void errorChanged();
    void screenChanged(QScreen *);
    void errorOccurred(QScreenCapture::Error error, const QString &errorString);
    void frameRateChanged();

private:
    void setCaptureSession(QMediaCaptureSession *captureSession);
    QPlatformSurfaceCapture *platformScreenCapture() const;
    friend class QMediaCaptureSession;
    Q_DISABLE_COPY(QScreenCapture)
    Q_DECLARE_PRIVATE(QScreenCapture)
};

QT_END_NAMESPACE

#endif // QSCREENCAPTURE_H
