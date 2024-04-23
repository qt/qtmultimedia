// Copyright (C) 2016 Jolla Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgstreamerbufferprobe_p.h>

#include <common/qgst_p.h>

QT_BEGIN_NAMESPACE

QGstreamerBufferProbe::QGstreamerBufferProbe(Flags flags)
    : m_flags(flags)
{
}

QGstreamerBufferProbe::~QGstreamerBufferProbe() = default;

void QGstreamerBufferProbe::addProbeToPad(GstPad *pad, bool downstream)
{
    QGstCaps caps{
        gst_pad_get_current_caps(pad),
        QGstCaps::HasRef,
    };

    if (caps)
        probeCaps(caps.caps());

    if (m_flags & ProbeCaps) {
        m_capsProbeId = gst_pad_add_probe(
                    pad,
                    downstream
                        ? GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM
                        : GST_PAD_PROBE_TYPE_EVENT_UPSTREAM,
                    capsProbe,
                    this,
                    nullptr);
    }
    if (m_flags & ProbeBuffers) {
        m_bufferProbeId = gst_pad_add_probe(
                    pad, GST_PAD_PROBE_TYPE_BUFFER, bufferProbe, this, nullptr);
    }
}

void QGstreamerBufferProbe::removeProbeFromPad(GstPad *pad)
{
    if (m_capsProbeId != -1) {
        gst_pad_remove_probe(pad, m_capsProbeId);
        m_capsProbeId = -1;
    }
    if (m_bufferProbeId != -1) {
        gst_pad_remove_probe(pad, m_bufferProbeId);
        m_bufferProbeId = -1;
    }
}

void QGstreamerBufferProbe::probeCaps(GstCaps *)
{
}

bool QGstreamerBufferProbe::probeBuffer(GstBuffer *)
{
    return true;
}

GstPadProbeReturn QGstreamerBufferProbe::capsProbe(GstPad *, GstPadProbeInfo *info, gpointer user_data)
{
    QGstreamerBufferProbe * const control = static_cast<QGstreamerBufferProbe *>(user_data);

    if (GstEvent * const event = gst_pad_probe_info_get_event(info)) {
        if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
            GstCaps *caps;
            gst_event_parse_caps(event, &caps);

            control->probeCaps(caps);
        }
    }
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn QGstreamerBufferProbe::bufferProbe(
        GstPad *, GstPadProbeInfo *info, gpointer user_data)
{
    QGstreamerBufferProbe * const control = static_cast<QGstreamerBufferProbe *>(user_data);
    if (GstBuffer * const buffer = gst_pad_probe_info_get_buffer(info))
        return control->probeBuffer(buffer) ? GST_PAD_PROBE_OK : GST_PAD_PROBE_DROP;
    return GST_PAD_PROBE_OK;
}

QT_END_NAMESPACE
