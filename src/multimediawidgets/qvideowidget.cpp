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
#include "qpaintervideosurface_p.h"

#include <qmediasource.h>
#include <qmediaservice.h>
#include <qvideowindowcontrol.h>

#include <qvideorenderercontrol.h>
#include <qvideosurfaceformat.h>
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

QRendererVideoWidgetBackend::QRendererVideoWidgetBackend(QWidget *widget)
    : m_widget(widget)
    , m_surface(new QPainterVideoSurface)
    , m_aspectRatioMode(Qt::KeepAspectRatio)
    , m_updatePaintDevice(true)
{
    connect(this, SIGNAL(brightnessChanged(int)), m_widget, SLOT(_q_brightnessChanged(int)));
    connect(this, SIGNAL(contrastChanged(int)), m_widget, SLOT(_q_contrastChanged(int)));
    connect(this, SIGNAL(hueChanged(int)), m_widget, SLOT(_q_hueChanged(int)));
    connect(this, SIGNAL(saturationChanged(int)), m_widget, SLOT(_q_saturationChanged(int)));
    connect(m_surface, SIGNAL(frameChanged()), this, SLOT(frameChanged()));
    connect(m_surface, SIGNAL(surfaceFormatChanged(QVideoSurfaceFormat)),
            this, SLOT(formatChanged(QVideoSurfaceFormat)));
}

QRendererVideoWidgetBackend::~QRendererVideoWidgetBackend()
{
    delete m_surface;
}

QAbstractVideoSurface *QRendererVideoWidgetBackend::videoSurface() const
{
    return m_surface;
}

void QRendererVideoWidgetBackend::setBrightness(int brightness)
{
    m_surface->setBrightness(brightness);

    emit brightnessChanged(brightness);
}

void QRendererVideoWidgetBackend::setContrast(int contrast)
{
    m_surface->setContrast(contrast);

    emit contrastChanged(contrast);
}

void QRendererVideoWidgetBackend::setHue(int hue)
{
    m_surface->setHue(hue);

    emit hueChanged(hue);
}

void QRendererVideoWidgetBackend::setSaturation(int saturation)
{
    m_surface->setSaturation(saturation);

    emit saturationChanged(saturation);
}

Qt::AspectRatioMode QRendererVideoWidgetBackend::aspectRatioMode() const
{
    return m_aspectRatioMode;
}

void QRendererVideoWidgetBackend::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_aspectRatioMode = mode;

    m_widget->updateGeometry();
}

void QRendererVideoWidgetBackend::setFullScreen(bool)
{
}

QSize QRendererVideoWidgetBackend::sizeHint() const
{
    return m_surface->surfaceFormat().sizeHint();
}

void QRendererVideoWidgetBackend::showEvent()
{
}

void QRendererVideoWidgetBackend::hideEvent(QHideEvent *)
{
#if QT_CONFIG(opengl)
    m_updatePaintDevice = true;
#endif
}

void QRendererVideoWidgetBackend::resizeEvent(QResizeEvent *)
{
    updateRects();
}

void QRendererVideoWidgetBackend::moveEvent(QMoveEvent *)
{
}

void QRendererVideoWidgetBackend::paintEvent(QPaintEvent *event)
{
    QPainter painter(m_widget);

    if (m_widget->testAttribute(Qt::WA_OpaquePaintEvent)) {
        QRegion borderRegion = event->region();
        borderRegion = borderRegion.subtracted(m_boundingRect);

        QBrush brush = m_widget->palette().window();

        for (const QRect &r : borderRegion)
            painter.fillRect(r, brush);
    }

    if (m_surface->isActive() && m_boundingRect.intersects(event->rect())) {
        m_surface->paint(&painter, m_boundingRect, m_sourceRect);

        m_surface->setReady(true);
    } else {
#if QT_CONFIG(opengl)
        if (m_updatePaintDevice && (painter.paintEngine()->type() == QPaintEngine::OpenGL
                || painter.paintEngine()->type() == QPaintEngine::OpenGL2)) {
            m_updatePaintDevice = false;

            m_surface->updateGLContext();
            if (m_surface->supportedShaderTypes() & QPainterVideoSurface::GlslShader) {
                m_surface->setShaderType(QPainterVideoSurface::GlslShader);
            } else {
                m_surface->setShaderType(QPainterVideoSurface::FragmentProgramShader);
            }
        }
#endif
    }

}

void QRendererVideoWidgetBackend::formatChanged(const QVideoSurfaceFormat &format)
{
    m_nativeSize = format.sizeHint();

    updateRects();

    m_widget->updateGeometry();
    m_widget->update();
}

void QRendererVideoWidgetBackend::frameChanged()
{
    m_widget->update(m_boundingRect);
}

