/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGST_P_H
#define QGST_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtmultimediaglobal_p.h>

#include <QtCore/qlist.h>

#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qvideoframe.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class QSize;

template <typename T> struct QGRange
{
    T min;
    T max;
};

class QGString
{
    char *str;
public:
    QGString(char *string) : str(string) {}
    ~QGString() { g_free(str); }
    operator QByteArray() { return QByteArray(str); }
    operator const char *() { return str; }
};

class QGValue
{
public:
    QGValue(const GValue *v) : value(v) {}
    const GValue *value;

    bool isNull() const { return !value; }

    std::optional<bool> toBool() const
    {
        if (!G_VALUE_HOLDS_BOOLEAN(value))
            return std::nullopt;
        return g_value_get_boolean(value);
    }
    std::optional<int> toInt() const
    {
        if (!G_VALUE_HOLDS_INT(value))
            return std::nullopt;
        return g_value_get_int(value);
    }
    const char *toString() const
    {
        return g_value_get_string(value);
    }
    std::optional<float> getFraction() const
    {
        if (!GST_VALUE_HOLDS_FRACTION(value))
            return std::nullopt;
        return (float)gst_value_get_fraction_numerator(value)/(float)gst_value_get_fraction_denominator(value);
    }

    std::optional<QGRange<float>> getFractionRange() const
    {
        if (!GST_VALUE_HOLDS_FRACTION_RANGE(value))
            return std::nullopt;
        QGValue min = gst_value_get_fraction_range_min(value);
        QGValue max = gst_value_get_fraction_range_max(value);
        return QGRange<float>{ *min.getFraction(), *max.getFraction() };
    }

    std::optional<QGRange<int>> toIntRange() const
    {
        if (!GST_VALUE_HOLDS_INT_RANGE(value))
            return std::nullopt;
        return QGRange<int>{ gst_value_get_int_range_min(value), gst_value_get_int_range_max(value) };
    }

    Q_MULTIMEDIA_EXPORT QList<QAudioFormat::SampleFormat> getSampleFormats() const;
};

class QGstStructure {
public:
    GstStructure *structure;
    QGstStructure(GstStructure *s) : structure(s) {}
    void free() { gst_structure_free(structure); structure = nullptr; }

    bool isNull() const { return !structure; }

    QByteArray name() const { return gst_structure_get_name(structure); }

    QGValue operator[](const char *name) const { return gst_structure_get_value(structure, name); }

    Q_MULTIMEDIA_EXPORT QSize resolution() const;
    Q_MULTIMEDIA_EXPORT QVideoFrame::PixelFormat pixelFormat() const;
    Q_MULTIMEDIA_EXPORT QSize pixelAspectRatio() const;
    Q_MULTIMEDIA_EXPORT QGRange<float> frameRateRange() const;

    QByteArray toString() const { return gst_structure_to_string(structure); }
};

class QGstCaps {
    const GstCaps *caps;
public:
    QGstCaps(const GstCaps *c) : caps(c) {}

    bool isNull() const { return !caps; }

    int size() const { return gst_caps_get_size(caps); }
    QGstStructure at(int index) { return gst_caps_get_structure(caps, index); }
    const GstCaps *get() const { return caps; }
    QByteArray toString() const
    {
        gchar *c = gst_caps_to_string(caps);
        QByteArray b(c);
        g_free(c);
        return b;
    }
};

class QGstMutableCaps {
    GstCaps *caps;
public:
    enum RefMode { HasRef, NeedsRef };
    QGstMutableCaps(GstCaps *c, RefMode mode = HasRef)
        : caps(c)
    {
        if (mode == NeedsRef)
            gst_caps_ref(caps);
    }
    QGstMutableCaps(const QGstMutableCaps &other)
        : caps(other.caps)
    {
        if (caps)
            gst_caps_ref(caps);
    }
    QGstMutableCaps &operator=(const QGstMutableCaps &other)
    {
        if (other.caps)
            gst_caps_ref(other.caps);
        if (caps)
            gst_caps_unref(caps);
        caps = other.caps;
        return *this;
    }
    ~QGstMutableCaps() {
        if (caps)
            gst_caps_unref(caps);
    }

