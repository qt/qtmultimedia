/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QtCore/qdebug.h>
#include <QtCore/qthread.h>
#include <QtCore/qvariant.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qwindow.h>
#include <qpa/qplatformnativeinterface.h>

#include "qgstxvimagebuffer_p.h"
#include "qvideosurfacegstsink_p.h"
#include "qgstvideobuffer_p.h"

QT_BEGIN_NAMESPACE

GstBufferClass *QGstXvImageBuffer::parent_class = NULL;

GType QGstXvImageBuffer::get_type(void)
{
    static GType buffer_type = 0;

    if (buffer_type == 0) {
        static const GTypeInfo buffer_info = {
            sizeof (GstBufferClass),
            NULL,
            NULL,
            QGstXvImageBuffer::class_init,
            NULL,
            NULL,
            sizeof(QGstXvImageBuffer),
            0,
            (GInstanceInitFunc)QGstXvImageBuffer::buffer_init,
            NULL
        };
        buffer_type = g_type_register_static(GST_TYPE_BUFFER,
                                             "QGstXvImageBuffer", &buffer_info, GTypeFlags(0));
    }
    return buffer_type;
}

void QGstXvImageBuffer::class_init(gpointer g_class, gpointer class_data)
{
    Q_UNUSED(class_data);
    GST_MINI_OBJECT_CLASS(g_class)->finalize =
            (GstMiniObjectFinalizeFunction)buffer_finalize;
    parent_class = (GstBufferClass*)g_type_class_peek_parent(g_class);
}

void QGstXvImageBuffer::buffer_init(QGstXvImageBuffer *xvImage, gpointer g_class)
{
    Q_UNUSED(g_class);
    xvImage->pool = 0;
    xvImage->shmInfo.shmaddr = ((char *) -1);
    xvImage->shmInfo.shmid = -1;
    xvImage->markedForDeletion = false;
}

void QGstXvImageBuffer::buffer_finalize(QGstXvImageBuffer * xvImage)
{
    if (xvImage->pool) {
        if (xvImage->markedForDeletion)
            xvImage->pool->destroyBuffer(xvImage);
        else
            xvImage->pool->recycleBuffer(xvImage);
    }
}


QGstXvImageBufferPool::QGstXvImageBufferPool(QObject *parent)
    :QObject(parent)
{
    m_threadId = QThread::currentThreadId();
}

QGstXvImageBufferPool::~QGstXvImageBufferPool()
{
}

bool QGstXvImageBufferPool::isFormatSupported(const QVideoSurfaceFormat &surfaceFormat) const
{
    bool ok = true;
    surfaceFormat.property("portId").toULongLong(&ok);
    if (!ok)
        return false;

    int xvFormatId = surfaceFormat.property("xvFormatId").toInt(&ok);
    if (!ok || xvFormatId < 0)
        return false;

    int dataSize = surfaceFormat.property("dataSize").toInt(&ok);
    if (!ok || dataSize<=0)
        return false;

    return true;
}

GType QGstXvImageBufferPool::bufferType() const
{
    return QGstXvImageBuffer::get_type();
}

GstBuffer *QGstXvImageBufferPool::takeBuffer(
    const QVideoSurfaceFormat &format, GstCaps *caps)
{
    m_poolMutex.lock();

    m_caps = caps;
    if (format != m_format) {
        doClear();
        m_format = format;
    }


    if (m_pool.isEmpty()) {
        //qDebug() << "QGstXvImageBufferPool::takeBuffer: no buffer available, allocate the new one" << QThread::currentThreadId() << m_threadId;
        if (QThread::currentThreadId() == m_threadId) {
            doAlloc();
        } else {
            QMetaObject::invokeMethod(this, "queuedAlloc", Qt::QueuedConnection);
            m_allocWaitCondition.wait(&m_poolMutex, 300);
        }
    }
    QGstXvImageBuffer *res = 0;

    if (!m_pool.isEmpty()) {
        res = m_pool.takeLast();
    }

    m_poolMutex.unlock();

    return GST_BUFFER(res);
}

QAbstractVideoBuffer::HandleType QGstXvImageBufferPool::handleType() const
{
    return QAbstractVideoBuffer::XvShmImageHandle;
}

QAbstractVideoBuffer *QGstXvImageBufferPool::prepareVideoBuffer(GstBuffer *buffer, int bytesPerLine)
{
    if (!G_TYPE_CHECK_INSTANCE_TYPE(buffer, bufferType()))
        return 0;

    QGstXvImageBuffer *xvBuffer = reinterpret_cast<QGstXvImageBuffer *>(buffer);
    QVariant handle = QVariant::fromValue(xvBuffer->xvImage);
    return new QGstVideoBuffer(buffer, bytesPerLine, QAbstractVideoBuffer::XvShmImageHandle, handle);
}

