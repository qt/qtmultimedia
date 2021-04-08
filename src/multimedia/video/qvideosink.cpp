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

#include "qvideosink.h"

#include "qvideosurfaceformat.h"
#include "qvideoframe.h"

#include <qvariant.h>
#include <qpainter.h>
#include <qmatrix4x4.h>
#include <QDebug>
#include <private/qplatformmediaintegration_p.h>
#include <private/qplatformvideosink_p.h>

QT_BEGIN_NAMESPACE

class QVideoSinkPrivate {
public:
    QVideoSinkPrivate(QVideoSink *q)
        : q_ptr(q)
    {
        videoSink = QPlatformMediaIntegration::instance()->createVideoSink(q);
    }
    ~QVideoSinkPrivate()
    {
        delete videoSink;
    }
    QVideoSink *q_ptr = nullptr;
    QPlatformVideoSink *videoSink = nullptr;
    QVideoSink::GraphicsType type = QVideoSink::Memory;
    QVideoSurfaceFormat surfaceFormat;
    QSize nativeResolution;
    bool active = false;
    WId window = 0;
    QRhi *rhi = nullptr;
    Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio;
    QRectF targetRect;
    int brightness = 0;
    int contrast = 0;
    int saturation = 0;
    int hue = 0;
    Qt::BGMode backgroundMode = Qt::OpaqueMode;
};

QVideoSink::QVideoSink(QObject *parent)
    : QObject(parent),
    d(new QVideoSinkPrivate(this))
{
    qRegisterMetaType<QVideoFrame>();
}

QVideoSink::~QVideoSink()
{
    delete d;
}

QVideoSink::GraphicsType QVideoSink::graphicsType() const
{
    return d->videoSink->graphicsType();
}

void QVideoSink::setGraphicsType(QVideoSink::GraphicsType type)
{
    d->videoSink->setGraphicsType(type);
}

bool QVideoSink::isGraphicsTypeSupported(QVideoSink::GraphicsType type)
{
    // ####
    return type == NativeWindow;
}

WId QVideoSink::nativeWindowId() const
{
    return d->videoSink->winId();
}

void QVideoSink::setNativeWindowId(WId id)
{
    d->videoSink->setWinId(id);
}

QRhi *QVideoSink::rhi() const
{
    return d->rhi;
}

void QVideoSink::setRhi(QRhi *rhi)
{
    if (d->rhi == rhi)
        return;
    d->rhi = rhi;
    d->videoSink->setRhi(rhi);
}

void QVideoSink::setFullScreen(bool fullscreen)
{
    d->videoSink->setFullScreen(fullscreen);
}

bool QVideoSink::isFullscreen() const
{
    return d->videoSink->isFullScreen();
}

Qt::AspectRatioMode QVideoSink::aspectRatioMode() const
{
    return d->videoSink->aspectRatioMode();
}

void QVideoSink::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    d->videoSink->setAspectRatioMode(mode);
}

QRectF QVideoSink::targetRect() const
{
    return d->targetRect;
}

void QVideoSink::setTargetRect(const QRectF &rect)
{
    d->videoSink->setDisplayRect(rect.toRect());
    d->targetRect = rect;
}

int QVideoSink::brightness() const
{
    return d->videoSink->brightness();
}

void QVideoSink::setBrightness(int brightness)
{
    d->videoSink->setBrightness(brightness);
}

int QVideoSink::contrast() const
{
    return d->videoSink->contrast();
}

void QVideoSink::setContrast(int contrast)
{
    d->videoSink->setContrast(contrast);
}

int QVideoSink::hue() const
{
    return d->videoSink->hue();
}

void QVideoSink::setHue(int hue)
{
    d->videoSink->setHue(hue);
}

int QVideoSink::saturation() const
{
    return d->videoSink->saturation();
}

void QVideoSink::setSaturation(int saturation)
{
    d->videoSink->setSaturation(saturation);
}

Qt::BGMode QVideoSink::backgroundMode() const
{
    return d->backgroundMode;
}

