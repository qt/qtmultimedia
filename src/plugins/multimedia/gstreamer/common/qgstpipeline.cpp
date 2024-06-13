// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qmap.h>
#include <QtCore/qtimer.h>
#include <QtCore/qmutex.h>
#include <QtCore/qlist.h>
#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qproperty.h>

#include "qgstpipeline_p.h"
#include "qgstreamermessage_p.h"

QT_BEGIN_NAMESPACE

static constexpr GstSeekFlags rateChangeSeekFlags =
#if GST_CHECK_VERSION(1, 18, 0)
        GST_SEEK_FLAG_INSTANT_RATE_CHANGE;
#else
        GST_SEEK_FLAG_FLUSH;
#endif

class QGstPipelinePrivate : public QObject
{
public:
    int m_ref = 0;
    guint m_tag = 0;
    GstBus *m_bus = nullptr;
    QTimer *m_intervalTimer = nullptr;
    QMutex filterMutex;
    QList<QGstreamerSyncMessageFilter*> syncFilters;
    QList<QGstreamerBusMessageFilter*> busFilters;
    bool inStoppedState = true;
    mutable std::chrono::nanoseconds m_position{};
    double m_rate = 1.;
    bool m_flushOnConfigChanges = false;
    bool m_pendingFlush = false;

    int m_configCounter = 0;
    GstState m_savedState = GST_STATE_NULL;

    explicit QGstPipelinePrivate(GstBus *bus, QObject *parent = nullptr);
    ~QGstPipelinePrivate();

    void ref() { ++ m_ref; }
    void deref() { if (!--m_ref) delete this; }

    void installMessageFilter(QGstreamerSyncMessageFilter *filter);
    void removeMessageFilter(QGstreamerSyncMessageFilter *filter);
    void installMessageFilter(QGstreamerBusMessageFilter *filter);
    void removeMessageFilter(QGstreamerBusMessageFilter *filter);

    void processMessage(const QGstreamerMessage &msg)
    {
        for (QGstreamerBusMessageFilter *filter : std::as_const(busFilters)) {
            if (filter->processBusMessage(msg))
                break;
        }
    }

private:
    static GstBusSyncReply syncGstBusFilter(GstBus *bus, GstMessage *message,
                                            QGstPipelinePrivate *d)
    {
        if (!message)
            return GST_BUS_PASS;

        Q_UNUSED(bus);
        QMutexLocker lock(&d->filterMutex);

        for (QGstreamerSyncMessageFilter *filter : std::as_const(d->syncFilters)) {
            if (filter->processSyncMessage(
                        QGstreamerMessage{ message, QGstreamerMessage::NeedsRef })) {
                gst_message_unref(message);
                return GST_BUS_DROP;
            }
        }

        return GST_BUS_PASS;
    }

    void processMessage(GstMessage *message)
    {
        if (!message)
            return;

        QGstreamerMessage msg{
            message,
            QGstreamerMessage::NeedsRef,
        };

        processMessage(msg);
    }

    static gboolean busCallback(GstBus *, GstMessage *message, gpointer data)
    {
        static_cast<QGstPipelinePrivate *>(data)->processMessage(message);
        return TRUE;
    }
};

QGstPipelinePrivate::QGstPipelinePrivate(GstBus* bus, QObject* parent)
  : QObject(parent),
    m_bus(bus)
{
    // glib event loop can be disabled either by env variable or QT_NO_GLIB define, so check the dispacher
    QAbstractEventDispatcher *dispatcher = QCoreApplication::eventDispatcher();
    const bool hasGlib = dispatcher && dispatcher->inherits("QEventDispatcherGlib");
    if (!hasGlib) {
        m_intervalTimer = new QTimer(this);
        m_intervalTimer->setInterval(250);
        QObject::connect(m_intervalTimer, &QTimer::timeout, this, [this] {
            GstMessage *message;
            while ((message = gst_bus_poll(m_bus, GST_MESSAGE_ANY, 0)) != nullptr) {
                processMessage(message);
                gst_message_unref(message);
            }
        });
        m_intervalTimer->start();
    } else {
        m_tag = gst_bus_add_watch_full(bus, G_PRIORITY_DEFAULT, busCallback, this, nullptr);
    }

    gst_bus_set_sync_handler(bus, (GstBusSyncHandler)syncGstBusFilter, this, nullptr);
}

QGstPipelinePrivate::~QGstPipelinePrivate()
{
    delete m_intervalTimer;

    if (m_tag)
        gst_bus_remove_watch(m_bus);

    gst_bus_set_sync_handler(m_bus, nullptr, nullptr, nullptr);
    gst_object_unref(GST_OBJECT(m_bus));
}

void QGstPipelinePrivate::installMessageFilter(QGstreamerSyncMessageFilter *filter)
{
    if (filter) {
        QMutexLocker lock(&filterMutex);
        if (!syncFilters.contains(filter))
            syncFilters.append(filter);
    }
}

void QGstPipelinePrivate::removeMessageFilter(QGstreamerSyncMessageFilter *filter)
{
    if (filter) {
        QMutexLocker lock(&filterMutex);
        syncFilters.removeAll(filter);
    }
}

