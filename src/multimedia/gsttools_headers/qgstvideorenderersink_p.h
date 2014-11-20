/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGSTVIDEORENDERERSINK_P_H
#define QGSTVIDEORENDERERSINK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>

#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qqueue.h>
#include <QtCore/qpointer.h>
#include <QtCore/qwaitcondition.h>
#include <qvideosurfaceformat.h>
#include <qvideoframe.h>
#include <qabstractvideobuffer.h>

#include "qgstvideorendererplugin_p.h"

#include "qgstvideorendererplugin_p.h"

QT_BEGIN_NAMESPACE
class QAbstractVideoSurface;

class QGstDefaultVideoRenderer : public QGstVideoRenderer
{
public:
    QGstDefaultVideoRenderer();
    ~QGstDefaultVideoRenderer();

    GstCaps *getCaps(QAbstractVideoSurface *surface);
    bool start(QAbstractVideoSurface *surface, GstCaps *caps);
    void stop(QAbstractVideoSurface *surface);

    bool proposeAllocation(GstQuery *query);

    bool present(QAbstractVideoSurface *surface, GstBuffer *buffer);
    void flush(QAbstractVideoSurface *surface);

private:
    QVideoSurfaceFormat m_format;
    GstVideoInfo m_videoInfo;
    bool m_flushed;
};

class QVideoSurfaceGstDelegate : public QObject
{
    Q_OBJECT
public:
    QVideoSurfaceGstDelegate(QAbstractVideoSurface *surface);
    ~QVideoSurfaceGstDelegate();

    GstCaps *caps();

    bool start(GstCaps *caps);
    void stop();
    bool proposeAllocation(GstQuery *query);

    void flush();

    GstFlowReturn render(GstBuffer *buffer, bool show);

    bool event(QEvent *event);

    static void handleShowPrerollChange(GObject *o, GParamSpec *p, gpointer d);

private slots:
    bool handleEvent(QMutexLocker *locker);
    void updateSupportedFormats();

private:
    void notify();
    bool waitForAsyncEvent(QMutexLocker *locker, QWaitCondition *condition, unsigned long time);

    QPointer<QAbstractVideoSurface> m_surface;

    QMutex m_mutex;
    QWaitCondition m_setupCondition;
    QWaitCondition m_renderCondition;
    GstFlowReturn m_renderReturn;
    QList<QGstVideoRenderer *> m_renderers;
    QGstVideoRenderer *m_renderer;
    QGstVideoRenderer *m_activeRenderer;

    GstCaps *m_surfaceCaps;
    GstCaps *m_startCaps;
    GstBuffer *m_lastBuffer;

    bool m_notified;
    bool m_stop;
    bool m_render;
    bool m_flush;
};

class QGstVideoRendererSink
{
public:
    GstVideoSink parent;

    static QGstVideoRendererSink *createSink(QAbstractVideoSurface *surface);

private:
    static GType get_type();
    static void class_init(gpointer g_class, gpointer class_data);
    static void base_init(gpointer g_class);
    static void instance_init(GTypeInstance *instance, gpointer g_class);

    static void finalize(GObject *object);

    static GstStateChangeReturn change_state(GstElement *element, GstStateChange transition);

    static GstCaps *get_caps(GstBaseSink *sink, GstCaps *filter);
    static gboolean set_caps(GstBaseSink *sink, GstCaps *caps);

    static gboolean propose_allocation(GstBaseSink *sink, GstQuery *query);

    static GstFlowReturn preroll(GstBaseSink *sink, GstBuffer *buffer);
    static GstFlowReturn render(GstBaseSink *sink, GstBuffer *buffer);

private:
    QVideoSurfaceGstDelegate *delegate;
};


class QGstVideoRendererSinkClass
{
public:
    GstVideoSinkClass parent_class;
};

QT_END_NAMESPACE

#endif
