// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QDebug>

#include "qgstappsource_p.h"
#include <common/qgstutils_p.h>
#include "qnetworkreply.h"
#include "qloggingcategory.h"

static Q_LOGGING_CATEGORY(qLcAppSrc, "qt.multimedia.appsrc")

QT_BEGIN_NAMESPACE

QMaybe<QGstAppSource *> QGstAppSource::create(QObject *parent)
{
    QGstAppSrc appsrc = QGstAppSrc::create("appsrc");
    if (!appsrc)
        return qGstErrorMessageCannotFindElement("appsrc");

    return new QGstAppSource(appsrc, parent);
}

QGstAppSource::QGstAppSource(QGstAppSrc appsrc, QObject *parent)
    : QObject(parent), m_appSrc(std::move(appsrc))
{
    m_appSrc.set("emit-signals", false);
}

QGstAppSource::~QGstAppSource()
{
    m_appSrc.setStateSync(GST_STATE_NULL);
    streamDestroyed();
    qCDebug(qLcAppSrc) << "~QGstAppSrc";
}

bool QGstAppSource::setup(QIODevice *stream, qint64 offset)
{
    QMutexLocker locker(&m_mutex);

    if (m_appSrc.isNull())
        return false;

    if (!setStream(stream, offset))
        return false;

    GstAppSrcCallbacks callbacks{};
    callbacks.need_data = QGstAppSource::on_need_data;
    callbacks.enough_data = QGstAppSource::on_enough_data;
    callbacks.seek_data = QGstAppSource::on_seek_data;

    m_appSrc.setCallbacks(callbacks, this, nullptr);

    GstAppSrc *appSrc = m_appSrc.appSrc();
    m_maxBytes = gst_app_src_get_max_bytes(appSrc);
    m_suspended = false;

    if (m_sequential)
        m_streamType = GST_APP_STREAM_TYPE_STREAM;
    else
        m_streamType = GST_APP_STREAM_TYPE_RANDOM_ACCESS;
    gst_app_src_set_stream_type(appSrc, m_streamType);
    gst_app_src_set_size(appSrc, m_sequential ? -1 : m_stream->size() - m_offset);

    m_noMoreData = true;

    return true;
}

void QGstAppSource::setAudioFormat(const QAudioFormat &f)
{
    QMutexLocker locker(&m_mutex);

    m_format = f;
    if (!m_format.isValid())
        return;

    auto caps = QGstUtils::capsForAudioFormat(m_format);
    Q_ASSERT(!caps.isNull());
    m_appSrc.set("caps", caps);
    m_appSrc.set("format", GST_FORMAT_TIME);
}

void QGstAppSource::setExternalAppSrc(QGstAppSrc appsrc)
{
    QMutexLocker locker(&m_mutex);
    m_appSrc = std::move(appsrc);
}

bool QGstAppSource::setStream(QIODevice *stream, qint64 offset)
{
    if (m_stream) {
        disconnect(m_stream, &QIODevice::readyRead, this, &QGstAppSource::onDataReady);
        disconnect(m_stream, &QIODevice::destroyed, this, &QGstAppSource::streamDestroyed);
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
        connect(m_stream, &QIODevice::destroyed, this, &QGstAppSource::streamDestroyed);
        connect(m_stream, &QIODevice::readyRead, this, &QGstAppSource::onDataReady);
        m_sequential = m_stream->isSequential();
        m_offset = offset;
    }
    return true;
}

bool QGstAppSource::isStreamValid() const
{
    return m_stream != nullptr && m_stream->isOpen();
}

QGstElement QGstAppSource::element() const
{
    return m_appSrc;
}

void QGstAppSource::write(const char *data, qsizetype size)
{
    QMutexLocker locker(&m_mutex);

    qCDebug(qLcAppSrc) << "write" << size << m_noMoreData << m_dataRequestSize;
    if (!size)
        return;
    Q_ASSERT(!m_stream);
    m_buffer.append(data, size);
    m_noMoreData = false;
    pushData();
}

bool QGstAppSource::canAcceptMoreData() const
{
    QMutexLocker locker(&m_mutex);
    return m_noMoreData || m_dataRequestSize != 0;
}

void QGstAppSource::suspend()
{
    QMutexLocker locker(&m_mutex);
    m_suspended = true;
}

void QGstAppSource::resume()
{
    QMutexLocker locker(&m_mutex);
    m_suspended = false;
    m_noMoreData = true;
}

void QGstAppSource::onDataReady()
{
    qCDebug(qLcAppSrc) << "onDataReady" << m_stream->bytesAvailable() << m_stream->size();
    pushData();
}

void QGstAppSource::streamDestroyed()
{
    qCDebug(qLcAppSrc) << "stream destroyed";
    m_stream = nullptr;
    m_dataRequestSize = 0;
    streamedSamples = 0;
    sendEOS();
}

void QGstAppSource::pushData()
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

    GstFlowReturn ret = m_appSrc.pushBuffer(buffer);
    if (ret == GST_FLOW_ERROR) {
        qWarning() << "QGstAppSrc: push buffer error";
    } else if (ret == GST_FLOW_FLUSHING) {
        qWarning() << "QGstAppSrc: push buffer wrong state";
    }
    qCDebug(qLcAppSrc) << "end pushData" << (m_stream ? m_stream : nullptr) << m_buffer.size();

}

bool QGstAppSource::doSeek(qint64 value)
{
    if (isStreamValid())
        return m_stream->seek(value + m_offset);
    return false;
}

gboolean QGstAppSource::on_seek_data(GstAppSrc *, guint64 arg0, gpointer userdata)
{
    // we do get some spurious seeks to INT_MAX, ignore those
    if (arg0 == std::numeric_limits<quint64>::max())
        return true;

    QGstAppSource *self = reinterpret_cast<QGstAppSource *>(userdata);
    Q_ASSERT(self);

    QMutexLocker locker(&self->m_mutex);

    if (self->m_sequential)
        return false;

    self->doSeek(arg0);
    return true;
}

void QGstAppSource::on_enough_data(GstAppSrc *, gpointer userdata)
{
    qCDebug(qLcAppSrc) << "on_enough_data";
    QGstAppSource *self = static_cast<QGstAppSource *>(userdata);
    Q_ASSERT(self);
    QMutexLocker locker(&self->m_mutex);
    self->m_dataRequestSize = 0;
}

void QGstAppSource::on_need_data(GstAppSrc *, guint arg0, gpointer userdata)
{
    qCDebug(qLcAppSrc) << "on_need_data requesting bytes" << arg0;
    QGstAppSource *self = static_cast<QGstAppSource *>(userdata);
    Q_ASSERT(self);
    QMutexLocker locker(&self->m_mutex);
    self->m_dataRequestSize = arg0;
    self->pushData();
    qCDebug(qLcAppSrc) << "done on_need_data";
}

void QGstAppSource::sendEOS()
{
    qCDebug(qLcAppSrc) << "sending EOS";
    if (m_appSrc.isNull())
        return;

    gst_app_src_end_of_stream(GST_APP_SRC(m_appSrc.element()));
}

void QGstAppSource::eosOrIdle()
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

#include "moc_qgstappsource_p.cpp"
