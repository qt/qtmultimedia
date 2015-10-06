/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include "qsoundeffect.h"

#include <private/qdeclarativevideooutput_p.h>
#include "qabstractvideofilter.h"

#include "qdeclarativemultimediaglobal_p.h"
#include "qdeclarativemediametadata_p.h"
#include "qdeclarativeaudio_p.h"
#include "qdeclarativeradio_p.h"
#include "qdeclarativeradiodata_p.h"
#include "qdeclarativeplaylist_p.h"
#include "qdeclarativecamera_p.h"
#include "qdeclarativecamerapreviewprovider_p.h"
#include "qdeclarativecameraexposure_p.h"
#include "qdeclarativecameraflash_p.h"
#include "qdeclarativecamerafocus_p.h"
#include "qdeclarativecameraimageprocessing_p.h"
#include "qdeclarativecameraviewfinder_p.h"
#include "qdeclarativetorch_p.h"

QML_DECLARE_TYPE(QSoundEffect)

QT_BEGIN_NAMESPACE

static QObject *multimedia_global_object(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    return new QDeclarativeMultimediaGlobal(jsEngine);
}

class QMultimediaDeclarativeModule : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface/1.0")

public:
    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtMultimedia"));

        // 5.0 types
        qmlRegisterType<QSoundEffect>(uri, 5, 0, "SoundEffect");
        qmlRegisterType<QDeclarativeAudio>(uri, 5, 0, "Audio");
        qmlRegisterType<QDeclarativeAudio>(uri, 5, 0, "MediaPlayer");
        qmlRegisterType<QDeclarativeVideoOutput>(uri, 5, 0, "VideoOutput");
        qmlRegisterType<QDeclarativeRadio>(uri, 5, 0, "Radio");
        qmlRegisterType<QDeclarativeRadioData>(uri, 5, 0, "RadioData");
        qmlRegisterType<QDeclarativeCamera>(uri, 5, 0, "Camera");
        qmlRegisterType<QDeclarativeTorch>(uri, 5, 0, "Torch");
        qmlRegisterUncreatableType<QDeclarativeCameraCapture>(uri, 5, 0, "CameraCapture",
                                trUtf8("CameraCapture is provided by Camera"));
        qmlRegisterUncreatableType<QDeclarativeCameraRecorder>(uri, 5, 0, "CameraRecorder",
                                trUtf8("CameraRecorder is provided by Camera"));
        qmlRegisterUncreatableType<QDeclarativeCameraExposure>(uri, 5, 0, "CameraExposure",
                                trUtf8("CameraExposure is provided by Camera"));
        qmlRegisterUncreatableType<QDeclarativeCameraFocus>(uri, 5, 0, "CameraFocus",
                                trUtf8("CameraFocus is provided by Camera"));
        qmlRegisterUncreatableType<QDeclarativeCameraFlash>(uri, 5, 0, "CameraFlash",
                                trUtf8("CameraFlash is provided by Camera"));
        qmlRegisterUncreatableType<QDeclarativeCameraImageProcessing>(uri, 5, 0, "CameraImageProcessing",
                                trUtf8("CameraImageProcessing is provided by Camera"));

        // 5.2 types
        qmlRegisterType<QDeclarativeVideoOutput, 2>(uri, 5, 2, "VideoOutput");

        // 5.3 types
        // Nothing changed, but adding "import QtMultimedia 5.3" in QML will fail unless at
        // least one type is registered for that version.
        qmlRegisterType<QSoundEffect>(uri, 5, 3, "SoundEffect");

        // 5.4 types
        qmlRegisterSingletonType<QDeclarativeMultimediaGlobal>(uri, 5, 4, "QtMultimedia", multimedia_global_object);
        qmlRegisterType<QDeclarativeCamera, 1>(uri, 5, 4, "Camera");
        qmlRegisterUncreatableType<QDeclarativeCameraViewfinder>(uri, 5, 4, "CameraViewfinder",
                                trUtf8("CameraViewfinder is provided by Camera"));

        // 5.5 types
        qmlRegisterUncreatableType<QDeclarativeCameraImageProcessing, 1>(uri, 5, 5, "CameraImageProcessing", trUtf8("CameraImageProcessing is provided by Camera"));
        qmlRegisterType<QDeclarativeCamera, 2>(uri, 5, 5, "Camera");

        // 5.6 types
        qmlRegisterType<QDeclarativeAudio, 1>(uri, 5, 6, "Audio");
        qmlRegisterType<QDeclarativeAudio, 1>(uri, 5, 6, "MediaPlayer");
        qmlRegisterType<QDeclarativePlaylist>(uri, 5, 6, "Playlist");
        qmlRegisterType<QDeclarativePlaylistItem>(uri, 5, 6, "PlaylistItem");

        qmlRegisterType<QDeclarativeMediaMetaData>();
        qmlRegisterType<QAbstractVideoFilter>();
    }

    void initializeEngine(QQmlEngine *engine, const char *uri)
    {
        Q_UNUSED(uri);
        engine->addImageProvider("camera", new QDeclarativeCameraPreviewProvider);
    }
};

QT_END_NAMESPACE

#include "multimedia.moc"