QStringList QGstXvImageBufferPool::keys() const
{
    return QStringList() << QGstBufferPoolPluginKey;
}

void QGstXvImageBufferPool::queuedAlloc()
{
    QMutexLocker lock(&m_poolMutex);
    doAlloc();
    m_allocWaitCondition.wakeOne();
}

void QGstXvImageBufferPool::doAlloc()
{
    //should be always called from the main thread with m_poolMutex locked
    //Q_ASSERT(QThread::currentThread() == thread());

    XSync(display(), false);

    QGstXvImageBuffer *xvBuffer = (QGstXvImageBuffer *)gst_mini_object_new(QGstXvImageBuffer::get_type());

    quint64 portId = m_format.property("portId").toULongLong();
    int xvFormatId = m_format.property("xvFormatId").toInt();

    xvBuffer->xvImage = XvShmCreateImage(
            display(),
            portId,
            xvFormatId,
            0,
            m_format.frameWidth(),
            m_format.frameHeight(),
            &xvBuffer->shmInfo
            );

    if (!xvBuffer->xvImage) {
        qWarning() << "QGstXvImageBufferPool: XvShmCreateImage failed";
        return;
    }

    XSync(display(), false);

    xvBuffer->shmInfo.shmid = shmget(IPC_PRIVATE, xvBuffer->xvImage->data_size, IPC_CREAT | 0777);
    xvBuffer->shmInfo.shmaddr = xvBuffer->xvImage->data = (char*)shmat(xvBuffer->shmInfo.shmid, 0, 0);
    xvBuffer->shmInfo.readOnly = False;

    if (!XShmAttach(display(), &xvBuffer->shmInfo)) {
        qWarning() << "QGstXvImageBufferPool: XShmAttach failed";
        return;
    }

    XSync(display(), false);

    shmctl (xvBuffer->shmInfo.shmid, IPC_RMID, NULL);

    xvBuffer->pool = this;
    GST_MINI_OBJECT_CAST(xvBuffer)->flags = 0;
    gst_buffer_set_caps(GST_BUFFER_CAST(xvBuffer), m_caps);
    GST_BUFFER_DATA(xvBuffer) = (uchar*)xvBuffer->xvImage->data;
    GST_BUFFER_SIZE(xvBuffer) = xvBuffer->xvImage->data_size;

    m_allBuffers.append(xvBuffer);
    m_pool.append(xvBuffer);

    XSync(display(), false);
}


void QGstXvImageBufferPool::clear()
{
    QMutexLocker lock(&m_poolMutex);
    doClear();
}

void QGstXvImageBufferPool::doClear()
{
    foreach (QGstXvImageBuffer *xvBuffer, m_allBuffers) {
        xvBuffer->markedForDeletion = true;
    }
    m_allBuffers.clear();

    foreach (QGstXvImageBuffer *xvBuffer, m_pool) {
        gst_buffer_unref(GST_BUFFER(xvBuffer));
    }
    m_pool.clear();

    m_format = QVideoSurfaceFormat();
}

void QGstXvImageBufferPool::queuedDestroy()
{
    QMutexLocker lock(&m_destroyMutex);

    XSync(display(), false);

    foreach(XvShmImage xvImage, m_imagesToDestroy) {
        if (xvImage.shmInfo.shmaddr != ((void *) -1)) {
            XShmDetach(display(), &xvImage.shmInfo);
            XSync(display(), false);

            shmdt(xvImage.shmInfo.shmaddr);
        }

        if (xvImage.xvImage)
            XFree(xvImage.xvImage);
    }

    m_imagesToDestroy.clear();

    XSync(display(), false);
}

void QGstXvImageBufferPool::recycleBuffer(QGstXvImageBuffer *xvBuffer)
{
    QMutexLocker lock(&m_poolMutex);
    gst_buffer_ref(GST_BUFFER_CAST(xvBuffer));
    m_pool.append(xvBuffer);
}

void QGstXvImageBufferPool::destroyBuffer(QGstXvImageBuffer *xvBuffer)
{
    XvShmImage imageToDestroy;
    imageToDestroy.xvImage = xvBuffer->xvImage;
    imageToDestroy.shmInfo = xvBuffer->shmInfo;

    m_destroyMutex.lock();
    m_imagesToDestroy.append(imageToDestroy);
    m_destroyMutex.unlock();

    if (m_imagesToDestroy.size() == 1)
        QMetaObject::invokeMethod(this, "queuedDestroy", Qt::QueuedConnection);
}

Display *QGstXvImageBufferPool::display() const
{
    QWindow *window = QGuiApplication::topLevelWindows().first();
    Display *display = static_cast<Display *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("Display", window));

    return display;
}

QT_END_NAMESPACE

