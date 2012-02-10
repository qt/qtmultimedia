/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qmap.h>
#include <QtCore/qtimer.h>
#include <QtCore/qmutex.h>
#include <QtCore/qlist.h>

#include "qgstreamerbushelper_p.h"

QT_BEGIN_NAMESPACE


class QGstreamerBusHelperPrivate : public QObject
{
    Q_OBJECT
public:
    QGstreamerBusHelperPrivate(QGstreamerBusHelper *parent, GstBus* bus) :
        QObject(parent),
        m_bus(bus),
        m_helper(parent)
    {
#ifdef QT_NO_GLIB
        Q_UNUSED(bus);

        m_intervalTimer = new QTimer(this);
        m_intervalTimer->setInterval(250);

        connect(m_intervalTimer, SIGNAL(timeout()), SLOT(interval()));
        m_intervalTimer->start();
#else
        m_tag = gst_bus_add_watch_full(bus, 0, busCallback, this, NULL);
#endif

    }

    ~QGstreamerBusHelperPrivate()
    {
        m_helper = 0;
#ifdef QT_NO_GLIB
        m_intervalTimer->stop();
#else
        g_source_remove(m_tag);
#endif
    }

    GstBus* bus() const { return m_bus; }

private slots:
    void interval()
    {
        GstMessage* message;
        while ((message = gst_bus_poll(m_bus, GST_MESSAGE_ANY, 0)) != 0) {
            processMessage(message);
            gst_message_unref(message);
        }
    }

private:
    void processMessage(GstMessage* message)
    {
        QGstreamerMessage msg(message);
        foreach (QGstreamerBusMessageFilter *filter, busFilters) {
            if (filter->processBusMessage(msg))
                break;
        }
        emit m_helper->message(msg);
    }

    static gboolean busCallback(GstBus *bus, GstMessage *message, gpointer data)
    {
        Q_UNUSED(bus);
        reinterpret_cast<QGstreamerBusHelperPrivate*>(data)->processMessage(message);
        return TRUE;
    }

    guint m_tag;
    GstBus* m_bus;
    QGstreamerBusHelper*  m_helper;
    QTimer*     m_intervalTimer;

public:
    QMutex filterMutex;
    QList<QGstreamerSyncMessageFilter*> syncFilters;
    QList<QGstreamerBusMessageFilter*> busFilters;
};


static GstBusSyncReply syncGstBusFilter(GstBus* bus, GstMessage* message, QGstreamerBusHelperPrivate *d)
{
    Q_UNUSED(bus);
    QMutexLocker lock(&d->filterMutex);

    foreach (QGstreamerSyncMessageFilter *filter, d->syncFilters) {
        if (filter->processSyncMessage(QGstreamerMessage(message)))
            return GST_BUS_DROP;
    }

    return GST_BUS_PASS;
}


/*!
    \class gstreamer::QGstreamerBusHelper
    \internal
*/

QGstreamerBusHelper::QGstreamerBusHelper(GstBus* bus, QObject* parent):
    QObject(parent)
{
    d = new QGstreamerBusHelperPrivate(this, bus);
    gst_bus_set_sync_handler(bus, (GstBusSyncHandler)syncGstBusFilter, d);
}

QGstreamerBusHelper::~QGstreamerBusHelper()
{
    gst_bus_set_sync_handler(d->bus(),0,0);
}

void QGstreamerBusHelper::installMessageFilter(QObject *filter)
{
    QGstreamerSyncMessageFilter *syncFilter = qobject_cast<QGstreamerSyncMessageFilter*>(filter);
    if (syncFilter) {
        QMutexLocker lock(&d->filterMutex);
        if (!d->syncFilters.contains(syncFilter))
            d->syncFilters.append(syncFilter);
    }

    QGstreamerBusMessageFilter *busFilter = qobject_cast<QGstreamerBusMessageFilter*>(filter);
    if (busFilter && !d->busFilters.contains(busFilter))
        d->busFilters.append(busFilter);
}

void QGstreamerBusHelper::removeMessageFilter(QObject *filter)
{
    QGstreamerSyncMessageFilter *syncFilter = qobject_cast<QGstreamerSyncMessageFilter*>(filter);
    if (syncFilter) {
        QMutexLocker lock(&d->filterMutex);
        d->syncFilters.removeAll(syncFilter);
    }

    QGstreamerBusMessageFilter *busFilter = qobject_cast<QGstreamerBusMessageFilter*>(filter);
    if (busFilter)
        d->busFilters.removeAll(busFilter);
}

QT_END_NAMESPACE

#include "qgstreamerbushelper.moc"
