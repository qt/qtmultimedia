// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmath.h>
#include <private/qaudiohelpers_p.h>

#include "qgstreameraudiosink_p.h"
#include "qgstreameraudiodevice_p.h"
#include <sys/types.h>
#include <unistd.h>

#include <qgstpipeline_p.h>
#include <qgstappsrc_p.h>

#include <qgstutils_p.h>
#include <qgstreamermessage_p.h>

#include <utility>

QT_BEGIN_NAMESPACE

QMaybe<QPlatformAudioSink *> QGStreamerAudioSink::create(const QAudioDevice &device, QObject *parent)
{
    auto maybeAppSrc = QGstAppSrc::create();
    if (!maybeAppSrc)
        return maybeAppSrc.error();

    QGstElement audioconvert("audioconvert", "conv");
    if (!audioconvert)
        return errorMessageCannotFindElement("audioconvert");

    QGstElement volume("volume", "volume");
    if (!volume)
        return errorMessageCannotFindElement("volume");

    return new QGStreamerAudioSink(device, maybeAppSrc.value(), audioconvert, volume, parent);
}

QGStreamerAudioSink::QGStreamerAudioSink(const QAudioDevice &device, QGstAppSrc *appsrc,
                                         QGstElement audioconvert, QGstElement volume, QObject *parent)
    : QPlatformAudioSink(parent),
      m_device(device.id()),
      gstPipeline("pipeline"),
      gstVolume(std::move(volume)),
      m_appSrc(appsrc)
{
    gstPipeline.installMessageFilter(this);

    connect(m_appSrc, &QGstAppSrc::bytesProcessed, this, &QGStreamerAudioSink::bytesProcessedByAppSrc);
    connect(m_appSrc, &QGstAppSrc::noMoreData, this, &QGStreamerAudioSink::needData);
    gstAppSrc = m_appSrc->element();

    QGstElement queue("queue", "queue");

    if (m_volume != 1.)
        gstVolume.set("volume", m_volume);

    // link decodeBin to audioconvert in a callback once we get a pad from the decoder
    //    g_signal_connect (gstDecodeBin, "pad-added", (GCallback) padAdded, conv);

    const auto *audioInfo = static_cast<const QGStreamerAudioDeviceInfo *>(device.handle());
    gstOutput = gst_device_create_element(audioInfo->gstDevice, nullptr);

    gstPipeline.add(gstAppSrc, queue, /*gstDecodeBin, */ audioconvert, gstVolume, gstOutput);
    gstAppSrc.link(queue, audioconvert, gstVolume, gstOutput);
}

QGStreamerAudioSink::~QGStreamerAudioSink()
{
    close();
    gstPipeline = {};
    gstVolume = {};
    gstAppSrc = {};
    delete m_appSrc;
    m_appSrc = nullptr;
}

void QGStreamerAudioSink::setError(QAudio::Error error)
{
    if (m_errorState == error)
        return;

    m_errorState = error;
    emit errorChanged(error);
}

QAudio::Error QGStreamerAudioSink::error() const
{
    return m_errorState;
}

void QGStreamerAudioSink::setState(QAudio::State state)
{
    if (m_deviceState == state)
        return;

    m_deviceState = state;
    emit stateChanged(state);
}

QAudio::State QGStreamerAudioSink::state() const
{
    return m_deviceState;
}

void QGStreamerAudioSink::start(QIODevice *device)
{
    setState(QAudio::StoppedState);
    setError(QAudio::NoError);

    close();

    if (!m_format.isValid()) {
        setError(QAudio::OpenError);
        return;
    }

    m_pullMode = true;
    m_audioSource = device;

    if (!open()) {
        m_audioSource = nullptr;
        setError(QAudio::OpenError);
        return;
    }

    setState(QAudio::ActiveState);
}

