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

#ifndef QGSTREAMERGLTEXTURERENDERER_H
#define QGSTREAMERGLTEXTURERENDERER_H

#include <qvideorenderercontrol.h>
#include "qvideosurfacegstsink.h"

#include "qgstreamervideorendererinterface.h"
#include <QtGui/qcolor.h>

#include <X11/extensions/Xv.h>

QT_USE_NAMESPACE

class QGLContext;

class QGstreamerGLTextureRenderer : public QVideoRendererControl, public QGstreamerVideoRendererInterface
{
    Q_OBJECT
    Q_INTERFACES(QGstreamerVideoRendererInterface)

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
    void handleBusMessage(GstMessage* gm);
    void handleSyncMessage(GstMessage* gm);
    void precessNewStream();
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

#endif // QGSTREAMERVIDEORENDRER_H
