// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstappsource_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

#include <common/qgstutils_p.h>

Q_STATIC_LOGGING_CATEGORY(qLcAppSrc, "qt.multimedia.appsrc");

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
    gst_app_src_set_max_bytes(appSrc, maxBytes);

    if (m_sequential) {
        gst_app_src_set_stream_type(appSrc, GST_APP_STREAM_TYPE_STREAM);
        gst_app_src_set_size(appSrc, -1);
    } else {
        gst_app_src_set_stream_type(appSrc, GST_APP_STREAM_TYPE_RANDOM_ACCESS);
        gst_app_src_set_size(appSrc, m_stream->size() - m_offset);
    }

    return true;
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

    m_sequential = true;

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

void QGstAppSource::onDataReady()
{
    QMutexLocker locker(&m_mutex);
    qCDebug(qLcAppSrc) << "onDataReady" << m_stream->bytesAvailable() << m_stream->size();
    pushData(m_stream->bytesAvailable());
}

void QGstAppSource::streamDestroyed()
{
    QMutexLocker locker(&m_mutex);
    qCDebug(qLcAppSrc) << "stream destroyed";
    m_stream = nullptr;
    sendEOS();
}

void QGstAppSource::pushData(qint64 bytesToRead)
{
    if (m_appSrc.isNull()) {
        qCDebug(qLcAppSrc) << "push data: return immediately - no GstAppSource";
        return;
    }

    if (!m_stream) {
        qCDebug(qLcAppSrc) << "push data: return immediately - stream was destroyed";
        return;
    }

    if (!m_dataNeeded) {
        qCDebug(qLcAppSrc) << "push data: return immediately - no data needed";
        return;
    }

    Q_ASSERT(m_stream);

    qCDebug(qLcAppSrc) << "pushData" << m_stream;
    if ((m_stream && m_stream->atEnd())) {
        sendEOS();
        qCDebug(qLcAppSrc) << "end pushData" << m_stream;
        return;
    }

    qint64 size = m_stream->bytesAvailable();

    size = qMin(size, bytesToRead);
    qCDebug(qLcAppSrc) << "    reading" << size << "bytes of requested" << bytesToRead;

    GstBuffer *buffer = gst_buffer_new_and_alloc(size);

    if (m_sequential)
        buffer->offset = bytesReadSoFar;
    else
        buffer->offset = m_stream->pos();

    GstMapInfo mapInfo;
    gst_buffer_map(buffer, &mapInfo, GST_MAP_WRITE);
    void* bufferData = mapInfo.data;

    qint64 bytesRead;
    bytesRead = m_stream->read((char *)bufferData, size);

    buffer->offset_end = buffer->offset + bytesRead - 1;
    bytesReadSoFar += bytesRead;

    gst_buffer_unmap(buffer, &mapInfo);
    qCDebug(qLcAppSrc) << "pushing bytes into gstreamer" << buffer->offset << bytesRead;
    if (bytesRead == 0) {
        gst_buffer_unref(buffer);
        sendEOS();
        qCDebug(qLcAppSrc) << "end pushData" << m_stream;
        return;
    }

    GstFlowReturn ret = m_appSrc.pushBuffer(buffer);
    switch (ret) {
    case GST_FLOW_OK:
        break;

    default:
        qWarning() << "QGstAppSrc: push buffer error -" << gst_flow_get_name(ret);
        break;
    }

    qCDebug(qLcAppSrc) << "end pushData" << m_stream;
}

bool QGstAppSource::doSeek(qint64 streamPosition)
{
    if (isStreamValid())
        return m_stream->seek(streamPosition + m_offset);
    return false;
}

gboolean QGstAppSource::on_seek_data(GstAppSrc *, guint64 streamPosition, gpointer userdata)
{
    // we do get some spurious seeks to INT_MAX, ignore those
    if (streamPosition == std::numeric_limits<quint64>::max())
        return true;

    QGstAppSource *self = reinterpret_cast<QGstAppSource *>(userdata);
    Q_ASSERT(self);

    QMutexLocker locker(&self->m_mutex);

    if (self->m_sequential)
        return false;

    self->doSeek(streamPosition);
    return true;
}

void QGstAppSource::on_enough_data(GstAppSrc *, gpointer userdata)
{
    qCWarning(qLcAppSrc) << "on_enough_data";

    QGstAppSource *self = reinterpret_cast<QGstAppSource *>(userdata);
    Q_ASSERT(self);

    QMutexLocker locker(&self->m_mutex);
    self->m_dataNeeded = false;
}

void QGstAppSource::on_need_data(GstAppSrc *, guint numberOfBytes, gpointer userdata)
{
    qCDebug(qLcAppSrc) << "on_need_data requesting bytes" << numberOfBytes;
    QGstAppSource *self = static_cast<QGstAppSource *>(userdata);
    Q_ASSERT(self);
    QMutexLocker locker(&self->m_mutex);
    self->m_dataNeeded = true;
    self->pushData(numberOfBytes);
    qCDebug(qLcAppSrc) << "done on_need_data";
}

void QGstAppSource::sendEOS()
{
    qCDebug(qLcAppSrc) << "sending EOS";
    if (m_appSrc.isNull())
        return;

    gst_app_src_end_of_stream(GST_APP_SRC(m_appSrc.element()));
}

QT_END_NAMESPACE

#include "moc_qgstappsource_p.cpp"
