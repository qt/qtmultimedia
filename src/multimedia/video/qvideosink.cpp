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
    QVideoSinkPrivate()
    {
        videoSink = QPlatformMediaIntegration::instance()->createVideoSink();
    }
    ~QVideoSinkPrivate()
    {
        delete videoSink;
    }
    QPlatformVideoSink *videoSink = nullptr;
    QVideoSink::GraphicsType type = QVideoSink::Memory;
    QVideoSurfaceFormat surfaceFormat;
    QSize nativeResolution;
    bool active = false;
    WId window = 0;
    Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio;
    QRectF targetRect;
    int brightness = 0;
    int contrast = 0;
    int saturation = 0;
    int hue = 0;
    QMatrix4x4 transform;
    float opacity = 1.;
};

QVideoSink::QVideoSink(QObject *parent)
    : QObject(parent),
    d(new QVideoSinkPrivate)
{
}

QVideoSink::~QVideoSink()
{
    delete d;
}

QVideoSink::GraphicsType QVideoSink::graphicsType() const
{
    return d->type;
}

void QVideoSink::setGraphicsType(QVideoSink::GraphicsType type)
{
    d->type = type;
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

QMatrix4x4 QVideoSink::transform() const
{
    return d->transform;
}

void QVideoSink::setTransform(const QMatrix4x4 &transform)
{
    d->transform = transform;
}

float QVideoSink::opacity() const
{
    return d->opacity;
}

void QVideoSink::setOpacity(float opacity)
{
    d->opacity = opacity;
}

void QVideoSink::render(const QVideoFrame &frame)
{
    Q_UNUSED(frame);

}

void QVideoSink::paint(QPainter *painter, const QVideoFrame &f)
{
    QVideoFrame frame(f);
    if (!frame.isValid()) {
        painter->fillRect(d->targetRect, Qt::black);
        return;
    }

    auto imageFormat = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
    // Do not render into ARGB32 images using QPainter.
    // Using QImage::Format_ARGB32_Premultiplied is significantly faster.
    if (imageFormat == QImage::Format_ARGB32)
        imageFormat = QImage::Format_ARGB32_Premultiplied;

    QVideoSurfaceFormat::Direction scanLineDirection = QVideoSurfaceFormat::TopToBottom;//format.scanLineDirection();
    bool mirrored = false;//format.isMirrored();

    QRectF source = d->targetRect; // ####

    if (frame.handleType() == QVideoFrame::QPixmapHandle) {
        painter->drawPixmap(d->targetRect, frame.handle().value<QPixmap>(), source);
    } else if (frame.map(QVideoFrame::ReadOnly)) {
        QImage image = frame.image();

        auto oldOpacity = painter->opacity();
        const QTransform oldTransform = painter->transform();
        QTransform transform = oldTransform;
        QRectF targetRect = d->targetRect;
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
        painter->setOpacity(d->opacity);
        painter->drawImage(targetRect, image, source);
        painter->setTransform(oldTransform);
        painter->setOpacity(oldOpacity);

        frame.unmap();
    } else if (frame.isValid()) {
        // #### error handling
    } else {
        painter->fillRect(targetRect(), Qt::black);
    }
}

QPlatformVideoSink *QVideoSink::platformVideoSink()
{
    return d->videoSink;
}

QT_END_NAMESPACE

#include "moc_qvideosink.cpp"


