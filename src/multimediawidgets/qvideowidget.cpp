/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
    d->wasFullScreen = fullScreen;
}

/*!
    \fn QVideoWidget::fullScreenChanged(bool fullScreen)

    Signals that the \a fullScreen mode of a video widget has changed.

    \sa isFullScreen()
*/

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
        if (windowState() & Qt::WindowFullScreen) {
            if (!d->wasFullScreen)
                emit fullScreenChanged(d->wasFullScreen = true);
        } else {
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
