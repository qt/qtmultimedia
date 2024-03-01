// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qgstreamervideooutput_p.h>
#include <qgstreamervideosink_p.h>
#include <qgstsubtitlesink_p.h>
#include <qvideosink.h>

#include <QtCore/qloggingcategory.h>
#include <qthread.h>

static Q_LOGGING_CATEGORY(qLcMediaVideoOutput, "qt.multimedia.videooutput")

QT_BEGIN_NAMESPACE

QMaybe<QGstreamerVideoOutput *> QGstreamerVideoOutput::create(QObject *parent)
{
    QGstElement videoConvert = QGstElement::createFromFactory("videoconvertscale", "videoConvert");
    if (!videoConvert)
        return errorMessageCannotFindElement("videoconvertscale");

    QGstElement videoSink = QGstElement::createFromFactory("fakesink", "fakeVideoSink");
    if (!videoSink)
        return errorMessageCannotFindElement("fakesink");

    return new QGstreamerVideoOutput(videoConvert, videoSink, parent);
}

QGstreamerVideoOutput::QGstreamerVideoOutput(QGstElement convert, QGstElement sink, QObject *parent)
    : QObject(parent),
      gstVideoOutput(QGstBin::create("videoOutput")),
      videoConvert(std::move(convert)),
      videoSink(std::move(sink))
{
    videoQueue = QGstElement::createFromFactory("queue", "videoQueue");
    videoSink.set("sync", true);
    gstVideoOutput.add(videoQueue, videoConvert, videoSink);
    qLinkGstElements(videoQueue, videoConvert, videoSink);

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
    if (m_videoSink)
        m_videoSink->setPipeline(gstPipeline);

    QGstElement gstSink;
    if (m_videoSink) {
        gstSink = m_videoSink->gstSink();
        isFakeSink = false;
    } else {
        gstSink = QGstElement::createFromFactory("fakesink", "fakevideosink");
        Q_ASSERT(gstSink);
        gstSink.set("sync", true);
        isFakeSink = true;
    }

    if (videoSink == gstSink)
        return;

    gstPipeline.modifyPipelineWhileNotRunning([&] {
        if (!videoSink.isNull())
            gstVideoOutput.stopAndRemoveElements(videoSink);

        videoSink = gstSink;
        gstVideoOutput.add(videoSink);

        qLinkGstElements(videoConvert, videoSink);
        GstEvent *event = gst_event_new_reconfigure();
        gst_element_send_event(videoSink.element(), event);
        videoSink.syncStateWithParent();

        doLinkSubtitleStream();
    });

    qCDebug(qLcMediaVideoOutput) << "sinkChanged" << gstSink.name();

    GST_DEBUG_BIN_TO_DOT_FILE(gstPipeline.bin(),
                              GstDebugGraphDetails(/*GST_DEBUG_GRAPH_SHOW_ALL |*/ GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE |
                                                   GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES),
                              videoSink.name());

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

QT_END_NAMESPACE

#include "moc_qgstreamervideooutput_p.cpp"
