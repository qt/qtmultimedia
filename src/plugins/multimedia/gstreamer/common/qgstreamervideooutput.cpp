// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/qvideosink.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qthread.h>

#include <common/qgstreamervideooutput_p.h>
#include <common/qgstreamervideosink_p.h>
#include <common/qgstsubtitlesink_p.h>

static Q_LOGGING_CATEGORY(qLcMediaVideoOutput, "qt.multimedia.videooutput")

QT_BEGIN_NAMESPACE

QMaybe<QGstreamerVideoOutput *> QGstreamerVideoOutput::create(QObject *parent)
{
    QGstElement videoConvert;
    QGstElement videoScale;

    QGstElementFactoryHandle factory = QGstElementFactoryHandle{
        gst_element_factory_find("videoconvertscale"),
    };

    if (factory) { // videoconvertscale is only available in gstreamer 1.20
        videoConvert = QGstElement::createFromFactory(factory, "videoConvertScale");
    } else {
        videoConvert = QGstElement::createFromFactory("videoconvert", "videoConvert");
        if (!videoConvert)
            return errorMessageCannotFindElement("videoconvert");

        videoScale = QGstElement::createFromFactory("videoscale", "videoScale");
        if (!videoScale)
            return errorMessageCannotFindElement("videoscale");
    }

    QGstElement videoSink = QGstElement::createFromFactory("fakesink", "fakeVideoSink");
    if (!videoSink)
        return errorMessageCannotFindElement("fakesink");
    videoSink.set("sync", true);

    return new QGstreamerVideoOutput(videoConvert, videoScale, videoSink, parent);
}

QGstreamerVideoOutput::QGstreamerVideoOutput(QGstElement convert, QGstElement scale,
                                             QGstElement sink, QObject *parent)
    : QObject(parent),
      gstVideoOutput(QGstBin::create("videoOutput")),
      videoConvert(std::move(convert)),
      videoScale(std::move(scale)),
      videoSink(std::move(sink))
{
    videoQueue = QGstElement::createFromFactory("queue", "videoQueue");

    videoSink.set("sync", true);
    videoSink.set("async", false); // no asynchronous state changes

    if (videoScale) {
        gstVideoOutput.add(videoQueue, videoConvert, videoScale, videoSink);
        qLinkGstElements(videoQueue, videoConvert, videoScale, videoSink);
    } else {
        gstVideoOutput.add(videoQueue, videoConvert, videoSink);
        qLinkGstElements(videoQueue, videoConvert, videoSink);
    }

    gstVideoOutput.addGhostPad(videoQueue, "sink");
}

QGstreamerVideoOutput::~QGstreamerVideoOutput()
{
    gstVideoOutput.setStateSync(GST_STATE_NULL);
}

void QGstreamerVideoOutput::setVideoSink(QVideoSink *sink)
{
    auto *gstVideoSink = sink ? static_cast<QGstreamerVideoSink *>(sink->platformVideoSink()) : nullptr;
    if (gstVideoSink == m_videoSink)
        return;

    if (m_videoSink)
        m_videoSink->setPipeline({});

    m_videoSink = gstVideoSink;
    if (m_videoSink) {
        m_videoSink->setPipeline(gstPipeline);
        if (nativeSize.isValid())
            m_videoSink->setNativeSize(nativeSize);
    }
    QGstElement gstSink;
    if (m_videoSink) {
        gstSink = m_videoSink->gstSink();
    } else {
        gstSink = QGstElement::createFromFactory("fakesink", "fakevideosink");
        Q_ASSERT(gstSink);
        gstSink.set("sync", true);
        gstSink.set("async", false); // no asynchronous state changes
    }

    if (videoSink == gstSink)
        return;

    gstPipeline.modifyPipelineWhileNotRunning([&] {
        if (!videoSink.isNull())
            gstVideoOutput.stopAndRemoveElements(videoSink);

        videoSink = gstSink;
        gstVideoOutput.add(videoSink);

        if (videoScale)
            qLinkGstElements(videoScale, videoSink);
        else
            qLinkGstElements(videoConvert, videoSink);

        GstEvent *event = gst_event_new_reconfigure();
        gst_element_send_event(videoSink.element(), event);
        videoSink.syncStateWithParent();

        doLinkSubtitleStream();
    });

    qCDebug(qLcMediaVideoOutput) << "sinkChanged" << gstSink.name();

    gstPipeline.dumpGraph(videoSink.name().constData());
}

void QGstreamerVideoOutput::setPipeline(const QGstPipeline &pipeline)
{
    gstPipeline = pipeline;
    if (m_videoSink)
        m_videoSink->setPipeline(gstPipeline);
}

void QGstreamerVideoOutput::linkSubtitleStream(QGstElement src)
{
    qCDebug(qLcMediaVideoOutput) << "link subtitle stream" << src.isNull();
    if (src == subtitleSrc)
        return;

    gstPipeline.modifyPipelineWhileNotRunning([&] {
        subtitleSrc = src;
        doLinkSubtitleStream();
    });
}

void QGstreamerVideoOutput::unlinkSubtitleStream()
{
    if (subtitleSrc.isNull())
        return;
    qCDebug(qLcMediaVideoOutput) << "unlink subtitle stream";
    subtitleSrc = {};
    if (!subtitleSink.isNull()) {
        gstPipeline.modifyPipelineWhileNotRunning([&] {
            gstPipeline.stopAndRemoveElements(subtitleSink);
            return;
        });
        subtitleSink = {};
    }
    if (m_videoSink)
        m_videoSink->setSubtitleText({});
}

void QGstreamerVideoOutput::doLinkSubtitleStream()
{
    if (!subtitleSink.isNull()) {
        gstPipeline.stopAndRemoveElements(subtitleSink);
        subtitleSink = {};
    }
    if (!m_videoSink || subtitleSrc.isNull())
        return;
    if (subtitleSink.isNull()) {
        subtitleSink = m_videoSink->subtitleSink();
        gstPipeline.add(subtitleSink);
    }
    qLinkGstElements(subtitleSrc, subtitleSink);
}

void QGstreamerVideoOutput::updateNativeSize()
{
    if (!m_videoSink)
        return;

    m_videoSink->setNativeSize(qRotatedFrameSize(nativeSize, rotation));
}

void QGstreamerVideoOutput::setIsPreview()
{
    // configures the queue to be fast and lightweight for camera preview
    // also avoids blocking the queue in case we have an encodebin attached to the tee as well
    videoQueue.set("leaky", 2 /*downstream*/);
    videoQueue.set("silent", true);
    videoQueue.set("max-size-buffers", uint(1));
    videoQueue.set("max-size-bytes", uint(0));
    videoQueue.set("max-size-time", quint64(0));
}

void QGstreamerVideoOutput::flushSubtitles()
{
    if (!subtitleSink.isNull()) {
        auto pad = subtitleSink.staticPad("sink");
        auto *event = gst_event_new_flush_start();
        pad.sendEvent(event);
        event = gst_event_new_flush_stop(false);
        pad.sendEvent(event);
    }
}

void QGstreamerVideoOutput::setNativeSize(QSize sz)
{
    nativeSize = sz;
    updateNativeSize();
}

void QGstreamerVideoOutput::setRotation(QtVideo::Rotation rot)
{
    rotation = rot;
    updateNativeSize();
}

QT_END_NAMESPACE

#include "moc_qgstreamervideooutput_p.cpp"
