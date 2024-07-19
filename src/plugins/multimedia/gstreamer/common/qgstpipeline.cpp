// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qloggingcategory.h>

#include "qgstpipeline_p.h"
#include "qgst_bus_p.h"

#include <thread>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcGstPipeline, "qt.multimedia.gstpipeline");

static constexpr GstSeekFlags rateChangeSeekFlags =
#if GST_CHECK_VERSION(1, 18, 0)
        GST_SEEK_FLAG_INSTANT_RATE_CHANGE;
#else
        GST_SEEK_FLAG_FLUSH;
#endif

class QGstPipelinePrivate : public QGstBus
{
public:
    mutable std::chrono::nanoseconds m_position{};

    double m_rate = 1.;
    int m_configCounter = 0;
    GstState m_savedState = GST_STATE_NULL;

    explicit QGstPipelinePrivate(QGstBusHandle);
};

QGstPipelinePrivate::QGstPipelinePrivate(QGstBusHandle bus)
    : QGstBus{
          std::move(bus),
      }
{
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

void QGstPipeline::installMessageFilter(QGstreamerSyncMessageFilter *filter)
{
    QGstPipelinePrivate *d = getPrivate();
    d->installMessageFilter(filter);
}

void QGstPipeline::removeMessageFilter(QGstreamerSyncMessageFilter *filter)
{
    QGstPipelinePrivate *d = getPrivate();
    d->removeMessageFilter(filter);
}

void QGstPipeline::installMessageFilter(QGstreamerBusMessageFilter *filter)
{
    QGstPipelinePrivate *d = getPrivate();
    d->installMessageFilter(filter);
}

void QGstPipeline::removeMessageFilter(QGstreamerBusMessageFilter *filter)
{
    QGstPipelinePrivate *d = getPrivate();
    d->removeMessageFilter(filter);
}

GstStateChangeReturn QGstPipeline::setState(GstState state)
{
    return gst_element_set_state(element(), state);
}

bool QGstPipeline::processNextPendingMessage(GstMessageType types, std::chrono::nanoseconds timeout)
{
    QGstPipelinePrivate *d = getPrivate();
    return d->processNextPendingMessage(types, timeout);
}

bool QGstPipeline::processNextPendingMessage(std::chrono::nanoseconds timeout)
{
    return processNextPendingMessage(GST_MESSAGE_ANY, timeout);
}

void QGstPipeline::beginConfig()
{
    QGstPipelinePrivate *d = getPrivate();
    Q_ASSERT(!isNull());

    ++d->m_configCounter;
    if (d->m_configCounter > 1)
        return;

    GstState state;
    GstState pending;
    GstStateChangeReturn stateChangeReturn = gst_element_get_state(element(), &state, &pending, 0);
    switch (stateChangeReturn) {
    case GST_STATE_CHANGE_ASYNC: {
        if (state == GST_STATE_PLAYING) {
            // playing->paused transition in progress. wait for it to finish
            bool stateChangeSuccessful = this->finishStateChange();
            if (!stateChangeSuccessful)
                qWarning() << "QGstPipeline::beginConfig: timeout when waiting for state change";
        }

        state = pending;
        break;
    }
    case GST_STATE_CHANGE_FAILURE: {
        qDebug() << "QGstPipeline::beginConfig: state change failure";
        dumpGraph("beginConfigFailure");
        break;
    }

    case GST_STATE_CHANGE_NO_PREROLL:
    case GST_STATE_CHANGE_SUCCESS:
        break;
    }

    d->m_savedState = state;
    if (d->m_savedState == GST_STATE_PLAYING)
        setStateSync(GST_STATE_PAUSED);
}

void QGstPipeline::endConfig()
{
    QGstPipelinePrivate *d = getPrivate();
    Q_ASSERT(!isNull());

    --d->m_configCounter;
    if (d->m_configCounter)
        return;

    if (d->m_savedState == GST_STATE_PLAYING)
        setState(GST_STATE_PLAYING);
    d->m_savedState = GST_STATE_NULL;
}

void QGstPipeline::flush()
{
    seek(position());
}

void QGstPipeline::seek(std::chrono::nanoseconds pos, double rate)
{
    using namespace std::chrono_literals;

    // CAVEAT: we need to ensure that no async operation is currently in-flight, i.e
    // gst_element_get_state does not return GST_STATE_CHANGE_ASYNC.
    // Apparently this can happen when (a) the pipeline is changed due to new sinks being added or
    // the like and (b) during pending seeks.
    bool asyncChangeSuccess = waitForAsyncStateChangeComplete();
    if (!asyncChangeSuccess) {
        qWarning() << "QGstPipeline::seek: async pipeline change in progress. Seeking impossible";
        return;
    }

    QGstPipelinePrivate *d = getPrivate();
    // always adjust the rate, so it can be set before playback starts
    // setting position needs a loaded media file that's seekable

    qCDebug(qLcGstPipeline) << "QGstPipeline::seek to" << pos << "rate:" << rate;

    bool success = (rate > 0)
            ? gst_element_seek(element(), rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                               GST_SEEK_TYPE_SET, pos.count(), GST_SEEK_TYPE_END, 0)
            : gst_element_seek(element(), rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                               GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, pos.count());

    if (!success) {
        qDebug() << "seek: gst_element_seek failed" << pos;
        dumpGraph("seekSeekFailed");
        return;
    }

    d->m_position = pos;
}

void QGstPipeline::seek(std::chrono::nanoseconds pos)
{
    qCDebug(qLcGstPipeline) << "QGstPipeline::seek to" << pos;
    seek(pos, getPrivate()->m_rate);
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
    if (!forceFlushingSeek && GST_CHECK_VERSION(1, 18, 0)) {
        bool asyncChangeSuccess = waitForAsyncStateChangeComplete();
        if (!asyncChangeSuccess) {
            qWarning()
                    << "QGstPipeline::seek: async pipeline change in progress. Seeking impossible";
            return;
        }

        qCDebug(qLcGstPipeline) << "QGstPipeline::applyPlaybackRate instantly";
        bool success = gst_element_seek(
                element(), d->m_rate, GST_FORMAT_UNDEFINED, rateChangeSeekFlags, GST_SEEK_TYPE_NONE,
                GST_CLOCK_TIME_NONE, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
        if (!success) {
            qDebug() << "setPlaybackRate: gst_element_seek failed";
            dumpGraph("applyPlaybackRateSeekFailed");
        }
    } else {
        seek(position(), d->m_rate);
    }
}

void QGstPipeline::setPosition(std::chrono::nanoseconds pos)
{
    seek(pos);
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
        dumpGraph("positionQueryFailed");
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

QGstPipelinePrivate *QGstPipeline::getPrivate() const
{
    QGstPipelinePrivate *ret = getObject<QGstPipelinePrivate>("pipeline-private");
    Q_ASSERT(ret);
    return ret;
}

QT_END_NAMESPACE
