// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERVIDEOSOURCE_H
#define QGSTREAMERVIDEOSOURCE_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QtMM semi-private API (SPI), with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with future versions of Qt.
//

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtMultimedia/qtmultimediaglobal.h>

// NOLINTBEGIN (bugprone-reserved-identifier)
typedef struct _GstElement GstElement;
// NOLINTEND (bugprone-reserved-identifier)

QT_BEGIN_NAMESPACE

class QPlatformCamera;
class QGStreamerVideoSourcePrivate;
class QMediaCaptureSession;

class Q_MULTIMEDIA_EXPORT QGStreamerVideoSource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QString gstBinDescription READ gstBinDescription CONSTANT)

public:
    explicit QGStreamerVideoSource(const QString &gstBinDescription, QObject *parent = nullptr);
    explicit QGStreamerVideoSource(GstElement *gstElement, QObject *parent = nullptr);
    ~QGStreamerVideoSource() override;

    bool isActive() const;
    QString gstBinDescription() const;
    GstElement *gstElement() const;
    QMediaCaptureSession *captureSession() const;

public Q_SLOTS:
    void setActive(bool active);
    void start() { setActive(true); }
    void stop() { setActive(false); }

Q_SIGNALS:
    void activeChanged(bool active);

protected:
    explicit QGStreamerVideoSource(QObject *parent);

private:
    QPlatformCamera *platformVideoSource() const;

    void setCaptureSession(QMediaCaptureSession *session);

private:
    Q_DISABLE_COPY(QGStreamerVideoSource)
    Q_DECLARE_PRIVATE(QGStreamerVideoSource)
    friend class QMediaCaptureSession;
};

QT_END_NAMESPACE

#endif // QGSTREAMERVIDEOSOURCE_H
