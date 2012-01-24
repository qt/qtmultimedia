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

#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>
#include <QtCore/QDir>
#include <QtCore/QDebug>

#include "qgstreamerserviceplugin.h"

//#define QT_SUPPORTEDMIMETYPES_DEBUG

#ifdef QMEDIA_GSTREAMER_PLAYER
#include "qgstreamerplayerservice.h"
#endif

#if defined(QMEDIA_GSTREAMER_CAPTURE)
#include "qgstreamercaptureservice.h"
#endif

#ifdef QMEDIA_GSTREAMER_CAMERABIN
#include "camerabinservice.h"
#endif

#include <qmediaserviceprovider.h>

#include <linux/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/videodev2.h>


QStringList QGstreamerServicePlugin::keys() const
{
    return QStringList()
#ifdef QMEDIA_GSTREAMER_PLAYER
            << QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER)
#endif

#ifdef QMEDIA_GSTREAMER_CAPTURE
            << QLatin1String(Q_MEDIASERVICE_AUDIOSOURCE)
            << QLatin1String(Q_MEDIASERVICE_CAMERA)
#elif defined(QMEDIA_GSTREAMER_CAMERABIN)
            << QLatin1String(Q_MEDIASERVICE_CAMERA)
#endif
    ;

}

QMediaService* QGstreamerServicePlugin::create(const QString &key)
{
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        gst_init(NULL, NULL);
    }

#ifdef QMEDIA_GSTREAMER_PLAYER
    if (key == QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER))
        return new QGstreamerPlayerService;
#endif

#ifdef QMEDIA_GSTREAMER_CAMERABIN
    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA) && CameraBinService::isCameraBinAvailable())
        return new CameraBinService(key);
#endif

#ifdef QMEDIA_GSTREAMER_CAPTURE
    if (key == QLatin1String(Q_MEDIASERVICE_AUDIOSOURCE))
        return new QGstreamerCaptureService(key);

    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA))
        return new QGstreamerCaptureService(key);
#endif

    qWarning() << "Gstreamer service plugin: unsupported key:" << key;
    return 0;
}

void QGstreamerServicePlugin::release(QMediaService *service)
{
    delete service;
}

QMediaServiceProviderHint::Features QGstreamerServicePlugin::supportedFeatures(
        const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_MEDIAPLAYER)
        return QMediaServiceProviderHint::StreamPlayback | QMediaServiceProviderHint::VideoSurface;
    else if (service == Q_MEDIASERVICE_CAMERA)
        return QMediaServiceProviderHint::VideoSurface;
    else
        return QMediaServiceProviderHint::Features();
}

QList<QByteArray> QGstreamerServicePlugin::devices(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (m_cameraDevices.isEmpty())
            updateDevices();

        return m_cameraDevices;
    }

    return QList<QByteArray>();
}

QString QGstreamerServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (m_cameraDevices.isEmpty())
            updateDevices();

        for (int i=0; i<m_cameraDevices.count(); i++)
            if (m_cameraDevices[i] == device)
                return m_cameraDescriptions[i];
    }

    return QString();
}

QVariant QGstreamerServicePlugin::deviceProperty(const QByteArray &service, const QByteArray &device, const QByteArray &property)
{
    Q_UNUSED(service);
    Q_UNUSED(device);
    Q_UNUSED(property);
    return QVariant();
}

