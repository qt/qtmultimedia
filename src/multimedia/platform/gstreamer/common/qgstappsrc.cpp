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

#include <QDebug>

#include "qgstappsrc_p.h"
#include "qgstutils_p.h"

QGstAppSrc::QGstAppSrc(QObject *parent)
    : QObject(parent)
{
    m_callbacks.need_data   = &QGstAppSrc::on_need_data;
    m_callbacks.enough_data = &QGstAppSrc::on_enough_data;
    m_callbacks.seek_data   = &QGstAppSrc::on_seek_data;
}

QGstAppSrc::~QGstAppSrc()
{
    if (m_appSrc)
        gst_object_unref(G_OBJECT(m_appSrc));
}

bool QGstAppSrc::setup(GstElement* appsrc)
{
    if (m_appSrc) {
        gst_object_unref(G_OBJECT(m_appSrc));
        m_appSrc = nullptr;
    }

    if (!appsrc || (!m_stream && !m_buffer))
        return false;

    m_appSrc = GST_APP_SRC(appsrc);
    gst_object_ref(appsrc);
    gst_app_src_set_callbacks(m_appSrc, (GstAppSrcCallbacks*)&m_callbacks, this, (GDestroyNotify)&QGstAppSrc::destroy_notify);

    m_maxBytes = gst_app_src_get_max_bytes(m_appSrc);

    if (m_sequential)
        m_streamType = GST_APP_STREAM_TYPE_STREAM;
    else
        m_streamType = GST_APP_STREAM_TYPE_RANDOM_ACCESS;
    gst_app_src_set_stream_type(m_appSrc, m_streamType);
    gst_app_src_set_size(m_appSrc, (m_sequential) ? -1 : m_stream->size());
    if (m_format.isValid()) {
        GstCaps *caps = QGstUtils::capsForAudioFormat(m_format);
        if (caps) {
            g_object_set(m_appSrc, "caps",  caps, nullptr);
            g_object_set(m_appSrc, "format", GST_FORMAT_TIME, nullptr);
        } else {
            qWarning() << "QGstAppSrc: Invalid caps";
        }
    }

    return true;
}

void QGstAppSrc::setStream(QIODevice *stream)
{
    if (m_stream) {
        disconnect(m_stream, SIGNAL(readyRead()), this, SLOT(onDataReady()));
        disconnect(m_stream, SIGNAL(destroyed()), this, SLOT(streamDestroyed()));
        m_stream = nullptr;
    }

    if (m_appSrc) {
        gst_object_unref(G_OBJECT(m_appSrc));
        m_appSrc = nullptr;
    }

    m_dataRequestSize = ~0;
    m_dataRequested = false;
    m_enoughData = false;
    m_forceData = false;
    m_sequential = false;
    m_maxBytes = 0;

    if (stream) {
        m_stream = stream;
        connect(m_stream, SIGNAL(destroyed()), SLOT(streamDestroyed()));
        connect(m_stream, SIGNAL(readyRead()), this, SLOT(onDataReady()));
        m_sequential = m_stream->isSequential();
    }
}

QIODevice *QGstAppSrc::stream() const
{
    return m_stream;
}

GstAppSrc *QGstAppSrc::element()
{
    return m_appSrc;
}

void QGstAppSrc::onDataReady()
{
    if (!m_enoughData) {
        m_dataRequested = true;
        pushDataToAppSrc();
    }
}

void QGstAppSrc::streamDestroyed()
{
    if (sender() == m_stream) {
        m_stream = nullptr;
        sendEOS();
    }
}

void QGstAppSrc::pushDataToAppSrc()
{
    if ((!isStreamValid() && !m_buffer) || !m_appSrc)
        return;

    if (m_stream->atEnd()) {
        sendEOS();
        return;
    }

    if (m_dataRequested && !m_enoughData) {
        qint64 size;
        if (m_dataRequestSize == ~0u)
            size = qMin(m_stream->bytesAvailable(), queueSize());
        else
            size = qMin(m_stream->bytesAvailable(), (qint64)m_dataRequestSize);

        GstBuffer* buffer = gst_buffer_new_and_alloc(size);

        if (m_format.isValid()) {
            uint nSamples = size/4;

            GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(streamedSamples, GST_SECOND, 48000);
            GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(nSamples, GST_SECOND, 48000);
            streamedSamples += nSamples;
        }

        GstMapInfo mapInfo;
        gst_buffer_map(buffer, &mapInfo, GST_MAP_WRITE);
        void* bufferData = mapInfo.data;

        buffer->offset = m_stream->pos();
        qint64 bytesRead;
        if (m_buffer)
            bytesRead = m_buffer->read((char*)bufferData, size);
        else
            bytesRead = m_stream->read((char*)bufferData, size);
        buffer->offset_end =  buffer->offset + bytesRead - 1;

        gst_buffer_unmap(buffer, &mapInfo);

        if (bytesRead > 0) {
            m_dataRequested = false;
            m_enoughData = false;
            GstFlowReturn ret = gst_app_src_push_buffer (GST_APP_SRC (element()), buffer);
            if (ret == GST_FLOW_ERROR) {
                qWarning()<<"appsrc: push buffer error";
            } else if (ret == GST_FLOW_FLUSHING) {
                qWarning()<<"appsrc: push buffer wrong state";
            }
        }
    }

    if (m_stream->atEnd())
        sendEOS();
}

bool QGstAppSrc::doSeek(qint64 value)
{
    if (isStreamValid())
        return stream()->seek(value);
    return false;
}


gboolean QGstAppSrc::on_seek_data(GstAppSrc *element, guint64 arg0, gpointer userdata)
{
    // we do get some spurious seeks to INT_MAX, ignore those
    if (arg0 == std::numeric_limits<quint64>::max())
        return true;
    Q_UNUSED(element);
    QGstAppSrc *self = reinterpret_cast<QGstAppSrc*>(userdata);
    if (self && self->isStreamValid()) {
        if (!self->stream()->isSequential())
            QMetaObject::invokeMethod(self, "doSeek", Qt::AutoConnection, Q_ARG(qint64, arg0));
    }
    else
        return false;

    return true;
}

void QGstAppSrc::on_enough_data(GstAppSrc *element, gpointer userdata)
{
    Q_UNUSED(element);
    QGstAppSrc *self = static_cast<QGstAppSrc*>(userdata);
    if (self)
        self->m_enoughData = true;
}

void QGstAppSrc::on_need_data(GstAppSrc *element, guint arg0, gpointer userdata)
{
    Q_UNUSED(element);
    QGstAppSrc *self = static_cast<QGstAppSrc*>(userdata);
    if (self) {
        self->m_dataRequested = true;
        self->m_enoughData = false;
        self->m_dataRequestSize = arg0;
        QMetaObject::invokeMethod(self, "pushDataToAppSrc", Qt::AutoConnection);
    }
}

void QGstAppSrc::destroy_notify(gpointer data)
{
    Q_UNUSED(data);
}

void QGstAppSrc::sendEOS()
{
    if (!m_appSrc)
        return;

    gst_app_src_end_of_stream(GST_APP_SRC(m_appSrc));
    if (isStreamValid() && !stream()->isSequential())
        stream()->reset();
}