void QGstPipelinePrivate::installMessageFilter(QGstreamerBusMessageFilter *filter)
{
    if (filter && !busFilters.contains(filter))
        busFilters.append(filter);
}

void QGstPipelinePrivate::removeMessageFilter(QGstreamerBusMessageFilter *filter)
{
    if (filter)
        busFilters.removeAll(filter);
}

QGstPipeline QGstPipeline::create(const char *name)
{
    GstPipeline *pipeline = qGstCheckedCast<GstPipeline>(gst_pipeline_new(name));
    return adopt(pipeline);
}

QGstPipeline QGstPipeline::adopt(GstPipeline *pipeline)
{
    QGstPipelinePrivate *d = new QGstPipelinePrivate(gst_pipeline_get_bus(pipeline));
    g_object_set_data_full(qGstCheckedCast<GObject>(pipeline), "pipeline-private", d,
                           [](gpointer ptr) {
                               delete reinterpret_cast<QGstPipelinePrivate *>(ptr);
                               return;
                           });

    return QGstPipeline{
        pipeline,
        QGstPipeline::NeedsRef,
    };
}

QGstPipeline::QGstPipeline(GstPipeline *p, RefMode mode) : QGstBin(qGstCheckedCast<GstBin>(p), mode)
{
}

QGstPipeline::~QGstPipeline() = default;

bool QGstPipeline::inStoppedState() const
{
    QGstPipelinePrivate *d = getPrivate();
    return d->inStoppedState;
}

void QGstPipeline::setInStoppedState(bool stopped)
{
    QGstPipelinePrivate *d = getPrivate();
    d->inStoppedState = stopped;
}

void QGstPipeline::setFlushOnConfigChanges(bool flush)
{
    QGstPipelinePrivate *d = getPrivate();
    d->m_flushOnConfigChanges = flush;
}

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
    QGstPipelinePrivate *d = getPrivate();
    auto retval = gst_element_set_state(element(), state);
    if (d->m_pendingFlush) {
        d->m_pendingFlush = false;
        flush();
    }
    return retval;
}

void QGstPipeline::processMessages(GstMessageType types)
{
    QGstPipelinePrivate *d = getPrivate();
    QGstreamerMessage message{
        gst_bus_pop_filtered(d->m_bus, types),
        QGstreamerMessage::HasRef,
    };
    d->processMessage(message);
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

    if (d->m_flushOnConfigChanges)
        d->m_pendingFlush = true;
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

    QGstPipelinePrivate *d = getPrivate();
    // always adjust the rate, so it can be set before playback starts
    // setting position needs a loaded media file that's seekable

    bool success = (rate > 0)
            ? gst_element_seek(element(), d->m_rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                               GST_SEEK_TYPE_SET, pos.count(), GST_SEEK_TYPE_END, 0)
            : gst_element_seek(element(), d->m_rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                               GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, pos.count());

    if (!success) {
        qDebug() << "seek: gst_element_seek failed" << pos;
        return;
    }

    d->m_position = pos;
}

void QGstPipeline::seek(std::chrono::nanoseconds pos)
{
    seek(pos, getPrivate()->m_rate);
}

void QGstPipeline::setPlaybackRate(double rate)
{
    QGstPipelinePrivate *d = getPrivate();
    if (rate == d->m_rate)
        return;

    d->m_rate = rate;

    applyPlaybackRate(/*instantRateChange =*/true);
}

double QGstPipeline::playbackRate() const
{
    QGstPipelinePrivate *d = getPrivate();
    return d->m_rate;
}

void QGstPipeline::applyPlaybackRate(bool instantRateChange)
{
    QGstPipelinePrivate *d = getPrivate();

    bool success = gst_element_seek(element(), d->m_rate, GST_FORMAT_UNDEFINED,
                                    instantRateChange ? rateChangeSeekFlags : GST_SEEK_FLAG_FLUSH,
                                    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE, GST_SEEK_TYPE_NONE,
                                    GST_CLOCK_TIME_NONE);
    if (!success)
        qDebug() << "setPlaybackRate: gst_element_seek failed";
}

void QGstPipeline::setPosition(std::chrono::nanoseconds pos)
{
    seek(pos);
}

std::chrono::nanoseconds QGstPipeline::position() const
{
    QGstPipelinePrivate *d = getPrivate();
    std::optional<std::chrono::nanoseconds> pos = QGstElement::position();
    if (pos)
        d->m_position = *pos;
    else
        qDebug() << "QGstPipeline: failed to query position, using previous position";

    return d->m_position;
}

std::chrono::milliseconds QGstPipeline::positionInMs() const
{
    using namespace std::chrono;
    return round<milliseconds>(position());
}

QGstPipelinePrivate *QGstPipeline::getPrivate() const
{
    gpointer p = g_object_get_data(qGstCheckedCast<GObject>(object()), "pipeline-private");
    auto *d = reinterpret_cast<QGstPipelinePrivate *>(p);
    Q_ASSERT(d);
    return d;
}

QT_END_NAMESPACE
