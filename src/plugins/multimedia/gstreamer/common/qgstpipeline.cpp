// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>

#include "qgstpipeline_p.h"
#include "qgst_bus_observer_p.h"

#include <thread>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcGstPipeline, "qt.multimedia.gstpipeline");

struct QGstPipelinePrivate
{
    explicit QGstPipelinePrivate(QGstBusHandle);
    ~QGstPipelinePrivate();

    std::chrono::nanoseconds m_position{};
    double m_rate = 1.;
    std::unique_ptr<QGstBusObserver> m_busObserver;
};

QGstPipelinePrivate::QGstPipelinePrivate(QGstBusHandle bus)
    : m_busObserver{
          std::make_unique<QGstBusObserver>(std::move(bus)),
      }
{
    Q_ASSERT(QThread::isMainThread());
}

QGstPipelinePrivate::~QGstPipelinePrivate()
{
    m_busObserver->close();

    if (m_busObserver->currentThreadIsNotifierThread())
        return;

    // The QGstPipelinePrivate is owned the the GstPipeline and can be destroyed from a gstreamer
    // thread. In this case we cannot destroy the object immediately, but need to marshall it
    // through the event loop of the main thread
    QMetaObject::invokeMethod(qApp, [bus = std::move(m_busObserver)] {
        // nothing to do, we just extend the lifetime of the bus
    });
}

// QGstPipeline

QGstPipeline QGstPipeline::create(const char *name)
{
    GstPipeline *pipeline = qGstCheckedCast<GstPipeline>(gst_pipeline_new(name));
    return adopt(pipeline);
}

QGstPipeline QGstPipeline::createFromFactory(const char *factory, const char *name)
{
    QGstElement playbin3 = QGstElement::createFromFactory(factory, name);
    GstPipeline *pipeline = qGstCheckedCast<GstPipeline>(playbin3.element());

    return QGstPipeline::adopt(pipeline);
}

QGstPipeline QGstPipeline::adopt(GstPipeline *pipeline)
{
    QGstPipeline wrappedObject{
        pipeline,
        QGstPipeline::NeedsRef,
    };

    QGstBusHandle bus{
        gst_pipeline_get_bus(pipeline),
        QGstBusHandle::HasRef,
    };

    auto d = std::make_unique<QGstPipelinePrivate>(std::move(bus));
    wrappedObject.set("pipeline-private", std::move(d));

    return wrappedObject;
}

QGstPipeline::QGstPipeline(GstPipeline *p, RefMode mode) : QGstBin(qGstCheckedCast<GstBin>(p), mode)
{
}

QGstPipeline::~QGstPipeline() = default;

void QGstPipeline::installMessageFilter(QGstreamerBusMessageFilter *filter)
{
    QGstPipelinePrivate *d = getPrivate();
    d->m_busObserver->installMessageFilter(filter);
}

void QGstPipeline::removeMessageFilter(QGstreamerBusMessageFilter *filter)
{
    QGstPipelinePrivate *d = getPrivate();
    d->m_busObserver->removeMessageFilter(filter);
}

GstStateChangeReturn QGstPipeline::setState(GstState state)
{
    return gst_element_set_state(element(), state);
}

bool QGstPipeline::processNextPendingMessage(GstMessageType types, std::chrono::nanoseconds timeout)
{
    QGstPipelinePrivate *d = getPrivate();
    return d->m_busObserver->processNextPendingMessage(types, timeout);
}

bool QGstPipeline::processNextPendingMessage(std::chrono::nanoseconds timeout)
{
    return processNextPendingMessage(GST_MESSAGE_ANY, timeout);
}

void QGstPipeline::flush()
{
    seek(position(), /*flush=*/true);
}

void QGstPipeline::seek(std::chrono::nanoseconds pos, double rate, bool flush)
{
    using namespace std::chrono_literals;

    QGstPipelinePrivate *d = getPrivate();
    // always adjust the rate, so it can be set before playback starts
    // setting position needs a loaded media file that's seekable

    qCDebug(qLcGstPipeline) << "QGstPipeline::seek to" << pos << "rate:" << rate
                            << (flush ? "flushing" : "not flushing");

    GstSeekFlags seekFlags = flush ? GST_SEEK_FLAG_FLUSH : GST_SEEK_FLAG_NONE;
    seekFlags = GstSeekFlags(seekFlags | GST_SEEK_FLAG_SEGMENT | GST_SEEK_FLAG_ACCURATE);

    bool success = (rate > 0)
            ? gst_element_seek(element(), rate, GST_FORMAT_TIME, seekFlags, GST_SEEK_TYPE_SET,
                               pos.count(), GST_SEEK_TYPE_END, 0)
            : gst_element_seek(element(), rate, GST_FORMAT_TIME, seekFlags, GST_SEEK_TYPE_SET, 0,
                               GST_SEEK_TYPE_SET, pos.count());

    if (!success) {
        qDebug() << "seek: gst_element_seek failed" << pos;
        dumpGraph("seekSeekFailed", false);
        return;
    }

    d->m_position = pos;
}

void QGstPipeline::seek(std::chrono::nanoseconds pos, bool flush)
{
    qCDebug(qLcGstPipeline) << "QGstPipeline::seek to" << pos;
    seek(pos, getPrivate()->m_rate, flush);
}

