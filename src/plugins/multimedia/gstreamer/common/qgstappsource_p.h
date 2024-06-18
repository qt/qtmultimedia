// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTAPPSRC_H
#define QGSTAPPSRC_H

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


#include <QtCore/qobject.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qatomic.h>
#include <QtCore/qmutex.h>

#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <common/qgst_p.h>
#include <gst/app/gstappsrc.h>

QT_BEGIN_NAMESPACE

class QGstAppSource : public QObject
{
    Q_OBJECT
public:
    static QMaybe<QGstAppSource *> create(QObject *parent = nullptr);
    ~QGstAppSource();

    bool setup(QIODevice *stream = nullptr, qint64 offset = 0);

    void setExternalAppSrc(QGstAppSrc);
    QGstElement element() const;

private Q_SLOTS:
    void onDataReady();
    void streamDestroyed();

private:
    bool doSeek(qint64);
    void pushData();

    QGstAppSource(QGstAppSrc appsrc, QObject *parent);

    bool setStream(QIODevice *, qint64 offset);
    bool isStreamValid() const;

    static gboolean on_seek_data(GstAppSrc *element, guint64 arg0, gpointer userdata);
    static void on_enough_data(GstAppSrc *element, gpointer userdata);
    static void on_need_data(GstAppSrc *element, uint arg0, gpointer userdata);

    void sendEOS();

    mutable QMutex m_mutex;

    QIODevice *m_stream = nullptr;

    QGstAppSrc m_appSrc;
    bool m_sequential = true;
    GstAppStreamType m_streamType = GST_APP_STREAM_TYPE_RANDOM_ACCESS;
    qint64 m_offset = 0;
    qint64 m_maxBytes = 0;
    qint64 bytesReadSoFar = 0;
    QAtomicInteger<unsigned int> m_dataRequestSize = 0;
};

QT_END_NAMESPACE

#endif
