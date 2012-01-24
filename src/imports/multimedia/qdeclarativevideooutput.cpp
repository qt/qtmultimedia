/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qdeclarativevideooutput_p.h"

#include "qsgvideonode_p.h"
#include "qsgvideonode_i420.h"
#include "qsgvideonode_rgb.h"

#include <QtQuick/QQuickItem>

#include <QtMultimedia/QAbstractVideoSurface>
#include <QtMultimedia/qmediaservice.h>
#include <QtMultimedia/qvideorenderercontrol.h>
#include <QtMultimedia/qvideosurfaceformat.h>


#include <QtCore/qmetaobject.h>

//#define DEBUG_VIDEOITEM
Q_DECLARE_METATYPE(QAbstractVideoSurface*)

QT_BEGIN_NAMESPACE

class QSGVideoItemSurface : public QAbstractVideoSurface
{
public:
    QSGVideoItemSurface(QDeclarativeVideoOutput *item, QObject *parent = 0) :
        QAbstractVideoSurface(parent),
        m_item(item)
    {
    }

    ~QSGVideoItemSurface()
    {
    }

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
    {
        QList<QVideoFrame::PixelFormat> formats;

        foreach (QSGVideoNodeFactory* factory, m_item->m_videoNodeFactories) {
            formats.append(factory->supportedPixelFormats(handleType));
        }

        return formats;
    }

    bool start(const QVideoSurfaceFormat &format)
    {
#ifdef DEBUG_VIDEOITEM
        qDebug() << Q_FUNC_INFO << format;
#endif

        if (!supportedPixelFormats(format.handleType()).contains(format.pixelFormat()))
            return false;

        return QAbstractVideoSurface::start(format);
    }

    void stop()
    {
        m_item->stop();
        QAbstractVideoSurface::stop();
    }

    virtual bool present(const QVideoFrame &frame)
    {
        if (!frame.isValid()) {
            qWarning() << Q_FUNC_INFO << "I'm getting bad frames here...";
            return false;
        }
        m_item->present(frame);
        return true;
    }

private:
    QDeclarativeVideoOutput *m_item;
};

/*!
    \qmlclass VideoOutput QDeclarativeVideoOutput
    \brief The VideoOutput element allows you to render video or camera viewfinder.

    \ingroup multimedia_qml

    This element is part of the \bold{QtMultimedia 5.0} module.

    \qml
    import QtQuick 2.0
    import QtMultimedia 5.0

    Rectangle {
        width: 800
        height: 600
        color: "black"

        MediaPlayer {
            id: player
            source: "file://video.webm"
            playing: true
        }

        VideoOutput {
            id: videoOutput
            source: player
            anchors.fill: parent
        }
    }

    \endqml

    The VideoOutput item supports untransformed, stretched, and uniformly scaled video presentation.
    For a description of stretched uniformly scaled presentation, see the \l fillMode property
    description.

    \sa MediaPlayer, Camera
*/

/*!
    \internal
    \class QDeclarativeVideoOutput
    \brief The QDeclarativeVideoOutput class provides a video output item.
*/

QDeclarativeVideoOutput::QDeclarativeVideoOutput(QQuickItem *parent) :
    QQuickItem(parent),
    m_sourceType(NoSource),
    m_fillMode(PreserveAspectFit),
    m_geometryDirty(true),
    m_orientation(0)
{
    setFlag(ItemHasContents, true);
    m_surface = new QSGVideoItemSurface(this);
    connect(m_surface, SIGNAL(surfaceFormatChanged(QVideoSurfaceFormat)),
            this, SLOT(_q_updateNativeSize(QVideoSurfaceFormat)), Qt::QueuedConnection);

    m_videoNodeFactories.append(new QSGVideoNodeFactory_I420);
    m_videoNodeFactories.append(new QSGVideoNodeFactory_RGB);
}

QDeclarativeVideoOutput::~QDeclarativeVideoOutput()
{
    if (m_source && m_sourceType == VideoSurfaceSource) {
        if (m_source.data()->property("videoSurface").value<QAbstractVideoSurface*>() == m_surface)
            m_source.data()->setProperty("videoSurface", QVariant::fromValue<QAbstractVideoSurface*>(0));
    }

    m_source.clear();
    _q_updateMediaObject();
    delete m_surface;
    qDeleteAll(m_videoNodeFactories);
}