void QGstPipeline::setPlaybackRate(double rate, bool forceFlushingSeek)
{
    QGstPipelinePrivate *d = getPrivate();
    if (rate == d->m_rate)
        return;

    d->m_rate = rate;

    qCDebug(qLcGstPipeline) << "QGstPipeline::setPlaybackRate to" << rate;

    applyPlaybackRate(forceFlushingSeek);
}

double QGstPipeline::playbackRate() const
{
    QGstPipelinePrivate *d = getPrivate();
    return d->m_rate;
}

void QGstPipeline::applyPlaybackRate(bool forceFlushingSeek)
{
    QGstPipelinePrivate *d = getPrivate();

    // do not GST_SEEK_FLAG_FLUSH with GST_SEEK_TYPE_NONE
    // https://gitlab.freedesktop.org/gstreamer/gstreamer/-/issues/3604
    if (!forceFlushingSeek) {
        bool asyncChangeSuccess = waitForAsyncStateChangeComplete();
        if (!asyncChangeSuccess) {
            qWarning()
                    << "QGstPipeline::seek: async pipeline change in progress. Seeking impossible";
            return;
        }

        qCDebug(qLcGstPipeline) << "QGstPipeline::applyPlaybackRate instantly";
        bool success = gst_element_seek(
                element(), d->m_rate, GST_FORMAT_UNDEFINED, GST_SEEK_FLAG_INSTANT_RATE_CHANGE,
                GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
        if (!success) {
            qDebug() << "setPlaybackRate: gst_element_seek failed";
            dumpGraph("applyPlaybackRateSeekFailed", false);
        }
    } else {
        seek(position(), d->m_rate);
    }
}

void QGstPipeline::setPosition(std::chrono::nanoseconds pos, bool flush)
{
    seek(pos, flush);
}

std::chrono::nanoseconds QGstPipeline::position() const
{
    QGstPipelinePrivate *d = getPrivate();
    std::optional<std::chrono::nanoseconds> pos = QGstElement::position();
    if (pos) {
        d->m_position = *pos;
        qCDebug(qLcGstPipeline) << "QGstPipeline::position:"
                                << std::chrono::round<std::chrono::milliseconds>(*pos);
    } else {
        qDebug() << "QGstPipeline: failed to query position, using previous position";
        dumpGraph("positionQueryFailed", false);
    }

    return d->m_position;
}

std::chrono::milliseconds QGstPipeline::positionInMs() const
{
    using namespace std::chrono;
    return round<milliseconds>(position());
}

void QGstPipeline::setPositionAndRate(std::chrono::nanoseconds pos, double rate)
{
    QGstPipelinePrivate *d = getPrivate();
    d->m_rate = rate;
    seek(pos, rate);
}

std::optional<std::chrono::nanoseconds>
QGstPipeline::queryPosition(std::chrono::nanoseconds timeout) const
{
    using namespace std::chrono_literals;
    using namespace std::chrono;

    std::chrono::nanoseconds totalSleepTime{};

    for (;;) {
        std::optional<nanoseconds> dur = QGstElement::duration();
        if (dur)
            return dur;

        if (totalSleepTime >= timeout)
            return std::nullopt;
        std::this_thread::sleep_for(20ms);
        totalSleepTime += 20ms;
    }
}

std::optional<std::chrono::nanoseconds>
QGstPipeline::queryDuration(std::chrono::nanoseconds timeout) const
{
    using namespace std::chrono_literals;
    using namespace std::chrono;

    std::chrono::nanoseconds totalSleepTime{};

    for (;;) {
        std::optional<nanoseconds> dur = QGstElement::duration();
        if (dur)
            return dur;

        if (totalSleepTime >= timeout)
            return std::nullopt;

        std::this_thread::sleep_for(20ms);
        totalSleepTime += 20ms;
    }
}

std::optional<std::pair<std::chrono::nanoseconds, std::chrono::nanoseconds>>
QGstPipeline::queryPositionAndDuration(std::chrono::nanoseconds timeout) const
{
    using namespace std::chrono_literals;
    using namespace std::chrono;

    std::chrono::nanoseconds totalSleepTime{};

    std::optional<nanoseconds> dur;
    std::optional<nanoseconds> pos;

    for (;;) {
        if (!dur)
            dur = QGstElement::duration();
        if (!pos)
            pos = QGstElement::position();

        if (dur && pos)
            return std::pair{ *dur, *pos };

        if (totalSleepTime >= timeout)
            return std::nullopt;

        std::this_thread::sleep_for(20ms);
        totalSleepTime += 20ms;
    }
}

void QGstPipeline::seekToEndWithEOS()
{
    QGstPipelinePrivate *d = getPrivate();

    gst_element_seek(element(), d->m_rate, GST_FORMAT_TIME, GST_SEEK_FLAG_NONE, GST_SEEK_TYPE_END,
                     0, GST_SEEK_TYPE_END, 0);
}

QGstPipelinePrivate *QGstPipeline::getPrivate() const
{
    QGstPipelinePrivate *ret = getObject<QGstPipelinePrivate>("pipeline-private");
    Q_ASSERT(ret);
    return ret;
}

QT_END_NAMESPACE
