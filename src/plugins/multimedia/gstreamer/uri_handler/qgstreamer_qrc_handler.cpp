// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamer_qrc_handler_p.h"

#include <QtCore/qfile.h>
#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qurl.h>

#include <gst/base/gstbasesrc.h>

QT_BEGIN_NAMESPACE

namespace {

// qt helpers

using namespace Qt::Literals;

std::optional<QString> qQUrlToQrcPath(const QUrl &url)
{
    if (url.scheme() == "qrc"_L1)
        return ':'_L1 + url.path();
    return std::nullopt;
}

std::optional<QUrl> qQrcPathToQUrl(QStringView path)
{
    if (!path.empty() && path[0] == ':'_L1)
        return QUrl(u"qrc://"_s + path.mid(1));
    return std::nullopt;
}

// glib / gstreamer object

enum PropertyId : uint8_t {
    PROP_NONE,
    PROP_URI,
};

struct QGstQrcSrc
{
    void getProperty(guint propId, GValue *value, const GParamSpec *pspec) const;
    void setProperty(guint propId, const GValue *value, const GParamSpec *pspec);
    auto lockObject() const;

    bool start();
    bool stop();

    std::optional<guint64> size();
    GstFlowReturn fill(guint64 offset, guint length, GstBuffer *buf);
    void getURI(GValue *value) const;
    bool setURI(const char *location, GError **err = nullptr);

    GstBaseSrc baseSrc;
    QFile file;
};

void QGstQrcSrc::getProperty(guint propId, GValue *value, const GParamSpec *pspec) const
{
    switch (propId) {
    case PROP_URI:
        return getURI(value);

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(this, propId, pspec);
        break;
    }
}

void QGstQrcSrc::setProperty(guint propId, const GValue *value, const GParamSpec *pspec)
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

auto QGstQrcSrc::lockObject() const
{
    GST_OBJECT_LOCK(this);
    return qScopeGuard([this] {
        GST_OBJECT_UNLOCK(this);
    });
}

bool QGstQrcSrc::start()
{
    auto lock = lockObject();
    if (file.fileName().isEmpty()) {
        GST_ELEMENT_ERROR(this, RESOURCE, NOT_FOUND, ("No resource name specified for reading."),
                          (nullptr));
        return false;
    }

    bool opened = file.open(QIODevice::ReadOnly);
    if (!opened) {
        GST_ELEMENT_ERROR(this, RESOURCE, NOT_FOUND, (nullptr),
                          ("No such resource \"%s\"", file.fileName().toUtf8().constData()));
        return false;
    }

    gst_base_src_set_dynamic_size(&baseSrc, false);

    Q_ASSERT(file.isOpen());
    return true;
}

bool QGstQrcSrc::stop()
{
    auto lock = lockObject();
    file.close();
    return true;
}

std::optional<guint64> QGstQrcSrc::size()
{
    auto lock = lockObject();
    if (!file.isOpen())
        return std::nullopt;
    return file.size();
}

GstFlowReturn QGstQrcSrc::fill(guint64 offset, guint length, GstBuffer *buf)
{
    auto lock = lockObject();

    if (!file.isOpen())
        return GST_FLOW_ERROR;

    if (G_UNLIKELY(offset != guint64(-1) && guint64(file.pos()) != offset)) {
        bool success = file.seek(int64_t(offset));
        if (!success) {
            GST_ELEMENT_ERROR(this, RESOURCE, READ, (nullptr), GST_ERROR_SYSTEM);
            return GST_FLOW_ERROR;
        }
    }

    GstMapInfo info;
    if (!gst_buffer_map(buf, &info, GST_MAP_WRITE)) {
        GST_ELEMENT_ERROR(this, RESOURCE, WRITE, (nullptr), ("Can't map buffer for writing"));
        return GST_FLOW_ERROR;
    }

    int64_t remain = length;
    int64_t totalRead = 0;
    while (remain) {
        int64_t bytesRead = file.read(reinterpret_cast<char *>(info.data) + totalRead, remain);
        if (bytesRead == -1) {
            if (file.atEnd()) {
                gst_buffer_unmap(buf, &info);
                gst_buffer_resize(buf, 0, 0);
                return GST_FLOW_EOS;
            }
            GST_ELEMENT_ERROR(this, RESOURCE, READ, (nullptr), GST_ERROR_SYSTEM);
            gst_buffer_unmap(buf, &info);
            gst_buffer_resize(buf, 0, 0);
            return GST_FLOW_ERROR;
        }

        remain -= bytesRead;
        totalRead += bytesRead;
    }

    gst_buffer_unmap(buf, &info);
    if (totalRead != length)
        gst_buffer_resize(buf, 0, totalRead);

    GST_BUFFER_OFFSET(buf) = offset;
    GST_BUFFER_OFFSET_END(buf) = offset + totalRead;

    return GST_FLOW_OK;
}

void QGstQrcSrc::getURI(GValue *value) const
{
    auto lock = lockObject();
    std::optional<QUrl> url = qQrcPathToQUrl(file.fileName());
    if (url)
        g_value_set_string(value, url->toString().toUtf8().constData());
    else
        g_value_set_string(value, nullptr);
}

bool QGstQrcSrc::setURI(const char *location, GError **err)
{
    Q_ASSERT(QLatin1StringView(location).startsWith("qrc:/"_L1));

    {
        auto lock = lockObject();
        GstState state = GST_STATE(this);
        if (state != GST_STATE_READY && state != GST_STATE_NULL) {
            g_warning("Changing the `uri' property on qrcsrc when the resource is open is not "
                      "supported.");
            if (err)
                g_set_error(err, GST_URI_ERROR, GST_URI_ERROR_BAD_STATE,
                            "Changing the `uri' property on qrcsrc when the resource is open "
                            "is not supported.");
            return false;
        }
    }

    std::optional<QString> path = qQUrlToQrcPath(QString::fromUtf8(location));

    {
        auto lock = lockObject();
        file.close();
        file.setFileName(path.value_or(u""_s));
    }

    g_object_notify(G_OBJECT(this), "uri");

    return true;
}

struct QGstQrcSrcClass
{
    GstBaseSrcClass parent_class;
};

// GObject
static void gst_qrc_src_class_init(QGstQrcSrcClass *klass);
static void gst_qrc_src_init(QGstQrcSrc *self);

GType gst_qrc_src_get_type();

template <typename T>
QGstQrcSrc *asQGstQrcSrc(T *obj)
{
    return (G_TYPE_CHECK_INSTANCE_CAST((obj), gst_qrc_src_get_type(), QGstQrcSrc));
}

// URI handler
void qGstInitURIHandler(gpointer, gpointer);

#define gst_qrc_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(QGstQrcSrc, gst_qrc_src, GST_TYPE_BASE_SRC,
                        G_IMPLEMENT_INTERFACE(GST_TYPE_URI_HANDLER, qGstInitURIHandler));

