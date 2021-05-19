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
#include <qaudioformat.h>

#include <QtCore/qobject.h>
#include <QtCore/qiodevice.h>
#include <QtCore/private/qringbuffer_p.h>
#include <QtCore/qatomic.h>

#include <private/qgst_p.h>
#include <gst/app/gstappsrc.h>

QT_BEGIN_NAMESPACE

class QNetworkReply;

class Q_MULTIMEDIA_EXPORT QGstAppSrc  : public QObject
{
    Q_OBJECT
public:
    QGstAppSrc(QObject *parent = 0);
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
