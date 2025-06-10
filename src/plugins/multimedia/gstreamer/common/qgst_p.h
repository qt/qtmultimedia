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

#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qsemaphore.h>

#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtMultimedia/private/qplatformmediaplayer_p.h>
#include <QtMultimedia/private/qsharedhandle_p.h>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/video-info.h>

#include "qgst_handle_types_p.h"

#include <type_traits>
#ifdef __cpp_lib_three_way_comparison
#  include <compare>
#endif

#if QT_CONFIG(gstreamer_photography)
#  define GST_USE_UNSTABLE_API
#  include <gst/interfaces/photography.h>
#  undef GST_USE_UNSTABLE_API
#endif


QT_BEGIN_NAMESPACE

namespace QGstImpl {

template <typename T>
struct GstObjectTraits
{
    // using Type = T;
    // template <typename U>
    // static bool isObjectOfType(U *);
    // template <typename U>
    // static T *cast(U *);
};

#define QGST_DEFINE_CAST_TRAITS(ClassName, MACRO_LABEL) \
    template <>                                         \
    struct GstObjectTraits<ClassName>                   \
    {                                                   \
        using Type = ClassName;                         \
        template <typename U>                           \
        static bool isObjectOfType(U *arg)              \
        {                                               \
            return GST_IS_##MACRO_LABEL(arg);           \
        }                                               \
        template <typename U>                           \
        static Type *cast(U *arg)                       \
        {                                               \
            return GST_##MACRO_LABEL##_CAST(arg);       \
        }                                               \
        template <typename U>                           \
        static Type *checked_cast(U *arg)               \
        {                                               \
            return GST_##MACRO_LABEL(arg);              \
        }                                               \
    };                                                  \
    static_assert(true, "ensure semicolon")

#define QGST_DEFINE_CAST_TRAITS_FOR_INTERFACE(ClassName, MACRO_LABEL) \
  template <>                                                         \
  struct GstObjectTraits<ClassName>                                   \
  {                                                                   \
    using Type = ClassName;                                           \
    template <typename U>                                             \
    static bool isObjectOfType(U *arg)                                \
    {                                                                 \
      return GST_IS_##MACRO_LABEL(arg);                               \
    }                                                                 \
    template <typename U>                                             \
    static Type *cast(U *arg)                                         \
    {                                                                 \
      return checked_cast(arg);                                       \
    }                                                                 \
    template <typename U>                                             \
    static Type *checked_cast(U *arg)                                 \
    {                                                                 \
      return GST_##MACRO_LABEL(arg);                                  \
    }                                                                 \
  };                                                                  \
  static_assert(true, "ensure semicolon")

QGST_DEFINE_CAST_TRAITS(GstBin, BIN);
QGST_DEFINE_CAST_TRAITS(GstClock, CLOCK);
QGST_DEFINE_CAST_TRAITS(GstElement, ELEMENT);
QGST_DEFINE_CAST_TRAITS(GstObject, OBJECT);
QGST_DEFINE_CAST_TRAITS(GstPad, PAD);
QGST_DEFINE_CAST_TRAITS(GstPipeline, PIPELINE);
QGST_DEFINE_CAST_TRAITS(GstBaseSink, BASE_SINK);
QGST_DEFINE_CAST_TRAITS(GstBaseSrc, BASE_SRC);
QGST_DEFINE_CAST_TRAITS(GstAppSink, APP_SINK);
QGST_DEFINE_CAST_TRAITS(GstAppSrc, APP_SRC);

QGST_DEFINE_CAST_TRAITS_FOR_INTERFACE(GstTagSetter, TAG_SETTER);


template <>
struct GstObjectTraits<GObject>
{
    using Type = GObject;
    template <typename U>
    static bool isObjectOfType(U *arg)
    {
        return G_IS_OBJECT(arg);
    }
    template <typename U>
    static Type *cast(U *arg)
    {
        return G_OBJECT(arg);
    }
    template <typename U>
    static Type *checked_cast(U *arg)
    {
        return G_OBJECT(arg);
    }
};

#undef QGST_DEFINE_CAST_TRAITS
#undef QGST_DEFINE_CAST_TRAITS_FOR_INTERFACE

} // namespace QGstImpl

