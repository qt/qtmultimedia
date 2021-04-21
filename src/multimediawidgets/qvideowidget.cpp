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

#include <private/qtmultimediaglobal_p.h>
#include "qvideowidget_p.h"

#include <QtCore/qobject.h>
#include <QtMultimedia/qtmultimediaglobal.h>
#include <qvideosink.h>

#include <qobject.h>
#include <qvideoframeformat.h>
#include <qpainter.h>

#include <qapplication.h>
#include <qevent.h>
#include <qboxlayout.h>
#include <qnamespace.h>

#include <qwindow.h>
#include <private/qhighdpiscaling_p.h>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#endif

using namespace Qt;

QT_BEGIN_NAMESPACE


void QVideoWidgetPrivate::_q_dimensionsChanged()
{
    q_func()->updateGeometry();
    q_func()->update();
}

void QVideoWidgetPrivate::_q_newFrame(const QVideoFrame &frame)
{
    lastFrame = frame;
    q_ptr->update();
}

/*!
    \class QVideoWidget


    \brief The QVideoWidget class provides a widget which presents video
    produced by a media object.
    \ingroup multimedia
    \inmodule QtMultimediaWidgets

    Attaching a QVideoWidget to a QMediaPlayer or QCamera allows it to display the
    video or image output of that object.

    \snippet multimedia-snippets/video.cpp Video widget

    \b {Note}: Only a single display output can be attached to a media
    object at one time.

    \sa QCamera, QMediaPlayer, QGraphicsVideoItem
*/

/*!
    Constructs a new video widget.

    The \a parent is passed to QWidget.
*/
QVideoWidget::QVideoWidget(QWidget *parent)
    : QWidget(parent, {})
    , d_ptr(new QVideoWidgetPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->videoSink = new QVideoSink(this);
    d_ptr->videoSink->setTargetRect(rect());

    d_ptr->videoSink->setNativeWindowId(winId());
    connect(d_ptr->videoSink, SIGNAL(newVideoFrame(const QVideoFrame &)), this, SLOT(_q_newFrame(const QVideoFrame &)));
    connect(d_ptr->videoSink, &QVideoSink::brightnessChanged, this, &QVideoWidget::brightnessChanged);
    connect(d_ptr->videoSink, &QVideoSink::contrastChanged, this, &QVideoWidget::contrastChanged);
    connect(d_ptr->videoSink, &QVideoSink::hueChanged, this, &QVideoWidget::hueChanged);
    connect(d_ptr->videoSink, &QVideoSink::saturationChanged, this, &QVideoWidget::saturationChanged);
}

/*!
    Destroys a video widget.
*/
QVideoWidget::~QVideoWidget()
{
    delete d_ptr;
}

QVideoSink *QVideoWidget::videoSink() const
{
    return d_ptr->videoSink;
}

/*!
    \property QVideoWidget::aspectRatioMode
    \brief how video is scaled with respect to its aspect ratio.
*/

Qt::AspectRatioMode QVideoWidget::aspectRatioMode() const
{
    return d_func()->aspectRatioMode;
}

void QVideoWidget::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    Q_D(QVideoWidget);

    if (mode == d->aspectRatioMode)
        return;
    d->aspectRatioMode = mode;
    d->videoSink->setAspectRatioMode(mode);
    emit aspectRatioModeChanged(mode);
}

/*!
    \property QVideoWidget::fullScreen
    \brief whether video display is confined to a window or is fullScreen.
*/

void QVideoWidget::setFullScreen(bool fullScreen)
{
    Q_D(QVideoWidget);

    Qt::WindowFlags flags = windowFlags();

    if (fullScreen) {
        d->nonFullScreenFlags = flags & (Qt::Window | Qt::SubWindow);
        flags |= Qt::Window;
        flags &= ~Qt::SubWindow;
        setWindowFlags(flags);

        showFullScreen();
    } else {
        flags &= ~(Qt::Window | Qt::SubWindow); //clear the flags...
        flags |= d->nonFullScreenFlags; //then we reset the flags (window and subwindow)
        setWindowFlags(flags);

        showNormal();
    }
    d->wasFullScreen = fullScreen;
    d->videoSink->setFullScreen(fullScreen);
    emit fullScreenChanged(fullScreen);
}

/*!
    \fn QVideoWidget::fullScreenChanged(bool fullScreen)

    Signals that the \a fullScreen mode of a video widget has changed.

    \sa isFullScreen()
*/

/*!
    \property QVideoWidget::brightness
    \brief an adjustment to the brightness of displayed video.

    Valid brightness values range between -1. and 1., the default is 0.
*/

float QVideoWidget::brightness() const
{
    return d_func()->videoSink->brightness();
}

void QVideoWidget::setBrightness(float brightness)
{
    Q_D(QVideoWidget);
    d->videoSink->setBrightness(brightness);
    float boundedBrightness = qBound(-1., brightness, 1.);

    if (boundedBrightness == d->videoSink->brightness())
        return;

    d->videoSink->setBrightness(boundedBrightness);
    emit brightnessChanged(boundedBrightness);
}

