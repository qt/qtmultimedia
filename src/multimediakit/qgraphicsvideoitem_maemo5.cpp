/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qpointer.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qbasictimer.h>
#include <QtCore/qcoreevent.h>
#include <QtGui/qgraphicsscene.h>
#include <QtGui/qgraphicsview.h>
#include <QtGui/qscrollbar.h>
#include <QtGui/qx11info_x11.h>

#include "qgraphicsvideoitem.h"

#include <qmediaobject.h>
#include <qmediaservice.h>
#include <qpaintervideosurface_p.h>
#include <qvideorenderercontrol.h>

#include <qvideosurfaceformat.h>

#include "qxvideosurface_maemo5_p.h"


QT_BEGIN_NAMESPACE

//#define DEBUG_GFX_VIDEO_ITEM

//update overlay geometry slightly later,
//to ensure color key is alredy replaced with static frame
#define GEOMETRY_UPDATE_DELAY 20
//this is necessary to prevent flickering, see maemo bug 8798
//on geometry changes, the color key is replaced with static image frame
//until the overlay is re-initialized
#define SOFTWARE_RENDERING_DURATION 150

#ifdef  __ARM_NEON__

/*
* ARM NEON optimized implementation of UYVY -> RGB16 convertor
*/
static void uyvy422_to_rgb16_line_neon (uint8_t * dst, const uint8_t * src, int n)
{
     /* and this is the NEON code itself */
     static __attribute__ ((aligned (16))) uint16_t acc_r[8] = {
       22840, 22840, 22840, 22840, 22840, 22840, 22840, 22840,
     };
     static __attribute__ ((aligned (16))) uint16_t acc_g[8] = {
       17312, 17312, 17312, 17312, 17312, 17312, 17312, 17312,
     };
     static __attribute__ ((aligned (16))) uint16_t acc_b[8] = {
       28832, 28832, 28832, 28832, 28832, 28832, 28832, 28832,
     };
     /*
      * Registers:
      * q0, q1 : d0, d1, d2, d3  - are used for initial loading of YUV data
      * q2     : d4, d5          - are used for storing converted RGB data
      * q3     : d6, d7          - are used for temporary storage
      *
      * q6     : d12, d13        - are used for converting to RGB16
      * q7     : d14, d15        - are used for storing RGB16 data
      * q4-q5 - reserved
      *
      * q8, q9 : d16, d17, d18, d19  - are used for expanded Y data
      * q10    : d20, d21
      * q11    : d22, d23
      * q12    : d24, d25
      * q13    : d26, d27
      * q13, q14, q15            - various constants (#16, #149, #204, #50, #104, #154)
      */
     asm volatile (".macro convert_macroblock size\n"
         /* load up to 16 source pixels in UYVY format */
         ".if \\size == 16\n"
         "pld [%[src], #128]\n"
         "vld1.32 {d0, d1, d2, d3}, [%[src]]!\n"
         ".elseif \\size == 8\n"
         "vld1.32 {d0, d1}, [%[src]]!\n"
         ".elseif \\size == 4\n"
         "vld1.32 {d0}, [%[src]]!\n"
         ".elseif \\size == 2\n"
         "vld1.32 {d0[0]}, [%[src]]!\n"
         ".else\n" ".error \"unsupported macroblock size\"\n" ".endif\n"
         /* convert from 'packed' to 'planar' representation */
         "vuzp.8      d0, d1\n"    /* d1 - separated Y data (first 8 bytes) */
         "vuzp.8      d2, d3\n"    /* d3 - separated Y data (next 8 bytes) */
         "vuzp.8      d0, d2\n"    /* d0 - separated U data, d2 - separated V data */
         /* split even and odd Y color components */
         "vuzp.8      d1, d3\n"    /* d1 - evenY, d3 - oddY */
         /* clip upper and lower boundaries */
         "vqadd.u8    q0, q0, q4\n"
         "vqadd.u8    q1, q1, q4\n"
         "vqsub.u8    q0, q0, q5\n"
         "vqsub.u8    q1, q1, q5\n"
         "vshr.u8     d4, d2, #1\n"    /* d4 = V >> 1 */
         "vmull.u8    q8, d1, d27\n"       /* q8 = evenY * 149 */
         "vmull.u8    q9, d3, d27\n"       /* q9 = oddY * 149 */
         "vld1.16     {d20, d21}, [%[acc_r], :128]\n"      /* q10 - initialize accumulator for red */
         "vsubw.u8    q10, q10, d4\n"      /* red acc -= (V >> 1) */
         "vmlsl.u8    q10, d2, d28\n"      /* red acc -= V * 204 */
         "vld1.16     {d22, d23}, [%[acc_g], :128]\n"      /* q11 - initialize accumulator for green */
         "vmlsl.u8    q11, d2, d30\n"      /* green acc -= V * 104 */
         "vmlsl.u8    q11, d0, d29\n"      /* green acc -= U * 50 */
         "vld1.16     {d24, d25}, [%[acc_b], :128]\n"      /* q12 - initialize accumulator for blue */
         "vmlsl.u8    q12, d0, d30\n"      /* blue acc -= U * 104 */
         "vmlsl.u8    q12, d0, d31\n"      /* blue acc -= U * 154 */
         "vhsub.s16   q3, q8, q10\n"       /* calculate even red components */
         "vhsub.s16   q10, q9, q10\n"      /* calculate odd red components */
         "vqshrun.s16 d0, q3, #6\n"        /* right shift, narrow and saturate even red components */
         "vqshrun.s16 d3, q10, #6\n"       /* right shift, narrow and saturate odd red components */
         "vhadd.s16   q3, q8, q11\n"       /* calculate even green components */
         "vhadd.s16   q11, q9, q11\n"      /* calculate odd green components */
         "vqshrun.s16 d1, q3, #6\n"        /* right shift, narrow and saturate even green components */
         "vqshrun.s16 d4, q11, #6\n"       /* right shift, narrow and saturate odd green components */
         "vhsub.s16   q3, q8, q12\n"       /* calculate even blue components */
         "vhsub.s16   q12, q9, q12\n"      /* calculate odd blue components */
         "vqshrun.s16 d2, q3, #6\n"        /* right shift, narrow and saturate even blue components */
         "vqshrun.s16 d5, q12, #6\n"       /* right shift, narrow and saturate odd blue components */
         "vzip.8      d0, d3\n"    /* join even and odd red components */
         "vzip.8      d1, d4\n"    /* join even and odd green components */
         "vzip.8      d2, d5\n"    /* join even and odd blue components */
         "vshll.u8     q7, d0, #8\n" //red
         "vshll.u8     q6, d1, #8\n" //greed
         "vsri.u16   q7, q6, #5\n"
         "vshll.u8     q6, d2, #8\n" //blue
         "vsri.u16   q7, q6, #11\n" //now there is rgb16 in q7
         ".if \\size == 16\n"
         "vst1.16 {d14, d15}, [%[dst]]!\n"
         //"vst3.8  {d0, d1, d2}, [%[dst]]!\n"
         "vshll.u8     q7, d3, #8\n" //red
         "vshll.u8     q6, d4, #8\n" //greed
         "vsri.u16   q7, q6, #5\n"
         "vshll.u8     q6, d5, #8\n" //blue
         "vsri.u16   q7, q6, #11\n" //now there is rgb16 in q7
         //"vst3.8  {d3, d4, d5}, [%[dst]]!\n"
         "vst1.16 {d14, d15}, [%[dst]]!\n"
         ".elseif \\size == 8\n"
         "vst1.16 {d14, d15}, [%[dst]]!\n"
         //"vst3.8  {d0, d1, d2}, [%[dst]]!\n"
         ".elseif \\size == 4\n"
         "vst1.8 {d14}, [%[dst]]!\n"
         ".elseif \\size == 2\n"
         "vst1.8 {d14[0]}, [%[dst]]!\n"
         "vst1.8 {d14[1]}, [%[dst]]!\n"
         ".else\n"
         ".error \"unsupported macroblock size\"\n"
         ".endif\n"
         ".endm\n"
         "vmov.u8     d8, #15\n"  /* add this to U/V to saturate upper boundary */
         "vmov.u8     d9, #20\n"   /* add this to Y to saturate upper boundary */
         "vmov.u8     d10, #31\n"  /* sub this from U/V to saturate lower boundary */
         "vmov.u8     d11, #36\n"  /* sub this from Y to saturate lower boundary */
         "vmov.u8     d26, #16\n"
         "vmov.u8     d27, #149\n"
         "vmov.u8     d28, #204\n"
         "vmov.u8     d29, #50\n"
         "vmov.u8     d30, #104\n"
         "vmov.u8     d31, #154\n"
         "subs        %[n], %[n], #16\n"
         "blt         2f\n"
         "1:\n"
         "convert_macroblock 16\n"
         "subs        %[n], %[n], #16\n"
         "bge         1b\n"
         "2:\n"
         "tst         %[n], #8\n"
         "beq         3f\n"
         "convert_macroblock 8\n"
         "3:\n"
         "tst         %[n], #4\n"
         "beq         4f\n"
         "convert_macroblock 4\n"
         "4:\n"
         "tst         %[n], #2\n"
         "beq         5f\n"
         "convert_macroblock 2\n"
         "5:\n"
         ".purgem convert_macroblock\n":[src] "+&r" (src),[dst] "+&r" (dst),
         [n] "+&r" (n)
         :[acc_r] "r" (&acc_r[0]),[acc_g] "r" (&acc_g[0]),[acc_b] "r" (&acc_b[0])
         :"cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15",
         "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
         "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31");
}

