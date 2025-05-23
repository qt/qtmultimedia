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

#include <private/qtmultimediaglobal_p.h>
#include <private/qmultimediautils_p.h>
#include <qaudioformat.h>

#include <QtCore/qobject.h>
#include <QtCore/qiodevice.h>
#include <QtCore/private/qringbuffer_p.h>
#include <QtCore/qatomic.h>
#include <QtCore/qmutex.h>

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
    void setAudioFormat(const QAudioFormat &f);

    void setExternalAppSrc(QGstAppSrc);
    QGstElement element() const;

    void write(const char *data, qsizetype size);

    bool canAcceptMoreData() const;

    void suspend();
    void resume();

Q_SIGNALS:
    void bytesProcessed(int bytes);
    void noMoreData();

private Q_SLOTS:
    void pushData();
    bool doSeek(qint64);
    void onDataReady();

    void streamDestroyed();
private:
    QGstAppSource(QGstAppSrc appsrc, QObject *parent);

    bool setStream(QIODevice *, qint64 offset);
    bool isStreamValid() const;

    static gboolean on_seek_data(GstAppSrc *element, guint64 arg0, gpointer userdata);
    static void on_enough_data(GstAppSrc *element, gpointer userdata);
    static void on_need_data(GstAppSrc *element, uint arg0, gpointer userdata);

    void sendEOS();
    void eosOrIdle();

    mutable QMutex m_mutex;

    QIODevice *m_stream = nullptr;
    QRingBuffer m_buffer;
    QAudioFormat m_format;

    QGstAppSrc m_appSrc;
    bool m_sequential = true;
    bool m_suspended = false;
    bool m_noMoreData = false;
    GstAppStreamType m_streamType = GST_APP_STREAM_TYPE_RANDOM_ACCESS;
    qint64 m_offset = 0;
    qint64 m_maxBytes = 0;
    qint64 bytesReadSoFar = 0;
    QAtomicInteger<unsigned int> m_dataRequestSize = 0;
    int streamedSamples = 0;
};

QT_END_NAMESPACE

#endif