QIODevice *QGStreamerAudioSink::start()
{
    setState(QAudio::StoppedState);
    setError(QAudio::NoError);

    close();

    if (!m_format.isValid()) {
        setError(QAudio::OpenError);
        return nullptr;
    }

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

bool QGStreamerAudioSink::processBusMessage(const QGstreamerMessage &message)
{
    auto *msg = message.rawMessage();
    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
        setState(QAudio::IdleState);
        break;
    case GST_MESSAGE_ERROR: {
        setError(QAudio::IOError);
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

bool QGStreamerAudioSink::open()
{
    if (m_opened)
        return true;

    if (gstOutput.isNull()) {
        setError(QAudio::OpenError);
        setState(QAudio::StoppedState);
        return false;
    }

//    qDebug() << "GST caps:" << gst_caps_to_string(caps);
    m_appSrc->setup(m_audioSource, m_audioSource ? m_audioSource->pos() : 0);
    m_appSrc->setAudioFormat(m_format);

    /* run */
    gstPipeline.setState(GST_STATE_PLAYING);

    m_opened = true;

    m_timeStamp.restart();
    m_bytesProcessed = 0;

    return true;
}

void QGStreamerAudioSink::close()
{
    if (!m_opened)
        return;

    if (!gstPipeline.setStateSync(GST_STATE_NULL))
        qWarning() << "failed to close the audio output stream";

    if (!m_pullMode && m_audioSource)
        delete m_audioSource;
    m_audioSource = nullptr;
    m_opened = false;
}

qint64 QGStreamerAudioSink::write(const char *data, qint64 len)
{
    if (!len)
        return 0;
    if (m_errorState == QAudio::UnderrunError)
        m_errorState = QAudio::NoError;

    m_appSrc->write(data, len);
    return len;
}

void QGStreamerAudioSink::stop()
{
    if (m_deviceState == QAudio::StoppedState)
        return;

    close();

    setError(QAudio::NoError);
    setState(QAudio::StoppedState);
}

qsizetype QGStreamerAudioSink::bytesFree() const
{
    if (m_deviceState != QAudio::ActiveState && m_deviceState != QAudio::IdleState)
        return 0;

    return m_appSrc->canAcceptMoreData() ? 4096*4 : 0;
}

void QGStreamerAudioSink::setBufferSize(qsizetype value)
{
    m_bufferSize = value;
    if (!gstAppSrc.isNull())
        gst_app_src_set_max_bytes(GST_APP_SRC(gstAppSrc.element()), value);
}

qsizetype QGStreamerAudioSink::bufferSize() const
{
    return m_bufferSize;
}

qint64 QGStreamerAudioSink::processedUSecs() const
{
    qint64 result = qint64(1000000) * m_bytesProcessed /
        m_format.bytesPerFrame() /
        m_format.sampleRate();

    return result;
}

void QGStreamerAudioSink::resume()
{
    if (m_deviceState == QAudio::SuspendedState) {
        m_appSrc->resume();
        gstPipeline.setState(GST_STATE_PLAYING);

        setState(m_suspendedInState);
        setError(QAudio::NoError);
    }
}

void QGStreamerAudioSink::setFormat(const QAudioFormat &format)
{
    m_format = format;
}

QAudioFormat QGStreamerAudioSink::format() const
{
    return m_format;
}

void QGStreamerAudioSink::suspend()
{
    if (m_deviceState == QAudio::ActiveState || m_deviceState == QAudio::IdleState) {
        m_suspendedInState = m_deviceState;
        setError(QAudio::NoError);
        setState(QAudio::SuspendedState);

        gstPipeline.setState(GST_STATE_PAUSED);
        m_appSrc->suspend();
        // ### elapsed time
    }
}

void QGStreamerAudioSink::reset()
{
    stop();
}

GStreamerOutputPrivate::GStreamerOutputPrivate(QGStreamerAudioSink *audio)
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
    if (m_audioDevice->state() == QAudio::IdleState)
        m_audioDevice->setState(QAudio::ActiveState);
    return m_audioDevice->write(data, len);
}

void QGStreamerAudioSink::setVolume(qreal vol)
{
    if (m_volume == vol)
        return;

    m_volume = vol;
    if (!gstVolume.isNull())
        gstVolume.set("volume", vol);
}

qreal QGStreamerAudioSink::volume() const
{
    return m_volume;
}

void QGStreamerAudioSink::bytesProcessedByAppSrc(int bytes)
{
    m_bytesProcessed += bytes;
    setState(QAudio::ActiveState);
    setError(QAudio::NoError);
}

void QGStreamerAudioSink::needData()
{
    if (state() != QAudio::StoppedState && state() != QAudio::IdleState) {
        setState(QAudio::IdleState);
        setError(m_audioSource && m_audioSource->atEnd() ? QAudio::NoError : QAudio::UnderrunError);
    }
}

QT_END_NAMESPACE

#include "moc_qgstreameraudiosink_p.cpp"
