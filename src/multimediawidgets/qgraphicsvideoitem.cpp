// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    \fn int QGraphicsVideoItem::type() const
    \reimp

    Returns an int representing the type of the video item.
*/
/*!
    \variable QGraphicsVideoItem::d_ptr
    \internal
*/
/*!
    \enum QGraphicsVideoItem::anonymous
    \internal

    \omitvalue Type
*/
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