    bool isNull() const { return !caps; }

    int size() const { return gst_caps_get_size(caps); }
    QGstStructure at(int index) { return gst_caps_get_structure(caps, index); }
    GstCaps *get() { return caps; }
    QByteArray toString() const
    {
        gchar *c = gst_caps_to_string(caps);
        QByteArray b(c);
        g_free(c);
        return b;
    }
};

class QGstObject
{
protected:
    GstObject *m_object = nullptr;
public:
    enum RefMode { HasRef, NeedsRef };

    QGstObject() = default;
    QGstObject(GstObject *o, RefMode mode = HasRef)
        : m_object(o)
    {
        if (mode == NeedsRef)
            // Use ref_sink to remove any floating references
            gst_object_ref_sink(m_object);
    }
    QGstObject(const QGstObject &other)
        : m_object(other.m_object)
    {
        if (m_object)
            gst_object_ref(m_object);
    }
    QGstObject &operator=(const QGstObject &other)
    {
        if (other.m_object)
            gst_object_ref(other.m_object);
        if (m_object)
            gst_object_unref(m_object);
        m_object = other.m_object;
        return *this;
    }
    ~QGstObject() {
        if (m_object)
            gst_object_unref(m_object);
    }

    friend bool operator==(const QGstObject &a, const QGstObject &b)
    { return a.m_object == b.m_object; }
    friend bool operator!=(const QGstObject &a, const QGstObject &b)
    { return a.m_object != b.m_object; }

    bool isNull() const { return !m_object; }

    void set(const char *property, const char *str) { g_object_set(m_object, property, str, nullptr); }
    void set(const char *property, bool b) { g_object_set(m_object, property, gboolean(b), nullptr); }
    void set(const char *property, uint i) { g_object_set(m_object, property, guint(i), nullptr); }
    void set(const char *property, int i) { g_object_set(m_object, property, gint(i), nullptr); }
    void set(const char *property, qint64 i) { g_object_set(m_object, property, gint64(i), nullptr); }
    void set(const char *property, quint64 i) { g_object_set(m_object, property, guint64(i), nullptr); }
    void set(const char *property, double d) { g_object_set(m_object, property, gdouble(d), nullptr); }

    QGString getString(const char *property) const
    { char *s = nullptr; g_object_get(m_object, property, &s, nullptr); return s; }
    bool getBool(const char *property) const { gboolean b = false; g_object_get(m_object, property, &b, nullptr); return b; }
    uint getUInt(const char *property) const { guint i = 0; g_object_get(m_object, property, &i, nullptr); return i; }
    int getInt(const char *property) const { gint i = 0; g_object_get(m_object, property, &i, nullptr); return i; }
    quint64 getUInt64(const char *property) const { guint64 i = 0; g_object_get(m_object, property, &i, nullptr); return i; }
    qint64 getInt64(const char *property) const { gint64 i = 0; g_object_get(m_object, property, &i, nullptr); return i; }
    double getDouble(const char *property) const { gdouble d = 0; g_object_get(m_object, property, &d, nullptr); return d; }

    GstObject *object() const { return m_object; }
    const char *name() const { return GST_OBJECT_NAME(m_object); }
};

class QGstElement;

class QGstPad : public QGstObject
{
public:
    QGstPad() = default;
    QGstPad(const QGstObject &o)
        : QGstPad(GST_PAD(o.object()), NeedsRef)
    {}
    QGstPad(GstPad *pad, RefMode mode)
        : QGstObject(&pad->object, mode)
    {}

    QGstMutableCaps currentCaps() const
    { return QGstMutableCaps(gst_pad_get_current_caps(pad())); }

