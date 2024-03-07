// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgst_debug_p.h"
#include "qgstreamermessage_p.h"

QT_BEGIN_NAMESPACE

// NOLINTBEGIN(performance-unnecessary-value-param)

QDebug operator<<(QDebug dbg, const QGString &str)
{
    return dbg << str.get();
}

QDebug operator<<(QDebug dbg, const QGstCaps &caps)
{
    return dbg << caps.caps();
}

QDebug operator<<(QDebug dbg, const QGstStructure &structure)
{
    return dbg << structure.structure;
}

QDebug operator<<(QDebug dbg, const QGValue &value)
{
    return dbg << value.value;
}

QDebug operator<<(QDebug dbg, const QGstreamerMessage &msg)
{
    return dbg << msg.message();
}

QDebug operator<<(QDebug dbg, const QUniqueGErrorHandle &handle)
{
    return dbg << handle.get();
}

QDebug operator<<(QDebug dbg, const QGstElement &element)
{
    return dbg << element.element();
}

QDebug operator<<(QDebug dbg, const QGstPad &pad)
{
    return dbg << pad.pad();
}

QDebug operator<<(QDebug dbg, const GstCaps *caps)
{
    if (caps)
        return dbg << QGString(gst_caps_to_string(caps));
    else
        return dbg << "null";
}

QDebug operator<<(QDebug dbg, const GstVideoInfo *info)
{
#if GST_CHECK_VERSION(1, 20, 0)
    return dbg << QGstCaps(gst_video_info_to_caps(info));
#else
    return dbg << QGstCaps(gst_video_info_to_caps(const_cast<GstVideoInfo *>(info)));
#endif
}

QDebug operator<<(QDebug dbg, const GstStructure *structure)
{
    if (structure)
        return dbg << QGString(gst_structure_to_string(structure));
    else
        return dbg << "null";
}

QDebug operator<<(QDebug dbg, const GstObject *object)
{
    dbg << QGString{gst_object_get_name(const_cast<GstObject*>(object))};

    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();

        dbg << "{";

        guint numProperties;
        GParamSpec **properties = g_object_class_list_properties(G_OBJECT_GET_CLASS(object), &numProperties);

        for (guint i = 0; i < numProperties; i++) {
            GParamSpec *param = properties[i];

            const gchar *name = g_param_spec_get_name(param);
            constexpr bool trace_blurb = false;
            if constexpr (trace_blurb) {
                const gchar *blurb = g_param_spec_get_blurb(param);
                dbg << name << " (" << blurb << "): ";
            } else
                dbg << name << ": ";

            bool readable = bool(param->flags & G_PARAM_READABLE);
            if (!readable) {
                dbg << "(not readable)";
            } else if (QLatin1StringView(name) == QLatin1StringView("parent")) {
                if (object->parent)
                    dbg << QGString{ gst_object_get_name(object->parent) };
                else
                    dbg << "(none)";
            } else {
                GValue value = {};
                g_object_get_property(&const_cast<GstObject *>(object)->object, param->name,
                                      &value);
                dbg << &value;
            }
            if (i != numProperties - 1)
                dbg << ", ";
        }

        dbg << "}";

        g_free(properties);
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, const GstElement *element)
{
    return dbg << GST_OBJECT_CAST(element); // LATER: output other members?
}

QDebug operator<<(QDebug dbg, const GstPad *pad)
{
    return dbg << GST_OBJECT_CAST(pad); // LATER: output other members?
}

QDebug operator<<(QDebug dbg, const GstDevice *device)
{
    GstDevice *d = const_cast<GstDevice *>(device);
    QDebugStateSaver saver(dbg);
    dbg.nospace();

    dbg << gst_device_get_display_name(d) << "(" << gst_device_get_device_class(d) << ") ";
    dbg << "Caps: " << QGstCaps(gst_device_get_caps(d)) << ", ";
    dbg << "Properties: " << QUniqueGstStructureHandle{ gst_device_get_properties(d) }.get();
    return dbg;
}

