// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include <QSemaphore>
#include <QtCore/qlist.h>

#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qvideoframe.h>

#include <gst/gst.h>
#include <gst/video/video-info.h>

#include <functional>

#if QT_CONFIG(gstreamer_photography)
#define GST_USE_UNSTABLE_API
#include <gst/interfaces/photography.h>
#undef GST_USE_UNSTABLE_API
#endif
#include <qdebug.h>

QT_BEGIN_NAMESPACE

class QSize;
class QGstStructure;
class QGstCaps;
class QGstPipelinePrivate;
class QCameraFormat;

template <typename T> struct QGRange
{
    T min;
    T max;
};

class QGString
{
    char *str;
public:
    QGString(const QGString &) = delete;
    QGString(QGString &&) = delete;

    explicit QGString(char *string) : str(string) { }
    ~QGString() { g_free(str); }

    QGString &operator=(const QGString &) = delete;
    QGString &operator=(QGString &&) = delete;

    explicit operator QByteArray() const { return QByteArray(str); }
    explicit operator const char *() const { return str; }
};

inline QDebug operator<<(QDebug dbg, const QGString &str)
{
    dbg << (const char *)str;
    return dbg;
}

class QGValue
{
public:
    explicit QGValue(const GValue *v) : value(v) { }
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
    std::optional<int> toInt64() const
    {
        if (!G_VALUE_HOLDS_INT64(value))
            return std::nullopt;
        return g_value_get_int64(value);
    }
    template<typename T>
    T *getPointer() const
    {
        return value ? static_cast<T *>(g_value_get_pointer(value)) : nullptr;
    }

    const char *toString() const
    {
        return value ? g_value_get_string(value) : nullptr;
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
        QGValue min = QGValue{ gst_value_get_fraction_range_min(value) };
        QGValue max = QGValue{ gst_value_get_fraction_range_max(value) };
        return QGRange<float>{ *min.getFraction(), *max.getFraction() };
    }

    std::optional<QGRange<int>> toIntRange() const
    {
        if (!GST_VALUE_HOLDS_INT_RANGE(value))
            return std::nullopt;
        return QGRange<int>{ gst_value_get_int_range_min(value), gst_value_get_int_range_max(value) };
    }

    inline QGstStructure toStructure() const;
    inline QGstCaps toCaps() const;

    inline bool isList() const { return value && GST_VALUE_HOLDS_LIST(value); }
    inline int listSize() const { return gst_value_list_get_size(value); }
    inline QGValue at(int index) const { return QGValue{ gst_value_list_get_value(value, index) }; }

    Q_MULTIMEDIA_EXPORT QList<QAudioFormat::SampleFormat> getSampleFormats() const;
};

namespace QGstPointerImpl {

template <typename RefcountedObject>
struct QGstRefcountingAdaptor;

template <typename GstType>
class QGstObjectWrapper
{
    using Adaptor = QGstRefcountingAdaptor<GstType>;

    GstType *m_object = nullptr;

public:
    enum RefMode { HasRef, NeedsRef };

    constexpr QGstObjectWrapper() = default;

    explicit QGstObjectWrapper(GstType *object, RefMode mode = NeedsRef) : m_object(object)
    {
        if (m_object && mode == NeedsRef)
            Adaptor::ref(m_object);
    }

    QGstObjectWrapper(const QGstObjectWrapper &other) : m_object(other.m_object)
    {
        if (m_object)
            Adaptor::ref(m_object);
    }

    ~QGstObjectWrapper()
    {
        if (m_object)
            Adaptor::unref(m_object);
    }

    QGstObjectWrapper(QGstObjectWrapper &&other) noexcept
        : m_object(std::exchange(other.m_object, nullptr))
    {
    }

    QGstObjectWrapper &
    operator=(const QGstObjectWrapper &other) // NOLINT: bugprone-unhandled-self-assign
    {
        if (m_object != other.m_object) {
            GstType *originalObject = m_object;

            m_object = other.m_object;
            if (m_object)
                Adaptor::ref(m_object);
            if (originalObject)
                Adaptor::unref(originalObject);
        }
        return *this;
    }

    QGstObjectWrapper &operator=(QGstObjectWrapper &&other) noexcept
    {
        if (this != &other) {
            GstType *originalObject = m_object;
            m_object = std::exchange(other.m_object, nullptr);

            if (originalObject)
                Adaptor::unref(originalObject);
        }
        return *this;
    }