template <typename DestinationType, typename SourceType>
bool qIsGstObjectOfType(SourceType *arg)
{
    using Traits = QGstImpl::GstObjectTraits<DestinationType>;
    return arg && Traits::isObjectOfType(arg);
}

template <typename DestinationType, typename SourceType>
DestinationType *qGstSafeCast(SourceType *arg)
{
    using Traits = QGstImpl::GstObjectTraits<DestinationType>;
    if (arg && Traits::isObjectOfType(arg))
        return Traits::cast(arg);
    return nullptr;
}

template <typename DestinationType, typename SourceType>
DestinationType *qGstCheckedCast(SourceType *arg)
{
    using Traits = QGstImpl::GstObjectTraits<DestinationType>;
    if (arg)
        Q_ASSERT(Traits::isObjectOfType(arg));
    return Traits::cast(arg);
}

class QSize;
class QGstStructureView;
class QGstCaps;
class QCameraFormat;

template <typename T> struct QGRange
{
    T min;
    T max;

#ifdef __cpp_impl_three_way_comparison
    auto operator<=> (const QGRange &) const = default;
#else
    bool operator==(const QGRange &rhs) const
    {
        return std::tie(min, max) == std::tie(rhs.min, rhs.max);
    }
#endif
};

struct QGString : QUniqueGStringHandle
{
    using QUniqueGStringHandle::QUniqueGStringHandle;

    QLatin1StringView asStringView() const { return QLatin1StringView{ get() }; }
    QByteArrayView asByteArrayView() const { return QByteArrayView{ get() }; }
    QString toQString() const { return QString::fromUtf8(get()); }

#ifdef __cpp_lib_three_way_comparison
    // clang-format off
    friend auto operator<=>(const QGString &lhs, const QGString &rhs)
    {
        return lhs.asStringView() <=> rhs.asStringView();
    }
    friend auto operator<=>(const QGString &lhs, const QLatin1StringView rhs)
    {
        return lhs.asStringView() <=> rhs;
    }
    friend auto operator<=>(const QGString &lhs, const QByteArrayView rhs)
    {
        return lhs.asByteArrayView() <=> rhs;
    }
    friend auto operator<=>(const QLatin1StringView lhs, const QGString &rhs)
    {
        return lhs <=> rhs.asStringView();
    }
    friend auto operator<=>(const QByteArrayView lhs, const QGString &rhs)
    {
        return lhs <=> rhs.asByteArrayView();
    }
    // clang-format on
#else
    // remove once we're on c++20
    bool operator==(const QGString &str) const { return asStringView() == str.asStringView(); }
    bool operator==(const QLatin1StringView str) const { return asStringView() == str; }
    bool operator==(const QByteArrayView str) const { return asByteArrayView() == str; }

    bool operator!=(const QGString &str) const { return asStringView() != str.asStringView(); }
    bool operator!=(const QLatin1StringView str) const { return asStringView() != str; }
    bool operator!=(const QByteArrayView str) const { return asByteArrayView() != str; }

    friend bool operator<(const QGString &lhs, const QGString &rhs)
    {
        return lhs.asStringView() < rhs.asStringView();
    }
    friend bool operator<(const QGString &lhs, const QLatin1StringView rhs)
    {
        return lhs.asStringView() < rhs;
    }
    friend bool operator<(const QGString &lhs, const QByteArrayView rhs)
    {
        return lhs.asByteArrayView() < rhs;
    }
    friend bool operator<(const QLatin1StringView lhs, const QGString &rhs)
    {
        return lhs < rhs.asStringView();
    }
    friend bool operator<(const QByteArrayView lhs, const QGString &rhs)
    {
        return lhs < rhs.asByteArrayView();
    }
#endif

    explicit operator QByteArrayView() const { return asByteArrayView(); }
    explicit operator QByteArray() const { return QByteArray{ asByteArrayView() }; }
};

