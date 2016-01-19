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

#ifndef QGSTREAMERGLTEXTURERENDERER_H
#define QGSTREAMERGLTEXTURERENDERER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qvideorenderercontrol.h>
#include <private/qvideosurfacegstsink_p.h>
#include <private/qgstreamerbushelper_p.h>

#include "qgstreamervideorendererinterface_p.h"
#include <QtGui/qcolor.h>

#include <X11/extensions/Xv.h>

QT_BEGIN_NAMESPACE

class QGLContext;

class QGstreamerGLTextureRenderer : public QVideoRendererControl,
        public QGstreamerVideoRendererInterface,
        public QGstreamerSyncMessageFilter,
        public QGstreamerBusMessageFilter
{
    Q_OBJECT
    Q_INTERFACES(QGstreamerVideoRendererInterface QGstreamerSyncMessageFilter QGstreamerBusMessageFilter)

    Q_PROPERTY(bool overlayEnabled READ overlayEnabled WRITE setOverlayEnabled)
    Q_PROPERTY(qulonglong winId READ winId WRITE setWinId)
    Q_PROPERTY(QRect overlayGeometry READ overlayGeometry WRITE setOverlayGeometry)
    Q_PROPERTY(QColor colorKey READ colorKey)
    Q_PROPERTY(QSize nativeSize READ nativeSize NOTIFY nativeSizeChanged)

public:
    QGstreamerGLTextureRenderer(QObject *parent = 0);
    virtual ~QGstreamerGLTextureRenderer();

    QAbstractVideoSurface *surface() const;
    void setSurface(QAbstractVideoSurface *surface);

    GstElement *videoSink();

    bool isReady() const;
    bool processBusMessage(const QGstreamerMessage &message);
    bool processSyncMessage(const QGstreamerMessage &message);
    void stopRenderer();

    int framebufferNumber() const;

    bool overlayEnabled() const;
    WId winId() const;
    QRect overlayGeometry() const;
    QColor colorKey() const;
    QSize nativeSize() const;

public slots:
    void renderGLFrame(int);

    void setOverlayEnabled(bool);
    void setWinId(WId id);
    void setOverlayGeometry(const QRect &geometry);
    void repaintOverlay();

signals:
    void sinkChanged();
    void readyChanged(bool);
    void nativeSizeChanged();

private slots:
    void handleFormatChange();
    void updateNativeVideoSize();

private:
    static void handleFrameReady(GstElement *sink, gint frame, gpointer data);
    static gboolean padBufferProbe(GstPad *pad, GstBuffer *buffer, gpointer user_data);

    GstElement *m_videoSink;
    QAbstractVideoSurface *m_surface;
    QGLContext *m_context;
    QSize m_nativeSize;

    WId m_winId;
    QColor m_colorKey;
    QRect m_displayRect;
    bool m_overlayEnabled;
    int m_bufferProbeId;

    QMutex m_mutex;
    QWaitCondition m_renderCondition;
};

QT_END_NAMESPACE

#endif // QGSTREAMERVIDEORENDRER_H
