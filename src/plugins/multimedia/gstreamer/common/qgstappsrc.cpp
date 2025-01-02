// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QDebug>

#include "qgstappsrc_p.h"
#include "qgstutils_p.h"
#include "qnetworkreply.h"
#include "qloggingcategory.h"

static Q_LOGGING_CATEGORY(qLcAppSrc, "qt.multimedia.appsrc")

QT_BEGIN_NAMESPACE

QMaybe<QGstAppSrc *> QGstAppSrc::create(QObject *parent)
{
    QGstElement appsrc("appsrc", "appsrc");
    if (!appsrc)
        return errorMessageCannotFindElement("appsrc");

    return new QGstAppSrc(appsrc, parent);
}

QGstAppSrc::QGstAppSrc(QGstElement appsrc, QObject *parent)
    : QObject(parent), m_appSrc(std::move(appsrc))
{
}

QGstAppSrc::~QGstAppSrc()
{
    m_appSrc.setStateSync(GST_STATE_NULL);
    streamDestroyed();
    qCDebug(qLcAppSrc) << "~QGstAppSrc";
}

bool QGstAppSrc::setup(QIODevice *stream, qint64 offset)
{
    if (m_appSrc.isNull())
        return false;

    if (!setStream(stream, offset))
        return false;

    auto *appSrc = GST_APP_SRC(m_appSrc.element());
    GstAppSrcCallbacks m_callbacks;
    memset(&m_callbacks, 0, sizeof(GstAppSrcCallbacks));
    m_callbacks.need_data   = &QGstAppSrc::on_need_data;
    m_callbacks.enough_data = &QGstAppSrc::on_enough_data;
    m_callbacks.seek_data   = &QGstAppSrc::on_seek_data;
    gst_app_src_set_callbacks(appSrc, (GstAppSrcCallbacks*)&m_callbacks, this, nullptr);

    m_maxBytes = gst_app_src_get_max_bytes(appSrc);
    m_suspended = false;

    if (m_sequential)
        m_streamType = GST_APP_STREAM_TYPE_STREAM;
    else
        m_streamType = GST_APP_STREAM_TYPE_RANDOM_ACCESS;
    gst_app_src_set_stream_type(appSrc, m_streamType);
    gst_app_src_set_size(appSrc, m_sequential ? -1 : m_stream->size() - m_offset);

    m_networkReply = qobject_cast<QNetworkReply *>(m_stream);
    m_noMoreData = true;

    return true;
}

void QGstAppSrc::setAudioFormat(const QAudioFormat &f)
{
    m_format = f;
    if (!m_format.isValid())
        return;

    auto caps = QGstUtils::capsForAudioFormat(m_format);
    Q_ASSERT(!caps.isNull());
    m_appSrc.set("caps", caps);
    m_appSrc.set("format", GST_FORMAT_TIME);
}

void QGstAppSrc::setExternalAppSrc(const QGstElement &appsrc)
{
    m_appSrc = appsrc;
}

bool QGstAppSrc::setStream(QIODevice *stream, qint64 offset)
{
    if (m_stream) {
        disconnect(m_stream, SIGNAL(readyRead()), this, SLOT(onDataReady()));
        disconnect(m_stream, SIGNAL(destroyed()), this, SLOT(streamDestroyed()));
        m_stream = nullptr;
    }

    m_dataRequestSize = 0;
    m_sequential = true;
    m_maxBytes = 0;
    streamedSamples = 0;

    if (stream) {
        if (!stream->isOpen() && !stream->open(QIODevice::ReadOnly))
            return false;
        m_stream = stream;
        connect(m_stream, SIGNAL(destroyed()), SLOT(streamDestroyed()));
        connect(m_stream, SIGNAL(readyRead()), this, SLOT(onDataReady()));
        m_sequential = m_stream->isSequential();
        m_offset = offset;
    }
    return true;
}

QGstElement QGstAppSrc::element()
{
    return m_appSrc;
}

void QGstAppSrc::write(const char *data, qsizetype size)
{
    qCDebug(qLcAppSrc) << "write" << size << m_noMoreData << m_dataRequestSize;
    if (!size)
        return;
    Q_ASSERT(!m_stream);
    m_buffer.append(data, size);
    m_noMoreData = false;
    pushData();
}

void QGstAppSrc::onDataReady()
{
    qCDebug(qLcAppSrc) << "onDataReady" << m_stream->bytesAvailable() << m_stream->size();
    pushData();
}

void QGstAppSrc::streamDestroyed()
{
    qCDebug(qLcAppSrc) << "stream destroyed";
    m_stream = nullptr;
    m_dataRequestSize = 0;
    streamedSamples = 0;
    sendEOS();
}

