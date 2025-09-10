// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamer_qiodevice_handler_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qglobal.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>
#include <QtCore/qspan.h>
#include <QtCore/qurl.h>
#include <QtCore/quuid.h>

#include <gst/base/gstbasesrc.h>
#include <map>
#include <memory>
#include <utility>

QT_BEGIN_NAMESPACE

namespace {

using namespace Qt::Literals;

// QIODeviceRegistry

class QIODeviceRegistry : public QObject
{
public:
    struct Record
    {
        explicit Record(QByteArray, QIODevice *);

        void unsetDevice();
        bool isValid() const;

        const QByteArray id;

        template <typename Functor>
        auto runWhileLocked(Functor &&f)
        {
            QMutexLocker guard(&mutex);
            return f(device);
        }

    private:
        QIODevice *device;
        mutable QMutex mutex;
    };
    using SharedRecord = std::shared_ptr<Record>;

    QByteArray registerQIODevice(QIODevice *);
    SharedRecord findRecord(QByteArrayView);

private:
    void unregisterDevice(QIODevice *);
    void deviceDestroyed(QIODevice *);

    QMutex m_registryMutex;
    std::map<QByteArray, SharedRecord, std::less<>> m_registry;
    std::map<QIODevice *, QByteArray> m_reverseLookupTable;
};

QByteArray QIODeviceRegistry::registerQIODevice(QIODevice *device)
{
    Q_ASSERT(device);

    if (device->isSequential())
        qWarning() << "GStreamer: sequential QIODevices are not fully supported";

    QMutexLocker lock(&m_registryMutex);

    auto it = m_reverseLookupTable.find(device);
    if (it != m_reverseLookupTable.end())
        return it->second;

    QByteArray identifier =
            "qiodevice:/"_ba + QUuid::createUuid().toByteArray(QUuid::StringFormat::Id128);

    m_registry.emplace(identifier, std::make_shared<Record>(identifier, device));

    QMetaObject::Connection destroyedConnection = QObject::connect(
            device, &QObject::destroyed, this,
            [this, device] {
        // Caveat: if the QIODevice has not been closed, we unregister the device, however gstreamer
        // worker threads have a chance to briefly read from a partially destroyed QIODevice.
        // There's nothing we can do about it
        unregisterDevice(device);
    },
            Qt::DirectConnection);

    QObject::connect(
            device, &QIODevice::aboutToClose, this,
            [this, device, destroyedConnection = std::move(destroyedConnection)] {
        unregisterDevice(device);
        disconnect(destroyedConnection);
    },
            Qt::DirectConnection);

    m_reverseLookupTable.emplace(device, identifier);
    return identifier;
}

QIODeviceRegistry::SharedRecord QIODeviceRegistry::findRecord(QByteArrayView id)
{
    QMutexLocker registryLock(&m_registryMutex);

    auto it = m_registry.find(id);
    if (it != m_registry.end())
        return it->second;
    return {};
}

void QIODeviceRegistry::unregisterDevice(QIODevice *device)
{
    QMutexLocker registryLock(&m_registryMutex);
    auto reverseLookupIt = m_reverseLookupTable.find(device);
    if (reverseLookupIt == m_reverseLookupTable.end())
        return;

    auto it = m_registry.find(reverseLookupIt->second);
    Q_ASSERT(it != m_registry.end());

    it->second->unsetDevice();
    m_reverseLookupTable.erase(reverseLookupIt);
    m_registry.erase(it);
}

QIODeviceRegistry::Record::Record(QByteArray id, QIODevice *device)
    : id {
          std::move(id),
      },
      device {
          device,
      }
{
    if (!device->isOpen())
        device->open(QIODevice::ReadOnly);
}

void QIODeviceRegistry::Record::unsetDevice()
{
    QMutexLocker lock(&mutex);
    device = nullptr;
}

bool QIODeviceRegistry::Record::isValid() const
{
    QMutexLocker lock(&mutex);
    return device;
}

Q_GLOBAL_STATIC(QIODeviceRegistry, gQIODeviceRegistry);

// qt helpers

// glib / gstreamer object
enum PropertyId : uint8_t {
    PROP_NONE,
    PROP_URI,
};

struct QGstQIODeviceSrc
{
    void getProperty(guint propId, GValue *value, const GParamSpec *pspec) const;
    void setProperty(guint propId, const GValue *value, const GParamSpec *pspec);
    auto lockObject() const;

    bool start();
    bool stop();

    bool isSeekable();
    std::optional<guint64> size();
    GstFlowReturn fill(guint64 offset, guint length, GstBuffer *buf);
    void getURI(GValue *value) const;
    bool setURI(const char *location, GError **err = nullptr);