    friend bool operator==(const QGstObjectWrapper &a, const QGstObjectWrapper &b)
    {
        return a.m_object == b.m_object;
    }
    friend bool operator!=(const QGstObjectWrapper &a, const QGstObjectWrapper &b)
    {
        return a.m_object != b.m_object;
    }

    explicit operator bool() const { return bool(m_object); }
    bool isNull() const { return !m_object; }
    GstType *release() { return std::exchange(m_object, nullptr); }

protected:
    GstType *get() const { return m_object; }
};

} // namespace QGstPointerImpl

class QGstreamerMessage;

class QGstStructure {
public:
    const GstStructure *structure = nullptr;
    QGstStructure() = default;
    QGstStructure(const GstStructure *s) : structure(s) {}
    void free() { if (structure) gst_structure_free(const_cast<GstStructure *>(structure)); structure = nullptr; }

    bool isNull() const { return !structure; }

    QByteArrayView name() const { return gst_structure_get_name(structure); }

    QGValue operator[](const char *name) const
    {
        return QGValue{ gst_structure_get_value(structure, name) };
    }

    Q_MULTIMEDIA_EXPORT QSize resolution() const;
    Q_MULTIMEDIA_EXPORT QVideoFrameFormat::PixelFormat pixelFormat() const;
    Q_MULTIMEDIA_EXPORT QGRange<float> frameRateRange() const;
    Q_MULTIMEDIA_EXPORT QGstreamerMessage getMessage();

    QByteArray toString() const
    {
        char *s = gst_structure_to_string(structure);
        QByteArray str(s);
        g_free(s);
        return str;
    }
    QGstStructure copy() const { return gst_structure_copy(structure); }
};

template <>
struct QGstPointerImpl::QGstRefcountingAdaptor<GstCaps>
{
    static void ref(GstCaps *arg) noexcept { gst_caps_ref(arg); }
    static void unref(GstCaps *arg) noexcept { gst_caps_unref(arg); }
};

class QGstCaps : public QGstPointerImpl::QGstObjectWrapper<GstCaps>
{
    using BaseClass = QGstPointerImpl::QGstObjectWrapper<GstCaps>;

public:
    using BaseClass::BaseClass;
    QGstCaps(const QGstCaps &) = default;
    QGstCaps(QGstCaps &&) noexcept = default;
    QGstCaps &operator=(const QGstCaps &) = default;
    QGstCaps &operator=(QGstCaps &&) noexcept = default;

    enum MemoryFormat { CpuMemory, GLTexture, DMABuf };

    int size() const { return int(gst_caps_get_size(get())); }
    QGstStructure at(int index) const { return gst_caps_get_structure(get(), index); }
    GstCaps *caps() const { return get(); }
    MemoryFormat memoryFormat() const {
        auto *features = gst_caps_get_features(get(), 0);
        if (gst_caps_features_contains(features, "memory:GLMemory"))
            return GLTexture;
        else if (gst_caps_features_contains(features, "memory:DMABuf"))
            return DMABuf;
        return CpuMemory;
    }
    QVideoFrameFormat formatForCaps(GstVideoInfo *info) const;

    void addPixelFormats(const QList<QVideoFrameFormat::PixelFormat> &formats, const char *modifier = nullptr);

    static QGstCaps create() {
        return QGstCaps(gst_caps_new_empty(), HasRef);
    }

    static QGstCaps fromCameraFormat(const QCameraFormat &format);
};

inline QDebug operator<<(QDebug dbg, const GstCaps *caps)
{
    dbg << QGString(gst_caps_to_string(caps));
    return dbg;
}

inline QDebug operator<<(QDebug dbg, const QGstCaps &caps)
{
    dbg << caps.caps();
    return dbg;
}

template <>
struct QGstPointerImpl::QGstRefcountingAdaptor<GstObject>
{
    static void ref(GstObject *arg) noexcept { gst_object_ref_sink(arg); }
    static void unref(GstObject *arg) noexcept { gst_object_unref(arg); }
};

class QGstObject : public QGstPointerImpl::QGstObjectWrapper<GstObject>
{
    using BaseClass = QGstPointerImpl::QGstObjectWrapper<GstObject>;

public:
    using BaseClass::BaseClass;
    QGstObject(const QGstObject &) = default;
    QGstObject(QGstObject &&) noexcept = default;

    virtual ~QGstObject() = default;

