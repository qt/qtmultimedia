/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtDeclarative/qdeclarativeextensionplugin.h>
#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include "private/qsoundeffect_p.h"

#include "qdeclarativemediametadata_p.h"
#include "qdeclarativeaudio_p.h"
#include "qdeclarativevideooutput_p.h"
#include "qdeclarativeradio_p.h"
#include "qdeclarativebackgroundaudio_p.h"
#include "qdeclarativecamera_p.h"
#include "qdeclarativecamerapreviewprovider_p.h"
#include "qdeclarativecameraexposure_p.h"
#include "qdeclarativecameraflash_p.h"
#include "qdeclarativecamerafocus_p.h"
#include "qdeclarativecameraimageprocessing_p.h"


QML_DECLARE_TYPE(QSoundEffect)

QT_BEGIN_NAMESPACE

class QMultimediaDeclarativeModule : public QDeclarativeExtensionPlugin
{
    Q_OBJECT
public:
    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtMultimedia")
                || QLatin1String(uri) == QLatin1String("Qt.multimediakit"));

        qmlRegisterType<QSoundEffect>(uri, 4, 0, "SoundEffect");
        qmlRegisterType<QDeclarativeAudio>(uri, 4, 0, "Audio");
        qmlRegisterType<QDeclarativeAudio>(uri, 4, 0, "MediaPlayer");
        qmlRegisterType<QDeclarativeVideoOutput>(uri, 4, 0, "VideoOutput");
        qmlRegisterType<QDeclarativeRadio>(uri, 4, 0, "Radio");
        qmlRegisterType<QDeclarativeBackgroundAudio>(uri, 4, 0, "BackgroundAudio");
        qmlRegisterType<QDeclarativeCamera>(uri, 4, 0, "Camera");
        qmlRegisterUncreatableType<QDeclarativeCameraCapture>(uri, 4, 0, "CameraCapture",
                                trUtf8("CameraCapture is only provided by Camera element"));
        qmlRegisterUncreatableType<QDeclarativeCameraRecorder>(uri, 4, 0, "CameraRecorder",
                                trUtf8("CameraRecorder is only provided by Camera element"));
        qmlRegisterUncreatableType<QDeclarativeCameraExposure>(uri, 4, 0, "CameraExposure",
                                trUtf8("CameraExposure is only provided by Camera element"));
        qmlRegisterUncreatableType<QDeclarativeCameraFocus>(uri, 4, 0, "CameraFocus",
                                trUtf8("CameraFocus is only provided by Camera element"));
        qmlRegisterUncreatableType<QDeclarativeCameraImageProcessing>(uri, 4, 0, "CameraImageProcessing",
                                trUtf8("CameraImageProcessing is only provided by Camera element"));
        qmlRegisterType<QDeclarativeMediaMetaData>();
    }

    void initializeEngine(QDeclarativeEngine *engine, const char *uri)
    {
        Q_UNUSED(uri);
        engine->addImageProvider("camera", new QDeclarativeCameraPreviewProvider);
    }
};

QT_END_NAMESPACE

#include "multimedia.moc"

Q_EXPORT_PLUGIN2(qmultimediadeclarativemodule, QT_PREPEND_NAMESPACE(QMultimediaDeclarativeModule));