void QVideoSink::setBackgroundMode(Qt::BGMode mode)
{
    d->backgroundMode = mode;
}

void QVideoSink::paint(QPainter *painter, const QVideoFrame &f)
{
    QVideoFrame frame(f);
    if (!frame.isValid()) {
        painter->fillRect(d->targetRect, painter->background());
        return;
    }

    auto imageFormat = QVideoSurfaceFormat::imageFormatFromPixelFormat(frame.pixelFormat());
    // Do not render into ARGB32 images using QPainter.
    // Using QImage::Format_ARGB32_Premultiplied is significantly faster.
    if (imageFormat == QImage::Format_ARGB32)
        imageFormat = QImage::Format_ARGB32_Premultiplied;

    QVideoSurfaceFormat::Direction scanLineDirection = QVideoSurfaceFormat::TopToBottom;//format.scanLineDirection();
    bool mirrored = false;//format.isMirrored();

    QSizeF size = frame.size();
    QRectF source = QRectF(0, 0, size.width(), size.height());
    QRectF targetRect = d->targetRect;
    if (d->aspectRatioMode == Qt::KeepAspectRatio) {
        size.scale(targetRect.size(), Qt::KeepAspectRatio);
        targetRect = QRect(0, 0, size.width(), size.height());
        targetRect.moveCenter(d->targetRect.center());
        // we might not be drawing every pixel, fill the leftover black
        if (d->backgroundMode == Qt::OpaqueMode && d->targetRect != targetRect) {
            if (targetRect.top() > d->targetRect.top()) {
                QRectF top(d->targetRect.left(), d->targetRect.top(), d->targetRect.width(), targetRect.top() - d->targetRect.top());
                painter->fillRect(top, Qt::black);
            }
            if (targetRect.left() > d->targetRect.left()) {
                QRectF top(d->targetRect.left(), targetRect.top(), targetRect.left() - d->targetRect.left(), targetRect.height());
                painter->fillRect(top, Qt::black);
            }
            if (targetRect.right() < d->targetRect.right()) {
                QRectF top(targetRect.right(), targetRect.top(), d->targetRect.right() - targetRect.right(), targetRect.height());
                painter->fillRect(top, Qt::black);
            }
            if (targetRect.bottom() < d->targetRect.bottom()) {
                QRectF top(d->targetRect.left(), targetRect.bottom(), d->targetRect.width(), d->targetRect.bottom() - targetRect.bottom());
                painter->fillRect(top, Qt::black);
            }
        }
    } else if (d->aspectRatioMode == Qt::KeepAspectRatioByExpanding) {
        QSizeF targetSize = targetRect.size();
        targetSize.scale(size, Qt::KeepAspectRatio);

        QRectF s(0, 0, targetSize.width(), targetSize.height());
        s.moveCenter(source.center());
        source = s;
    }

    if (frame.map(QVideoFrame::ReadOnly)) {
        QImage image = frame.toImage();

        const QTransform oldTransform = painter->transform();
        QTransform transform = oldTransform;
        if (scanLineDirection == QVideoSurfaceFormat::BottomToTop) {
            transform.scale(1, -1);
            transform.translate(0, -targetRect.bottom());
            targetRect = QRectF(targetRect.x(), 0, targetRect.width(), targetRect.height());
        }

        if (mirrored) {
            transform.scale(-1, 1);
            transform.translate(-targetRect.right(), 0);
            targetRect = QRectF(0, targetRect.y(), targetRect.width(), targetRect.height());
        }
        painter->setTransform(transform);
        painter->drawImage(targetRect, image, source);
        painter->setTransform(oldTransform);

        frame.unmap();
    } else if (frame.isValid()) {
        // #### error handling
    } else {
        painter->fillRect(d->targetRect, Qt::black);
    }
}

QPlatformVideoSink *QVideoSink::platformVideoSink()
{
    return d->videoSink;
}

QT_END_NAMESPACE

#include "moc_qvideosink.cpp"


