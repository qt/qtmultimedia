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

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmath.h>
#include <private/qaudiohelpers_p.h>

#include "qaudioinput_gstreamer_p.h"
#include "qaudiodeviceinfo_gstreamer_p.h"
#include <sys/types.h>
#include <unistd.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

QGStreamerAudioInput::QGStreamerAudioInput(const QByteArray &device)
    : m_device(device)
{
}

QGStreamerAudioInput::~QGStreamerAudioInput()
{
    close();
}

void QGStreamerAudioInput::setError(QAudio::Error error)
{
    if (m_errorState == error)
        return;

    m_errorState = error;
    emit errorChanged(error);
}

QAudio::Error QGStreamerAudioInput::error() const
{
    return m_errorState;
}

void QGStreamerAudioInput::setState(QAudio::State state)
{
    if (m_deviceState == state)
        return;

    m_deviceState = state;
    emit stateChanged(state);
}

QAudio::State QGStreamerAudioInput::state() const
{
    return m_deviceState;
}

void QGStreamerAudioInput::setFormat(const QAudioFormat &format)
{
    if (m_deviceState == QAudio::StoppedState)
        m_format = format;
}

QAudioFormat QGStreamerAudioInput::format() const
{
    return m_format;
}

void QGStreamerAudioInput::start(QIODevice *device)
{
    setState(QAudio::StoppedState);
    setError(QAudio::NoError);

    close();

    if (!open())
        return;

    m_pullMode = true;
    m_audioSink = device;

    setState(QAudio::ActiveState);
}

QIODevice *QGStreamerAudioInput::start()
{
    setState(QAudio::StoppedState);
    setError(QAudio::NoError);

    close();

    if (!open())
        return nullptr;

    m_pullMode = false;
    m_audioSink = new GStreamerInputPrivate(this);
    m_audioSink->open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    setState(QAudio::IdleState);

    return m_audioSink;
}

void QGStreamerAudioInput::stop()
{
    if (m_deviceState == QAudio::StoppedState)
        return;

    close();

    setError(QAudio::NoError);
    setState(QAudio::StoppedState);
}

bool QGStreamerAudioInput::open()
{
    if (m_opened)
        return true;

    QGStreamerAudioDeviceInfo deviceInfo(m_device, QAudio::AudioInput);
    if (!deviceInfo.gstDevice) {
        setError(QAudio::OpenError);
        setState(QAudio::StoppedState);
        return false;
    }

    gstInput = QGstElement(gst_device_create_element(deviceInfo.gstDevice, nullptr));
    if (gstInput.isNull()) {
        setError(QAudio::OpenError);
        setState(QAudio::StoppedState);
        return false;
    }

    auto *gstCaps = QGstUtils::capsForAudioFormat(m_format);

    if (!gstCaps) {
        setError(QAudio::OpenError);
        setState(QAudio::StoppedState);
        return false;
    }


#ifdef DEBUG_AUDIO
    qDebug() << "Opening input" << QTime::currentTime();
    qDebug() << "Caps: " << gst_caps_to_string(gstCaps);
#endif

    gstPipeline = QGstPipeline("pipeline");

    auto *gstBus = gst_pipeline_get_bus(gstPipeline.pipeline());
    gst_bus_add_watch(gstBus, &QGStreamerAudioInput::busMessage, this);
    gst_object_unref (gstBus);

    gstAppSink = createAppSink();
    g_object_set(gstAppSink.object(), "caps", gstCaps, nullptr);

    QGstElement conv("audioconvert", "conv");
    gstVolume = QGstElement("volume", "volume");
    if (m_volume != 1.)
        gstVolume.set("volume", m_volume);

    gstPipeline.add(gstInput, gstVolume, conv, gstAppSink);
    gstInput.link(gstVolume, conv, gstAppSink);

    gstPipeline.setState(GST_STATE_PLAYING);

    m_opened = true;

    m_clockStamp.restart();
    m_timeStamp.restart();
    m_elapsedTimeOffset = 0;
    m_totalTimeValue = 0;

    return true;
}

void QGStreamerAudioInput::close()
{
    if (!m_opened)
        return;

    gstPipeline.setState(GST_STATE_NULL);
    gstPipeline = {};
    gstVolume = {};
    gstAppSink = {};
    gstInput = {};

    if (!m_pullMode && m_audioSink) {
        delete m_audioSink;
    }
    m_audioSink = nullptr;
    m_opened = false;
}