class QGValue
{
public:
    explicit QGValue(const GValue *v);
    const GValue *value;

    bool isNull() const;

    std::optional<bool> toBool() const;
    std::optional<int> toInt() const;
    std::optional<int> toInt64() const;
    template<typename T>
    T *getPointer() const
    {
        return value ? static_cast<T *>(g_value_get_pointer(value)) : nullptr;
    }

    const char *toString() const;
    std::optional<float> getFraction() const;
    std::optional<QGRange<float>> getFractionRange() const;
    std::optional<QGRange<int>> toIntRange() const;

    QGstStructureView toStructure() const;
    QGstCaps toCaps() const;

    bool isList() const;
    int listSize() const;
    QGValue at(int index) const;

    QList<QAudioFormat::SampleFormat> getSampleFormats() const;
};

class QGstreamerMessage;

class QGstStructureView
{
public:
    const GstStructure *structure = nullptr;
    explicit QGstStructureView(const GstStructure *);
    explicit QGstStructureView(const QUniqueGstStructureHandle &);

    QUniqueGstStructureHandle clone() const;

    bool isNull() const;
    QByteArrayView name() const;
    QGValue operator[](const char *fieldname) const;

    QGstCaps caps() const;
    QGstTagListHandle tags() const;

    QSize resolution() const;
    QVideoFrameFormat::PixelFormat pixelFormat() const;
    QGRange<float> frameRateRange() const;
    std::optional<QGRange<QSize>> resolutionRange() const;
    QGstreamerMessage getMessage();
    std::optional<Fraction> pixelAspectRatio() const;
    QSize nativeSize() const;
};

struct QSharedGstCapsTraits
{
    using Type = GstCaps *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static GstCaps *ref(GstCaps *arg) noexcept { return gst_caps_ref(arg); }
    static bool unref(GstCaps *arg) noexcept
    {
        gst_caps_unref(arg);
        return true;
    }
};

class QGstCaps : public QtPrivate::QSharedHandle<QSharedGstCapsTraits>
{
    using BaseClass = QtPrivate::QSharedHandle<QSharedGstCapsTraits>;

public:
    using BaseClass::BaseClass;
    QGstCaps(const QGstCaps &) = default;
    QGstCaps(QGstCaps &&) noexcept = default;
    QGstCaps &operator=(const QGstCaps &) = default;
    QGstCaps &operator=(QGstCaps &&) noexcept = default;

    enum MemoryFormat { CpuMemory, GLTexture, DMABuf };

    int size() const;
    QGstStructureView at(int index) const;
    GstCaps *caps() const;

    MemoryFormat memoryFormat() const;
    std::optional<std::pair<QVideoFrameFormat, GstVideoInfo>> formatAndVideoInfo() const;

    void addPixelFormats(const QList<QVideoFrameFormat::PixelFormat> &formats, const char *modifier = nullptr);
    void setResolution(QSize);

    static QGstCaps create();

    static QGstCaps fromCameraFormat(const QCameraFormat &format);

    QGstCaps copy() const;
};

struct QSharedGstObjectTraits
{
    using Type = GstObject *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static GstObject *ref(GstObject *arg) noexcept
    {
        gst_object_ref_sink(arg);
        return arg;
    }
    static bool unref(GstObject *arg) noexcept
    {
        gst_object_unref(arg);
        return true;
    }
};

class QGObjectHandlerConnection;

class QGstObject : public QtPrivate::QSharedHandle<QSharedGstObjectTraits>
{
    using BaseClass = QtPrivate::QSharedHandle<QSharedGstObjectTraits>;

public:
    using BaseClass::BaseClass;
    QGstObject(const QGstObject &) = default;
    QGstObject(QGstObject &&) noexcept = default;

    QGstObject &operator=(const QGstObject &) = default;
    QGstObject &operator=(QGstObject &&) noexcept = default;