    GstBaseSrc baseSrc;
    QIODeviceRegistry::SharedRecord record;
};

void QGstQIODeviceSrc::getProperty(guint propId, GValue *value, const GParamSpec *pspec) const
{
    switch (propId) {
    case PROP_URI:
        return getURI(value);

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(this, propId, pspec);
        break;
    }
}

void QGstQIODeviceSrc::setProperty(guint propId, const GValue *value, const GParamSpec *pspec)
{
    switch (propId) {
    case PROP_URI:
        setURI(g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(this, propId, pspec);
        break;
    }
}

auto QGstQIODeviceSrc::lockObject() const
{
    GST_OBJECT_LOCK(this);
    return qScopeGuard([this] {
        GST_OBJECT_UNLOCK(this);
    });
}

bool QGstQIODeviceSrc::start()
{
    auto lock = lockObject();

    return record && record->isValid();
}

bool QGstQIODeviceSrc::stop()
{
    return true;
}

bool QGstQIODeviceSrc::isSeekable()
{
    auto lock = lockObject();
    return record->runWhileLocked([&](QIODevice *device) {
        return !device->isSequential();
    });
}

std::optional<guint64> QGstQIODeviceSrc::size()
{
    auto lock = lockObject();
    if (!record)
        return std::nullopt;

    qint64 size = record->runWhileLocked([&](QIODevice *device) {
        return device->size();
    });

    if (size == -1)
        return std::nullopt;
    return size;
}

GstFlowReturn QGstQIODeviceSrc::fill(guint64 offset, guint length, GstBuffer *buf)
{
    auto lock = lockObject();

    if (!record)
        return GST_FLOW_ERROR;

    GstMapInfo info;
    if (!gst_buffer_map(buf, &info, GST_MAP_WRITE)) {
        GST_ELEMENT_ERROR(this, RESOURCE, WRITE, (nullptr), ("Can't map buffer for writing"));
        return GST_FLOW_ERROR;
    }

    int64_t totalRead = 0;
    GstFlowReturn ret = record->runWhileLocked([&](QIODevice *device) -> GstFlowReturn {
        if (device->pos() != qint64(offset)) {
            bool success = device->seek(offset);
            if (!success) {
                qWarning() << "seek on iodevice failed";
                return GST_FLOW_ERROR;
            }
        }

        int64_t remain = length;
        while (remain) {
            int64_t bytesRead =
                    device->read(reinterpret_cast<char *>(info.data + totalRead), remain);
            if (bytesRead == -1) {
                if (device->atEnd()) {
                    return GST_FLOW_EOS;
                }
                GST_ELEMENT_ERROR(this, RESOURCE, READ, (nullptr), GST_ERROR_SYSTEM);
                return GST_FLOW_ERROR;
            }

            remain -= bytesRead;
            totalRead += bytesRead;
        }

        return GST_FLOW_OK;
    });

    if (ret != GST_FLOW_OK) {
        gst_buffer_unmap(buf, &info);
        gst_buffer_resize(buf, 0, 0);
        return ret;
    }

    gst_buffer_unmap(buf, &info);
    if (totalRead != length)
        gst_buffer_resize(buf, 0, totalRead);

    GST_BUFFER_OFFSET(buf) = offset;
    GST_BUFFER_OFFSET_END(buf) = offset + totalRead;

    return GST_FLOW_OK;
}

void QGstQIODeviceSrc::getURI(GValue *value) const
{
    auto lock = lockObject();
    if (record)
        g_value_set_string(value, record->id);
    else
        g_value_set_string(value, nullptr);
}

bool QGstQIODeviceSrc::setURI(const char *location, GError **err)
{
    Q_ASSERT(QLatin1StringView(location).startsWith("qiodevice:/"_L1));

    {
        auto lock = lockObject();
        GstState state = GST_STATE(this);
        if (state != GST_STATE_READY && state != GST_STATE_NULL) {
            g_warning(
                    "Changing the `uri' property on qiodevicesrc when the resource is open is not "
                    "supported.");
            if (err)
                g_set_error(err, GST_URI_ERROR, GST_URI_ERROR_BAD_STATE,
                            "Changing the `uri' property on qiodevicesrc when the resource is open "
                            "is not supported.");
            return false;
        }
    }

    auto newRecord = gQIODeviceRegistry->findRecord(QByteArrayView{ location });

    {
        auto lock = lockObject();
        record = std::move(newRecord);
    }

    g_object_notify(G_OBJECT(this), "uri");

    return true;
}

struct QGstQIODeviceSrcClass
{
    GstBaseSrcClass parent_class;
};

// GObject
static void gst_qiodevice_src_class_init(QGstQIODeviceSrcClass *klass);
static void gst_qiodevice_src_init(QGstQIODeviceSrc *self);

GType gst_qiodevice_src_get_type();

template <typename T>
QGstQIODeviceSrc *asQGstQIODeviceSrc(T *obj)
{
    return (G_TYPE_CHECK_INSTANCE_CAST((obj), gst_qiodevice_src_get_type(), QGstQIODeviceSrc));
}

// URI handler
void qGstInitQIODeviceURIHandler(gpointer, gpointer);

#define gst_qiodevice_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(QGstQIODeviceSrc, gst_qiodevice_src, GST_TYPE_BASE_SRC,
                        G_IMPLEMENT_INTERFACE(GST_TYPE_URI_HANDLER, qGstInitQIODeviceURIHandler));

// implementations

void gst_qiodevice_src_init(QGstQIODeviceSrc *self)
{
    using SharedRecord = QIODeviceRegistry::SharedRecord;

    new (reinterpret_cast<void *>(&self->record)) SharedRecord;

    static constexpr guint defaultBlockSize = 16384;
    gst_base_src_set_blocksize(&self->baseSrc, defaultBlockSize);
}

void gst_qiodevice_src_class_init(QGstQIODeviceSrcClass *klass)
{
    // GObject
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->set_property = [](GObject *instance, guint propId, const GValue *value,
                                    GParamSpec *pspec) {
        QGstQIODeviceSrc *src = asQGstQIODeviceSrc(instance);
        return src->setProperty(propId, value, pspec);
    };

    gobjectClass->get_property = [](GObject *instance, guint propId, GValue *value,
                                    GParamSpec *pspec) {
        QGstQIODeviceSrc *src = asQGstQIODeviceSrc(instance);
        return src->getProperty(propId, value, pspec);
    };

    g_object_class_install_property(
            gobjectClass, PROP_URI,
            g_param_spec_string("uri", "QRC Location", "Path of the qrc to read", nullptr,
                                GParamFlags(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
                                            | GST_PARAM_MUTABLE_READY)));

    gobjectClass->finalize = [](GObject *instance) {
        QGstQIODeviceSrc *src = asQGstQIODeviceSrc(instance);
        using SharedRecord = QIODeviceRegistry::SharedRecord;
        src->record.~SharedRecord();
        G_OBJECT_CLASS(parent_class)->finalize(instance);
    };

    // GstElement
    GstElementClass *gstelementClass = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(gstelementClass, "QRC Source", "Source/QRC",
                                          "Read from arbitrary point in QRC resource",
                                          "Tim Blechmann <tim.blechmann@qt.io>");

    static GstStaticPadTemplate srctemplate =
            GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

    gst_element_class_add_static_pad_template(gstelementClass, &srctemplate);

    // GstBaseSrc
    GstBaseSrcClass *gstbasesrcClass = GST_BASE_SRC_CLASS(klass);
    gstbasesrcClass->start = [](GstBaseSrc *basesrc) -> gboolean {
        QGstQIODeviceSrc *src = asQGstQIODeviceSrc(basesrc);
        return src->start();
    };
    gstbasesrcClass->stop = [](GstBaseSrc *basesrc) -> gboolean {
        QGstQIODeviceSrc *src = asQGstQIODeviceSrc(basesrc);
        return src->stop();
    };

    gstbasesrcClass->is_seekable = [](GstBaseSrc *basesrc) -> gboolean {
        QGstQIODeviceSrc *src = asQGstQIODeviceSrc(basesrc);
        return src->isSeekable();
    };
    gstbasesrcClass->get_size = [](GstBaseSrc *basesrc, guint64 *size) -> gboolean {
        QGstQIODeviceSrc *src = asQGstQIODeviceSrc(basesrc);
        auto optionalSize = src->size();
        if (!optionalSize)
            return false;
        *size = optionalSize.value();
        return true;
    };
    gstbasesrcClass->fill = [](GstBaseSrc *basesrc, guint64 offset, guint length,
                               GstBuffer *buf) -> GstFlowReturn {
        QGstQIODeviceSrc *src = asQGstQIODeviceSrc(basesrc);
        return src->fill(offset, length, buf);
    };
}

void qGstInitQIODeviceURIHandler(gpointer g_handlerInterface, gpointer)
{
    GstURIHandlerInterface *iface = (GstURIHandlerInterface *)g_handlerInterface;

    iface->get_type = [](GType) {
        return GST_URI_SRC;
    };
    iface->get_protocols = [](GType) {
        static constexpr const gchar *protocols[] = {
            "qiodevice",
            nullptr,
        };
        return protocols;
    };
    iface->get_uri = [](GstURIHandler *handler) -> gchar * {
        QGstQIODeviceSrc *src = asQGstQIODeviceSrc(handler);
        auto lock = src->lockObject();
        if (src->record)
            return g_strdup(src->record->id.constData());
        return nullptr;
    };
    iface->set_uri = [](GstURIHandler *handler, const gchar *uri, GError **err) -> gboolean {
        QGstQIODeviceSrc *src = asQGstQIODeviceSrc(handler);
        return src->setURI(uri, err);
    };
}

} // namespace

// plugin registration

void qGstRegisterQIODeviceHandler(GstPlugin *plugin)
{
    gst_element_register(plugin, "qiodevicesrc", GST_RANK_PRIMARY, gst_qiodevice_src_get_type());
}

QUrl qGstRegisterQIODevice(QIODevice *device)
{
    return QUrl{
        QString::fromLatin1(gQIODeviceRegistry->registerQIODevice(device)),
    };
}

QT_END_NAMESPACE
