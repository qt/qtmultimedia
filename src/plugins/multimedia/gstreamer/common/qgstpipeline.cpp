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

class QGstPipelinePrivate : public QObject
{
    Q_OBJECT
public:

    int m_ref = 0;
    guint m_tag = 0;
    GstBus *m_bus = nullptr;
    QTimer *m_intervalTimer = nullptr;
    QMutex filterMutex;
    QList<QGstreamerSyncMessageFilter*> syncFilters;
    QList<QGstreamerBusMessageFilter*> busFilters;
    bool inStoppedState = true;
    mutable qint64 m_position = 0;
    double m_rate = 1.;
    bool m_flushOnConfigChanges = false;
    bool m_pendingFlush = false;

    int m_configCounter = 0;
    GstState m_savedState = GST_STATE_NULL;

    QGstPipelinePrivate(GstBus* bus, QObject* parent = 0);
    ~QGstPipelinePrivate();

    void ref() { ++ m_ref; }
    void deref() { if (!--m_ref) delete this; }

    void installMessageFilter(QGstreamerSyncMessageFilter *filter);
    void removeMessageFilter(QGstreamerSyncMessageFilter *filter);
    void installMessageFilter(QGstreamerBusMessageFilter *filter);
    void removeMessageFilter(QGstreamerBusMessageFilter *filter);

    static GstBusSyncReply syncGstBusFilter(GstBus* bus, GstMessage* message, QGstPipelinePrivate *d)
    {
        Q_UNUSED(bus);
        QMutexLocker lock(&d->filterMutex);

        for (QGstreamerSyncMessageFilter *filter : std::as_const(d->syncFilters)) {
            if (filter->processSyncMessage(QGstreamerMessage(message))) {
                gst_message_unref(message);
                return GST_BUS_DROP;
            }
        }

        return GST_BUS_PASS;
    }

private Q_SLOTS:
    void interval()
    {
        GstMessage* message;
        while ((message = gst_bus_poll(m_bus, GST_MESSAGE_ANY, 0)) != nullptr) {
            processMessage(message);
            gst_message_unref(message);
        }
    }
    void doProcessMessage(const QGstreamerMessage& msg)
    {
        for (QGstreamerBusMessageFilter *filter : std::as_const(busFilters)) {
            if (filter->processBusMessage(msg))
                break;
        }
    }

private:
    void processMessage(GstMessage* message)
    {
        QGstreamerMessage msg(message);
        doProcessMessage(msg);
    }

    void queueMessage(GstMessage* message)
    {
        QGstreamerMessage msg(message);
        QMetaObject::invokeMethod(this, "doProcessMessage", Qt::QueuedConnection,
                                  Q_ARG(QGstreamerMessage, msg));
    }

    static gboolean busCallback(GstBus *bus, GstMessage *message, gpointer data)
    {
        Q_UNUSED(bus);
        static_cast<QGstPipelinePrivate *>(data)->queueMessage(message);
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
        connect(m_intervalTimer, SIGNAL(timeout()), SLOT(interval()));
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

QGstPipeline::QGstPipeline(const QGstPipeline &o)
    : QGstBin(o.bin(), NeedsRef),
    d(o.d)
{
    if (d)
        d->ref();
}

QGstPipeline &QGstPipeline::operator=(const QGstPipeline &o)
{
    if (this == &o)
        return *this;
    if (o.d)
        o.d->ref();
    if (d)
        d->deref();
    QGstBin::operator=(o);
    d = o.d;
    return *this;
}

QGstPipeline::QGstPipeline(const char *name)
    : QGstBin(GST_BIN(gst_pipeline_new(name)), NeedsRef)
{
    d = new QGstPipelinePrivate(gst_pipeline_get_bus(pipeline()));
    d->ref();
}

QGstPipeline::QGstPipeline(GstPipeline *p)
    : QGstBin(&p->bin, NeedsRef)
{
    d = new QGstPipelinePrivate(gst_pipeline_get_bus(pipeline()));
    d->ref();
}

QGstPipeline::~QGstPipeline()
{
    if (d)
        d->deref();
}

bool QGstPipeline::inStoppedState() const
{
    Q_ASSERT(d);
    return d->inStoppedState;
}

void QGstPipeline::setInStoppedState(bool stopped)
{
    Q_ASSERT(d);
    d->inStoppedState = stopped;
}

void QGstPipeline::setFlushOnConfigChanges(bool flush)
{
    d->m_flushOnConfigChanges = flush;
}

void QGstPipeline::installMessageFilter(QGstreamerSyncMessageFilter *filter)
{
    Q_ASSERT(d);
    d->installMessageFilter(filter);
}

void QGstPipeline::removeMessageFilter(QGstreamerSyncMessageFilter *filter)
{
    Q_ASSERT(d);
    d->removeMessageFilter(filter);
}

void QGstPipeline::installMessageFilter(QGstreamerBusMessageFilter *filter)
{
    Q_ASSERT(d);
    d->installMessageFilter(filter);
}

void QGstPipeline::removeMessageFilter(QGstreamerBusMessageFilter *filter)
{
    Q_ASSERT(d);
    d->removeMessageFilter(filter);
}

GstStateChangeReturn QGstPipeline::setState(GstState state)
{
    auto retval = gst_element_set_state(element(), state);
    if (d->m_pendingFlush) {
        d->m_pendingFlush = false;
        flush();
    }
    return retval;
}

void QGstPipeline::beginConfig()
{
    if (!d)
        return;
    Q_ASSERT(!isNull());

    ++d->m_configCounter;
    if (d->m_configCounter > 1)
        return;

    d->m_savedState = state();
    if (d->m_savedState == GST_STATE_PLAYING)
        setStateSync(GST_STATE_PAUSED);
}

void QGstPipeline::endConfig()
{
    if (!d)
        return;
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
    seek(position(), d->m_rate);
}

bool QGstPipeline::seek(qint64 pos, double rate)
{
    // always adjust the rate, so it can be  set before playback starts
    // setting position needs a loaded media file that's seekable
    d->m_rate = rate;
    qint64 from = rate > 0 ? pos : 0;
    qint64 to = rate > 0 ? duration() : pos;
    bool success = gst_element_seek(element(), rate, GST_FORMAT_TIME,
                                    GstSeekFlags(GST_SEEK_FLAG_FLUSH),
                                    GST_SEEK_TYPE_SET, from,
                                    GST_SEEK_TYPE_SET, to);
    if (!success)
        return false;

    d->m_position = pos;
    return true;
}

bool QGstPipeline::setPlaybackRate(double rate)
{
    if (rate == d->m_rate)
        return false;
    seek(position(), rate);
    return true;
}

double QGstPipeline::playbackRate() const
{
    return d->m_rate;
}

bool QGstPipeline::setPosition(qint64 pos)
{
    return seek(pos, d->m_rate);
}

qint64 QGstPipeline::position() const
{
    gint64 pos;
    if (gst_element_query_position(element(), GST_FORMAT_TIME, &pos))
        d->m_position = pos;
    return d->m_position;
}

qint64 QGstPipeline::duration() const
{
    gint64 d;
    if (!gst_element_query_duration(element(), GST_FORMAT_TIME, &d))
        return 0.;
    return d;
}

QT_END_NAMESPACE

#include "qgstpipeline.moc"