void QGstreamerServicePlugin::updateDevices() const
{
    m_cameraDevices.clear();
    m_cameraDescriptions.clear();

#ifdef Q_WS_MAEMO_6
    m_cameraDevices << "primary" << "secondary";
    m_cameraDescriptions << tr("Main camera") << tr("Front camera");
    return;
#endif

    QDir devDir("/dev");
    devDir.setFilter(QDir::System);

    QFileInfoList entries = devDir.entryInfoList(QStringList() << "video*");

    foreach( const QFileInfo &entryInfo, entries ) {
        //qDebug() << "Try" << entryInfo.filePath();

        int fd = ::open(entryInfo.filePath().toLatin1().constData(), O_RDWR );
        if (fd == -1)
            continue;

        bool isCamera = false;

        v4l2_input input;
        memset(&input, 0, sizeof(input));
        for (; ::ioctl(fd, VIDIOC_ENUMINPUT, &input) >= 0; ++input.index) {
            if(input.type == V4L2_INPUT_TYPE_CAMERA || input.type == 0) {
                isCamera = ::ioctl(fd, VIDIOC_S_INPUT, input.index) != 0;
                break;
            }
        }

        if (isCamera) {
            // find out its driver "name"
            QString name;
            struct v4l2_capability vcap;
            memset(&vcap, 0, sizeof(struct v4l2_capability));

            if (ioctl(fd, VIDIOC_QUERYCAP, &vcap) != 0)
                name = entryInfo.fileName();
            else
                name = QString((const char*)vcap.card);
            //qDebug() << "found camera: " << name;

            m_cameraDevices.append(entryInfo.filePath().toLocal8Bit());
            m_cameraDescriptions.append(name);
        }
        ::close(fd);
    }
}

namespace {
    const char* getCodecAlias(const QString &codec)
    {
        if (codec.startsWith("avc1."))
            return "video/x-h264";

        if (codec.startsWith("mp4a."))
            return "audio/mpeg4";

        if (codec.startsWith("mp4v.20."))
            return "video/mpeg4";

        if (codec == "samr")
            return "audio/amr";

        return 0;
    }

    const char* getMimeTypeAlias(const QString &mimeType)
    {
        if (mimeType == "video/mp4")
            return "video/mpeg4";

        if (mimeType == "audio/mp4")
            return "audio/mpeg4";

        if (mimeType == "video/ogg"
            || mimeType == "audio/ogg")
            return "application/ogg";

        return 0;
    }
}

QtMultimedia::SupportEstimate QGstreamerServicePlugin::hasSupport(const QString &mimeType,
                                                                     const QStringList& codecs) const
{
    if (m_supportedMimeTypeSet.isEmpty())
        updateSupportedMimeTypes();

    QString mimeTypeLowcase = mimeType.toLower();
    bool containsMimeType = m_supportedMimeTypeSet.contains(mimeTypeLowcase);
    if (!containsMimeType) {
        const char* mimeTypeAlias = getMimeTypeAlias(mimeTypeLowcase);
        containsMimeType = m_supportedMimeTypeSet.contains(mimeTypeAlias);
        if (!containsMimeType) {
            containsMimeType = m_supportedMimeTypeSet.contains("video/" + mimeTypeLowcase)
                               || m_supportedMimeTypeSet.contains("video/x-" + mimeTypeLowcase)
                               || m_supportedMimeTypeSet.contains("audio/" + mimeTypeLowcase)
                               || m_supportedMimeTypeSet.contains("audio/x-" + mimeTypeLowcase);
        }
    }

    int supportedCodecCount = 0;
    foreach(const QString &codec, codecs) {
        QString codecLowcase = codec.toLower();
        const char* codecAlias = getCodecAlias(codecLowcase);
        if (codecAlias) {
            if (m_supportedMimeTypeSet.contains(codecAlias))
                supportedCodecCount++;
        } else if (m_supportedMimeTypeSet.contains("video/" + codecLowcase)
                   || m_supportedMimeTypeSet.contains("video/x-" + codecLowcase)
                   || m_supportedMimeTypeSet.contains("audio/" + codecLowcase)
                   || m_supportedMimeTypeSet.contains("audio/x-" + codecLowcase)) {
            supportedCodecCount++;
        }
    }
    if (supportedCodecCount > 0 && supportedCodecCount == codecs.size())
        return QtMultimedia::ProbablySupported;

    if (supportedCodecCount == 0 && !containsMimeType)
        return QtMultimedia::NotSupported;

    return QtMultimedia::MaybeSupported;
}

