/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qdeclarativevideooutput_p.h"

#include "qsgvideonode_p.h"
#include "qsgvideonode_i420.h"
#include "qsgvideonode_rgb.h"

#include <QtDeclarative/qquickitem.h>

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

    \ingroup qml-multimedia

    This element is part of the \bold{QtMultimedia 4.0} module.

    \qml
    import QtQuick 2.0
    import QtMultimedia 4.0

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
    m_source.clear();
    _q_updateMediaObject();
    delete m_surface;
    qDeleteAll(m_videoNodeFactories);
}

/*!
    \qmlproperty variant VideoOutput::source

    This property holds the source item providing the video frames like MediaPlayer or Camera.
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

    if (m_source && m_sourceType == VideoSurfaceSource)
        m_source.data()->setProperty("videoSurface", QVariant::fromValue<QAbstractVideoSurface*>(0));

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

        setImplicitWidth(size.width());
        setImplicitHeight(size.height());
    }
}

void QDeclarativeVideoOutput::_q_updateGeometry()
{
    QRectF rect(0, 0, width(), height());

    if (m_nativeSize.isEmpty()) {
        //this is necessary for item to receive the
        //first paint event and configure video surface.
        m_boundingRect = rect;
        m_sourceRect = QRectF(0, 0, 1, 1);
    } else if (m_fillMode == Stretch) {
        m_boundingRect = rect;
        m_sourceRect = QRectF(0, 0, 1, 1);
    } else if (m_fillMode == PreserveAspectFit) {
        QSizeF size = m_nativeSize;
        size.scale(rect.size(), Qt::KeepAspectRatio);

        m_boundingRect = QRectF(0, 0, size.width(), size.height());
        m_boundingRect.moveCenter(rect.center());

        m_sourceRect = QRectF(0, 0, 1, 1);
    } else if (m_fillMode == PreserveAspectCrop) {
        m_boundingRect = rect;

        QSizeF size = rect.size();
        size.scale(m_nativeSize, Qt::KeepAspectRatio);

        m_sourceRect = QRectF(
                0, 0, size.width() / m_nativeSize.width(), size.height() / m_nativeSize.height());
        m_sourceRect.moveCenter(QPointF(0.5, 0.5));
    }
}

int QDeclarativeVideoOutput::orientation() const
{
    return m_orientation;
}

void QDeclarativeVideoOutput::setOrientation(int orientation)
{
    // Make sure it's a multiple of 90.
    if (orientation % 90)
        return;

    // If the new orientation is the same effect
    // as the old one, don't update the video node stuff
    if ((m_orientation % 360) == (orientation % 360)) {
        m_orientation = orientation;
        emit orientationChanged();
        return;
    }

    // Otherwise, a new orientation
    // See if we need to change aspect ratio orientation too
    bool oldAspect = qIsDefaultAspect(m_orientation);
    bool newAspect = qIsDefaultAspect(orientation);

    m_orientation = orientation;

    if (oldAspect != newAspect) {
        m_nativeSize.transpose();

        setImplicitWidth(m_nativeSize.width());
        setImplicitHeight(m_nativeSize.height());
    }

    update();
    emit orientationChanged();
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
    videoNode->setTexturedRectGeometry(m_boundingRect, m_sourceRect, (360 + (m_orientation % 360)) % 360);
    videoNode->setCurrentFrame(m_frame);
    return videoNode;
}

QT_END_NAMESPACE
