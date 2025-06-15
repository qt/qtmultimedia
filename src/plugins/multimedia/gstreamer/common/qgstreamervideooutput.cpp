// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamervideooutput_p.h"

#include <QtMultimedia/qvideosink.h>

#include <QtCore/qloggingcategory.h>

#include <common/qgstreamervideosink_p.h>
#include <common/qgstsubtitlesink_p.h>

Q_STATIC_LOGGING_CATEGORY(qLcMediaVideoOutput, "qt.multimedia.videooutput");

QT_BEGIN_NAMESPACE

static QGstElement makeVideoConvertScale(const char *name)
{
    QGstElementFactoryHandle factory = QGstElement::findFactory("videoconvertscale");
    if (factory) // videoconvertscale is only available in gstreamer 1.20
        return QGstElement::createFromFactory(factory, name);

    return QGstBin::createFromPipelineDescription("videoconvert ! videoscale", name,
                                                  /*ghostUnlinkedPads=*/true);
}

q23::expected<QGstreamerVideoOutput *, QString> QGstreamerVideoOutput::create(QObject *parent)
{
    QGstElementFactoryHandle factory = QGstElement::findFactory("videoconvertscale");

    static std::optional<QString> elementCheck = []() -> std::optional<QString> {
        std::optional<QString> error = qGstErrorMessageIfElementsNotAvailable("fakesink", "queue");
        if (error)
            return error;

        QGstElementFactoryHandle factory = QGstElement::findFactory("videoconvertscale");
        if (factory)
            return std::nullopt;

        return qGstErrorMessageIfElementsNotAvailable("videoconvert", "videoscale");
    }();

    if (elementCheck)
        return q23::unexpected{ *elementCheck };

    return new QGstreamerVideoOutput(parent);
}

QGstreamerVideoOutput::QGstreamerVideoOutput(QObject *parent)
    : QObject(parent),
      m_outputBin{
          QGstBin::create("videoOutput"),
      },
      m_videoQueue{
          QGstElement::createFromFactory("queue", "videoQueue"),
      },
      m_videoConvertScale{
          makeVideoConvertScale("videoConvertScale"),
      },
      m_videoSink{
          QGstElement::createFromFactory("fakesink", "fakeVideoSink"),
      }
{
    m_videoSink.set("sync", true);

    m_outputBin.add(m_videoQueue, m_videoConvertScale, m_videoSink);
    qLinkGstElements(m_videoQueue, m_videoConvertScale, m_videoSink);

    m_subtitleSink = QGstSubtitleSink::createSink(this);

    m_outputBin.addGhostPad(m_videoQueue, "sink");
}

QGstreamerVideoOutput::~QGstreamerVideoOutput()
{
    QObject::disconnect(m_subtitleConnection);
    m_outputBin.setStateSync(GST_STATE_NULL);
}

void QGstreamerVideoOutput::setVideoSink(QVideoSink *sink)
{
    using namespace std::chrono_literals;

    auto *gstSink = sink ? static_cast<QGstreamerVideoSink *>(sink->platformVideoSink()) : nullptr;
    if (gstSink == m_platformVideoSink)
        return;

    m_platformVideoSink = gstSink;
    if (m_platformVideoSink) {
        m_platformVideoSink->setActive(m_isActive);
        if (m_nativeSize.isValid())
            m_platformVideoSink->setNativeSize(m_nativeSize);
    }
    QGstElement videoSink;
    if (m_platformVideoSink) {
        videoSink = m_platformVideoSink->gstSink();
    } else {
        videoSink = QGstElement::createFromFactory("fakesink", "fakevideosink");
        Q_ASSERT(videoSink);
        videoSink.set("sync", true);
    }

    QObject::disconnect(m_subtitleConnection);
    if (sink) {
        m_subtitleConnection = QObject::connect(this, &QGstreamerVideoOutput::subtitleChanged, sink,
                                                [sink](const QString &subtitle) {
            sink->setSubtitleText(subtitle);
        });
        sink->setSubtitleText(m_lastSubtitleString);
    }

    if (m_videoSink == videoSink)
        return;

    m_videoConvertScale.src().modifyPipelineInIdleProbe([&] {
        if (m_videoSink)
            m_outputBin.stopAndRemoveElements(m_videoSink);

        m_videoSink = std::move(videoSink);
        m_outputBin.add(m_videoSink);

        qLinkGstElements(m_videoConvertScale, m_videoSink);

        GstEvent *event = gst_event_new_reconfigure();
        gst_element_send_event(m_videoSink.element(), event);
        m_videoSink.syncStateWithParent();
    });

    qCDebug(qLcMediaVideoOutput) << "sinkChanged" << m_videoSink.name();
    m_videoConvertScale.dumpPipelineGraph(m_videoSink.name().constData());
}

void QGstreamerVideoOutput::setActive(bool isActive)
{
    if (m_isActive == isActive)
        return;

    m_isActive = isActive;
    if (m_platformVideoSink)
        m_platformVideoSink->setActive(isActive);
}

void QGstreamerVideoOutput::updateNativeSize()
{
    if (!m_platformVideoSink)
        return;

    m_platformVideoSink->setNativeSize(qRotatedFrameSize(m_nativeSize, m_rotation));
}

void QGstreamerVideoOutput::setIsPreview()
{
    // configures the queue to be fast and lightweight for camera preview
    // also avoids blocking the queue in case we have an encodebin attached to the tee as well
    m_videoQueue.set("leaky", 2 /*downstream*/);
    m_videoQueue.set("silent", true);
    m_videoQueue.set("max-size-buffers", int(1));
    m_videoQueue.set("max-size-bytes", unsigned(0));
    m_videoQueue.set("max-size-time", uint64_t(0));
}

void QGstreamerVideoOutput::flushSubtitles()
{
    if (m_subtitleSink) {
        auto pad = m_subtitleSink.staticPad("sink");
        auto *event = gst_event_new_flush_start();
        pad.sendEvent(event);
        event = gst_event_new_flush_stop(false);
        pad.sendEvent(event);
    }
}

void QGstreamerVideoOutput::setNativeSize(QSize sz)
{
    m_nativeSize = sz;
    updateNativeSize();
}

void QGstreamerVideoOutput::setRotation(QtVideo::Rotation rot)
{
    m_rotation = rot;
    updateNativeSize();
}

void QGstreamerVideoOutput::updateSubtitle(QString string)
{
    // GStreamer thread

    QMetaObject::invokeMethod(this, [this, string = std::move(string)]() mutable {
        m_lastSubtitleString = string;
        Q_EMIT subtitleChanged(std::move(string));
    });
}

QT_END_NAMESPACE

#include "moc_qgstreamervideooutput_p.cpp"