    QGstObject &operator=(const QGstObject &) = default;
    QGstObject &operator=(QGstObject &&) noexcept = default;

    void set(const char *property, const char *str) { g_object_set(get(), property, str, nullptr); }
    void set(const char *property, bool b) { g_object_set(get(), property, gboolean(b), nullptr); }
    void set(const char *property, uint i) { g_object_set(get(), property, guint(i), nullptr); }
    void set(const char *property, int i) { g_object_set(get(), property, gint(i), nullptr); }
    void set(const char *property, qint64 i) { g_object_set(get(), property, gint64(i), nullptr); }
    void set(const char *property, quint64 i)
    {
        g_object_set(get(), property, guint64(i), nullptr);
    }
    void set(const char *property, double d) { g_object_set(get(), property, gdouble(d), nullptr); }
    void set(const char *property, const QGstObject &o)
    {
        g_object_set(get(), property, o.object(), nullptr);
    }
    void set(const char *property, const QGstCaps &c)
    {
        g_object_set(get(), property, c.caps(), nullptr);
    }

    QGString getString(const char *property) const
    {
        char *s = nullptr;
        g_object_get(get(), property, &s, nullptr);
        return QGString(s);
    }

    QGstStructure getStructure(const char *property) const
    {
        GstStructure *s = nullptr;
        g_object_get(get(), property, &s, nullptr);
        return QGstStructure(s);
    }
    bool getBool(const char *property) const
    {
        gboolean b = false;
        g_object_get(get(), property, &b, nullptr);
        return b;
    }
    uint getUInt(const char *property) const
    {
        guint i = 0;
        g_object_get(get(), property, &i, nullptr);
        return i;
    }
    int getInt(const char *property) const
    {
        gint i = 0;
        g_object_get(get(), property, &i, nullptr);
        return i;
    }
    quint64 getUInt64(const char *property) const
    {
        guint64 i = 0;
        g_object_get(get(), property, &i, nullptr);
        return i;
    }
    qint64 getInt64(const char *property) const
    {
        gint64 i = 0;
        g_object_get(get(), property, &i, nullptr);
        return i;
    }
    float getFloat(const char *property) const
    {
        gfloat d = 0;
        g_object_get(get(), property, &d, nullptr);
        return d;
    }
    double getDouble(const char *property) const
    {
        gdouble d = 0;
        g_object_get(get(), property, &d, nullptr);
        return d;
    }
    QGstObject getObject(const char *property) const
    {
        GstObject *o = nullptr;
        g_object_get(get(), property, &o, nullptr);
        return QGstObject(o, HasRef);
    }

    void connect(const char *name, GCallback callback, gpointer userData)
    {
        g_signal_connect(get(), name, callback, userData);
    }

    GstObject *object() const { return get(); }
    const char *name() const { return get() ? GST_OBJECT_NAME(get()) : "(null)"; }
};

class QGstElement;

class QGstPad : public QGstObject
{
public:
    using QGstObject::QGstObject;
    QGstPad(const QGstPad &) = default;
    QGstPad(QGstPad &&) noexcept = default;

    explicit QGstPad(const QGstObject &o) : QGstPad(GST_PAD(o.object()), NeedsRef) { }
    explicit QGstPad(GstPad *pad, RefMode mode = NeedsRef) : QGstObject(&pad->object, mode) { }

    QGstPad &operator=(const QGstPad &) = default;
    QGstPad &operator=(QGstPad &&) noexcept = default;

    QGstCaps currentCaps() const
    {
        return QGstCaps(gst_pad_get_current_caps(pad()), QGstCaps::HasRef);
    }
    QGstCaps queryCaps() const
    {
        return QGstCaps(gst_pad_query_caps(pad(), nullptr), QGstCaps::HasRef);
    }

    bool isLinked() const { return gst_pad_is_linked(pad()); }
    bool link(const QGstPad &sink) const
    {
        return gst_pad_link(pad(), sink.pad()) == GST_PAD_LINK_OK;
    }
    bool unlink(const QGstPad &sink) const { return gst_pad_unlink(pad(), sink.pad()); }
    bool unlinkPeer() const { return unlink(peer()); }
    QGstPad peer() const { return QGstPad(gst_pad_get_peer(pad()), HasRef); }
    inline QGstElement parent() const;

    GstPad *pad() const { return GST_PAD_CAST(object()); }

