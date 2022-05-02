/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QABSTRACTVIDEOSINK_H
#define QABSTRACTVIDEOSINK_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/qobject.h>
#include <QtGui/qwindowdefs.h>

QT_BEGIN_NAMESPACE

class QRectF;
class QVideoFrameFormat;
class QVideoFrame;

class QVideoSinkPrivate;
class QPlatformVideoSink;
class QRhi;

class Q_MULTIMEDIA_EXPORT QVideoSink : public QObject
{
    Q_OBJECT
public:
    QVideoSink(QObject *parent = nullptr);
    ~QVideoSink();

    QRhi *rhi() const;
    void setRhi(QRhi *rhi);

    QSize videoSize() const;

    QString subtitleText() const;
    void setSubtitleText(const QString &subtitle);

    void setVideoFrame(const QVideoFrame &frame);
    QVideoFrame videoFrame() const;

    QPlatformVideoSink *platformVideoSink() const;
Q_SIGNALS:
    void videoFrameChanged(const QVideoFrame &frame) const;
    void subtitleTextChanged(const QString &subtitleText) const;

    void videoSizeChanged();

private:
    friend class QMediaPlayerPrivate;
    friend class QMediaCaptureSessionPrivate;
    void setSource(QObject *source);

    QVideoSinkPrivate *d = nullptr;
};

QT_END_NAMESPACE

#endif