/*!
    \qmlproperty variant VideoOutput::source

    This property holds the source item providing the video frames like MediaPlayer or Camera.

    If you are extending your own C++ classes to interoperate with VideoOutput, you can
    either provide a QObject based class with a \c mediaObject property that exposes a
    QMediaObject derived class that has a QVideoRendererControl available, or you can
    provide a QObject based class with a writable \c videoSurface property that can
    accept a QAbstractVideoSurface based class and can follow the correct protocol to
    deliver QVideoFrames to it.
*/

void QDeclarativeVideoOutput::setSource(QObject *source)
{
#ifdef DEBUG_VIDEOITEM
    qDebug() << Q_FUNC_INFO << source;
#endif

    if (source == m_source.data())
        return;

    if (m_source && m_sourceType == MediaObjectSource)
        disconnect(0, m_source.data(), SLOT(_q_updateMediaObject()));

    if (m_source && m_sourceType == VideoSurfaceSource) {
        if (m_source.data()->property("videoSurface").value<QAbstractVideoSurface*>() == m_surface)
            m_source.data()->setProperty("videoSurface", QVariant::fromValue<QAbstractVideoSurface*>(0));
    }

    m_surface->stop();

    m_source = source;

    if (m_source) {
        const QMetaObject *metaObject = m_source.data()->metaObject();

        int mediaObjectPropertyIndex = metaObject->indexOfProperty("mediaObject");
        if (mediaObjectPropertyIndex != -1) {
            const QMetaProperty mediaObjectProperty = metaObject->property(mediaObjectPropertyIndex);

            if (mediaObjectProperty.hasNotifySignal()) {
                QMetaMethod method = mediaObjectProperty.notifySignal();
                QMetaObject::connect(m_source.data(), method.methodIndex(),
                                     this, this->metaObject()->indexOfSlot("updateMediaObject()"),
                                     Qt::DirectConnection, 0);

            }
            m_sourceType = MediaObjectSource;
        } else if (metaObject->indexOfProperty("videoSurface") != -1) {
            m_source.data()->setProperty("videoSurface", QVariant::fromValue<QAbstractVideoSurface*>(m_surface));
            m_sourceType = VideoSurfaceSource;
        } else {
            m_sourceType = NoSource;
        }
    } else {
        m_sourceType = NoSource;
    }

    _q_updateMediaObject();
    emit sourceChanged();
}

void QDeclarativeVideoOutput::_q_updateMediaObject()
{
    QMediaObject *mediaObject = 0;

    if (m_source)
        mediaObject = qobject_cast<QMediaObject*>(m_source.data()->property("mediaObject").value<QObject*>());

#ifdef DEBUG_VIDEOITEM
    qDebug() << Q_FUNC_INFO << mediaObject;
#endif

    if (m_mediaObject.data() == mediaObject)
        return;

    if (m_rendererControl) {
        m_rendererControl.data()->setSurface(0);
        m_service.data()->releaseControl(m_rendererControl.data());
    }

    m_mediaObject = mediaObject;
    m_mediaObject.clear();
    m_service.clear();
    m_rendererControl.clear();

    if (mediaObject) {
        if (QMediaService *service = mediaObject->service()) {
            if (QMediaControl *control = service->requestControl(QVideoRendererControl_iid)) {
                if ((m_rendererControl = qobject_cast<QVideoRendererControl *>(control))) {
                    m_service = service;
                    m_mediaObject = mediaObject;
                    m_rendererControl.data()->setSurface(m_surface);
                } else {
                    qWarning() << Q_FUNC_INFO << "Media service has no renderer control available";
                    service->releaseControl(control);
                }
            }
        }
    }
}

void QDeclarativeVideoOutput::present(const QVideoFrame &frame)
{
    m_frameMutex.lock();
    m_frame = frame;
    m_frameMutex.unlock();

    update();
}

void QDeclarativeVideoOutput::stop()
{
    present(QVideoFrame());
}

/*
 * Helper - returns true if the given orientation has the same aspect as the default (e.g. 180*n)
 */
static inline bool qIsDefaultAspect(int o)
{
    return (o % 180) == 0;
}

/*
 * Return the orientation normailized to 0-359
 */
static inline int qNormalizedOrientation(int o)
{
    // Negative orientations give negative results
    int o2 = o % 360;
    if (o2 < 0)
        o2 += 360;
    return o2;
}

/*!
    \qmlproperty enumeration VideoOutput::fillMode

    Set this property to define how the video is scaled to fit the target area.

    \list
    \o Stretch - the video is scaled to fit.
    \o PreserveAspectFit - the video is scaled uniformly to fit without cropping
    \o PreserveAspectCrop - the video is scaled uniformly to fill, cropping if necessary
    \endlist

    The default fill mode is PreserveAspectFit.
*/

