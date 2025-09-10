// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgst_debug_p.h"
#include "qgstreamermessage_p.h"

#include <gst/gstclock.h>

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

QDebug operator<<(QDebug dbg, const QGstStructureView &structure)
{
    return dbg << structure.structure;
}

QDebug operator<<(QDebug dbg, const QUniqueGstStructureHandle &structure)
{
    return dbg << QGstStructureView{structure};
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

QDebug operator<<(QDebug dbg, const QUniqueGStringHandle &handle)
{
    return dbg << handle.get();
}

QDebug operator<<(QDebug dbg, const QGstStreamCollectionHandle &handle)
{
    return dbg << handle.get();
}

QDebug operator<<(QDebug dbg, const QGstStreamHandle &handle)
{
    return dbg << handle.get();
}

QDebug operator<<(QDebug dbg, const QGstTagListHandle &handle)
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
    return dbg << QGstCaps{
        gst_video_info_to_caps(info),
        QGstCaps::NeedsRef,
    };
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
    if (!object)
        return dbg << "null";

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
    if (!device)
        return dbg << "null";

    GstDevice *d = const_cast<GstDevice *>(device);
    QDebugStateSaver saver(dbg);
    dbg.nospace();

    dbg << gst_device_get_display_name(d) << "(" << gst_device_get_device_class(d) << ") ";
    dbg << "Caps: " << QGstCaps{ gst_device_get_caps(d), QGstCaps::NeedsRef, } << ", ";
    dbg << "Properties: " << QUniqueGstStructureHandle{ gst_device_get_properties(d) }.get();
    return dbg;
}

namespace {

struct Timepoint
{
    explicit Timepoint(guint64 us) : ts{ us } { }
    guint64 ts;
};

QDebug operator<<(QDebug dbg, Timepoint ts)
{
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%" GST_TIME_FORMAT, GST_TIME_ARGS(ts.ts));
    dbg << buffer;
    return dbg;
}

} // namespace

QDebug operator<<(QDebug dbg, const GstMessage *msg)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();

    dbg << GST_MESSAGE_TYPE_NAME(msg) << ", Source: " << GST_MESSAGE_SRC_NAME(msg);
    if (GST_MESSAGE_TIMESTAMP(msg) != 0xFFFFFFFFFFFFFFFF)
        dbg << ", Timestamp: " << GST_MESSAGE_TIMESTAMP(msg);

    switch (msg->type) {
    case GST_MESSAGE_ERROR: {
        QUniqueGErrorHandle err;
        QGString debug;
        gst_message_parse_error(const_cast<GstMessage *>(msg), &err, &debug);

        dbg << ", Error: " << err << " (" << debug << ")";
        break;
    }

    case GST_MESSAGE_WARNING: {
        QUniqueGErrorHandle err;
        QGString debug;
        gst_message_parse_warning(const_cast<GstMessage *>(msg), &err, &debug);

        dbg << ", Warning: " << err << " (" << debug << ")";
        break;
    }

    case GST_MESSAGE_INFO: {
        QUniqueGErrorHandle err;
        QGString debug;
        gst_message_parse_info(const_cast<GstMessage *>(msg), &err, &debug);

        dbg << ", Info: " << err << " (" << debug << ")";
        break;
    }

    case GST_MESSAGE_TAG: {
        QGstTagListHandle tagList;
        gst_message_parse_tag(const_cast<GstMessage *>(msg), &tagList);

        dbg << ", Tags: " << tagList;
        break;
    }

    case GST_MESSAGE_QOS: {
        gboolean live;
        guint64 running_time;
        guint64 stream_time;
        guint64 timestamp;
        guint64 duration;

        gst_message_parse_qos(const_cast<GstMessage *>(msg), &live, &running_time, &stream_time,
                              &timestamp, &duration);

        dbg << ", Live: " << bool(live) << ", Running time: " << Timepoint{ running_time }
            << ", Stream time: " << Timepoint{ stream_time }
            << ", Timestamp: " << Timepoint{ timestamp } << ", Duration: " << Timepoint{ duration };
        break;
    }

    case GST_MESSAGE_STATE_CHANGED: {
        GstState oldState;
        GstState newState;
        GstState pending;

        gst_message_parse_state_changed(const_cast<GstMessage *>(msg), &oldState, &newState,
                                        &pending);

        dbg << ", Transition: " << oldState << "->" << newState;

        if (pending != GST_STATE_VOID_PENDING)
            dbg << ", Pending State: " << pending;
        break;
    }

    case GST_MESSAGE_STREAM_COLLECTION: {
        QGstStreamCollectionHandle collection;
        gst_message_parse_stream_collection(const_cast<GstMessage *>(msg), &collection);

        dbg << ", " << collection;
        break;
    }

    case GST_MESSAGE_STREAMS_SELECTED: {
        QGstStreamCollectionHandle collection;
        gst_message_parse_streams_selected(const_cast<GstMessage *>(msg), &collection);

        dbg << ", " << collection;
        break;
    }

    case GST_MESSAGE_STREAM_STATUS: {
        GstStreamStatusType streamStatus;
        gst_message_parse_stream_status(const_cast<GstMessage *>(msg), &streamStatus, nullptr);

        dbg << ", Stream Status: " << streamStatus;
        break;
    }

    case GST_MESSAGE_BUFFERING: {
        int progress = 0;
        gst_message_parse_buffering(const_cast<GstMessage *>(msg), &progress);

        dbg << ", Buffering: " << progress << "%";
        break;
    }

    case GST_MESSAGE_SEGMENT_START: {
        gint64 pos;
        GstFormat fmt{};
        gst_message_parse_segment_start(const_cast<GstMessage *>(msg), &fmt, &pos);

        switch (fmt) {
        case GST_FORMAT_TIME: {
            dbg << ", Position: " << std::chrono::nanoseconds{ pos };
            break;
        }
        case GST_FORMAT_BYTES: {
            dbg << ", Position: " << pos << "Bytes";
            break;
        }
        default: {
            dbg << ", Position: " << pos;
            break;
        }
        }

        break;
    }

    default:
        break;
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, const GstTagList *tagList)
{
    if (tagList)
        dbg << QGString{ gst_tag_list_to_string(tagList) };
    else
        dbg << "NULL";

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

QDebug operator<<(QDebug dbg, const GstPadTemplate *padTemplate)
{
    QGstCaps caps = padTemplate
            ? QGstCaps{ gst_pad_template_get_caps(const_cast<GstPadTemplate *>(padTemplate)), QGstCaps::HasRef, }
            : QGstCaps{};

    dbg << caps;
    return dbg;
}

QDebug operator<<(QDebug dbg, const GstStreamCollection *streamCollection)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();

    GstStreamCollection *collection = const_cast<GstStreamCollection *>(streamCollection);
    dbg << "Stream Collection: {";

    qForeachStreamInCollection(collection, [&](GstStream *stream) {
        dbg << stream << ", ";
    });

    dbg << "}";
    return dbg;
}

