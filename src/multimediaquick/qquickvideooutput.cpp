// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qquickvideooutput_p.h"

#include <private/qvideooutputorientationhandler_p.h>
#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <private/qfactoryloader_p.h>
#include <QtCore/qloggingcategory.h>
#include <qvideosink.h>
#include <QtQuick/QQuickWindow>
#include <private/qquickwindow_p.h>
#include <qsgvideonode_p.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcVideo, "qt.multimedia.video")

namespace {

inline bool qIsDefaultAspect(int o)
{
    return (o % 180) == 0;
}

/*
 * Return the orientation normalized to 0-359
 */
inline int qNormalizedOrientation(int o)
{
    // Negative orientations give negative results
    int o2 = o % 360;
    if (o2 < 0)
        o2 += 360;
    return o2;
}

}

/*!
    \qmltype VideoOutput
    //! \instantiates QQuickVideoOutput
    \brief Render video or camera viewfinder.

    \ingroup multimedia_qml
    \ingroup multimedia_video_qml
    \inqmlmodule QtMultimedia

    \qml

    Rectangle {
        width: 800
        height: 600
        color: "black"

        MediaPlayer {
            id: player
            source: "file://video.webm"
            videoOutput: videoOutput
        }

        VideoOutput {
            id: videoOutput
            anchors.fill: parent
        }
    }

    \endqml

    The VideoOutput item supports untransformed, stretched, and uniformly scaled video presentation.
    For a description of stretched uniformly scaled presentation, see the \l fillMode property
    description.

    \sa MediaPlayer, Camera

\omit
    \section1 Screen Saver

    If it is likely that an application will be playing video for an extended
    period of time without user interaction it may be necessary to disable
    the platform's screen saver. The \l ScreenSaver (from \l QtSystemInfo)
    may be used to disable the screensaver in this fashion:

    \qml
    import QtSystemInfo

    ScreenSaver { screenSaverEnabled: false }
    \endqml
\endomit
*/

// TODO: Restore Qt System Info docs when the module is released

/*!
    \internal
    \class QQuickVideoOutput
    \brief The QQuickVideoOutput class provides a video output item.
*/

QQuickVideoOutput::QQuickVideoOutput(QQuickItem *parent) :
    QQuickItem(parent)
{
    setFlag(ItemHasContents, true);

    m_sink = new QVideoSink(this);
    qRegisterMetaType<QVideoFrameFormat>();
    QObject::connect(m_sink, &QVideoSink::videoFrameChanged, this,
                     [&](const QVideoFrame &frame) {
                         setFrame(frame);
                         emit frameUpdated(frame.size());
                     }, Qt::DirectConnection);

    QObject::connect(this, &QQuickVideoOutput::frameUpdated,
                     this, &QQuickVideoOutput::_q_newFrame);

    initRhiForSink();
}

QQuickVideoOutput::~QQuickVideoOutput()
{
}

/*!
    \qmlproperty object QtMultimedia::VideoOutput::videoSink

    This property holds the underlaying C++ QVideoSink object that is used
    to render the video frames to this VideoOutput element.

    Normal usage of VideoOutput from QML should not require using this property.
*/

QVideoSink *QQuickVideoOutput::videoSink() const
{
    return m_sink;
}

/*!
    \qmlproperty enumeration QtMultimedia::VideoOutput::fillMode

    Set this property to define how the video is scaled to fit the target area.

    \list
    \li Stretch - the video is scaled to fit.
    \li PreserveAspectFit - the video is scaled uniformly to fit without cropping
    \li PreserveAspectCrop - the video is scaled uniformly to fill, cropping if necessary
    \endlist

    The default fill mode is PreserveAspectFit.
*/

QQuickVideoOutput::FillMode QQuickVideoOutput::fillMode() const
{
    return FillMode(m_aspectRatioMode);
}

void QQuickVideoOutput::setFillMode(FillMode mode)
{
    if (mode == fillMode())
        return;

    m_aspectRatioMode = Qt::AspectRatioMode(mode);

    m_geometryDirty = true;
    update();

    emit fillModeChanged(mode);
}

