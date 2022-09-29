// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qtmultimediaglobal_p.h>
#include "qvideowidget_p.h"

#include <QtCore/qobject.h>
#include <QtMultimedia/qtmultimediaglobal.h>
#include <qvideosink.h>
#include <private/qvideowindow_p.h>

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

/*!
    \class QVideoWidget


    \brief The QVideoWidget class provides a widget which presents video
    produced by a media object.
    \ingroup multimedia
    \ingroup multimedia_video
    \inmodule QtMultimediaWidgets

    Attaching a QVideoWidget to a QMediaPlayer or QCamera allows it to display the
    video or image output of that object.

    \snippet multimedia-snippets/video.cpp Video widget

    \b {Note}: Only a single display output can be attached to a media
    object at one time.

    \sa QCamera, QMediaPlayer, QGraphicsVideoItem
*/
/*!
    \variable QVideoWidget::d_ptr
    \internal
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
    d_ptr->videoWindow = new QVideoWindow;
    d_ptr->videoWindow->setFlag(Qt::WindowTransparentForInput, true);
    d_ptr->windowContainer = QWidget::createWindowContainer(d_ptr->videoWindow, this, Qt::WindowTransparentForInput);
    d_ptr->windowContainer->move(0, 0);
    d_ptr->windowContainer->resize(size());

    connect(d_ptr->videoWindow, &QVideoWindow::aspectRatioModeChanged, this, &QVideoWidget::aspectRatioModeChanged);
}

/*!
    Destroys a video widget.
*/
QVideoWidget::~QVideoWidget()
{
    delete d_ptr->videoWindow;
    delete d_ptr;
}

/*!
    Returns the QVideoSink instance.
*/
QVideoSink *QVideoWidget::videoSink() const
{
    return d_ptr->videoWindow->videoSink();
}

/*!
    \property QVideoWidget::aspectRatioMode
    \brief how video is scaled with respect to its aspect ratio.
*/

Qt::AspectRatioMode QVideoWidget::aspectRatioMode() const
{
    return d_ptr->videoWindow->aspectRatioMode();
}

void QVideoWidget::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    d_ptr->videoWindow->setAspectRatioMode(mode);
}

/*!
    \property QVideoWidget::fullScreen
    \brief whether video display is confined to a window or is fullScreen.
*/

void QVideoWidget::setFullScreen(bool fullScreen)
{
    Q_D(QVideoWidget);
    if (isFullScreen() == fullScreen)
        return;

    Qt::WindowFlags flags = windowFlags();

    if (fullScreen) {
        // get the position of the widget in global coordinates
        QPoint position = mapToGlobal(QPoint(0,0));
        d->nonFullScreenFlags = flags & (Qt::Window | Qt::SubWindow);
        d_ptr->nonFullscreenPos = pos();
        flags |= Qt::Window;
        flags &= ~Qt::SubWindow;
        setWindowFlags(flags);
        // move the widget to the position it had on screen, so that showFullScreen() will
        // place it on the correct screen
        move(position);

        showFullScreen();
    } else {
        flags &= ~(Qt::Window | Qt::SubWindow); //clear the flags...
        flags |= d->nonFullScreenFlags; //then we reset the flags (window and subwindow)
        setWindowFlags(flags);

        showNormal();
        move(d_ptr->nonFullscreenPos);
        d_ptr->nonFullscreenPos = {};
    }
}

/*!
  Returns the size hint for the current back end,
  if there is one, or else the size hint from QWidget.
 */
QSize QVideoWidget::sizeHint() const
{
    auto size = videoSink()->videoSize();
    if (size.isValid())
        return size;

    return QWidget::sizeHint();
}

/*!
  \reimp
  Current event \a event.
  Returns the value of the base class QWidget::event(QEvent *event) function.
*/
bool QVideoWidget::event(QEvent *event)
{
    Q_D(QVideoWidget);

    if (event->type() == QEvent::WindowStateChange) {
        bool fullScreen = bool(windowState() & Qt::WindowFullScreen);
        if (fullScreen != d->wasFullScreen) {
            emit fullScreenChanged(fullScreen);
            d->wasFullScreen = fullScreen;
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
    QWidget::hideEvent(event);
}

/*!
  \reimp
  Handles the resize \a event.
 */
void QVideoWidget::resizeEvent(QResizeEvent *event)
{
    d_ptr->windowContainer->resize(event->size());
    QWidget::resizeEvent(event);
}

/*!
  \reimp
  Handles the move \a event.
 */
void QVideoWidget::moveEvent(QMoveEvent * /*event*/)
{
}

QT_END_NAMESPACE

#include "moc_qvideowidget.cpp"