    GstEvent *stickyEvent(GstEventType type) { return gst_pad_get_sticky_event(pad(), type, 0); }
    bool sendEvent(GstEvent *event) { return gst_pad_send_event (pad(), event); }

    template<auto Member, typename T>
    void addProbe(T *instance, GstPadProbeType type) {
        struct Impl {
            static GstPadProbeReturn callback(GstPad *pad, GstPadProbeInfo *info, gpointer userData) {
                return (static_cast<T *>(userData)->*Member)(QGstPad(pad, NeedsRef), info);
            };
        };

        gst_pad_add_probe (pad(), type, Impl::callback, instance, nullptr);
    }

    void doInIdleProbe(std::function<void()> work) {
        struct CallbackData {
            QSemaphore waitDone;
            std::function<void()> work;
        } cd;
        cd.work = work;

        auto callback= [](GstPad *, GstPadProbeInfo *, gpointer p) {
            auto cd = reinterpret_cast<CallbackData*>(p);
            cd->work();
            cd->waitDone.release();
            return GST_PAD_PROBE_REMOVE;
        };

        gst_pad_add_probe(pad(), GST_PAD_PROBE_TYPE_IDLE, callback, &cd, nullptr);
        cd.waitDone.acquire();
    }

    template<auto Member, typename T>
    void addEosProbe(T *instance) {
        struct Impl {
            static GstPadProbeReturn callback(GstPad */*pad*/, GstPadProbeInfo *info, gpointer userData) {
                if (GST_EVENT_TYPE(GST_PAD_PROBE_INFO_DATA(info)) != GST_EVENT_EOS)
                    return GST_PAD_PROBE_PASS;
                (static_cast<T *>(userData)->*Member)();
                return GST_PAD_PROBE_REMOVE;
            };
        };

        gst_pad_add_probe (pad(), GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, Impl::callback, instance, nullptr);
    }
};

class QGstClock : public QGstObject
{
public:
    QGstClock() = default;
    explicit QGstClock(const QGstObject &o) : QGstClock(GST_CLOCK(o.object())) { }
    explicit QGstClock(GstClock *clock, RefMode mode = NeedsRef) : QGstObject(&clock->object, mode)
    {
    }

    GstClock *clock() const { return GST_CLOCK_CAST(object()); }

    GstClockTime time() const { return gst_clock_get_time(clock()); }
};

class QGstElement : public QGstObject
{
public:
    using QGstObject::QGstObject;

    QGstElement(const QGstElement &) = default;
    QGstElement(QGstElement &&) noexcept = default;
    QGstElement &operator=(const QGstElement &) = default;
    QGstElement &operator=(QGstElement &&) noexcept = default;

    explicit QGstElement(GstElement *element, RefMode mode = NeedsRef)
        : QGstObject(&element->object, mode)
    {
    }

