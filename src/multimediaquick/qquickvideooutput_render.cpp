/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Research In Motion
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

#include "qquickvideooutput_render_p.h"
#include "qquickvideooutput_p.h"
#include <QtCore/qobject.h>
#include <QtCore/qloggingcategory.h>
#include <private/qsgvideonode_p.h>
#include <qvideosink.h>

#include <QtQuick/QQuickWindow>
#include <private/qquickwindow_p.h>
#include <QtCore/QRunnable>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcVideo)

QQuickVideoBackend::QQuickVideoBackend(QQuickVideoOutput *parent)
    : q(parent),
      m_frameChanged(false)
{
}

QQuickVideoBackend::~QQuickVideoBackend()
{
    delete m_sink;
}

void QQuickVideoBackend::releaseResources()
{
    // Called on the gui thread when the window is closed or changed.
    invalidateSceneGraph();
}

void QQuickVideoBackend::invalidateSceneGraph()
{
    // Called on the render thread, e.g. when the context is lost.
//    QMutexLocker lock(&m_frameMutex);
}

void QQuickVideoBackend::itemChange(QQuickItem::ItemChange change,
                                      const QQuickItem::ItemChangeData &changeData)
{
    if (change == QQuickItem::ItemSceneChange) {
        if (changeData.window)
            QObject::connect(changeData.window, SIGNAL(sceneGraphInvalidated()),
                             q, SLOT(_q_invalidateSceneGraph()), Qt::DirectConnection);
    }
}

QSize QQuickVideoBackend::nativeSize() const
{
    return m_surfaceFormat.viewport().size();
}

void QQuickVideoBackend::updateGeometry()
{
    const QRectF viewport = m_surfaceFormat.viewport();
    const QSizeF frameSize = m_surfaceFormat.frameSize();
    const QRectF normalizedViewport(viewport.x() / frameSize.width(),
                                    viewport.y() / frameSize.height(),
                                    viewport.width() / frameSize.width(),
                                    viewport.height() / frameSize.height());
    const QRectF rect(0, 0, q->width(), q->height());
    if (nativeSize().isEmpty()) {
        m_renderedRect = rect;
        m_sourceTextureRect = normalizedViewport;
    } else if (q->fillMode() == QQuickVideoOutput::Stretch) {
        m_renderedRect = rect;
        m_sourceTextureRect = normalizedViewport;
    } else if (q->fillMode() == QQuickVideoOutput::PreserveAspectFit) {
        m_sourceTextureRect = normalizedViewport;
        m_renderedRect = q->contentRect();
    } else if (q->fillMode() == QQuickVideoOutput::PreserveAspectCrop) {
        m_renderedRect = rect;
        const qreal contentHeight = q->contentRect().height();
        const qreal contentWidth = q->contentRect().width();

        // Calculate the size of the source rectangle without taking the viewport into account
        const qreal relativeOffsetLeft = -q->contentRect().left() / contentWidth;
        const qreal relativeOffsetTop = -q->contentRect().top() / contentHeight;
        const qreal relativeWidth = rect.width() / contentWidth;
        const qreal relativeHeight = rect.height() / contentHeight;

        // Now take the viewport size into account
        const qreal totalOffsetLeft = normalizedViewport.x() + relativeOffsetLeft * normalizedViewport.width();
        const qreal totalOffsetTop = normalizedViewport.y() + relativeOffsetTop * normalizedViewport.height();
        const qreal totalWidth = normalizedViewport.width() * relativeWidth;
        const qreal totalHeight = normalizedViewport.height() * relativeHeight;

        if (qIsDefaultAspect(q->orientation())) {
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

QSGNode *QQuickVideoBackend::updatePaintNode(QSGNode *oldNode,
                                                           QQuickItem::UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
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
            videoNode = new QSGVideoNode(m_surfaceFormat);
            qCDebug(qLcVideo) << "updatePaintNode: Video node created. Handle type:" << m_frame.handleType();
        }
    }

    if (!videoNode) {
        m_frameChanged = false;
        m_frame = QVideoFrame();
        return nullptr;
    }

    // Negative rotations need lots of %360
    videoNode->setTexturedRectGeometry(m_renderedRect, m_sourceTextureRect,
                                       qNormalizedOrientation(q->orientation()));
    if (m_frameChanged) {
        videoNode->setCurrentFrame(m_frame);

        if ((q->flushMode() == QQuickVideoOutput::FirstFrame && !m_frameOnFlush.isValid())
            || q->flushMode() == QQuickVideoOutput::LastFrame) {
            m_frameOnFlush = m_frame;
        }

        //don't keep the frame for more than really necessary
        m_frameChanged = false;
        m_frame = QVideoFrame();
    }
    return videoNode;
}

QVideoSink *QQuickVideoBackend::videoSink() const
{
    if (!m_sink) {
        m_sink = new QVideoSink(q);
        if (q->window())
            m_sink->setRhi(QQuickWindowPrivate::get(q->window())->rhi);
        qRegisterMetaType<QVideoFrameFormat>();
        QObject::connect(m_sink, SIGNAL(newVideoFrame(const QVideoFrame &)),
                         q, SLOT(_q_newFrame(const QVideoFrame &)), Qt::QueuedConnection);
    }
    return m_sink;
}

QRectF QQuickVideoBackend::adjustedViewport() const
{
    return m_surfaceFormat.viewport();
}

void QQuickVideoBackend::present(const QVideoFrame &frame)
{
    m_frameMutex.lock();
    m_surfaceFormat = frame.surfaceFormat();
    m_frame = frame.isValid() ? frame : m_frameOnFlush;
    m_frameChanged = true;
    m_frameMutex.unlock();

    q->update();
}

void QQuickVideoBackend::stop()
{
    present(QVideoFrame());
}

QT_END_NAMESPACE
