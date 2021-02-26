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

#include "qaudiooutput_gstreamer_p.h"
#include "qaudiodeviceinfo_gstreamer_p.h"
#include <sys/types.h>
#include <unistd.h>

#include <private/qgstreamerbushelper_p.h>
#include <private/qgstappsrc_p.h>

#include <private/qgstutils_p.h>

QT_BEGIN_NAMESPACE

QGStreamerAudioOutput::QGStreamerAudioOutput(const QByteArray &device)
    : m_device(device)
{
    QGStreamerAudioDeviceInfo audioInfo(device, QAudio::AudioOutput);
    gstOutput = gst_device_create_element(audioInfo.gstDevice, nullptr);
    gst_object_ref(gstOutput);
}

QGStreamerAudioOutput::~QGStreamerAudioOutput()
{
    if (gstOutput)
        gst_object_unref(gstOutput);
    close();
    QCoreApplication::processEvents();
}

void QGStreamerAudioOutput::setError(QAudio::Error error)
{
    if (m_errorState == error)
        return;

    m_errorState = error;
    emit errorChanged(error);
}

QAudio::Error QGStreamerAudioOutput::error() const
{
    return m_errorState;
}

void QGStreamerAudioOutput::setState(QAudio::State state)
{
    if (m_deviceState == state)
        return;

    m_deviceState = state;
    emit stateChanged(state);
}

QAudio::State QGStreamerAudioOutput::state() const
{
    return m_deviceState;
}

void QGStreamerAudioOutput::start(QIODevice *device)
{
    setState(QAudio::StoppedState);
    setError(QAudio::NoError);

    close();

    m_pullMode = true;
    m_audioSource = device;

    if (!open()) {
        m_audioSource = nullptr;
        return;
    }

    setState(QAudio::ActiveState);
}

QIODevice *QGStreamerAudioOutput::start()
{
    setState(QAudio::StoppedState);
    setError(QAudio::NoError);

    close();

    m_pullMode = false;

    if (!open())
        return nullptr;

    m_audioSource = new GStreamerOutputPrivate(this);
    m_audioSource->open(QIODevice::WriteOnly|QIODevice::Unbuffered);

    setState(QAudio::IdleState);

    return m_audioSource;
}

#if 0
static void padAdded(GstElement *element, GstPad *pad, gpointer data)
{
  GstElement *other = static_cast<GstElement *>(data);

  gchar *name = gst_pad_get_name(pad);
  qDebug("A new pad %s was created for %s\n", name, gst_element_get_name(element));
  g_free(name);

  qDebug("element %s will be linked to %s\n",
           gst_element_get_name(element),
           gst_element_get_name(other));
  gst_element_link(element, other);
}
#endif

