// Copyright (C) 2021 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstsubtitlesink_p.h"
#include "qgst_debug_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

namespace {
GstBaseSinkClass *gst_sink_parent_class;
thread_local QAbstractSubtitleObserver *gst_current_observer;

class QGstSubtitleSinkClass
{
public:
    GstBaseSinkClass parent_class;
};

} // namespace

#define ST_SINK(s) QGstSubtitleSink *sink(reinterpret_cast<QGstSubtitleSink *>(s))

QGstElement QGstSubtitleSink::createSink(QAbstractSubtitleObserver *observer)
{
    gst_current_observer = observer;

    QGstSubtitleSink *gstSink = reinterpret_cast<QGstSubtitleSink *>(
            g_object_new(QGstSubtitleSink::get_type(), nullptr));

    return QGstElement{
        qGstCheckedCast<GstElement>(gstSink),
        QGstElement::NeedsRef,
    };
}

GType QGstSubtitleSink::get_type()
{
    // clang-format off
    static constexpr GTypeInfo info =
    {
        sizeof(QGstSubtitleSinkClass), // class_size
        base_init,                     // base_init
        nullptr,                       // base_finalize
        class_init,                    // class_init
        nullptr,                       // class_finalize
        nullptr,                       // class_data
        sizeof(QGstSubtitleSink),      // instance_size
        0,                             // n_preallocs
        instance_init,                 // instance_init
        nullptr                        // value_table
    };
    // clang-format on

    static const GType type = []() {
        const auto result = g_type_register_static(
                GST_TYPE_BASE_SINK, "QGstSubtitleSink", &info, GTypeFlags(0));
        return result;
    }();

    return type;
}

void QGstSubtitleSink::class_init(gpointer g_class, gpointer class_data)
{
    Q_UNUSED(class_data);

    gst_sink_parent_class = reinterpret_cast<GstBaseSinkClass *>(g_type_class_peek_parent(g_class));

    GstBaseSinkClass *base_sink_class = reinterpret_cast<GstBaseSinkClass *>(g_class);
    base_sink_class->render = QGstSubtitleSink::render;
    base_sink_class->get_caps = QGstSubtitleSink::get_caps;
    base_sink_class->set_caps = QGstSubtitleSink::set_caps;
    base_sink_class->propose_allocation = QGstSubtitleSink::propose_allocation;
    base_sink_class->wait_event = QGstSubtitleSink::wait_event;

    GstElementClass *element_class = reinterpret_cast<GstElementClass *>(g_class);
    element_class->change_state = QGstSubtitleSink::change_state;
    gst_element_class_set_metadata(element_class,
        "Qt built-in subtitle sink",
        "Sink/Subtitle",
        "Qt default built-in subtitle sink",
        "The Qt Company");

    GObjectClass *object_class = reinterpret_cast<GObjectClass *>(g_class);
    object_class->finalize = QGstSubtitleSink::finalize;
}

void QGstSubtitleSink::base_init(gpointer g_class)
{
    static GstStaticPadTemplate sink_pad_template =
            GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("ANY"));

    gst_element_class_add_pad_template(
            GST_ELEMENT_CLASS(g_class), gst_static_pad_template_get(&sink_pad_template));
}

void QGstSubtitleSink::instance_init(GTypeInstance *instance, gpointer /*g_class*/)
{
    ST_SINK(instance);

    Q_ASSERT(gst_current_observer);
    sink->observer = gst_current_observer;
    gst_current_observer = nullptr;
}

void QGstSubtitleSink::finalize(GObject *object)
{
    // Chain up
    G_OBJECT_CLASS(gst_sink_parent_class)->finalize(object);
}

GstStateChangeReturn QGstSubtitleSink::change_state(GstElement *element, GstStateChange transition)
{
    return GST_ELEMENT_CLASS(gst_sink_parent_class)->change_state(element, transition);
}

GstCaps *QGstSubtitleSink::get_caps(GstBaseSink *base, GstCaps *filter)
{
    return gst_sink_parent_class->get_caps(base, filter);
}

gboolean QGstSubtitleSink::set_caps(GstBaseSink *base, GstCaps *caps)
{
    qDebug() << "set_caps:" << caps;
    return gst_sink_parent_class->set_caps(base, caps);
}

gboolean QGstSubtitleSink::propose_allocation(GstBaseSink *base, GstQuery *query)
{
    return gst_sink_parent_class->propose_allocation(base, query);
}

GstFlowReturn QGstSubtitleSink::wait_event(GstBaseSink *base, GstEvent *event)
{
    GstFlowReturn retval = gst_sink_parent_class->wait_event(base, event);
    ST_SINK(base);
    if (event->type == GST_EVENT_GAP) {
        // qDebug() << "gap, clearing subtitle";
        sink->observer->updateSubtitle(QString());
    }
    return retval;
}

GstFlowReturn QGstSubtitleSink::render(GstBaseSink *base, GstBuffer *buffer)
{
    ST_SINK(base);
    GstMemory *mem = gst_buffer_get_memory(buffer, 0);
    GstMapInfo info;
    QString subtitle;
    if (gst_memory_map(mem, &info, GST_MAP_READ))
        subtitle = QString::fromUtf8(reinterpret_cast<const char *>(info.data));
    gst_memory_unmap(mem, &info);
//    qDebug() << "render" << buffer << subtitle;
    sink->observer->updateSubtitle(subtitle);
    return GST_FLOW_OK;
}

QT_END_NAMESPACE