QDeclarativeVideoOutput::FillMode QDeclarativeVideoOutput::fillMode() const
{
    return m_fillMode;
}

void QDeclarativeVideoOutput::setFillMode(FillMode mode)
{
    if (mode == m_fillMode)
        return;

    m_fillMode = mode;
    m_geometryDirty = true;
    update();

    emit fillModeChanged(mode);
}

void QDeclarativeVideoOutput::_q_updateNativeSize(const QVideoSurfaceFormat &format)
{
    QSize size = format.sizeHint();
    if (!qIsDefaultAspect(m_orientation)) {
        size.transpose();
    }

    if (m_nativeSize != size) {
        m_nativeSize = size;

        m_geometryDirty = true;

        setImplicitWidth(size.width());
        setImplicitHeight(size.height());

        emit sourceRectChanged();
    }
}

/* Based on fill mode and our size, figure out the source/dest rects */
void QDeclarativeVideoOutput::_q_updateGeometry()
{
    QRectF rect(0, 0, width(), height());

    if (!m_geometryDirty && m_lastSize == rect)
        return;

    QRectF oldContentRect(m_contentRect);

    m_geometryDirty = false;
    m_lastSize = rect;

    if (m_nativeSize.isEmpty()) {
        //this is necessary for item to receive the
        //first paint event and configure video surface.
        m_renderedRect = rect;
        m_contentRect = rect;
        m_sourceTextureRect = QRectF(0, 0, 1, 1);
    } else if (m_fillMode == Stretch) {
        m_renderedRect = rect;
        m_contentRect = rect;
        m_sourceTextureRect = QRectF(0, 0, 1, 1);
    } else if (m_fillMode == PreserveAspectFit) {
        QSizeF size = m_nativeSize;
        size.scale(rect.size(), Qt::KeepAspectRatio);

        m_renderedRect = QRectF(0, 0, size.width(), size.height());
        m_renderedRect.moveCenter(rect.center());
        m_contentRect = m_renderedRect;

        m_sourceTextureRect = QRectF(0, 0, 1, 1);
    } else if (m_fillMode == PreserveAspectCrop) {
        m_renderedRect = rect;

        QSizeF scaled = m_nativeSize;
        scaled.scale(rect.size(), Qt::KeepAspectRatioByExpanding);

        m_contentRect = QRectF(QPointF(), scaled);
        m_contentRect.moveCenter(rect.center());

        if (qIsDefaultAspect(m_orientation)) {
            m_sourceTextureRect = QRectF((-m_contentRect.left()) / m_contentRect.width(),
                                  (-m_contentRect.top()) / m_contentRect.height(),
                                  rect.width() / m_contentRect.width(),
                                  rect.height() / m_contentRect.height());
        } else {
            m_sourceTextureRect = QRectF((-m_contentRect.top()) / m_contentRect.height(),
                                  (-m_contentRect.left()) / m_contentRect.width(),
                                  rect.height() / m_contentRect.height(),
                                  rect.width() / m_contentRect.width());
        }
    }

    if (m_contentRect != oldContentRect)
        emit contentRectChanged();
}
/*!
    \qmlproperty int VideoOutput::orientation

    In some cases the source video stream requires a certain
    orientation to be correct.  This includes
    sources like a camera viewfinder, where the displayed
    viewfinder should match reality, no matter what rotation
    the rest of the user interface has.

    This property allows you to apply a rotation (in steps
    of 90 degrees) to compensate for any user interface
    rotation, with positive values in the anti-clockwise direction.

    The orientation change will also affect the mapping
    of coordinates from source to viewport.
*/
int QDeclarativeVideoOutput::orientation() const
{
    return m_orientation;
}

void QDeclarativeVideoOutput::setOrientation(int orientation)
{
    // Make sure it's a multiple of 90.
    if (orientation % 90)
        return;

    // If there's no actual change, return
    if (m_orientation == orientation)
        return;

    // If the new orientation is the same effect
    // as the old one, don't update the video node stuff
    if ((m_orientation % 360) == (orientation % 360)) {
        m_orientation = orientation;
        emit orientationChanged();
        return;
    }

    m_geometryDirty = true;

    // Otherwise, a new orientation
    // See if we need to change aspect ratio orientation too
    bool oldAspect = qIsDefaultAspect(m_orientation);
    bool newAspect = qIsDefaultAspect(orientation);

    m_orientation = orientation;

    if (oldAspect != newAspect) {
        m_nativeSize.transpose();

        setImplicitWidth(m_nativeSize.width());
        setImplicitHeight(m_nativeSize.height());

        // Source rectangle does not change for orientation
    }

    update();
    emit orientationChanged();
}