gboolean QGStreamerAudioInput::busMessage(GstBus *, GstMessage *msg, gpointer user_data)
{
    QGStreamerAudioInput *input = static_cast<QGStreamerAudioInput *>(user_data);
    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
        input->stop();
        break;
    case GST_MESSAGE_ERROR: {
        input->setError(QAudio::IOError);
        gchar  *debug;
        GError *error;

        gst_message_parse_error (msg, &error, &debug);
        g_free (debug);

        qDebug("Error: %s\n", error->message);
        g_error_free (error);

        break;
    }
    default:
        break;
    }
    return false;
}

int QGStreamerAudioInput::bytesReady() const
{
    return m_buffer.size();
}

void QGStreamerAudioInput::resume()
{
    if (m_deviceState == QAudio::SuspendedState || m_deviceState == QAudio::IdleState) {
        gstPipeline.setState(GST_STATE_PLAYING);

        setState(QAudio::ActiveState);
        setError(QAudio::NoError);
    }
}

void QGStreamerAudioInput::setVolume(qreal vol)
{
    if (m_volume == vol)
        return;

    m_volume = vol;
    if (!gstVolume.isNull())
        gstVolume.set("volume", vol);
}

qreal QGStreamerAudioInput::volume() const
{
    return m_volume;
}

void QGStreamerAudioInput::setBufferSize(int value)
{
    m_bufferSize = value;
}

int QGStreamerAudioInput::bufferSize() const
{
    return m_bufferSize;
}

qint64 QGStreamerAudioInput::processedUSecs() const
{
    return 0;
}

void QGStreamerAudioInput::suspend()
{
    if (m_deviceState == QAudio::ActiveState) {
        setError(QAudio::NoError);
        setState(QAudio::SuspendedState);

        gstPipeline.setState(GST_STATE_PLAYING);
    }
}

qint64 QGStreamerAudioInput::elapsedUSecs() const
{
    if (m_deviceState == QAudio::StoppedState)
        return 0;

    return m_clockStamp.elapsed() * qint64(1000);
}

void QGStreamerAudioInput::reset()
{
    stop();
    m_bytesAvailable = 0;
}

//#define MAX_BUFFERS_IN_QUEUE 4

QGstElement QGStreamerAudioInput::createAppSink()
{
    QGstElement sink("appsink", "appsink");
    GstAppSink *appSink = reinterpret_cast<GstAppSink *>(sink.element());

    GstAppSinkCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.eos = &eos;
    callbacks.new_sample = &new_sample;
    gst_app_sink_set_callbacks(appSink, &callbacks, this, nullptr);
//    gst_app_sink_set_max_buffers(appSink, MAX_BUFFERS_IN_QUEUE);
    gst_base_sink_set_sync(GST_BASE_SINK(appSink), FALSE);

    return sink;
}

GstFlowReturn QGStreamerAudioInput::new_sample(GstAppSink *sink, gpointer user_data)
{
    // "Note that the preroll buffer will also be returned as the first buffer when calling gst_app_sink_pull_buffer()."
    QGStreamerAudioInput *control = static_cast<QGStreamerAudioInput*>(user_data);

    GstSample *sample = gst_app_sink_pull_sample(sink);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo mapInfo;
    gst_buffer_map(buffer, &mapInfo, GST_MAP_READ);
    const char *bufferData = (const char*)mapInfo.data;
    gsize bufferSize = mapInfo.size;

    if (!control->m_pullMode) {
        // need to store that data in the QBuffer
        control->m_buffer.append(bufferData, bufferSize);
        control->m_audioSink->readyRead();
    } else {
        control->m_audioSink->write(bufferData, bufferSize);
    }

    gst_buffer_unmap(buffer, &mapInfo);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

void QGStreamerAudioInput::eos(GstAppSink *, gpointer user_data)
{
    QGStreamerAudioInput *control = static_cast<QGStreamerAudioInput*>(user_data);
    control->setState(QAudio::StoppedState);
}

GStreamerInputPrivate::GStreamerInputPrivate(QGStreamerAudioInput *audio)
{
    m_audioDevice = qobject_cast<QGStreamerAudioInput*>(audio);
}

qint64 GStreamerInputPrivate::readData(char *data, qint64 len)
{
    return m_audioDevice->m_buffer.read(data, len);
}

qint64 GStreamerInputPrivate::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

qint64 GStreamerInputPrivate::bytesAvailable() const
{
    return m_audioDevice->m_buffer.size();
}


QT_END_NAMESPACE

#include "moc_qaudioinput_gstreamer_p.cpp"