void QRendererVideoWidgetBackend::updateRects()
{
    QRect rect = m_widget->rect();

    if (m_nativeSize.isEmpty()) {
        m_boundingRect = QRect();
    } else if (m_aspectRatioMode == Qt::IgnoreAspectRatio) {
        m_boundingRect = rect;
        m_sourceRect = QRectF(0, 0, 1, 1);
    } else if (m_aspectRatioMode == Qt::KeepAspectRatio) {
        QSize size = m_nativeSize;
        size.scale(rect.size(), Qt::KeepAspectRatio);

        m_boundingRect = QRect(0, 0, size.width(), size.height());
        m_boundingRect.moveCenter(rect.center());

        m_sourceRect = QRectF(0, 0, 1, 1);
    } else if (m_aspectRatioMode == Qt::KeepAspectRatioByExpanding) {
        m_boundingRect = rect;

        QSizeF size = rect.size();
        size.scale(m_nativeSize, Qt::KeepAspectRatio);

        m_sourceRect = QRectF(
                0, 0, size.width() / m_nativeSize.width(), size.height() / m_nativeSize.height());
        m_sourceRect.moveCenter(QPointF(0.5, 0.5));
    }
}

bool QVideoWidgetPrivate::createBackend()
{
    backend = new QRendererVideoWidgetBackend(q_func());
    backend->setBrightness(brightness);
    backend->setContrast(contrast);
    backend->setHue(hue);
    backend->setSaturation(saturation);
    backend->setAspectRatioMode(aspectRatioMode);

    return true;
}

void QVideoWidgetPrivate::_q_brightnessChanged(int b)
{
    if (b != brightness)
        emit q_func()->brightnessChanged(brightness = b);
}

void QVideoWidgetPrivate::_q_contrastChanged(int c)
{
    if (c != contrast)
        emit q_func()->contrastChanged(contrast = c);
}

void QVideoWidgetPrivate::_q_hueChanged(int h)
{
    if (h != hue)
        emit q_func()->hueChanged(hue = h);
}

void QVideoWidgetPrivate::_q_saturationChanged(int s)
{
    if (s != saturation)
        emit q_func()->saturationChanged(saturation = s);
}


void QVideoWidgetPrivate::_q_fullScreenChanged(bool fullScreen)
{
    if (!fullScreen && q_func()->isFullScreen())
        q_func()->showNormal();
}

void QVideoWidgetPrivate::_q_dimensionsChanged()
{
    q_func()->updateGeometry();
    q_func()->update();
}

/*!
    \class QVideoWidget


    \brief The QVideoWidget class provides a widget which presents video
    produced by a media object.
    \ingroup multimedia
    \inmodule QtMultimediaWidgets

    Attaching a QVideoWidget to a QMediaSource allows it to display the
    video or image output of that media object.  A QVideoWidget is attached
    to media object by passing a pointer to the QMediaSource in its
    constructor, and detached by destroying the QVideoWidget.

    \snippet multimedia-snippets/video.cpp Video widget

    \b {Note}: Only a single display output can be attached to a media
    object at one time.

    \sa QMediaSource, QMediaPlayer, QGraphicsVideoItem
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
}

/*!
  \internal
*/
QVideoWidget::QVideoWidget(QVideoWidgetPrivate &dd, QWidget *parent)
    : QWidget(parent, {})
    , d_ptr(&dd)
{
    d_ptr->q_ptr = this;

    QPalette palette = QWidget::palette();
    palette.setColor(QPalette::Window, Qt::black);
    setPalette(palette);
}

/*!
    Destroys a video widget.
*/
QVideoWidget::~QVideoWidget()
{
    delete d_ptr;
}

/*!
    \since 5.15
    \property QVideoWidget::videoSurface
    \brief Returns the underlaying video surface that can render video frames
    to the current widget.
    This property is never \c nullptr.
    Example of how to render video frames to QVideoWidget:
    \snippet multimedia-snippets/video.cpp Widget Surface
    \sa QMediaPlayer::setVideoOutput
*/

QAbstractVideoSurface *QVideoWidget::videoSurface() const
{
    auto d = const_cast<QVideoWidgetPrivate *>(d_func());

    if (!d->backend)
        d->createBackend();

    return d->backend->videoSurface();
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

    if (d->backend) {
        d->backend->setAspectRatioMode(mode);
        d->aspectRatioMode = d->backend->aspectRatioMode();
    } else {
        d->aspectRatioMode = mode;
    }
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
}

/*!
    \fn QVideoWidget::fullScreenChanged(bool fullScreen)

    Signals that the \a fullScreen mode of a video widget has changed.

    \sa isFullScreen()
*/

/*!
    \property QVideoWidget::brightness
    \brief an adjustment to the brightness of displayed video.

    Valid brightness values range between -100 and 100, the default is 0.
*/

int QVideoWidget::brightness() const
{
    return d_func()->brightness;
}