    void set(const char *property, const char *str);
    void set(const char *property, bool b);
    void set(const char *property, int32_t i);
    void set(const char *property, uint32_t i);
    void set(const char *property, int64_t i);
    void set(const char *property, uint64_t i);
    void set(const char *property, double d);
    void set(const char *property, const QGstObject &o);
    void set(const char *property, const QGstCaps &c);
    void set(const char *property, void *object, GDestroyNotify destroyFunction);

    template <typename Object>
    void set(const char *property, Object *object, GDestroyNotify destroyFunction)
    {
        set(property, static_cast<void *>(object), destroyFunction);
    }

    template <typename Object>
    void set(const char *property, std::unique_ptr<Object> object)
    {
        set(property, static_cast<void *>(object.release()), qDeleteFromVoidPointer<Object>);
    }

    template <typename T>
    static void qDeleteFromVoidPointer(void *ptr)
    {
        delete reinterpret_cast<T *>(ptr);
    }

    QGString getString(const char *property) const;
    QGstStructureView getStructure(const char *property) const;
    bool getBool(const char *property) const;
    uint getUInt(const char *property) const;
    int getInt(const char *property) const;
    quint64 getUInt64(const char *property) const;
    qint64 getInt64(const char *property) const;
    float getFloat(const char *property) const;
    double getDouble(const char *property) const;
    QGstObject getGstObject(const char *property) const;
    void *getObject(const char *property) const;

    template <typename T>
    T *getObject(const char *property) const
    {
        void *rawObject = getObject(property);
        return reinterpret_cast<T *>(rawObject);
    }

    QGObjectHandlerConnection connect(const char *name, GCallback callback, gpointer userData);
    void disconnect(gulong handlerId);

    GType type() const;
    QLatin1StringView typeName() const;
    GstObject *object() const;
    QLatin1StringView name() const;
};

class QGObjectHandlerConnection
{
public:
    QGObjectHandlerConnection(QGstObject object, gulong handler);

    QGObjectHandlerConnection() = default;
    QGObjectHandlerConnection(const QGObjectHandlerConnection &) = default;
    QGObjectHandlerConnection(QGObjectHandlerConnection &&) = default;
    QGObjectHandlerConnection &operator=(const QGObjectHandlerConnection &) = default;
    QGObjectHandlerConnection &operator=(QGObjectHandlerConnection &&) = default;

    void disconnect();

private:
    static constexpr gulong invalidHandlerId = std::numeric_limits<gulong>::max();

    QGstObject object;
    gulong handlerId = invalidHandlerId;
};

// disconnects in dtor
class QGObjectHandlerScopedConnection
{
public:
    QGObjectHandlerScopedConnection(QGObjectHandlerConnection connection);

    QGObjectHandlerScopedConnection() = default;
    QGObjectHandlerScopedConnection(const QGObjectHandlerScopedConnection &) = delete;
    QGObjectHandlerScopedConnection &operator=(const QGObjectHandlerScopedConnection &) = delete;
    QGObjectHandlerScopedConnection(QGObjectHandlerScopedConnection &&) = default;
    QGObjectHandlerScopedConnection &operator=(QGObjectHandlerScopedConnection &&) = default;

    ~QGObjectHandlerScopedConnection();

    void disconnect();

private:
    QGObjectHandlerConnection connection;
};

class QGstElement;

class QGstPad : public QGstObject
{
public:
    using QGstObject::QGstObject;
    QGstPad(const QGstPad &) = default;
    QGstPad(QGstPad &&) noexcept = default;

    explicit QGstPad(const QGstObject &o);
    explicit QGstPad(GstPad *pad, RefMode mode);

    QGstPad &operator=(const QGstPad &) = default;
    QGstPad &operator=(QGstPad &&) noexcept = default;

    QGstCaps currentCaps() const;
    QGstCaps queryCaps() const;

    QGstTagListHandle tags() const;
    QGString streamId() const;

    std::optional<QPlatformMediaPlayer::TrackType>
    inferTrackTypeFromName() const; // for decodebin3 etc

    bool isLinked() const;
    bool link(const QGstPad &sink) const;
    bool unlink(const QGstPad &sink) const;
    bool unlinkPeer() const;
    QGstPad peer() const;
    QGstElement parent() const;