/*!
    \qmlproperty rectangle VideoOutput::contentRect

    This property holds the item coordinates of the area that
    would contain video to render.  With certain fill modes,
    this rectangle will be larger than the visible area of this
    element.

    This property is useful when other coordinates are specified
    in terms of the source dimensions - this applied for relative
    (normalized) frame coordinates in the range of 0 to 1.0.

    \sa mapRectToItem(), mapPointToItem()

    Areas outside this will be transparent.
*/
QRectF QDeclarativeVideoOutput::contentRect() const
{
    return m_contentRect;
}

/*!
    \qmlproperty rectangle VideoOutput::sourceRect

    This property holds the area of the source video
    content that is considered for rendering.  The
    values are in source pixel coordinates.

    Note that typically the top left corner of this rectangle
    will be \c {0,0} while the width and height will be the
    width and height of the input content.

    The orientation setting does not affect this rectangle.
*/
QRectF QDeclarativeVideoOutput::sourceRect() const
{
    // We might have to transpose back
    QSizeF size = m_nativeSize;
    if (!qIsDefaultAspect(m_orientation)) {
        size.transpose();
    }
    return QRectF(QPointF(), size); // XXX ignores viewport
}

/*!
    \qmlmethod mapNormalizedPointToItem

    Given normalized coordinates \a point (that is, each
    component in the range of 0 to 1.0), return the mapped point
    that it corresponds to (in item coordinates).
    This mapping is affected by the orientation.

    Depending on the fill mode, this point may lie outside the rendered
    rectangle.
 */
QPointF QDeclarativeVideoOutput::mapNormalizedPointToItem(const QPointF &point) const
{
    qreal dx = point.x();
    qreal dy = point.y();

    if (qIsDefaultAspect(m_orientation)) {
        dx *= m_contentRect.width();
        dy *= m_contentRect.height();
    } else {
        dx *= m_contentRect.height();
        dy *= m_contentRect.width();
    }

    switch (qNormalizedOrientation(m_orientation)) {
        case 0:
        default:
            return m_contentRect.topLeft() + QPointF(dx, dy);
        case 90:
            return m_contentRect.bottomLeft() + QPointF(dy, -dx);
        case 180:
            return m_contentRect.bottomRight() + QPointF(-dx, -dy);
        case 270:
            return m_contentRect.topRight() + QPointF(-dy, dx);
    }
}

/*!
    \qmlmethod mapNormalizedRectToItem

    Given a rectangle \a rectangle in normalized
    coordinates (that is, each component in the range of 0 to 1.0),
    return the mapped rectangle that it corresponds to (in item coordinates).
    This mapping is affected by the orientation.

    Depending on the fill mode, this rectangle may extend outside the rendered
    rectangle.
 */
QRectF QDeclarativeVideoOutput::mapNormalizedRectToItem(const QRectF &rectangle) const
{
    return QRectF(mapNormalizedPointToItem(rectangle.topLeft()),
                  mapNormalizedPointToItem(rectangle.bottomRight())).normalized();
}

/*!
    \qmlmethod mapPointToItem

    Given a point \a point in item coordinates, return the
    corresponding point in source coordinates.  This mapping is
    affected by the orientation.

    If the supplied point lies outside the rendered area, the returned
    point will be outside the source rectangle.
 */
QPointF QDeclarativeVideoOutput::mapPointToSource(const QPointF &point) const
{
    QPointF norm = mapPointToSourceNormalized(point);

    if (qIsDefaultAspect(m_orientation))
        return QPointF(norm.x() * m_nativeSize.width(), norm.y() * m_nativeSize.height());
    else
        return QPointF(norm.x() * m_nativeSize.height(), norm.y() * m_nativeSize.width());
}

/*!
    \qmlmethod mapRectToSource

    Given a rectangle \a rectangle in item coordinates, return the
    corresponding rectangle in source coordinates.  This mapping is
    affected by the orientation.

    This mapping is affected by the orientation.

    If the supplied point lies outside the rendered area, the returned
    point will be outside the source rectangle.
 */