void QVideoWidget::setBrightness(int brightness)
{
    Q_D(QVideoWidget);

    int boundedBrightness = qBound(-100, brightness, 100);

    if (d->backend)
        d->backend->setBrightness(boundedBrightness);
    else if (d->brightness != boundedBrightness)
        emit brightnessChanged(d->brightness = boundedBrightness);
}

/*!
    \fn QVideoWidget::brightnessChanged(int brightness)

    Signals that a video widgets's \a brightness adjustment has changed.

    \sa brightness()
*/

/*!
    \property QVideoWidget::contrast
    \brief an adjustment to the contrast of displayed video.

    Valid contrast values range between -100 and 100, the default is 0.

*/

int QVideoWidget::contrast() const
{
    return d_func()->contrast;
}

void QVideoWidget::setContrast(int contrast)
{
    Q_D(QVideoWidget);

    int boundedContrast = qBound(-100, contrast, 100);

    if (d->backend)
        d->backend->setContrast(boundedContrast);
    else if (d->contrast != boundedContrast)
        emit contrastChanged(d->contrast = boundedContrast);
}

/*!
    \fn QVideoWidget::contrastChanged(int contrast)

    Signals that a video widgets's \a contrast adjustment has changed.

    \sa contrast()
*/

/*!
    \property QVideoWidget::hue
    \brief an adjustment to the hue of displayed video.

    Valid hue values range between -100 and 100, the default is 0.
*/

int QVideoWidget::hue() const
{
    return d_func()->hue;
}

void QVideoWidget::setHue(int hue)
{
    Q_D(QVideoWidget);

    int boundedHue = qBound(-100, hue, 100);

    if (d->backend)
        d->backend->setHue(boundedHue);
    else if (d->hue != boundedHue)
        emit hueChanged(d->hue = boundedHue);
}

/*!
    \fn QVideoWidget::hueChanged(int hue)

    Signals that a video widgets's \a hue has changed.

    \sa hue()
*/

/*!
    \property QVideoWidget::saturation
    \brief an adjustment to the saturation of displayed video.

    Valid saturation values range between -100 and 100, the default is 0.
*/

int QVideoWidget::saturation() const
{
    return d_func()->saturation;
}

void QVideoWidget::setSaturation(int saturation)
{
    Q_D(QVideoWidget);

    int boundedSaturation = qBound(-100, saturation, 100);

    if (d->backend)
        d->backend->setSaturation(boundedSaturation);
    else if (d->saturation != boundedSaturation)
        emit saturationChanged(d->saturation = boundedSaturation);
}

/*!
    \fn QVideoWidget::saturationChanged(int saturation)

    Signals that a video widgets's \a saturation has changed.

    \sa saturation()
*/

/*!
  Returns the size hint for the current back end,
  if there is one, or else the size hint from QWidget.
 */
QSize QVideoWidget::sizeHint() const
{
    Q_D(const QVideoWidget);

    if (d->backend)
        return d->backend->sizeHint();

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
            if (d->backend)
                d->backend->setFullScreen(true);

            if (!d->wasFullScreen)
                emit fullScreenChanged(d->wasFullScreen = true);
        } else {
            if (d->backend)
                d->backend->setFullScreen(false);

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
    Q_D(QVideoWidget);

    QWidget::showEvent(event);

    if (d->backend)
        d->backend->showEvent();
}

/*!
  \reimp
  Handles the hide \a event.
*/
void QVideoWidget::hideEvent(QHideEvent *event)
{
    Q_D(QVideoWidget);

    if (d->backend)
        d->backend->hideEvent(event);

    QWidget::hideEvent(event);
}

/*!
  \reimp
  Handles the resize \a event.
 */
void QVideoWidget::resizeEvent(QResizeEvent *event)
{
    Q_D(QVideoWidget);

    QWidget::resizeEvent(event);

    if (d->backend)
        d->backend->resizeEvent(event);
}

/*!
  \reimp
  Handles the move \a event.
 */
void QVideoWidget::moveEvent(QMoveEvent *event)
{
    Q_D(QVideoWidget);

    if (d->backend)
        d->backend->moveEvent(event);
}

/*!
  \reimp
  Handles the paint \a event.
 */
void QVideoWidget::paintEvent(QPaintEvent *event)
{
    Q_D(QVideoWidget);

    if (d->backend) {
        d->backend->paintEvent(event);
    } else if (testAttribute(Qt::WA_OpaquePaintEvent)) {
        QPainter painter(this);

        painter.fillRect(event->rect(), palette().window());
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
        if (d->windowBackend)
            d->windowBackend->showEvent();
    }

    return false;
}
#endif

QT_END_NAMESPACE

#include "moc_qvideowidget.cpp"
#include "moc_qvideowidget_p.cpp"