    GstPad *pad() const;

    GstEvent *stickyEvent(GstEventType type);
    bool sendEvent(GstEvent *event);
    void sendFlushStartStop(bool resetTime);

    template <auto Member, typename T>
    void addProbe(T *instance, GstPadProbeType type);

    template <typename Functor>
    void doInIdleProbe(Functor &&work);

    template <auto Member, typename T>
    void addEosProbe(T *instance);

    template <typename Functor>
    void modifyPipelineInIdleProbe(Functor &&f);

    void sendFlushIfPaused();
};

class QGstClock : public QGstObject
{
public:
    QGstClock() = default;
    explicit QGstClock(const QGstObject &o);
    explicit QGstClock(GstClock *clock, RefMode mode);

    GstClock *clock() const;
    GstClockTime time() const;
};

class QGstBin;
class QGstPipeline;

class QGstElement : public QGstObject
{
public:
    using QGstObject::QGstObject;

    QGstElement(const QGstElement &) = default;
    QGstElement(QGstElement &&) noexcept = default;
    QGstElement &operator=(const QGstElement &) = default;
    QGstElement &operator=(QGstElement &&) noexcept = default;

    explicit QGstElement(GstElement *element, RefMode mode);
    static QGstElement createFromFactory(const char *factory, const char *name = nullptr);
    static QGstElement createFromFactory(GstElementFactory *, const char *name = nullptr);
    static QGstElement createFromFactory(const QGstElementFactoryHandle &,
                                         const char *name = nullptr);
    static QGstElement createFromDevice(const QGstDeviceHandle &, const char *name = nullptr);
    static QGstElement createFromDevice(GstDevice *, const char *name = nullptr);
    static QGstElement createFromPipelineDescription(const char *);
    static QGstElement createFromPipelineDescription(const QByteArray &);

    static QGstElementFactoryHandle findFactory(const char *);
    static QGstElementFactoryHandle findFactory(const QByteArray &name);

    QGstPad staticPad(const char *name) const;
    QGstPad src() const;
    QGstPad sink() const;
    QGstPad getRequestPad(const char *name) const;
    void releaseRequestPad(const QGstPad &pad) const;

    GstState state(std::chrono::nanoseconds timeout = std::chrono::seconds(0)) const;
    GstStateChangeReturn setState(GstState state);
    bool setStateSync(GstState state, std::chrono::nanoseconds timeout = std::chrono::seconds(1));
    bool syncStateWithParent();
    bool finishStateChange(std::chrono::nanoseconds timeout = std::chrono::seconds(5));
    bool hasAsyncStateChange(std::chrono::nanoseconds timeout = std::chrono::seconds(0)) const;
    bool waitForAsyncStateChangeComplete(
            std::chrono::nanoseconds timeout = std::chrono::seconds(5)) const;

    void lockState(bool locked);
    bool isStateLocked() const;

    void sendEvent(GstEvent *event) const;
    void sendEos() const;

    std::optional<std::chrono::nanoseconds> duration() const;
    std::optional<std::chrono::milliseconds> durationInMs() const;
    std::optional<std::chrono::nanoseconds> position() const;
    std::optional<std::chrono::milliseconds> positionInMs() const;
    std::optional<bool> canSeek() const;

    template <auto Member, typename T>
    QGObjectHandlerConnection onPadAdded(T *instance)
    {
        struct Impl
        {
            static void callback(GstElement *e, GstPad *pad, gpointer userData)
            {
                (static_cast<T *>(userData)->*Member)(QGstElement(e, NeedsRef),
                                                      QGstPad(pad, NeedsRef));
            };
        };

        return connect("pad-added", G_CALLBACK(Impl::callback), instance);
    }
    template <auto Member, typename T>
    QGObjectHandlerConnection onPadRemoved(T *instance)
    {
        struct Impl
        {
            static void callback(GstElement *e, GstPad *pad, gpointer userData)
            {
                (static_cast<T *>(userData)->*Member)(QGstElement(e, NeedsRef),
                                                      QGstPad(pad, NeedsRef));
            };
        };

        return connect("pad-removed", G_CALLBACK(Impl::callback), instance);
    }
    template <auto Member, typename T>
    QGObjectHandlerConnection onNoMorePads(T *instance)
    {
        struct Impl
        {
            static void callback(GstElement *e, gpointer userData)
            {
                (static_cast<T *>(userData)->*Member)(QGstElement(e, NeedsRef));
            };
        };

        return connect("no-more-pads", G_CALLBACK(Impl::callback), instance);
    }

