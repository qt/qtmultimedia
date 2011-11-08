/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QGSTXVIMAGEBUFFER_P_H
#define QGSTXVIMAGEBUFFER_P_H

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

#include <qabstractvideobuffer.h>
#include <qvideosurfaceformat.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qqueue.h>

#ifndef QT_NO_XVIDEO

#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>


#include <gst/gst.h>
#include "qgstbufferpoolinterface_p.h"

class QGstXvImageBufferPool;

struct QGstXvImageBuffer {
    GstBuffer buffer;
    QGstXvImageBufferPool *pool;
    XvImage *xvImage;
    XShmSegmentInfo shmInfo;
    bool markedForDeletion;

    static GType get_type(void);
    static void class_init(gpointer g_class, gpointer class_data);
    static void buffer_init(QGstXvImageBuffer *xvimage, gpointer g_class);
    static void buffer_finalize(QGstXvImageBuffer * xvimage);
    static GstBufferClass *parent_class;
};

Q_DECLARE_METATYPE(XvImage*)

class QGstXvImageBufferPool : public QObject, public QGstBufferPoolInterface {
Q_OBJECT
friend class QGstXvImageBuffer;
public:
    QGstXvImageBufferPool(QObject *parent = 0);
    virtual ~QGstXvImageBufferPool();

    bool isFormatSupported(const QVideoSurfaceFormat &format) const;

    GType bufferType() const;
    GstBuffer *takeBuffer(const QVideoSurfaceFormat &format, GstCaps *caps);
    void clear();

    QAbstractVideoBuffer::HandleType handleType() const;
    QAbstractVideoBuffer *prepareVideoBuffer(GstBuffer *buffer, int bytesPerLine);

private slots:
    void queuedAlloc();
    void queuedDestroy();

    void doClear();

    void recycleBuffer(QGstXvImageBuffer *);
    void destroyBuffer(QGstXvImageBuffer *);

private:
    void doAlloc();

    struct XvShmImage {
        XvImage *xvImage;
        XShmSegmentInfo shmInfo;
    };

    QMutex m_poolMutex;
    QMutex m_allocMutex;
    QWaitCondition m_allocWaitCondition;
    QMutex m_destroyMutex;
    QVideoSurfaceFormat m_format;
    GstCaps *m_caps;
    QList<QGstXvImageBuffer*> m_pool;
    QList<QGstXvImageBuffer*> m_allBuffers;
    QList<XvShmImage> m_imagesToDestroy;
    Qt::HANDLE m_threadId;
};

#endif //QT_NO_XVIDEO

#endif
