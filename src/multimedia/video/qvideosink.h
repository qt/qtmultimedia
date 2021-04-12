/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    enum GraphicsType
    {
        Memory,
        NativeWindow,
        RhiTexture
    };

    QVideoSink(QObject *parent = nullptr);
    ~QVideoSink();

    GraphicsType graphicsType() const;
    void setGraphicsType(GraphicsType type);

    static bool isGraphicsTypeSupported(GraphicsType type);

    // setter sets graphics type to NativeWindow
    WId nativeWindowId() const;
    void setNativeWindowId(WId id);

    QRhi *rhi() const;
    void setRhi(QRhi *rhi);

    void setFullScreen(bool fullscreen);
    bool isFullscreen() const;

    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);

    QRectF targetRect() const;
    void setTargetRect(const QRectF &rect);

    int brightness() const;
    void setBrightness(int brightness);

    int contrast() const;
    void setContrast(int contrast);

    int hue() const;
    void setHue(int hue);

    int saturation() const;
    void setSaturation(int saturation);

    Qt::BGMode backgroundMode() const;
    void setBackgroundMode(Qt::BGMode mode);

    void paint(QPainter *painter, const QVideoFrame &frame);

    QPlatformVideoSink *platformVideoSink();

Q_SIGNALS:
    // would never get called in windowed mode
    QVideoFrame newVideoFrame(const QVideoFrame &frame) const;

private:
    QVideoSinkPrivate *d = nullptr;
};

QT_END_NAMESPACE

#endif