    GstClockTime baseTime() const;
    void setBaseTime(GstClockTime time) const;

    GstElement *element() const;

    QGstElement getParent() const;
    QGstBin getParentBin() const;
    QGstPipeline getPipeline() const;
    QGstBin getRootBin() const;

    void removeFromParent();
    void dumpPipelineGraph(const char *filename) const;

private:
    QGstQueryHandle &positionQuery() const;
    mutable QGstQueryHandle m_positionQuery;
};

// QGstPad implementations

template <auto Member, typename T>
void QGstPad::addProbe(T *instance, GstPadProbeType type)
{
    auto callback = [](GstPad *pad, GstPadProbeInfo *info, gpointer userData) {
        return (static_cast<T *>(userData)->*Member)(QGstPad(pad, NeedsRef), info);
    };

    gst_pad_add_probe(pad(), type, callback, instance, nullptr);
}

template <typename Functor>
void QGstPad::doInIdleProbe(Functor &&work)
{
    using namespace std::chrono_literals;

    struct CallbackData
    {
        QSemaphore waitDone;
        std::once_flag onceFlag;
        Functor work;

        void run()
        {
            std::call_once(onceFlag, [&] {
                work();
            });
        }
    };

    CallbackData cd{ QSemaphore{}, {}, std::forward<Functor>(work) };

    auto callback = [](GstPad *, GstPadProbeInfo *, gpointer p) {
        auto cd = reinterpret_cast<CallbackData *>(p);
        cd->run();
        cd->waitDone.release();
        return GST_PAD_PROBE_REMOVE;
    };

    gulong probe = gst_pad_add_probe(pad(), GST_PAD_PROBE_TYPE_IDLE, callback, &cd, nullptr);
    if (probe == 0)
        return; // probe was executed

    bool success = cd.waitDone.try_acquire_for(250ms);
    if (success)
        return;

    // the probe has not been executed for 250ms, probably because the element has transitioned
    // from playing to paused. Flushing the pad would unblock paused pads
    sendFlushIfPaused();

    success = cd.waitDone.try_acquire_for(1s);
    if (success)
        return;

    // if the idle probe still has not been called, we remove it and call it
    // explicitly to avoid deadlocking the application. Probably not exactly safe,
    // but better than deadlocking
    qWarning() << "QGstPad::doInIdleProbe blocked for 1s. Executing the pad probe manually";
    parent().dumpPipelineGraph("doInIdleProbeHang");
    gst_pad_remove_probe(pad(), probe);
    cd.run();
}

template <auto Member, typename T>
void QGstPad::addEosProbe(T *instance)
{
    auto callback = [](GstPad *, GstPadProbeInfo *info, gpointer userData) {
        if (GST_EVENT_TYPE(GST_PAD_PROBE_INFO_DATA(info)) != GST_EVENT_EOS)
            return GST_PAD_PROBE_PASS;
        (static_cast<T *>(userData)->*Member)();
        return GST_PAD_PROBE_REMOVE;
    };

    gst_pad_add_probe(pad(), GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, callback, instance, nullptr);
}
template <typename Functor>
void QGstPad::modifyPipelineInIdleProbe(Functor &&f)
{
    using namespace std::chrono_literals;

    GstPadDirection direction = gst_pad_get_direction(pad());

    switch (direction) {
    case GstPadDirection::GST_PAD_SINK: {
        // modifying a source: we need to flush the sink pad before we can modify downstream
        // elements
        sendFlushIfPaused();
        doInIdleProbe(f);
        return;
    }
    case GstPadDirection::GST_PAD_SRC: {
        // modifying a sink: we need to use the idle probes iff the pipeline is playing
        if (parent().state(1s) == GstState::GST_STATE_PLAYING)
            doInIdleProbe(f);
        else
            f();
        return;
    }

    default:
        Q_UNREACHABLE();
    }
}

