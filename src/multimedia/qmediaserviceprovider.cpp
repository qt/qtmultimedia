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

#include <qtmultimediaglobal_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qmap.h>

#include "qmediaservice.h"
#include "qmediaserviceprovider_p.h"
#include "qmediaserviceproviderplugin.h"
#include "qmediapluginloader_p.h"
#include "qmediaplayer.h"
#include "qvideodeviceselectorcontrol.h"

#if QT_CONFIG(gstreamer)
#include <private/qgstreamercaptureserviceplugin_p.h>
#elif defined(Q_OS_DARWIN)
#include <private/avfcameraserviceplugin_p.h>
#elif defined(Q_OS_ANDROID)
#include <private/qandroidmediaserviceplugin_p.h>
#elif defined(Q_OS_QNX)
#include <private/neutrinoserviceplugin_p.h>
#endif

QT_BEGIN_NAMESPACE

class Loader
{
#define GET_PLUGIN(Key, Class) \
    if (key.toUtf8() == Key) { \
        static QObject *instance = nullptr; \
        if (!instance) \
            instance = new Class; \
        return instance; \
    }

public:
    QObject *instance(const QString &key) {
#if QT_CONFIG(gstreamer)
        GET_PLUGIN(Q_MEDIASERVICE_CAMERA, QGstreamerCaptureServicePlugin)
        GET_PLUGIN(Q_MEDIASERVICE_AUDIOSOURCE, QGstreamerCaptureServicePlugin)
#elif defined(Q_OS_DARWIN)
        GET_PLUGIN(Q_MEDIASERVICE_CAMERA, AVFServicePlugin)
#elif defined(Q_OS_ANDROID)
        GET_PLUGIN(Q_MEDIASERVICE_CAMERA, QAndroidMediaServicePlugin)
#endif
        return nullptr;
    }
};

Q_GLOBAL_STATIC(Loader, loader);

class QPluginServiceProvider : public QMediaServiceProvider
{
    struct MediaServiceData {
        QByteArray type;
        QMediaServiceProviderPlugin *plugin = nullptr;
    };

    QMap<const QMediaService*, MediaServiceData> mediaServiceData;

public:
    QMediaService* requestService(const QByteArray &type) override
    {
        QString key(QLatin1String(type.constData()));

        QList<QMediaServiceProviderPlugin *>plugins;
        QObject *instance = loader()->instance(key);
        QMediaServiceProviderPlugin *plugin = qobject_cast<QMediaServiceProviderPlugin*>(instance);

        if (plugin != nullptr) {
            QMediaService *service = plugin->create(key);
            if (service != nullptr) {
                MediaServiceData d;
                d.type = type;
                d.plugin = plugin;
                mediaServiceData.insert(service, d);
            }

            return service;
        }

        qWarning() << "defaultServiceProvider::requestService(): no service found for -" << key;
        return nullptr;
    }

    void releaseService(QMediaService *service) override
    {
        if (service != nullptr) {
            MediaServiceData d = mediaServiceData.take(service);

            if (d.plugin != nullptr)
                d.plugin->release(service);
        }
    }
};

Q_GLOBAL_STATIC(QPluginServiceProvider, pluginProvider);

/*!
    \class QMediaServiceProvider
    \obsolete
    \ingroup multimedia
    \ingroup multimedia_control
    \ingroup multimedia_core

    \internal

    \brief The QMediaServiceProvider class provides an abstract allocator for media services.
*/

/*!
    \internal
    \fn QMediaServiceProvider::requestService(const QByteArray &type)

    Requests an instance of a \a type service which best matches the given \a
    hint.

    Returns a pointer to the requested service, or a null pointer if there is
    no suitable service.

    The returned service must be released with releaseService when it is
    finished with.
*/

/*!
    \internal
    \fn QMediaServiceProvider::releaseService(QMediaService *service)

    Releases a media \a service requested with requestService().
*/

static QMediaServiceProvider *qt_defaultMediaServiceProvider = nullptr;

/*!
    Sets a media service \a provider as the default.
    It's useful for unit tests to provide mock service.

    \internal
*/
void QMediaServiceProvider::setDefaultServiceProvider(QMediaServiceProvider *provider)
{
    qt_defaultMediaServiceProvider = provider;
}


/*!
    \internal
    Returns a default provider of media services.
*/
QMediaServiceProvider *QMediaServiceProvider::defaultServiceProvider()
{
    return qt_defaultMediaServiceProvider != nullptr
            ? qt_defaultMediaServiceProvider
            : static_cast<QMediaServiceProvider *>(pluginProvider());
}

/*!
    \class QMediaServiceProviderPlugin
    \obsolete
    \inmodule QtMultimedia
    \brief The QMediaServiceProviderPlugin class interface provides an interface for QMediaService
    plug-ins.

    A media service provider plug-in may implement one or more of
    QMediaServiceSupportedFormatsInterface and QMediaServiceSupportedDevicesInterface
    to identify the features it supports.
*/

/*!
    \fn QMediaServiceProviderPlugin::create(const QString &key)

    Constructs a new instance of the QMediaService identified by \a key.

    The QMediaService returned must be destroyed with release().
*/

/*!
    \fn QMediaServiceProviderPlugin::release(QMediaService *service)

    Destroys a media \a service constructed with create().
*/


/*!
    \class QMediaServiceSupportedFormatsInterface
    \obsolete
    \inmodule QtMultimedia
    \brief The QMediaServiceSupportedFormatsInterface class interface
    identifies if a media service plug-in supports a media format.

    A QMediaServiceProviderPlugin may implement this interface.
*/

/*!
    \fn QMediaServiceSupportedFormatsInterface::~QMediaServiceSupportedFormatsInterface()

    Destroys a media service supported formats interface.
*/

/*!
    \fn QMediaServiceSupportedFormatsInterface::hasSupport(const QString &mimeType, const QStringList& codecs) const

    Returns the level of support a media service plug-in has for a \a mimeType
    and set of \a codecs.
*/

/*!
    \fn QMediaServiceSupportedFormatsInterface::supportedMimeTypes() const

    Returns a list of MIME types supported by the media service plug-in.
*/

QT_END_NAMESPACE

#include "moc_qmediaserviceprovider_p.cpp"
#include "moc_qmediaserviceproviderplugin.cpp"