/*!
    \fn QVideoWidget::brightnessChanged(float brightness)

    Signals that a video widgets's \a brightness adjustment has changed.

    \sa brightness()
*/

/*!
    \property QVideoWidget::contrast
    \brief an adjustment to the contrast of displayed video.

    Valid contrast values range between -1. and 1., the default is 0.

*/

float QVideoWidget::contrast() const
{
    return d_func()->videoSink->contrast();
}

void QVideoWidget::setContrast(float contrast)
{
    Q_D(QVideoWidget);
    d->videoSink->setContrast(contrast);
}

/*!
    \fn QVideoWidget::contrastChanged(float contrast)

    Signals that a video widgets's \a contrast adjustment has changed.

    \sa contrast()
*/

/*!
    \property QVideoWidget::hue
    \brief an adjustment to the hue of displayed video.

    Valid hue values range between -1. and 1., the default is 0.
*/

float QVideoWidget::hue() const
{
    return d_func()->videoSink->hue();
}

void QVideoWidget::setHue(float hue)
{
    Q_D(QVideoWidget);
    d->videoSink->setHue(hue);
}

/*!
    \fn QVideoWidget::hueChanged(float hue)

    Signals that a video widgets's \a hue has changed.

    \sa hue()
*/

/*!
    \property QVideoWidget::saturation
    \brief an adjustment to the saturation of displayed video.

    Valid saturation values range between -1. and 1., the default is 0.
*/

float QVideoWidget::saturation() const
{
    return d_func()->videoSink->saturation();
}

void QVideoWidget::setSaturation(float saturation)
{
    Q_D(QVideoWidget);
    d->videoSink->setSaturation(saturation);
}

/*!
    \fn QVideoWidget::saturationChanged(float saturation)

    Signals that a video widgets's \a saturation has changed.

    \sa saturation()
*/

/*!
  Returns the size hint for the current back end,
  if there is one, or else the size hint from QWidget.
 */
QSize QVideoWidget::sizeHint() const
{
//    Q_D(const QVideoWidget);

// #####
//    if (d->video)
//        return d->backend->sizeHint();

    return QWidget::sizeHint();
}

/*!
  \reimp
  Current event \a event.
  Returns the value of the baseclass QWidget::event(QEvent *event) function.
*/
bool QVideoWidget::event(QEvent *event)
{
    Q_D(QVideoWidget);

    if (event->type() == QEvent::WindowStateChange) {
        if (windowState() & Qt::WindowFullScreen) {
            d->videoSink->setFullScreen(true);
            d->videoSink->setTargetRect(QRectF(0, 0, window()->width(), window()->height()));

            if (!d->wasFullScreen)
                emit fullScreenChanged(d->wasFullScreen = true);
        } else {
            d->videoSink->setFullScreen(false);

            if (d->wasFullScreen)
                emit fullScreenChanged(d->wasFullScreen = false);
        }
    }

    return QWidget::event(event);
}

/*!
  \reimp
  Handles the show \a event.
 */
void QVideoWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

/*!
  \reimp
  Handles the hide \a event.
*/
void QVideoWidget::hideEvent(QHideEvent *event)
{
    // ### maybe suspend video decoding???
    QWidget::hideEvent(event);
}

/*!
  \reimp
  Handles the resize \a event.
 */
void QVideoWidget::resizeEvent(QResizeEvent *event)
{
    d_ptr->videoSink->setTargetRect(QRectF(0, 0, event->size().width(), event->size().height()));
    QWidget::resizeEvent(event);
}

/*!
  \reimp
  Handles the move \a event.
 */
void QVideoWidget::moveEvent(QMoveEvent */*event*/)
{
}

/*!
  \reimp
  Handles the paint \a event.
 */
void QVideoWidget::paintEvent(QPaintEvent *event)
{
    Q_D(QVideoWidget);

    if (d->videoSink && d->lastFrame.isValid()) {
        QPainter painter(this);
        d->videoSink->paint(&painter, d->lastFrame);
        return;
    } else if (testAttribute(Qt::WA_OpaquePaintEvent)) {
        QPainter painter(this);
        painter.fillRect(event->rect(), Qt::black);
    }
}

#if defined(Q_OS_WIN)
bool QVideoWidget::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_D(QVideoWidget);
    Q_UNUSED(eventType);
    Q_UNUSED(result);

    MSG *mes = reinterpret_cast<MSG *>(message);
    if (mes->message == WM_PAINT || mes->message == WM_ERASEBKGND) {
//        if (d->windowBackend)
//            d->windowBackend->showEvent();
    }

    return false;
}
#endif

QT_END_NAMESPACE

#include "moc_qvideowidget.cpp"
#include "moc_qvideowidget_p.cpp"