    explicit QGstElement(const char *factory, const char *name = nullptr)
        : QGstElement(gst_element_factory_make(factory, name), NeedsRef)
    {
#ifndef QT_NO_DEBUG
        if (!get())
            qWarning() << "Failed to make element" << name << "from factory" << factory;
#endif
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

    void unlink(const QGstElement &next)
    { gst_element_unlink(element(), next.element()); }

    QGstPad staticPad(const char *name) const { return QGstPad(gst_element_get_static_pad(element(), name), HasRef); }
    QGstPad src() const { return staticPad("src"); }
    QGstPad sink() const { return staticPad("sink"); }
    QGstPad getRequestPad(const char *name) const
    {
#if GST_CHECK_VERSION(1,19,1)
        return QGstPad(gst_element_request_pad_simple(element(), name), HasRef);
#else
        return QGstPad(gst_element_get_request_pad(element(), name), HasRef);
#endif
    }
    void releaseRequestPad(const QGstPad &pad) const { return gst_element_release_request_pad(element(), pad.pad()); }

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
            change = gst_element_get_state(element(), nullptr, &state, 1000*1e6 /*nano seconds*/);
        }
#ifndef QT_NO_DEBUG
        if (change != GST_STATE_CHANGE_SUCCESS && change != GST_STATE_CHANGE_NO_PREROLL)
            qWarning() << "Could not change state of" << name() << "to"
                       << gst_element_state_get_name(state)
                       << gst_element_state_change_return_get_name(change);
#endif
        return change == GST_STATE_CHANGE_SUCCESS;
    }
    bool syncStateWithParent() { return gst_element_sync_state_with_parent(element()) == TRUE; }
    bool finishStateChange()
    {
        auto change = gst_element_get_state(element(), nullptr, nullptr, 1000*1e6 /*nano seconds*/);
#ifndef QT_NO_DEBUG
        if (change != GST_STATE_CHANGE_SUCCESS && change != GST_STATE_CHANGE_NO_PREROLL)
            qWarning() << "Could finish change state of" << name();
#endif
        return change == GST_STATE_CHANGE_SUCCESS;
    }

    void lockState(bool locked) { gst_element_set_locked_state(element(), locked); }
    bool isStateLocked() const { return gst_element_is_locked_state(element()); }

    void sendEvent(GstEvent *event) const { gst_element_send_event(element(), event); }
    void sendEos() const { sendEvent(gst_event_new_eos()); }

    template<auto Member, typename T>
    void onPadAdded(T *instance) {
        struct Impl {
            static void callback(GstElement *e, GstPad *pad, gpointer userData) {
                (static_cast<T *>(userData)->*Member)(QGstElement(e), QGstPad(pad, NeedsRef));
            };
        };

        connect("pad-added", G_CALLBACK(Impl::callback), instance);
    }
    template<auto Member, typename T>
    void onPadRemoved(T *instance) {
        struct Impl {
            static void callback(GstElement *e, GstPad *pad, gpointer userData) {
                (static_cast<T *>(userData)->*Member)(QGstElement(e), QGstPad(pad, NeedsRef));
            };
        };

        connect("pad-removed", G_CALLBACK(Impl::callback), instance);
    }
    template<auto Member, typename T>
    void onNoMorePads(T *instance) {
        struct Impl {
            static void callback(GstElement *e, gpointer userData) {
                (static_cast<T *>(userData)->*Member)(QGstElement(e));
            };
        };

        connect("no-more-pads", G_CALLBACK(Impl::callback), instance);
    }

    GstClockTime baseTime() const { return gst_element_get_base_time(element()); }
    void setBaseTime(GstClockTime time) const { gst_element_set_base_time(element(), time); }

    GstElement *element() const { return GST_ELEMENT_CAST(get()); }
};

inline QGstElement QGstPad::parent() const
{
    return QGstElement(gst_pad_get_parent_element(pad()), HasRef);
}

class QGstBin : public QGstElement
{
public:
    using QGstElement::QGstElement;
    QGstBin(const QGstBin &) = default;
    QGstBin(QGstBin &&) noexcept = default;
    QGstBin &operator=(const QGstBin &) = default;
    QGstBin &operator=(QGstBin &&) noexcept = default;

    explicit QGstBin(const char *name) : QGstElement(gst_bin_new(name), NeedsRef) { }
    explicit QGstBin(GstBin *bin, RefMode mode = NeedsRef) : QGstElement(&bin->element, mode) { }

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

    GstBin *bin() const { return GST_BIN_CAST(get()); }

    void addGhostPad(const QGstElement &child, const char *name)
    {
        addGhostPad(name, child.staticPad(name));
    }
    void addGhostPad(const char *name, const QGstPad &pad)
    {
        gst_element_add_pad(element(), gst_ghost_pad_new(name, pad.pad()));
    }
};

inline QGstStructure QGValue::toStructure() const
{
    if (!value || !GST_VALUE_HOLDS_STRUCTURE(value))
        return QGstStructure();
    return QGstStructure(gst_value_get_structure(value));
}

inline QGstCaps QGValue::toCaps() const
{
    if (!value || !GST_VALUE_HOLDS_CAPS(value))
        return {};
    return QGstCaps(gst_caps_copy(gst_value_get_caps(value)), QGstCaps::HasRef);
}

inline QString errorMessageCannotFindElement(std::string_view element)
{
    return QStringLiteral("Could not find the %1 GStreamer element").arg(element.data());
}

struct QGstTagListHandleTraits
{
    using Type = GstTagList *;
    static Type invalidValue() noexcept { return nullptr; }
    static bool close(Type handle) noexcept
    {
        gst_tag_list_unref(handle);
        return true;
    }
};

struct QGstClockHandleTraits
{
    using Type = GstClock *;
    static Type invalidValue() noexcept { return nullptr; }
    static bool close(Type handle) noexcept
    {
        gst_object_unref(handle);
        return true;
    }
};

QT_END_NAMESPACE

#endif
