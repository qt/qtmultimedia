/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtDeclarative/qdeclarativeextensionplugin.h>
#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>

#include "qdeclarative_audioengine_p.h"
#include "qdeclarative_soundinstance_p.h"
#include "qdeclarative_sound_p.h"
#include "qdeclarative_playvariation_p.h"
#include "qdeclarative_audiocategory_p.h"
#include "qdeclarative_audiolistener_p.h"
#include "qdeclarative_audiosample_p.h"
#include "qdeclarative_attenuationmodel_p.h"

QT_BEGIN_NAMESPACE

class QAudioEngineDeclarativeModule : public QDeclarativeExtensionPlugin
{
    Q_OBJECT
public:
    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtAudioEngine"));

        qmlRegisterType<QDeclarativeAudioEngine>(uri, 1, 0, "AudioEngine");
        qmlRegisterType<QDeclarativeAudioSample>(uri, 1, 0, "AudioSample");
        qmlRegisterType<QDeclarativeAudioCategory>(uri, 1, 0, "AudioCategory");
        qmlRegisterType<QDeclarativeSoundCone>(uri, 1, 0, "");
        qmlRegisterType<QDeclarativeSound>(uri, 1, 0, "Sound");
        qmlRegisterType<QDeclarativePlayVariation>(uri, 1, 0, "PlayVariation");
        qmlRegisterType<QDeclarativeAudioListener>(uri, 1, 0, "AudioListener");
        qmlRegisterType<QDeclarativeSoundInstance>(uri, 1, 0, "SoundInstance");

        qmlRegisterType<QDeclarativeAttenuationModelLinear>(uri, 1, 0, "AttenuationModelLinear");
        qmlRegisterType<QDeclarativeAttenuationModelInverse>(uri, 1, 0, "AttenuationModelInverse");
    }
};

QT_END_NAMESPACE

#include "audioengine.moc"

Q_EXPORT_PLUGIN2("QtAudioEngine", QT_PREPEND_NAMESPACE(QAudioEngineDeclarativeModule));