// implementations

void gst_qrc_src_class_init(QGstQrcSrcClass *klass)
{
    // GObject
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->set_property = [](GObject *instance, guint propId, const GValue *value,
                                    GParamSpec *pspec) {
        QGstQrcSrc *src = asQGstQrcSrc(instance);
        return src->setProperty(propId, value, pspec);
    };

    gobjectClass->get_property = [](GObject *instance, guint propId, GValue *value,
                                    GParamSpec *pspec) {
        QGstQrcSrc *src = asQGstQrcSrc(instance);
        return src->getProperty(propId, value, pspec);
    };

    g_object_class_install_property(
            gobjectClass, PROP_URI,
            g_param_spec_string("uri", "QRC Location", "Path of the qrc to read", nullptr,
                                GParamFlags(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
                                            | GST_PARAM_MUTABLE_READY)));

    gobjectClass->finalize = [](GObject *instance) {
        QGstQrcSrc *src = asQGstQrcSrc(instance);
        src->file.~QFile();
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
        QGstQrcSrc *src = asQGstQrcSrc(basesrc);
        return src->start();
    };
    gstbasesrcClass->stop = [](GstBaseSrc *basesrc) -> gboolean {
        QGstQrcSrc *src = asQGstQrcSrc(basesrc);
        return src->stop();
    };

    gstbasesrcClass->is_seekable = [](GstBaseSrc *) -> gboolean {
        return true;
    };
    gstbasesrcClass->get_size = [](GstBaseSrc *basesrc, guint64 *size) -> gboolean {
        QGstQrcSrc *src = asQGstQrcSrc(basesrc);
        auto optionalSize = src->size();
        if (!optionalSize)
            return false;
        *size = optionalSize.value();
        return true;
    };
    gstbasesrcClass->fill = [](GstBaseSrc *basesrc, guint64 offset, guint length,
                               GstBuffer *buf) -> GstFlowReturn {
        QGstQrcSrc *src = asQGstQrcSrc(basesrc);
        return src->fill(offset, length, buf);
    };
}

void gst_qrc_src_init(QGstQrcSrc *self)
{
    new (reinterpret_cast<void *>(&self->file)) QFile;

    static constexpr guint defaultBlockSize = 16384;
    gst_base_src_set_blocksize(&self->baseSrc, defaultBlockSize);
}

void qGstInitURIHandler(gpointer g_handlerInterface, gpointer)
{
    GstURIHandlerInterface *iface = (GstURIHandlerInterface *)g_handlerInterface;

    iface->get_type = [](GType) {
        return GST_URI_SRC;
    };
    iface->get_protocols = [](GType) {
        static constexpr const gchar *protocols[] = {
            "qrc",
            nullptr,
        };
        return protocols;
    };
    iface->get_uri = [](GstURIHandler *handler) -> gchar * {
        QGstQrcSrc *src = asQGstQrcSrc(handler);
        auto lock = src->lockObject();
        std::optional<QUrl> url = qQrcPathToQUrl(src->file.fileName());
        if (url)
            return g_strdup(url->toString().toUtf8().constData());

        return nullptr;
    };
    iface->set_uri = [](GstURIHandler *handler, const gchar *uri, GError **err) -> gboolean {
        QGstQrcSrc *src = asQGstQrcSrc(handler);
        return src->setURI(uri, err);
    };
}

} // namespace

// plugin registration

void qGstRegisterQRCHandler(GstPlugin *plugin)
{
    gst_element_register(plugin, "qrcsrc", GST_RANK_PRIMARY, gst_qrc_src_get_type());
}

QT_END_NAMESPACE