#endif

class QGraphicsVideoItemPrivate
{
public:
    QGraphicsVideoItemPrivate()
        : q_ptr(0)
        , surface(0)
        , mediaObject(0)
        , service(0)
        , rendererControl(0)
        , savedViewportUpdateMode(QGraphicsView::FullViewportUpdate)
        , aspectRatioMode(Qt::KeepAspectRatio)
        , rect(0.0, 0.0, 320, 240)
        , softwareRenderingEnabled(false)
    {
    }

    QGraphicsVideoItem *q_ptr;

    QXVideoSurface *surface;
    QMediaObject *mediaObject;
    QMediaService *service;
    QVideoRendererControl *rendererControl;
    QPointer<QGraphicsView> currentView;
    QGraphicsView::ViewportUpdateMode savedViewportUpdateMode;

    Qt::AspectRatioMode aspectRatioMode;
    QRectF rect;
    QRectF boundingRect;
    QRectF sourceRect;
    QSizeF nativeSize;

    QPixmap lastFrame;
    QBasicTimer softwareRenderingTimer;
    QBasicTimer geometryUpdateTimer;
    bool softwareRenderingEnabled;
    QRect overlayRect;

    void clearService();
    void updateRects();
    void updateLastFrame();

    void _q_present();
    void _q_updateNativeSize();
    void _q_serviceDestroyed();
    void _q_mediaObjectDestroyed();
};