QRectF QDeclarativeVideoOutput::mapRectToSource(const QRectF &rectangle) const
{
    return QRectF(mapPointToSource(rectangle.topLeft()),
                  mapPointToSource(rectangle.bottomRight())).normalized();
}

/*!
    \qmlmethod mapPointToItemNormalized

    Given a point \a point in item coordinates, return the
    corresponding point in normalized source coordinates.  This mapping is
    affected by the orientation.

    If the supplied point lies outside the rendered area, the returned
    point will be outside the source rectangle.  No clamping is performed.
 */
QPointF QDeclarativeVideoOutput::mapPointToSourceNormalized(const QPointF &point) const
{
    if (m_contentRect.isEmpty())
        return QPointF();

    // Normalize the item source point
    qreal nx = (point.x() - m_contentRect.left()) / m_contentRect.width();
    qreal ny = (point.y() - m_contentRect.top()) / m_contentRect.height();

    const qreal one(1.0f);

    // For now, the origin of the source rectangle is 0,0
    switch (qNormalizedOrientation(m_orientation)) {
        case 0:
        default:
            return QPointF(nx, ny);
        case 90:
            return QPointF(one - ny, nx);
        case 180:
            return QPointF(one - nx, one - ny);
        case 270:
            return QPointF(ny, one - nx);
    }
}

/*!
    \qmlmethod mapRectToSourceNormalized

    Given a rectangle \a rectangle in item coordinates, return the
    corresponding rectangle in normalized source coordinates.  This mapping is
    affected by the orientation.

    This mapping is affected by the orientation.

    If the supplied point lies outside the rendered area, the returned
    point will be outside the source rectangle.  No clamping is performed.
 */
QRectF QDeclarativeVideoOutput::mapRectToSourceNormalized(const QRectF &rectangle) const
{
    return QRectF(mapPointToSourceNormalized(rectangle.topLeft()),
                  mapPointToSourceNormalized(rectangle.bottomRight())).normalized();
}

/*!
    \qmlmethod mapPointToItem

    Given a point \a point in source coordinates, return the
    corresponding point in item coordinates.  This mapping is
    affected by the orientation.

    Depending on the fill mode, this point may lie outside the rendered
    rectangle.
 */
QPointF QDeclarativeVideoOutput::mapPointToItem(const QPointF &point) const
{
    if (m_nativeSize.isEmpty())
        return QPointF();

    // Just normalize and use that function
    // m_nativeSize is transposed in some orientations
    if (qIsDefaultAspect(m_orientation))
        return mapNormalizedPointToItem(QPointF(point.x() / m_nativeSize.width(), point.y() / m_nativeSize.height()));
    else
        return mapNormalizedPointToItem(QPointF(point.x() / m_nativeSize.height(), point.y() / m_nativeSize.width()));
}

/*!
    \qmlmethod mapRectToItem

    Given a rectangle \a rectangle in source coordinates, return the
    corresponding rectangle in item coordinates.  This mapping is
    affected by the orientation.

    Depending on the fill mode, this rectangle may extend outside the rendered
    rectangle.

 */
QRectF QDeclarativeVideoOutput::mapRectToItem(const QRectF &rectangle) const
{
    return QRectF(mapPointToItem(rectangle.topLeft()),
                  mapPointToItem(rectangle.bottomRight())).normalized();
}


QSGNode *QDeclarativeVideoOutput::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGVideoNode *videoNode = static_cast<QSGVideoNode *>(oldNode);

    QMutexLocker lock(&m_frameMutex);

    if (videoNode && videoNode->pixelFormat() != m_frame.pixelFormat()) {
#ifdef DEBUG_VIDEOITEM
        qDebug() << "updatePaintNode: deleting old video node because frame format changed...";
#endif
        delete videoNode;
        videoNode = 0;
    }

    if (!m_frame.isValid()) {
#ifdef DEBUG_VIDEOITEM
        qDebug() << "updatePaintNode: no frames yet... aborting...";
#endif
        return 0;
    }

    if (videoNode == 0) {
        foreach (QSGVideoNodeFactory* factory, m_videoNodeFactories) {
            videoNode = factory->createNode(m_surface->surfaceFormat());
            if (videoNode)
                break;
        }
    }

    if (videoNode == 0)
        return 0;

    _q_updateGeometry();
    // Negative rotations need lots of %360
    videoNode->setTexturedRectGeometry(m_renderedRect, m_sourceTextureRect, qNormalizedOrientation(m_orientation));
    videoNode->setCurrentFrame(m_frame);
    return videoNode;
}

QT_END_NAMESPACE