QDebug operator<<(QDebug dbg, const GstStream *cstream)
{
    GstStream *stream = const_cast<GstStream *>(cstream);

    QDebugStateSaver saver(dbg);
    dbg.nospace();

    dbg << gst_stream_get_stream_id(stream) << " (" << gst_stream_get_stream_type(stream) << ")";

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

#define ADD_ENUM_SWITCH(value) \
    case value:                \
        return dbg << #value;  \
        static_assert(true, "enforce semicolon")

QDebug operator<<(QDebug dbg, GstPadDirection direction)
{
    switch (direction) {
        ADD_ENUM_SWITCH(GST_PAD_UNKNOWN);
        ADD_ENUM_SWITCH(GST_PAD_SRC);
        ADD_ENUM_SWITCH(GST_PAD_SINK);
    default:
        Q_UNREACHABLE_RETURN(dbg);
    }
}

QDebug operator<<(QDebug dbg, GstStreamStatusType type)
{
    switch (type) {
        ADD_ENUM_SWITCH(GST_STREAM_STATUS_TYPE_CREATE);
        ADD_ENUM_SWITCH(GST_STREAM_STATUS_TYPE_ENTER);
        ADD_ENUM_SWITCH(GST_STREAM_STATUS_TYPE_LEAVE);
        ADD_ENUM_SWITCH(GST_STREAM_STATUS_TYPE_DESTROY);
        ADD_ENUM_SWITCH(GST_STREAM_STATUS_TYPE_START);
        ADD_ENUM_SWITCH(GST_STREAM_STATUS_TYPE_PAUSE);
        ADD_ENUM_SWITCH(GST_STREAM_STATUS_TYPE_STOP);
    default:
        Q_UNREACHABLE_RETURN(dbg);
    }
    return dbg;
}

#undef ADD_ENUM_SWITCH

QDebug operator<<(QDebug dbg, GstStreamType streamType)
{
    dbg << gst_stream_type_get_name(streamType);
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

    if (G_VALUE_TYPE(value) == GST_TYPE_PAD_DIRECTION) {
        GstPadDirection direction = static_cast<GstPadDirection>(g_value_get_enum(value));
        return dbg << direction;
    }

    if (G_VALUE_TYPE(value) == GST_TYPE_PAD_TEMPLATE) {
        GstPadTemplate *padTemplate = static_cast<GstPadTemplate *>(g_value_get_object(value));
        return dbg << padTemplate;
    }

    dbg << "(not implemented: " << G_VALUE_TYPE_NAME(value) << ")";

    return dbg;
}

QDebug operator<<(QDebug dbg, const GError *error)
{
    return dbg << error->message;
}

QCompactGstMessageAdaptor::QCompactGstMessageAdaptor(const QGstreamerMessage &m)
    : QCompactGstMessageAdaptor{
          m.message(),
      }
{
}

QCompactGstMessageAdaptor::QCompactGstMessageAdaptor(GstMessage *m)
    : msg{
          m,
      }
{
}

QDebug operator<<(QDebug dbg, const QCompactGstMessageAdaptor &m)
{
    std::optional<QDebugStateSaver> saver(dbg);
    dbg.nospace();

    switch (GST_MESSAGE_TYPE(m.msg)) {
    case GST_MESSAGE_ERROR: {
        QUniqueGErrorHandle err;
        QGString debug;
        gst_message_parse_error(m.msg, &err, &debug);
        dbg << err << " (" << debug << ")";
        return dbg;
    }

    case GST_MESSAGE_WARNING: {
        QUniqueGErrorHandle err;
        QGString debug;
        gst_message_parse_warning(m.msg, &err, &debug);
        dbg << err << " (" << debug << ")";
        return dbg;
    }

    case GST_MESSAGE_INFO: {
        QUniqueGErrorHandle err;
        QGString debug;
        gst_message_parse_info(m.msg, &err, &debug);

        dbg << err << " (" << debug << ")";
        return dbg;
    }

    case GST_MESSAGE_STATE_CHANGED: {
        GstState oldState;
        GstState newState;
        GstState pending;

        gst_message_parse_state_changed(m.msg, &oldState, &newState, &pending);

        dbg << oldState << " -> " << newState;
        if (pending != GST_STATE_VOID_PENDING)
            dbg << " (pending: " << pending << ")";
        return dbg;
    }

    default: {
        saver.reset();
        return dbg << m.msg;
    }
    }
}

QDebug operator<<(QDebug dbg, GstPlayMessage type)
{
    return dbg << gst_play_message_get_name(type);
}

QDebug operator<<(QDebug dbg, GstPlayState state)
{
    return dbg << gst_play_state_get_name(state);
}

QGstPlayMessageAdaptor::QGstPlayMessageAdaptor(const QGstreamerMessage &m)
    : QGstPlayMessageAdaptor{
          m.message(),
      }
{
}

QGstPlayMessageAdaptor::QGstPlayMessageAdaptor(GstMessage *m)
    : msg{
          m,
      }
{
}

QDebug operator<<(QDebug dbg, const QGstPlayMessageAdaptor &m)
{
    using namespace std::chrono;
    Q_ASSERT(gst_play_is_play_message(m.msg));

    std::optional<QDebugStateSaver> saver(dbg);

    GstPlayMessage type;
    gst_play_message_parse_type(m.msg, &type);

    switch (type) {
    case GST_PLAY_MESSAGE_URI_LOADED:
    case GST_PLAY_MESSAGE_END_OF_STREAM:
    case GST_PLAY_MESSAGE_MEDIA_INFO_UPDATED:
    case GST_PLAY_MESSAGE_VOLUME_CHANGED:
    case GST_PLAY_MESSAGE_MUTE_CHANGED:
    case GST_PLAY_MESSAGE_SEEK_DONE:
        return dbg << type;

    case GST_PLAY_MESSAGE_POSITION_UPDATED: {
        GstClockTime position;
        gst_play_message_parse_position_updated(m.msg, &position);
        return dbg << type << nanoseconds(position);
    }
    case GST_PLAY_MESSAGE_DURATION_CHANGED: {
        GstClockTime duration;
        gst_play_message_parse_duration_updated(m.msg, &duration);
        return dbg << type << nanoseconds(duration);
    }

    case GST_PLAY_MESSAGE_STATE_CHANGED: {
        GstPlayState state;
        gst_play_message_parse_state_changed(m.msg, &state);
        return dbg << type << state;
    }
    case GST_PLAY_MESSAGE_BUFFERING: {
        guint percent;
        gst_play_message_parse_buffering_percent(m.msg, &percent);
        return dbg << type << percent;
    }
    case GST_PLAY_MESSAGE_ERROR: {
        QUniqueGErrorHandle err;
        QUniqueGstStructureHandle details;
        gst_play_message_parse_error(m.msg, &err, &details);
        return dbg << type << err << details.get();
    }
    case GST_PLAY_MESSAGE_WARNING: {
        QUniqueGErrorHandle err;
        QUniqueGstStructureHandle details;
        gst_play_message_parse_warning(m.msg, &err, &details);
        return dbg << type << err << details.get();
    };

    case GST_PLAY_MESSAGE_VIDEO_DIMENSIONS_CHANGED: {
        guint width, height;
        gst_play_message_parse_video_dimensions_changed(m.msg, &width, &height);
        return dbg << type << QSize(width, height);
    }
    default:
        Q_UNREACHABLE_RETURN(dbg);
    }
}

QT_END_NAMESPACE