    bool isLinked() const { return gst_pad_is_linked(pad()); }
    bool link(const QGstPad &sink) const { return gst_pad_link(pad(), sink.pad()) == GST_PAD_LINK_OK; }
    bool unlink(const QGstPad &sink) const { return gst_pad_unlink(pad(), sink.pad()); }
    QGstPad peer() const { return QGstPad(gst_pad_get_peer(pad()), HasRef); }
    inline QGstElement parent() const;

    GstPad *pad() const { return GST_PAD_CAST(object()); }

    template<auto Member, typename T>
    void addProbe(T *instance, GstPadProbeType type) {
        struct Impl {
            static GstPadProbeReturn callback(GstPad * pad, GstPadProbeInfo */*info*/, gpointer userData) {
                (static_cast<T *>(userData)->*Member)(QGstPad(pad, NeedsRef));
                return GST_PAD_PROBE_REMOVE;
            };
        };

        gst_pad_add_probe (pad(), type, Impl::callback, instance, NULL);
    }
};

class QGstElement : public QGstObject
{
public:
    QGstElement() = default;
    QGstElement(const QGstObject &o)
        : QGstElement(GST_ELEMENT(o.object()), NeedsRef)
    {}
    QGstElement(GstElement *element, RefMode mode)
        : QGstObject(&element->object, mode)
    {}

    QGstElement(const char *factory, const char *name = nullptr)
        : QGstElement(gst_element_factory_make(factory, name), NeedsRef)
    {
    }

    bool link(const QGstElement &next)
    { return gst_element_link(element(), next.element()); }
    bool link(const QGstElement &n1, const QGstElement &n2)
    { return gst_element_link_many(element(), n1.element(), n2.element(), nullptr); }
    bool link(const QGstElement &n1, const QGstElement &n2, const QGstElement &n3)
    { return gst_element_link_many(element(), n1.element(), n2.element(), n3.element(), nullptr); }
    bool link(const QGstElement &n1, const QGstElement &n2, const QGstElement &n3, const QGstElement &n4)
    { return gst_element_link_many(element(), n1.element(), n2.element(), n3.element(), n4.element(), nullptr); }
    bool link(const QGstElement &n1, const QGstElement &n2, const QGstElement &n3, const QGstElement &n4, const QGstElement &n5)
    { return gst_element_link_many(element(), n1.element(), n2.element(), n3.element(), n4.element(), n5.element(), nullptr); }

    QGstPad staticPad(const char *name) { return QGstPad(gst_element_get_static_pad(element(), name), HasRef); }
    QGstPad getRequestPad(const char *name) { return QGstPad(gst_element_get_request_pad(element(), name), HasRef); }
    void releaseRequestPad(const QGstPad &pad) { return gst_element_release_request_pad(element(), pad.pad()); }

    GstState state() const
    {
        GstState state;
        gst_element_get_state(element(), &state, nullptr, 0);
        return state;
    }
    GstStateChangeReturn setState(GstState state) { return gst_element_set_state(element(), state); }
    bool setStateSync(GstState state)
    {
        auto change = gst_element_set_state(element(), state);
        if (change == GST_STATE_CHANGE_ASYNC) {
            change = gst_element_get_state(element(), nullptr, &state, 100*1e6 /*100ms*/);
        }
        return change == GST_STATE_CHANGE_SUCCESS;
    }

    bool seek(qint64 pos, double rate)
    {
        return gst_element_seek(element(), rate, GST_FORMAT_TIME,
                                 GstSeekFlags(GST_SEEK_FLAG_FLUSH),
                                 GST_SEEK_TYPE_SET, pos,
                                 GST_SEEK_TYPE_SET, -1);
    }
    qint64 duration() const
    {
        gint64 d;
        if (!gst_element_query_duration(element(), GST_FORMAT_TIME, &d))
            return 0.;
        return d;
    }
    qint64 position() const
    {
        gint64 pos;
        if (!gst_element_query_position(element(), GST_FORMAT_TIME, &pos))
            return 0.;
        return pos;
    }