QDebug operator<<(QDebug dbg, const GstMessage *msg)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();

    dbg << GST_MESSAGE_TYPE_NAME(msg) << ", Source: " << GST_MESSAGE_SRC_NAME(msg)
        << ", Timestamp: " << GST_MESSAGE_TIMESTAMP(msg);
    return dbg;
}

QDebug operator<<(QDebug dbg, const GstTagList *tagList)
{
    dbg << QGString{ gst_tag_list_to_string(tagList) };
    return dbg;
}

QDebug operator<<(QDebug dbg, const GstQuery *query)
{
    dbg << GST_QUERY_TYPE_NAME(query);
    return dbg;
}

QDebug operator<<(QDebug dbg, const GstEvent *event)
{
    dbg << GST_EVENT_TYPE_NAME(event);
    return dbg;
}

QDebug operator<<(QDebug dbg, GstState state)
{
    return dbg << gst_element_state_get_name(state);
}

QDebug operator<<(QDebug dbg, GstStateChange transition)
{
    return dbg << gst_state_change_get_name(transition);
}

QDebug operator<<(QDebug dbg, GstStateChangeReturn stateChangeReturn)
{
    return dbg << gst_element_state_change_return_get_name(stateChangeReturn);
}

QDebug operator<<(QDebug dbg, GstMessageType type)
{
    return dbg << gst_message_type_get_name(type);
}

QDebug operator<<(QDebug dbg, GstPadDirection direction)
{
    switch (direction) {
    case GST_PAD_UNKNOWN:
        return dbg << "GST_PAD_UNKNOWN";
    case GST_PAD_SRC:
        return dbg << "GST_PAD_SRC";
    case GST_PAD_SINK:
        return dbg << "GST_PAD_SINK";
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, const GValue *value)
{
    switch (G_VALUE_TYPE(value)) {
    case G_TYPE_STRING:
        return dbg << g_value_get_string(value);
    case G_TYPE_BOOLEAN:
        return dbg << g_value_get_boolean(value);
    case G_TYPE_ULONG:
        return dbg << g_value_get_ulong(value);
    case G_TYPE_LONG:
        return dbg << g_value_get_long(value);
    case G_TYPE_UINT:
        return dbg << g_value_get_uint(value);
    case G_TYPE_INT:
        return dbg << g_value_get_int(value);
    case G_TYPE_UINT64:
        return dbg << g_value_get_uint64(value);
    case G_TYPE_INT64:
        return dbg << g_value_get_int64(value);
    case G_TYPE_FLOAT:
        return dbg << g_value_get_float(value);
    case G_TYPE_DOUBLE:
        return dbg << g_value_get_double(value);
    default:
        break;
    }

    if (GST_VALUE_HOLDS_BITMASK(value)) {
        QDebugStateSaver saver(dbg);
        return dbg << Qt::hex << gst_value_get_bitmask(value);
    }

    if (GST_VALUE_HOLDS_FRACTION(value))
        return dbg << gst_value_get_fraction_numerator(value) << "/"
                   << gst_value_get_fraction_denominator(value);

    if (GST_VALUE_HOLDS_CAPS(value))
        return dbg << gst_value_get_caps(value);

    if (GST_VALUE_HOLDS_STRUCTURE(value))
        return dbg << gst_value_get_structure(value);

    if (GST_VALUE_HOLDS_ARRAY(value)) {
        const guint size = gst_value_array_get_size(value);
        const guint last = size - 1;
        dbg << "[";
        for (guint index = 0; index != size; ++index) {
            dbg << gst_value_array_get_value(value, index);
            if (index != last)
                dbg << ", ";
        }
        dbg << "}";
        return dbg;
    }

    dbg << "(not implemented: " << G_VALUE_TYPE_NAME(value) << ")";

    return dbg;
}

QDebug operator<<(QDebug dbg, const GError *error)
{
    return dbg << error->message;
}

QT_END_NAMESPACE