void QGraphicsVideoItemPrivate::clearService()
{
    if (rendererControl) {
        surface->stop();
        rendererControl->setSurface(0);
        service->releaseControl(rendererControl);
        rendererControl = 0;
    }

    if (service) {
        QObject::disconnect(service, SIGNAL(destroyed()), q_ptr, SLOT(_q_serviceDestroyed()));
        service = 0;
    }
}

void QGraphicsVideoItemPrivate::updateRects()
{
    q_ptr->prepareGeometryChange();

    if (nativeSize.isEmpty()) {
        boundingRect = QRectF();
    } else if (aspectRatioMode == Qt::IgnoreAspectRatio) {
        boundingRect = rect;
        sourceRect = QRectF(0, 0, 1, 1);
    } else if (aspectRatioMode == Qt::KeepAspectRatio) {
        QSizeF size = nativeSize;
        size.scale(rect.size(), Qt::KeepAspectRatio);

        boundingRect = QRectF(0, 0, size.width(), size.height());
        boundingRect.moveCenter(rect.center());

        sourceRect = QRectF(0, 0, 1, 1);
    } else if (aspectRatioMode == Qt::KeepAspectRatioByExpanding) {
        boundingRect = rect;

        QSizeF size = rect.size();
        size.scale(nativeSize, Qt::KeepAspectRatio);

        sourceRect = QRectF(
                0, 0, size.width() / nativeSize.width(), size.height() / nativeSize.height());
        sourceRect.moveCenter(QPointF(0.5, 0.5));
    }
}