    template<auto Member, typename T>
    void onPadAdded(T *instance) {
        struct Impl {
            static void callback(GstElement *e, GstPad *pad, gpointer userData) {
                (static_cast<T *>(userData)->*Member)(QGstElement(e, NeedsRef), QGstPad(pad, NeedsRef));
            };
        };

        g_signal_connect (element(), "pad-added", G_CALLBACK(Impl::callback), instance);
    }
    template<auto Member, typename T>
    void onPadRemoved(T *instance) {
        struct Impl {
            static void callback(GstElement *e, GstPad *pad, gpointer userData) {
                (static_cast<T *>(userData)->*Member)(QGstElement(e, NeedsRef), QGstPad(pad, NeedsRef));
            };
        };

        g_signal_connect (element(), "pad-removed", G_CALLBACK(Impl::callback), instance);
    }

    GstElement *element() const { return GST_ELEMENT_CAST(m_object); }
};

inline QGstElement QGstPad::parent() const
{
    return QGstElement(gst_pad_get_parent_element(pad()), HasRef);
}

class QGstBin : public QGstElement
{
public:
    QGstBin(const QGstObject &o)
        : QGstBin(GST_BIN(o.object()), NeedsRef)
    {}
    QGstBin(const char *name = nullptr)
        : QGstElement(gst_bin_new(name), NeedsRef)
    {
    }
    QGstBin(GstBin *bin, RefMode mode)
        : QGstElement(&bin->element, mode)
    {}

    void add(const QGstElement &element)
    { gst_bin_add(bin(), element.element()); }
    void add(const QGstElement &e1, const QGstElement &e2)
    { gst_bin_add_many(bin(), e1.element(), e2.element(), nullptr); }
    void add(const QGstElement &e1, const QGstElement &e2, const QGstElement &e3)
    { gst_bin_add_many(bin(), e1.element(), e2.element(), e3.element(), nullptr); }
    void add(const QGstElement &e1, const QGstElement &e2, const QGstElement &e3, const QGstElement &e4)
    { gst_bin_add_many(bin(), e1.element(), e2.element(), e3.element(), e4.element(), nullptr); }
    void add(const QGstElement &e1, const QGstElement &e2, const QGstElement &e3, const QGstElement &e4, const QGstElement &e5)
    { gst_bin_add_many(bin(), e1.element(), e2.element(), e3.element(), e4.element(), e5.element(), nullptr); }
    void add(const QGstElement &e1, const QGstElement &e2, const QGstElement &e3, const QGstElement &e4, const QGstElement &e5, const QGstElement &e6)
    { gst_bin_add_many(bin(), e1.element(), e2.element(), e3.element(), e4.element(), e5.element(), e6.element(), nullptr); }
    void remove(const QGstElement &element)
    { gst_bin_remove(bin(), element.element()); }

    GstBin *bin() const { return GST_BIN_CAST(m_object); }
};

class QGstBus : public QGstObject
{
public:
    QGstBus() = default;
    QGstBus(GstBus *bus) : QGstObject(&bus->object, HasRef) {}

    GstBus *bus() { return GST_BUS_CAST(object()); }
};

class QGstPipeline : public QGstBin
{
public:
    QGstPipeline(const QGstObject &o)
        : QGstPipeline(GST_PIPELINE(o.object()), NeedsRef)
    {}
    QGstPipeline(const char *name = nullptr)
        : QGstBin(GST_BIN(gst_pipeline_new(name)), NeedsRef)
    {
    }
    QGstPipeline(GstPipeline *p, RefMode mode)
        : QGstBin(&p->bin, mode)
    {}

    GstPipeline *pipeline() const { return GST_PIPELINE_CAST(m_object); }
    QGstBus bus() { return gst_element_get_bus(element()); }
};

QT_END_NAMESPACE

#endif