void QGstreamerServicePlugin::updateSupportedMimeTypes() const
{
    //enumerate supported mime types
    gst_init(NULL, NULL);

    GList *plugins, *orig_plugins;
    orig_plugins = plugins = gst_default_registry_get_plugin_list ();

    while (plugins) {
        GList *features, *orig_features;

        GstPlugin *plugin = (GstPlugin *) (plugins->data);
        plugins = g_list_next (plugins);

        if (plugin->flags & (1<<1)) //GST_PLUGIN_FLAG_BLACKLISTED
            continue;

        orig_features = features = gst_registry_get_feature_list_by_plugin(gst_registry_get_default (),
                                                                        plugin->desc.name);
        while (features) {
            if (!G_UNLIKELY(features->data == NULL)) {
                GstPluginFeature *feature = GST_PLUGIN_FEATURE(features->data);
                if (GST_IS_ELEMENT_FACTORY (feature)) {
                    GstElementFactory *factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(feature));
                    if (factory
                       && factory->numpadtemplates > 0
                       && (qstrcmp(factory->details.klass, "Codec/Decoder/Audio") == 0
                          || qstrcmp(factory->details.klass, "Codec/Decoder/Video") == 0
                          || qstrcmp(factory->details.klass, "Codec/Demux") == 0 )) {
                        const GList *pads = factory->staticpadtemplates;
                        while (pads) {
                            GstStaticPadTemplate *padtemplate = (GstStaticPadTemplate*)(pads->data);
                            pads = g_list_next (pads);
                            if (padtemplate->direction != GST_PAD_SINK)
                                continue;
                            if (padtemplate->static_caps.string) {
                                GstCaps *caps = gst_static_caps_get(&padtemplate->static_caps);
                                if (!gst_caps_is_any (caps) && ! gst_caps_is_empty (caps)) {
                                    for (guint i = 0; i < gst_caps_get_size(caps); i++) {
                                        GstStructure *structure = gst_caps_get_structure(caps, i);
                                        QString nameLowcase = QString(gst_structure_get_name (structure)).toLower();

                                        m_supportedMimeTypeSet.insert(nameLowcase);
                                        if (nameLowcase.contains("mpeg")) {
                                            //Because mpeg version number is only included in the detail
                                            //description,  it is necessary to manually extract this information
                                            //in order to match the mime type of mpeg4.
                                            const GValue *value = gst_structure_get_value(structure, "mpegversion");
                                            if (value) {
                                                gchar *str = gst_value_serialize (value);
                                                QString versions(str);
                                                QStringList elements = versions.split(QRegExp("\\D+"), QString::SkipEmptyParts);
                                                foreach(const QString &e, elements)
                                                    m_supportedMimeTypeSet.insert(nameLowcase + e);
                                                g_free (str);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        gst_object_unref (factory);
                    }
                } else if (GST_IS_TYPE_FIND_FACTORY(feature)) {
                    QString name(gst_plugin_feature_get_name(feature));
                    if (name.contains('/')) //filter out any string without '/' which is obviously not a mime type
                        m_supportedMimeTypeSet.insert(name.toLower());
                }
            }
            features = g_list_next (features);
        }
        gst_plugin_feature_list_free (orig_features);
    }
    gst_plugin_list_free (orig_plugins);

#if defined QT_SUPPORTEDMIMETYPES_DEBUG
    QStringList list = m_supportedMimeTypeSet.toList();
    list.sort();
    if (qgetenv("QT_DEBUG_PLUGINS").toInt() > 0) {
        foreach(const QString &type, list)
            qDebug() << type;
    }
#endif
}

QStringList QGstreamerServicePlugin::supportedMimeTypes() const
{
    return QStringList();
}

Q_EXPORT_PLUGIN2(qtmedia_gstengine, QGstreamerServicePlugin);
