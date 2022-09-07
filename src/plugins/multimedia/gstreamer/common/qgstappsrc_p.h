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

#include <qgst_p.h>
#include <gst/app/gstappsrc.h>

QT_BEGIN_NAMESPACE

class QNetworkReply;

class Q_MULTIMEDIA_EXPORT QGstAppSrc  : public QObject
{
    Q_OBJECT
public:
    static QMaybe<QGstAppSrc *> create(QObject *parent = nullptr);
    ~QGstAppSrc();

    bool setup(QIODevice *stream = nullptr, qint64 offset = 0);
    void setAudioFormat(const QAudioFormat &f);

    void setExternalAppSrc(const QGstElement &appsrc);
    QGstElement element();

    void write(const char *data, qsizetype size);

    bool canAcceptMoreData() { return m_noMoreData || m_dataRequestSize != 0; }

    void suspend() { m_suspended = true; }
    void resume() { m_suspended = false; m_noMoreData = true; }

Q_SIGNALS:
    void bytesProcessed(int bytes);
    void noMoreData();

private Q_SLOTS:
    void pushData();
    bool doSeek(qint64);
    void onDataReady();

    void streamDestroyed();
private:
    QGstAppSrc(QGstElement appsrc, QObject *parent);

    bool setStream(QIODevice *, qint64 offset);
    bool isStreamValid() const
    {
        return m_stream != nullptr && m_stream->isOpen();
    }

    static gboolean on_seek_data(GstAppSrc *element, guint64 arg0, gpointer userdata);
    static void on_enough_data(GstAppSrc *element, gpointer userdata);
    static void on_need_data(GstAppSrc *element, uint arg0, gpointer userdata);

    void sendEOS();
    void eosOrIdle();

    QIODevice *m_stream = nullptr;
    QNetworkReply *m_networkReply = nullptr;
    QRingBuffer m_buffer;
    QAudioFormat m_format;

    QGstElement m_appSrc;
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