void QGraphicsVideoItemPrivate::updateLastFrame()
{
    lastFrame = QPixmap();

    if (!softwareRenderingEnabled)
        return;

    QVideoFrame lastVideoFrame = surface->lastFrame();

    if (!lastVideoFrame.isValid())
        return;

    if (lastVideoFrame.map(QAbstractVideoBuffer::ReadOnly)) {

#ifdef  __ARM_NEON__
        if (lastVideoFrame.pixelFormat() == QVideoFrame::Format_UYVY) {
            QImage lastImage(lastVideoFrame.size(), QImage::Format_RGB16);

            const uchar *src = lastVideoFrame.bits();
            uchar *dst = lastImage.bits();
            const int srcLineStep = lastVideoFrame.bytesPerLine();
            const int dstLineStep = lastImage.bytesPerLine();
            const int h = lastVideoFrame.height();
            const int w = lastVideoFrame.width();

            for (int y=0; y<h; y++) {
                uyvy422_to_rgb16_line_neon(dst, src, w);
                src += srcLineStep;
                dst += dstLineStep;
            }
            lastFrame = QPixmap::fromImage(
                lastImage.scaled(boundingRect.size().toSize(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
        } else
#endif
        {
            QImage::Format imgFormat = QVideoFrame::imageFormatFromPixelFormat(lastVideoFrame.pixelFormat());

            if (imgFormat != QImage::Format_Invalid) {
                QImage lastImage(lastVideoFrame.bits(),
                                 lastVideoFrame.width(),
                                 lastVideoFrame.height(),
                                 lastVideoFrame.bytesPerLine(),
                                 imgFormat);

                lastFrame = QPixmap::fromImage(
                        lastImage.scaled(boundingRect.size().toSize(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
            }
        }

        lastVideoFrame.unmap();
    }

}

void QGraphicsVideoItemPrivate::_q_present()
{
    q_ptr->update(boundingRect);
}

void QGraphicsVideoItemPrivate::_q_updateNativeSize()
{
    const QSize &size = surface->surfaceFormat().sizeHint();
    if (nativeSize != size) {
        lastFrame = QPixmap();
        nativeSize = size;

        updateRects();
        emit q_ptr->nativeSizeChanged(nativeSize);
    }
}

void QGraphicsVideoItemPrivate::_q_serviceDestroyed()
{
    rendererControl = 0;
    service = 0;

    surface->stop();
}

void QGraphicsVideoItemPrivate::_q_mediaObjectDestroyed()
{
    mediaObject = 0;

    clearService();
}

QGraphicsVideoItem::QGraphicsVideoItem(QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , d_ptr(new QGraphicsVideoItemPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->surface = new QXVideoSurface;

    setCacheMode(NoCache);
    setFlag(QGraphicsItem::ItemIgnoresParentOpacity);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

    connect(d_ptr->surface, SIGNAL(surfaceFormatChanged(QVideoSurfaceFormat)),
            this, SLOT(_q_updateNativeSize()));

    connect(d_ptr->surface, SIGNAL(activeChanged(bool)), this, SLOT(_q_present()));
}

QGraphicsVideoItem::~QGraphicsVideoItem()
{
    if (d_ptr->rendererControl) {
        d_ptr->rendererControl->setSurface(0);
        d_ptr->service->releaseControl(d_ptr->rendererControl);
    }

    if (d_ptr->currentView)
        d_ptr->currentView->setViewportUpdateMode(d_ptr->savedViewportUpdateMode);

    delete d_ptr->surface;
    delete d_ptr;
}

QMediaObject *QGraphicsVideoItem::mediaObject() const
{
    return d_func()->mediaObject;
}

bool QGraphicsVideoItem::setMediaObject(QMediaObject *object)
{
    Q_D(QGraphicsVideoItem);

    if (object == d->mediaObject)
        return true;

    d->clearService();

    d->mediaObject = object;

    if (d->mediaObject) {
        d->service = d->mediaObject->service();

        if (d->service) {
            d->rendererControl = qobject_cast<QVideoRendererControl *>(
                    d->service->requestControl(QVideoRendererControl_iid));

            if (d->rendererControl != 0) {
                connect(d->service, SIGNAL(destroyed()), this, SLOT(_q_serviceDestroyed()));
                d->rendererControl->setSurface(d->surface);
                return true;
            }

        }
    }

    return false;
}

Qt::AspectRatioMode QGraphicsVideoItem::aspectRatioMode() const
{
    return d_func()->aspectRatioMode;
}

void QGraphicsVideoItem::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    Q_D(QGraphicsVideoItem);

    d->aspectRatioMode = mode;
    d->updateRects();
}

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

QSizeF QGraphicsVideoItem::nativeSize() const
{
    return d_func()->nativeSize;
}

QRectF QGraphicsVideoItem::boundingRect() const
{
    return d_func()->boundingRect;
}

void QGraphicsVideoItem::paint(
        QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
#ifdef DEBUG_GFX_VIDEO_ITEM
    qDebug() << "QGraphicsVideoItem::paint";
#endif

    Q_UNUSED(option);
    Q_D(QGraphicsVideoItem);

    QGraphicsView *view = 0;
    if (scene() && !scene()->views().isEmpty())
        view = scene()->views().first();

    //it's necessary to switch vieport update mode to FullViewportUpdate
    //otherwise the video item area can be just scrolled without notifying overlay
    //about geometry changes
    if (view != d->currentView) {
        if (d->currentView) {
            d->currentView->setViewportUpdateMode(d->savedViewportUpdateMode);
        }

        d->currentView = view;
        if (view) {
            d->savedViewportUpdateMode = view->viewportUpdateMode();
            view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        }
    }

    QColor colorKey = Qt::black;
    bool geometryChanged = false;

    if (d->surface) {
        if (widget)
            d->surface->setWinId(widget->winId());

        QTransform transform = painter->combinedTransform();
        QRect overlayRect = transform.mapRect(boundingRect()).toRect();
        QRect currentSurfaceRect = d->surface->displayRect();

        if (widget) {
            //workaround for xvideo issue with U/V planes swapped
            QPoint topLeft = widget->mapToGlobal(overlayRect.topLeft());
            if ((topLeft.x() & 1) == 0 && topLeft.x() != 0)
                overlayRect.moveLeft(overlayRect.left()-1);
        }

        d->overlayRect = overlayRect;

        if (currentSurfaceRect != overlayRect) {
            if (!d->surface->displayRect().isEmpty()) {
                if (d->softwareRenderingEnabled) {
                    //recalculate scaled frame pixmap if area is resized
                    if (currentSurfaceRect.size() != overlayRect.size()) {
                        d->updateLastFrame();
                        d->surface->setDisplayRect( overlayRect );
                    }
                } else {
                    d->softwareRenderingEnabled = true;
                    d->updateLastFrame();

                    //don't set new geometry right now,
                    //but with small delay, to ensure the frame is already
                    //rendered on top of color key
                    if (!d->geometryUpdateTimer.isActive())
                        d->geometryUpdateTimer.start(GEOMETRY_UPDATE_DELAY, this);
                }
            } else
                d->surface->setDisplayRect( overlayRect );

            geometryChanged = true;
            d->softwareRenderingTimer.start(SOFTWARE_RENDERING_DURATION, this);

#ifdef DEBUG_GFX_VIDEO_ITEM
            qDebug() << "set video display rect:" << overlayRect;
#endif

        }

        colorKey = d->surface->colorKey();
    }


    if (!d->softwareRenderingEnabled) {
        painter->fillRect(d->boundingRect, colorKey);
    } else {
        if (!d->lastFrame.isNull()) {
            painter->drawPixmap(d->boundingRect.topLeft(), d->lastFrame );

        } else
            painter->fillRect(d->boundingRect, Qt::black);
    }
}

QVariant QGraphicsVideoItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    Q_D(QGraphicsVideoItem);

    if (change == ItemScenePositionHasChanged) {
        update(boundingRect());
    } else {
        return QGraphicsItem::itemChange(change, value);
    }

    return value;
}

void QGraphicsVideoItem::timerEvent(QTimerEvent *event)
{
    Q_D(QGraphicsVideoItem);

    if (event->timerId() == d->softwareRenderingTimer.timerId() && d->softwareRenderingEnabled) {
        d->softwareRenderingTimer.stop();
        d->softwareRenderingEnabled = false;
        d->updateLastFrame();
        // repaint last frame, to ensure geometry change is applyed in paused state
        d->surface->repaintLastFrame();
        d->_q_present();
    } else if ((event->timerId() == d->geometryUpdateTimer.timerId())) {
        d->geometryUpdateTimer.stop();
        //slightly delayed geometry update,
        //to avoid flicker at the first geometry change
        d->surface->setDisplayRect( d->overlayRect );
    }

    QGraphicsObject::timerEvent(event);
}

#include "moc_qgraphicsvideoitem.cpp"
QT_END_NAMESPACE