void QQuickVideoOutput::_q_newFrame(QSize size)
{
    update();

    if (!qIsDefaultAspect(m_orientation + m_frameOrientation)) {
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
void QQuickVideoOutput::_q_updateGeometry()
{
    const QRectF rect(0, 0, width(), height());
    const QRectF absoluteRect(x(), y(), width(), height());

    if (!m_geometryDirty && m_lastRect == absoluteRect)
        return;

    QRectF oldContentRect(m_contentRect);

    m_geometryDirty = false;
    m_lastRect = absoluteRect;

    const auto fill = m_aspectRatioMode;
    if (m_nativeSize.isEmpty()) {
        //this is necessary for item to receive the
        //first paint event and configure video surface.
        m_contentRect = rect;
    } else if (fill == Qt::IgnoreAspectRatio) {
        m_contentRect = rect;
    } else {
        QSizeF scaled = m_nativeSize;
        scaled.scale(rect.size(), fill);

        m_contentRect = QRectF(QPointF(), scaled);
        m_contentRect.moveCenter(rect.center());
    }

    updateGeometry();

    if (m_contentRect != oldContentRect)
        emit contentRectChanged();
}

/*!
    \qmlproperty int QtMultimedia::VideoOutput::orientation

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
int QQuickVideoOutput::orientation() const
{
    return m_orientation;
}

void QQuickVideoOutput::setOrientation(int orientation)
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
    \qmlproperty rectangle QtMultimedia::VideoOutput::contentRect

    This property holds the item coordinates of the area that
    would contain video to render.  With certain fill modes,
    this rectangle will be larger than the visible area of the
    \c VideoOutput.

    This property is useful when other coordinates are specified
    in terms of the source dimensions - this applied for relative
    (normalized) frame coordinates in the range of 0 to 1.0.

    Areas outside this will be transparent.
*/
QRectF QQuickVideoOutput::contentRect() const
{
    return m_contentRect;
}

/*!
    \qmlproperty rectangle QtMultimedia::VideoOutput::sourceRect

    This property holds the area of the source video
    content that is considered for rendering.  The
    values are in source pixel coordinates, adjusted for
    the source's pixel aspect ratio.

    Note that typically the top left corner of this rectangle
    will be \c {0,0} while the width and height will be the
    width and height of the input content. Only when the video
    source has a viewport set, these values will differ.

    The orientation setting does not affect this rectangle.

    \sa QVideoFrameFormat::viewport()
*/
QRectF QQuickVideoOutput::sourceRect() const
{
    // We might have to transpose back
    QSizeF size = m_nativeSize;
    if (!size.isValid())
        return {};

    if (!qIsDefaultAspect(m_orientation + m_frameOrientation))
        size.transpose();


    // Take the viewport into account for the top left position.
    // m_nativeSize is already adjusted to the viewport, as it originates
    // from QVideoFrameFormat::viewport(), which includes pixel aspect ratio
    const QRectF viewport = adjustedViewport();
    Q_ASSERT(viewport.size() == size);
    return QRectF(viewport.topLeft(), size);
}

void QQuickVideoOutput::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(newGeometry);
    Q_UNUSED(oldGeometry);

    QQuickItem::geometryChange(newGeometry, oldGeometry);

    // Explicitly listen to geometry changes here. This is needed since changing the position does
    // not trigger a call to updatePaintNode().
    // We need to react to position changes though, as the window backened's display rect gets
    // changed in that situation.
    _q_updateGeometry();
}

void QQuickVideoOutput::_q_invalidateSceneGraph()
{
    invalidateSceneGraph();
}

void QQuickVideoOutput::_q_sceneGraphInitialized()
{
    initRhiForSink();
}

void QQuickVideoOutput::releaseResources()
{
    // Called on the gui thread when the window is closed or changed.
    invalidateSceneGraph();
}

void QQuickVideoOutput::invalidateSceneGraph()
{
    // Called on the render thread, e.g. when the context is lost.
    //    QMutexLocker lock(&m_frameMutex);
    initRhiForSink();
}

void QQuickVideoOutput::initRhiForSink()
{
    QRhi *rhi = m_window ? QQuickWindowPrivate::get(m_window)->rhi : nullptr;
    m_sink->setRhi(rhi);
}

void QQuickVideoOutput::itemChange(QQuickItem::ItemChange change,
                                    const QQuickItem::ItemChangeData &changeData)
{
    if (change != QQuickItem::ItemSceneChange)
        return;

    if (changeData.window == m_window)
        return;
    if (m_window)
        disconnect(m_window);
    m_window = changeData.window;

    if (m_window) {
        // We want to receive the signals in the render thread
        QObject::connect(m_window, &QQuickWindow::sceneGraphInitialized, this, &QQuickVideoOutput::_q_sceneGraphInitialized,
                         Qt::DirectConnection);
        QObject::connect(m_window, &QQuickWindow::sceneGraphInvalidated,
                         this, &QQuickVideoOutput::_q_invalidateSceneGraph, Qt::DirectConnection);
    }
    initRhiForSink();
}

QSize QQuickVideoOutput::nativeSize() const
{
    return m_surfaceFormat.viewport().size();
}

void QQuickVideoOutput::updateGeometry()
{
    const QRectF viewport = m_surfaceFormat.viewport();
    const QSizeF frameSize = m_surfaceFormat.frameSize();
    const QRectF normalizedViewport(viewport.x() / frameSize.width(),
                                    viewport.y() / frameSize.height(),
                                    viewport.width() / frameSize.width(),
                                    viewport.height() / frameSize.height());
    const QRectF rect(0, 0, width(), height());
    if (nativeSize().isEmpty()) {
        m_renderedRect = rect;
        m_sourceTextureRect = normalizedViewport;
    } else if (m_aspectRatioMode == Qt::IgnoreAspectRatio) {
        m_renderedRect = rect;
        m_sourceTextureRect = normalizedViewport;
    } else if (m_aspectRatioMode == Qt::KeepAspectRatio) {
        m_sourceTextureRect = normalizedViewport;
        m_renderedRect = contentRect();
    } else if (m_aspectRatioMode == Qt::KeepAspectRatioByExpanding) {
        m_renderedRect = rect;
        const qreal contentHeight = contentRect().height();
        const qreal contentWidth = contentRect().width();

        // Calculate the size of the source rectangle without taking the viewport into account
        const qreal relativeOffsetLeft = -contentRect().left() / contentWidth;
        const qreal relativeOffsetTop = -contentRect().top() / contentHeight;
        const qreal relativeWidth = rect.width() / contentWidth;
        const qreal relativeHeight = rect.height() / contentHeight;

        // Now take the viewport size into account
        const qreal totalOffsetLeft = normalizedViewport.x() + relativeOffsetLeft * normalizedViewport.width();
        const qreal totalOffsetTop = normalizedViewport.y() + relativeOffsetTop * normalizedViewport.height();
        const qreal totalWidth = normalizedViewport.width() * relativeWidth;
        const qreal totalHeight = normalizedViewport.height() * relativeHeight;

        if (qIsDefaultAspect(orientation() + m_frameOrientation)) {
            m_sourceTextureRect = QRectF(totalOffsetLeft, totalOffsetTop,
                                         totalWidth, totalHeight);
        } else {
            m_sourceTextureRect = QRectF(totalOffsetTop, totalOffsetLeft,
                                         totalHeight, totalWidth);
        }
    }

    if (m_surfaceFormat.scanLineDirection() == QVideoFrameFormat::BottomToTop) {
        qreal top = m_sourceTextureRect.top();
        m_sourceTextureRect.setTop(m_sourceTextureRect.bottom());
        m_sourceTextureRect.setBottom(top);
    }

    if (m_surfaceFormat.isMirrored()) {
        qreal left = m_sourceTextureRect.left();
        m_sourceTextureRect.setLeft(m_sourceTextureRect.right());
        m_sourceTextureRect.setRight(left);
    }
}

QSGNode *QQuickVideoOutput::updatePaintNode(QSGNode *oldNode,
                                             QQuickItem::UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    _q_updateGeometry();

    QSGVideoNode *videoNode = static_cast<QSGVideoNode *>(oldNode);

    QMutexLocker lock(&m_frameMutex);

    if (m_frameChanged) {
        if (videoNode && videoNode->pixelFormat() != m_frame.pixelFormat()) {
            qCDebug(qLcVideo) << "updatePaintNode: deleting old video node because frame format changed";
            delete videoNode;
            videoNode = nullptr;
        }

        if (!m_frame.isValid()) {
            qCDebug(qLcVideo) << "updatePaintNode: no frames yet";
            m_frameChanged = false;
            return nullptr;
        }

        if (!videoNode) {
            // Get a node that supports our frame. The surface is irrelevant, our
            // QSGVideoItemSurface supports (logically) anything.
            updateGeometry();
            videoNode = new QSGVideoNode(this, m_surfaceFormat);
            qCDebug(qLcVideo) << "updatePaintNode: Video node created. Handle type:" << m_frame.handleType();
        }
    }

    if (!videoNode) {
        m_frameChanged = false;
        m_frame = QVideoFrame();
        return nullptr;
    }

    if (m_frameChanged) {
        videoNode->setCurrentFrame(m_frame);

        //don't keep the frame for more than really necessary
        m_frameChanged = false;
        m_frame = QVideoFrame();
    }

    // Negative rotations need lots of %360
    videoNode->setTexturedRectGeometry(m_renderedRect, m_sourceTextureRect,
                                       qNormalizedOrientation(orientation()));

    return videoNode;
}

QRectF QQuickVideoOutput::adjustedViewport() const
{
    return m_surfaceFormat.viewport();
}

void QQuickVideoOutput::setFrame(const QVideoFrame &frame)
{
    m_frameMutex.lock();
    m_surfaceFormat = frame.surfaceFormat();
    m_frame = frame;
    m_frameOrientation = frame.rotationAngle();
    m_frameChanged = true;
    m_frameMutex.unlock();
}

void QQuickVideoOutput::stop()
{
    setFrame({});
    update();
}

QT_END_NAMESPACE

#include "moc_qquickvideooutput_p.cpp"
