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

#include "qvideoframeformat.h"
#include "qvideoframe.h"
#include "qmediaplayer.h"
#include "qmediacapturesession.h"

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
    void unregisterSource()
    {
        if (!source)
            return;
        auto *old = source;
        source = nullptr;
        if (auto *player = qobject_cast<QMediaPlayer *>(old))
            player->setVideoSink(nullptr);
        else if (auto *capture = qobject_cast<QMediaCaptureSession *>(old))
            capture->setVideoSink(nullptr);
    }

    QVideoSink *q_ptr = nullptr;
    QPlatformVideoSink *videoSink = nullptr;
    QObject *source = nullptr;
    bool fullScreen = false;
    WId window = 0;
    QRhi *rhi = nullptr;
    Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio;
    QRectF targetRect;
    float brightness = 0;
    float contrast = 0;
    float saturation = 0;
    float hue = 0;
    Qt::BGMode backgroundMode = Qt::OpaqueMode;
};

/*!
    \class QVideoSink

    \brief The QVideoSink class represents a generic sink for video data.
    \inmodule QtMultimedia
    \ingroup multimedia

    The QVideoSink class can be used to retrieve video data on a frame by frame
    basis from Qt Multimedia.

    QVideoSink can operate in two modes. In the first mode, it can render the video
    stream to a native window of the underlying windowing system. In the other mode,
    it will provide individual video frames to the application developer through the
    newVideoFrame() signal.

    The video frame can then be used to read out the data of those frames and handle them
    further. When using QPainter, the QVideoFrame can be drawing using the paint() method
    in QVideoSink.

    QVideoFrame objects can consume a significant amount of memory or system resources and
    should thus not be held for longer than required by the application.

    \sa QMediaPlayer, QMediaCaptureSession

*/

/*!
    Constructs a new QVideoSink object with \a parent.
 */
QVideoSink::QVideoSink(QObject *parent)
    : QObject(parent),
    d(new QVideoSinkPrivate(this))
{
    qRegisterMetaType<QVideoFrame>();
}

/*!
    Destroys the object.
 */
QVideoSink::~QVideoSink()
{
    d->unregisterSource();
    delete d;
}

/*!
    Returns the native window id that the sink is currently rendering to.
 */
WId QVideoSink::nativeWindowId() const
{
    return d->window;
}

/*!
    Tells QVideoSink to render directly to the native window \a id. This is
    usually more resource efficient than rendering individual video frames.

    The newVideoFrame() signal will never get emitted in this mode.

    Setting \a id to 0 will stop rendering to a native window.
 */
void QVideoSink::setNativeWindowId(WId id)
{
    if (d->window == id)
        return;
    d->window = id;
    if (d->videoSink != nullptr)
        d->videoSink->setWinId(id);
}

/*!
    \internal
    Returns the QRhi instance being used to create texture data in the video frames.
 */
QRhi *QVideoSink::rhi() const
{
    return d->rhi;
}

/*!
    \internal
    Sets the QRhi instance being used to create texture data in the video frames
    to \a rhi.
 */
void QVideoSink::setRhi(QRhi *rhi)
{
    if (d->rhi == rhi)
        return;
    d->rhi = rhi;
    d->videoSink->setRhi(rhi);
}

/*!
    Render the video full screen. This is often more resource efficient than
    rendering to only parts of the screen.

    The newVideoFrame() signal will never get emitted when rendering full screen.
 */
void QVideoSink::setFullScreen(bool fullscreen)
{
    if (d->fullScreen == fullscreen)
        return;
    d->fullScreen = fullscreen;
    d->videoSink->setFullScreen(fullscreen);
    emit fullScreenChanged(d->fullScreen);
}

/*!
    Returns true when rendering full screen.
 */
bool QVideoSink::isFullScreen() const
{
    return d->fullScreen;
}

Qt::AspectRatioMode QVideoSink::aspectRatioMode() const
{
    return d->aspectRatioMode;
}

void QVideoSink::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    if (d->aspectRatioMode == mode)
        return;
    d->aspectRatioMode = mode;
    d->videoSink->setAspectRatioMode(mode);
    emit aspectRatioModeChanged(mode);
}

QRectF QVideoSink::targetRect() const
{
    return d->targetRect;
}

void QVideoSink::setTargetRect(const QRectF &rect)
{
    if (d->targetRect == rect)
        return;
    d->targetRect = rect;
    if (d->videoSink != nullptr)
        d->videoSink->setDisplayRect(rect.toRect());
}

#if 0
float QVideoSink::brightness() const
{
    return d->brightness;
}

void QVideoSink::setBrightness(float brightness)
{
    brightness = qBound(-1., brightness, 1.);
    if (d->brightness == brightness)
        return;
    d->brightness = brightness;
    d->videoSink->setBrightness(brightness);
    emit brightnessChanged(brightness);
}

float QVideoSink::contrast() const
{
    return d->contrast;
}

void QVideoSink::setContrast(float contrast)
{
    contrast = qBound(-1., contrast, 1.);
    if (d->contrast == contrast)
        return;
    d->contrast = contrast;
    d->videoSink->setContrast(contrast);
    emit contrastChanged(contrast);
}

float QVideoSink::hue() const
{
    return d->hue;
}

void QVideoSink::setHue(float hue)
{
    hue = qBound(-1., hue, 1.);
    if (d->hue == hue)
        return;
    d->hue = hue;
    d->videoSink->setHue(hue);
    emit hueChanged(hue);
}

float QVideoSink::saturation() const
{
    return d->saturation;
}

void QVideoSink::setSaturation(float saturation)
{
    saturation = qBound(-1., saturation, 1.);
    if (d->saturation == saturation)
        return;
    d->saturation = saturation;
    d->videoSink->setSaturation(saturation);
    emit saturationChanged(saturation);
}
#endif

Qt::BGMode QVideoSink::backgroundMode() const
{
    return d->backgroundMode;
}

void QVideoSink::setBackgroundMode(Qt::BGMode mode)
{
    d->backgroundMode = mode;
}

/*!
    \internal
*/
QPlatformVideoSink *QVideoSink::platformVideoSink() const
{
    return d->videoSink;
}

/*!
    Returns the size of the video currently being played back. If no video is
    being played, this method returns an invalid size.
 */
QSize QVideoSink::videoSize() const
{
    return d->videoSink ? d->videoSink->nativeSize() : QSize{};
}

void QVideoSink::setSource(QObject *source)
{
    if (d->source == source)
        return;
    if (source)
        d->unregisterSource();
    d->source = source;
}

QT_END_NAMESPACE

#include "moc_qvideosink.cpp"