void QGstAppSrc::pushData()
{
    if (m_appSrc.isNull() || !m_dataRequestSize || m_suspended) {
        qCDebug(qLcAppSrc) << "push data: return immediately" << m_appSrc.isNull() << m_dataRequestSize << m_suspended;
        return;
    }

    qCDebug(qLcAppSrc) << "pushData" << (m_stream ? m_stream : nullptr) << m_buffer.size();
    if ((m_stream && m_stream->atEnd())) {
        eosOrIdle();
        qCDebug(qLcAppSrc) << "end pushData" << (m_stream ? m_stream : nullptr) << m_buffer.size();
        return;
    }

    qint64 size;
    if (m_stream)
        size = m_stream->bytesAvailable();
    else
        size = m_buffer.size();

    if (!m_dataRequestSize)
        m_dataRequestSize = m_maxBytes;
    size = qMin(size, (qint64)m_dataRequestSize);
    qCDebug(qLcAppSrc) << "    reading" << size << "bytes" << size << m_dataRequestSize;

    GstBuffer* buffer = gst_buffer_new_and_alloc(size);

    if (m_sequential || !m_stream)
        buffer->offset = bytesReadSoFar;
    else
        buffer->offset = m_stream->pos();

    if (m_format.isValid()) {
        // timestamp raw audio data
        uint nSamples = size/m_format.bytesPerFrame();

        GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(streamedSamples, GST_SECOND, m_format.sampleRate());
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(nSamples, GST_SECOND, m_format.sampleRate());
        streamedSamples += nSamples;
    }

    GstMapInfo mapInfo;
    gst_buffer_map(buffer, &mapInfo, GST_MAP_WRITE);
    void* bufferData = mapInfo.data;

    qint64 bytesRead;
    if (m_stream)
        bytesRead = m_stream->read((char*)bufferData, size);
    else
        bytesRead = m_buffer.read((char*)bufferData, size);
    buffer->offset_end =  buffer->offset + bytesRead - 1;
    bytesReadSoFar += bytesRead;

    gst_buffer_unmap(buffer, &mapInfo);
    qCDebug(qLcAppSrc) << "pushing bytes into gstreamer" << buffer->offset << bytesRead;
    if (bytesRead == 0) {
        gst_buffer_unref(buffer);
        eosOrIdle();
        qCDebug(qLcAppSrc) << "end pushData" << (m_stream ? m_stream : nullptr) << m_buffer.size();
        return;
    }
    m_noMoreData = false;
    emit bytesProcessed(bytesRead);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(m_appSrc.element()), buffer);
    if (ret == GST_FLOW_ERROR) {
        qWarning() << "QGstAppSrc: push buffer error";
    } else if (ret == GST_FLOW_FLUSHING) {
        qWarning() << "QGstAppSrc: push buffer wrong state";
    }
    qCDebug(qLcAppSrc) << "end pushData" << (m_stream ? m_stream : nullptr) << m_buffer.size();

}

bool QGstAppSrc::doSeek(qint64 value)
{
    if (isStreamValid())
        return m_stream->seek(value + m_offset);
    return false;
}


gboolean QGstAppSrc::on_seek_data(GstAppSrc *, guint64 arg0, gpointer userdata)
{
    // we do get some spurious seeks to INT_MAX, ignore those
    if (arg0 == std::numeric_limits<quint64>::max())
        return true;

    QGstAppSrc *self = reinterpret_cast<QGstAppSrc*>(userdata);
    Q_ASSERT(self);
    if (self->m_sequential)
        return false;

    QMetaObject::invokeMethod(self, "doSeek", Qt::AutoConnection, Q_ARG(qint64, arg0));
    return true;
}

void QGstAppSrc::on_enough_data(GstAppSrc *, gpointer userdata)
{
    qCDebug(qLcAppSrc) << "on_enough_data";
    QGstAppSrc *self = static_cast<QGstAppSrc*>(userdata);
    Q_ASSERT(self);
    self->m_dataRequestSize = 0;
}

void QGstAppSrc::on_need_data(GstAppSrc *, guint arg0, gpointer userdata)
{
    qCDebug(qLcAppSrc) << "on_need_data requesting bytes" << arg0;
    QGstAppSrc *self = static_cast<QGstAppSrc*>(userdata);
    Q_ASSERT(self);
    self->m_dataRequestSize = arg0;
    QMetaObject::invokeMethod(self, "pushData", Qt::AutoConnection);
    qCDebug(qLcAppSrc) << "done on_need_data";
}

void QGstAppSrc::sendEOS()
{
    qCDebug(qLcAppSrc) << "sending EOS";
    if (m_appSrc.isNull())
        return;

    gst_app_src_end_of_stream(GST_APP_SRC(m_appSrc.element()));
}

void QGstAppSrc::eosOrIdle()
{
    qCDebug(qLcAppSrc) << "eosOrIdle";
    if (m_appSrc.isNull())
        return;

    if (!m_sequential) {
        sendEOS();
        return;
    }
    if (m_noMoreData)
        return;
    qCDebug(qLcAppSrc) << "    idle!";
    m_noMoreData = true;
    emit noMoreData();
}

QT_END_NAMESPACE

#include "moc_qgstappsrc_p.cpp"