template <typename... Ts>
std::enable_if_t<(std::is_base_of_v<QGstElement, Ts> && ...), void>
qLinkGstElements(const Ts &...ts)
{
    bool link_success = [&] {
        if constexpr (sizeof...(Ts) == 2)
            return gst_element_link(ts.element()...);
        else
            return gst_element_link_many(ts.element()..., nullptr);
    }();

    if (Q_UNLIKELY(!link_success)) {
        qWarning() << "qLinkGstElements: could not link elements: "
                   << std::initializer_list<const char *>{
                          (GST_ELEMENT_NAME(ts.element()))...,
                      };
    }
}

template <typename... Ts>
std::enable_if_t<(std::is_base_of_v<QGstElement, Ts> && ...), void>
qUnlinkGstElements(const Ts &...ts)
{
    if constexpr (sizeof...(Ts) == 2)
        gst_element_unlink(ts.element()...);
    else
        gst_element_unlink_many(ts.element()..., nullptr);
}

class QGstBin : public QGstElement
{
public:
    using QGstElement::QGstElement;
    QGstBin(const QGstBin &) = default;
    QGstBin(QGstBin &&) noexcept = default;
    QGstBin &operator=(const QGstBin &) = default;
    QGstBin &operator=(QGstBin &&) noexcept = default;

    explicit QGstBin(GstBin *bin, RefMode mode = NeedsRef);
    static QGstBin create(const char *name);
    static QGstBin createFromFactory(const char *factory, const char *name);
    static QGstBin createFromPipelineDescription(const QByteArray &pipelineDescription,
                                                 const char *name = nullptr,
                                                 bool ghostUnlinkedPads = false);
    static QGstBin createFromPipelineDescription(const char *pipelineDescription,
                                                 const char *name = nullptr,
                                                 bool ghostUnlinkedPads = false);

    template <typename... Ts>
    std::enable_if_t<(std::is_base_of_v<QGstElement, Ts> && ...), void> add(const Ts &...ts)
    {
        if constexpr (sizeof...(Ts) == 1)
            gst_bin_add(bin(), ts.element()...);
        else
            gst_bin_add_many(bin(), ts.element()..., nullptr);
    }

    template <typename... Ts>
    std::enable_if_t<(std::is_base_of_v<QGstElement, Ts> && ...), void> remove(const Ts &...ts)
    {
        if constexpr (sizeof...(Ts) == 1)
            gst_bin_remove(bin(), ts.element()...);
        else
            gst_bin_remove_many(bin(), ts.element()..., nullptr);
    }

    template <typename... Ts>
    std::enable_if_t<(std::is_base_of_v<QGstElement, std::remove_reference_t<Ts>> && ...), void>
    stopAndRemoveElements(Ts &&...ts)
    {
        bool stateChangeSuccessful = (ts.setStateSync(GST_STATE_NULL) && ...);
        Q_ASSERT(stateChangeSuccessful);
        remove(ts...);
    }

    GstBin *bin() const;

    void addGhostPad(const QGstElement &child, const char *name);
    void addGhostPad(const char *name, const QGstPad &pad);
    void addUnlinkedGhostPads(GstPadDirection);

    bool syncChildrenState();

    void dumpGraph(const char *fileNamePrefix, bool includeTimestamp = true) const;

    QGstElement findByName(const char *);

    void recalculateLatency();
};

class QGstBaseSink : public QGstElement
{
public:
    using QGstElement::QGstElement;

    explicit QGstBaseSink(GstBaseSink *, RefMode);

