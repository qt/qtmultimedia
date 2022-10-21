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

#include "qgraphicsvideoitem.h"
#include "qvideosink.h"

#include <qobject.h>
#include <qvideoframe.h>
#include <qvideoframeformat.h>

#include <QtCore/qcoreevent.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QGraphicsVideoItemPrivate
{
public:
    QGraphicsVideoItemPrivate()
        : rect(0.0, 0.0, 320, 240)
    {
    }

    QGraphicsVideoItem *q_ptr = nullptr;

    QVideoSink *sink = nullptr;
    QRectF rect;
    QRectF boundingRect;
    QSizeF nativeSize;
    QVideoFrame m_frame;
    Qt::AspectRatioMode m_aspectRatioMode = Qt::KeepAspectRatio;

    void updateRects();

    void _q_present(const QVideoFrame &);
};

void QGraphicsVideoItemPrivate::updateRects()
{
    q_ptr->prepareGeometryChange();

    boundingRect = rect;
    if (nativeSize.isEmpty())
        return;

    if (m_aspectRatioMode == Qt::KeepAspectRatio) {
        QSizeF size = nativeSize;
        size.scale(rect.size(), Qt::KeepAspectRatio);

        boundingRect = QRectF(0, 0, size.width(), size.height());
        boundingRect.moveCenter(rect.center());
    }
}

void QGraphicsVideoItemPrivate::_q_present(const QVideoFrame &frame)
{
    m_frame = frame;
    q_ptr->update(boundingRect);

    if (frame.isValid()) {
        const QSize &size = frame.surfaceFormat().viewport().size();
        if (nativeSize != size) {
            nativeSize = size;

            updateRects();
            emit q_ptr->nativeSizeChanged(nativeSize);
        }
    }
}

/*!
    \class QGraphicsVideoItem

    \brief The QGraphicsVideoItem class provides a graphics item which display video produced by a QMediaPlayer or QCamera.

    \inmodule QtMultimediaWidgets
    \ingroup multimedia

    Attaching a QGraphicsVideoItem to a QMediaPlayer or QCamera allows it to display
    the video or image output of that media object.

    \snippet multimedia-snippets/video.cpp Video graphics item

    \b {Note}: Only a single display output can be attached to a media
    object at one time.

    \sa QMediaPlayer, QVideoWidget, QCamera
*/

/*!
    Constructs a graphics item that displays video.

    The \a parent is passed to QGraphicsItem.
*/
QGraphicsVideoItem::QGraphicsVideoItem(QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , d_ptr(new QGraphicsVideoItemPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->sink = new QVideoSink(this);

    connect(d_ptr->sink, SIGNAL(videoFrameChanged(const QVideoFrame &)), this, SLOT(_q_present(const QVideoFrame &)));
}

/*!
    Destroys a video graphics item.
*/
QGraphicsVideoItem::~QGraphicsVideoItem()
{
    delete d_ptr;
}

/*!
    \since 6.0
    \property QGraphicsVideoItem::videoSink
    \brief Returns the underlying video sink that can render video frames
    to the current item.
    This property is never \c nullptr.
    Example of how to render video frames to QGraphicsVideoItem:
    \snippet multimedia-snippets/video.cpp GraphicsVideoItem Surface
    \sa QMediaPlayer::setVideoOutput
*/

QVideoSink *QGraphicsVideoItem::videoSink() const
{
    return d_func()->sink;
}

/*!
    \property QGraphicsVideoItem::aspectRatioMode
    \brief how a video is scaled to fit the graphics item's size.
*/

Qt::AspectRatioMode QGraphicsVideoItem::aspectRatioMode() const
{
    return d_func()->m_aspectRatioMode;
}

void QGraphicsVideoItem::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    Q_D(QGraphicsVideoItem);
    if (d->m_aspectRatioMode == mode)
        return;

    d->m_aspectRatioMode = mode;
    d->updateRects();
}

/*!
    \property QGraphicsVideoItem::offset
    \brief the video item's offset.

    QGraphicsVideoItem will draw video using the offset for its top left
    corner.
*/

QPointF QGraphicsVideoItem::offset() const
{
    return d_func()->rect.topLeft();
}

void QGraphicsVideoItem::setOffset(const QPointF &offset)
{
    Q_D(QGraphicsVideoItem);

    d->rect.moveTo(offset);
    d->updateRects();
}

/*!
    \property QGraphicsVideoItem::size
    \brief the video item's size.

    QGraphicsVideoItem will draw video scaled to fit size according to its
    fillMode.
*/

QSizeF QGraphicsVideoItem::size() const
{
    return d_func()->rect.size();
}

void QGraphicsVideoItem::setSize(const QSizeF &size)
{
    Q_D(QGraphicsVideoItem);

    d->rect.setSize(size.isValid() ? size : QSizeF(0, 0));
    d->updateRects();
}

/*!
    \property QGraphicsVideoItem::nativeSize
    \brief the native size of the video.
*/

QSizeF QGraphicsVideoItem::nativeSize() const
{
    return d_func()->nativeSize;
}

/*!
    \fn QGraphicsVideoItem::nativeSizeChanged(const QSizeF &size)

    Signals that the native \a size of the video has changed.
*/

/*!
    \reimp
*/
QRectF QGraphicsVideoItem::boundingRect() const
{
    return d_func()->boundingRect;
}

/*!
    \reimp
*/
void QGraphicsVideoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_D(QGraphicsVideoItem);

    Q_UNUSED(option);
    Q_UNUSED(widget);

    d->m_frame.paint(painter, d->rect, { Qt::transparent, d->m_aspectRatioMode });
}

/*!
    \reimp

    \internal
*/
QVariant QGraphicsVideoItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGraphicsItem::itemChange(change, value);
}

/*!
  \internal
*/
void QGraphicsVideoItem::timerEvent(QTimerEvent *event)
{
    QGraphicsObject::timerEvent(event);
}

QT_END_NAMESPACE

#include "moc_qgraphicsvideoitem.cpp"
