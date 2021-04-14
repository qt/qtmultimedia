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

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include "qsoundeffect.h"
#include "qmediaplayer.h"
#include "qmediametadata.h"
#include "qcamera.h"
#include "qmediacapturesession.h"
#include "qmediaencoder.h"

#include <private/qdeclarativevideooutput_p.h>

#include "qdeclarativemultimediaglobal_p.h"
#include "qdeclarativeplaylist_p.h"
#include "qdeclarativecamera_p.h"
#include "qdeclarativecamerapreviewprovider_p.h"
#include "qdeclarativecameraexposure_p.h"
#include "qdeclarativecameraflash_p.h"
#include "qdeclarativetorch_p.h"

QML_DECLARE_TYPE(QSoundEffect)

QT_BEGIN_NAMESPACE

static QObject *multimedia_global_object(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine);
    return new QDeclarativeMultimediaGlobal(jsEngine);
}

Q_DECLARE_METATYPE(QMediaMetaData)

class QMultimediaDeclarativeModule : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QMultimediaDeclarativeModule(QObject *parent = nullptr) : QQmlExtensionPlugin(parent)
    {
        volatile auto registration = qml_register_types_QtMultimedia;
        Q_UNUSED(registration);
    }
    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtMultimedia"));

        // 6.0 types
        qmlRegisterType<QDeclarativeCamera>(uri, 6, 0, "Camera");
        qmlRegisterUncreatableType<QDeclarativeCameraCapture>(uri, 6, 0, "CameraCapture",
                                tr("CameraCapture is provided by Camera"));
        qmlRegisterUncreatableType<QDeclarativeCameraRecorder>(uri, 6, 0, "CameraRecorder",
                                tr("CameraRecorder is provided by Camera"));
        qmlRegisterUncreatableType<QDeclarativeCameraExposure>(uri, 6, 0, "CameraExposure",
                                tr("CameraExposure is provided by Camera"));
        qmlRegisterUncreatableType<QDeclarativeCameraFlash>(uri, 6, 0, "CameraFlash",
                                tr("CameraFlash is provided by Camera"));
        qmlRegisterUncreatableType<QDeclarativeTorch>(uri, 6, 0, "CameraTorch",
                                tr("CameraTorch is provided by Camera"));

        qmlRegisterSingletonType<QDeclarativeMultimediaGlobal>(uri, 6, 0, "QtMultimedia", multimedia_global_object);

        qmlRegisterType<QDeclarativePlaylist>(uri, 6, 0, "Playlist");
        qmlRegisterType<QDeclarativePlaylistItem>(uri, 6, 0, "PlaylistItem");

        // The minor version used to be the current Qt 5 minor. For compatibility it is the last
        // Qt 5 release.
        qmlRegisterModule(uri, 6, 0);
    }

    void initializeEngine(QQmlEngine *engine, const char *uri) override
    {
        Q_UNUSED(uri);
        engine->addImageProvider("camera", new QDeclarativeCameraPreviewProvider);
    }
};

QT_END_NAMESPACE

#include "multimedia.moc"