    QGstBaseSink(const QGstBaseSink &) = default;
    QGstBaseSink(QGstBaseSink &&) noexcept = default;
    QGstBaseSink &operator=(const QGstBaseSink &) = default;
    QGstBaseSink &operator=(QGstBaseSink &&) noexcept = default;

    void setSync(bool);

    GstBaseSink *baseSink() const;
};

class QGstBaseSrc : public QGstElement
{
public:
    using QGstElement::QGstElement;

    explicit QGstBaseSrc(GstBaseSrc *, RefMode);

    QGstBaseSrc(const QGstBaseSrc &) = default;
    QGstBaseSrc(QGstBaseSrc &&) noexcept = default;
    QGstBaseSrc &operator=(const QGstBaseSrc &) = default;
    QGstBaseSrc &operator=(QGstBaseSrc &&) noexcept = default;

    GstBaseSrc *baseSrc() const;
};

class QGstAppSink : public QGstBaseSink
{
public:
    using QGstBaseSink::QGstBaseSink;

    explicit QGstAppSink(GstAppSink *, RefMode);

    QGstAppSink(const QGstAppSink &) = default;
    QGstAppSink(QGstAppSink &&) noexcept = default;
    QGstAppSink &operator=(const QGstAppSink &) = default;
    QGstAppSink &operator=(QGstAppSink &&) noexcept = default;

    static QGstAppSink create(const char *name);

    GstAppSink *appSink() const;

    void setMaxBuffers(int);
#  if GST_CHECK_VERSION(1, 24, 0)
    void setMaxBufferTime(std::chrono::nanoseconds);
#  endif

    void setCaps(const QGstCaps &caps);
    void setCallbacks(GstAppSinkCallbacks &callbacks, gpointer user_data, GDestroyNotify notify);

    QGstSampleHandle pullSample();
};

class QGstAppSrc : public QGstBaseSrc
{
public:
    using QGstBaseSrc::QGstBaseSrc;

    explicit QGstAppSrc(GstAppSrc *, RefMode);

    QGstAppSrc(const QGstAppSrc &) = default;
    QGstAppSrc(QGstAppSrc &&) noexcept = default;
    QGstAppSrc &operator=(const QGstAppSrc &) = default;
    QGstAppSrc &operator=(QGstAppSrc &&) noexcept = default;

    static QGstAppSrc create(const char *name);

    GstAppSrc *appSrc() const;

    void setCallbacks(GstAppSrcCallbacks &callbacks, gpointer user_data, GDestroyNotify notify);

    GstFlowReturn pushBuffer(GstBuffer *); // take ownership
};

inline GstClockTime qGstClockTimeFromChrono(std::chrono::nanoseconds ns)
{
    return ns.count();
}

QString qGstErrorMessageCannotFindElement(std::string_view element);

template <typename Arg, typename... Args>
std::optional<QString> qGstErrorMessageIfElementsNotAvailable(const Arg &arg, Args... args)
{
    QGstElementFactoryHandle factory = QGstElement::findFactory(arg);
    if (!factory)
        return qGstErrorMessageCannotFindElement(arg);

    if constexpr (sizeof...(args) != 0)
        return qGstErrorMessageIfElementsNotAvailable(args...);
    else
        return std::nullopt;
}

template <typename Functor>
void qForeachStreamInCollection(GstStreamCollection *collection, Functor &&f)
{
    guint size = gst_stream_collection_get_size(collection);
    for (guint index = 0; index != size; ++index)
        f(gst_stream_collection_get_stream(collection, index));
}

template <typename Functor>
void qForeachStreamInCollection(const QGstStreamCollectionHandle &collection, Functor &&f)
{
    qForeachStreamInCollection(collection.get(), std::forward<Functor>(f));
}

QT_END_NAMESPACE

namespace std {

template <>
struct hash<QT_PREPEND_NAMESPACE(QGstElement)>
{
    using argument_type = QT_PREPEND_NAMESPACE(QGstElement);
    using result_type = size_t;
    result_type operator()(const argument_type &e) const noexcept
    {
        return std::hash<void *>{}(e.element());
    }
};
} // namespace std

#endif