gboolean QGStreamerAudioOutput::busMessage(GstBus *, GstMessage *msg, gpointer user_data)
{
    QGStreamerAudioOutput *output = static_cast<QGStreamerAudioOutput *>(user_data);
    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
        output->stop();
        break;
    case GST_MESSAGE_ERROR: {
        output->setError(QAudio::IOError);
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

    return true;
}

bool QGStreamerAudioOutput::open()
{
    if (m_opened)
        return true;

    if (!gstOutput) {
        setError(QAudio::OpenError);
        setState(QAudio::StoppedState);
        return false;
    }

    gstPipeline = gst_pipeline_new ("pipeline");

    auto *gstBus = gst_pipeline_get_bus(GST_PIPELINE(gstPipeline));
    gst_bus_add_watch(gstBus, &QGStreamerAudioOutput::busMessage, this);
    gst_object_unref (gstBus);

    gstAppSrc = gst_element_factory_make("appsrc", "appsrc");

    m_bufferSize = gst_app_src_get_max_bytes(GST_APP_SRC(gstAppSrc));

//    qDebug() << "GST caps:" << gst_caps_to_string(caps);
    m_appSrc = new QGstAppSrc;
    if (m_audioSource) {
        m_appSrc->setStream(m_audioSource);
    } else {
        m_appSrc->setBuffer(&m_buffer);
    }
    m_appSrc->setAudioFormat(m_format);
    m_appSrc->setup(gstAppSrc);

//    gstDecodeBin = gst_element_factory_make ("decodebin", "dec");
    GstElement *conv = gst_element_factory_make("audioconvert", "conv");
    gstVolume = gst_element_factory_make ("volume", "volume");
    if (m_volume != 1.)
        g_object_set(gstVolume, "volume", m_volume, nullptr);

    gst_bin_add_many(GST_BIN (gstPipeline), gstAppSrc, /*gstDecodeBin, */ conv, gstVolume, gstOutput, nullptr);

    gst_element_link(gstAppSrc, conv);
    gst_element_link(conv, gstVolume);
    gst_element_link(gstVolume, gstOutput);
//    gst_element_link(gstVolume, gstOutput);

    // link decodeBin to audioconvert in a callback once we get a pad from the decoder
//    g_signal_connect (gstDecodeBin, "pad-added", (GCallback) padAdded, conv);

    /* run */
    gst_element_set_state(gstPipeline, GST_STATE_PLAYING);

    m_opened = true;

    m_elapsedTimeOffset = 0;
    m_timeStamp.restart();
    m_clockStamp.restart();

    return true;
}

void QGStreamerAudioOutput::close()
{
    if (!m_opened)
        return;

    gst_element_set_state(gstPipeline, GST_STATE_NULL);
    gst_object_unref (gstPipeline);
    gstVolume = nullptr;
    gstAppSrc = nullptr;

    if (!m_pullMode && m_audioSource) {
        delete m_audioSource;
        m_audioSource = nullptr;
    }
    m_opened = false;
    m_buffer.clear();
}

qint64 QGStreamerAudioOutput::write(const char *data, qint64 len)
{
    m_buffer.append(data, len);
    m_totalTimeValue += len;
    return len;
}

void QGStreamerAudioOutput::stop()
{
    if (m_deviceState == QAudio::StoppedState)
        return;

    close();

    setError(QAudio::NoError);
    setState(QAudio::StoppedState);
}

int QGStreamerAudioOutput::bytesFree() const
{
    if (m_deviceState != QAudio::ActiveState && m_deviceState != QAudio::IdleState)
        return 0;

    return m_bufferSize; // ### correct????
}

int QGStreamerAudioOutput::periodSize() const
{
    // max 5ms periods. Gstreamer itself will ask for 4k data at a time
    return qMin(4096, 5*m_format.sampleRate()*m_format.bytesPerFrame()/1000);
}

void QGStreamerAudioOutput::setBufferSize(int value)
{
    m_bufferSize = value;
    if (gstAppSrc)
        gst_app_src_set_max_bytes(GST_APP_SRC(gstAppSrc), value);
}

int QGStreamerAudioOutput::bufferSize() const
{
    return m_bufferSize;
}

void QGStreamerAudioOutput::setNotifyInterval(int ms)
{
    m_notifyInterval = qMax(0, ms);
}

int QGStreamerAudioOutput::notifyInterval() const
{
    return m_notifyInterval;
}

qint64 QGStreamerAudioOutput::processedUSecs() const
{
    qint64 result = qint64(1000000) * m_totalTimeValue /
        m_format.bytesPerFrame() /
        m_format.sampleRate();

    return result;
}

void QGStreamerAudioOutput::resume()
{
    if (m_deviceState == QAudio::SuspendedState) {
        gst_element_set_state (gstPipeline, GST_STATE_PLAYING);

        setState(m_pullMode ? QAudio::ActiveState : QAudio::IdleState);
        setError(QAudio::NoError);
    }
}

void QGStreamerAudioOutput::setFormat(const QAudioFormat &format)
{
    m_format = format;
}

QAudioFormat QGStreamerAudioOutput::format() const
{
    return m_format;
}

void QGStreamerAudioOutput::suspend()
{
    if (m_deviceState == QAudio::ActiveState || m_deviceState == QAudio::IdleState) {
        setError(QAudio::NoError);
        setState(QAudio::SuspendedState);

        gst_element_set_state (gstPipeline, GST_STATE_PAUSED);
        // ### elapsed time
    }
}

qint64 QGStreamerAudioOutput::elapsedUSecs() const
{
    if (m_deviceState == QAudio::StoppedState)
        return 0;

    return m_clockStamp.elapsed() * qint64(1000);
}

void QGStreamerAudioOutput::reset()
{
    stop();
}

GStreamerOutputPrivate::GStreamerOutputPrivate(QGStreamerAudioOutput *audio)
{
    m_audioDevice = audio;
}

qint64 GStreamerOutputPrivate::readData(char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 GStreamerOutputPrivate::writeData(const char *data, qint64 len)
{
    return m_audioDevice->write(data, len);
}

void QGStreamerAudioOutput::setVolume(qreal vol)
{
    if (m_volume == vol)
        return;

    m_volume = vol;
    if (gstVolume)
        g_object_set(gstVolume, "volume", vol, nullptr);
}

qreal QGStreamerAudioOutput::volume() const
{
    return m_volume;
}

void QGStreamerAudioOutput::setCategory(const QString &category)
{
    if (m_category != category) {
        m_category = category;
    }
}

QString QGStreamerAudioOutput::category() const
{
    return m_category;
}

QT_END_NAMESPACE

#include "moc_qaudiooutput_gstreamer_p.cpp"
