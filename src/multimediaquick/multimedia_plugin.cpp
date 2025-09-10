// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include "qsoundeffect.h"
#include "qmediaplayer.h"
#include "qmediametadata.h"
#include "qcamera.h"
#include "qmediacapturesession.h"
#include "qmediarecorder.h"

#include <private/qquickimagepreviewprovider_p.h>

QT_BEGIN_NAMESPACE

class QMultimediaQuickModule : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)

public:
    QMultimediaQuickModule(QObject *parent = nullptr)
        : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = qml_register_types_QtMultimedia;
        Q_UNUSED(registration);
    }

    void initializeEngine(QQmlEngine *engine, const char *uri) override
    {
        Q_UNUSED(uri);
        engine->addImageProvider("camera", new QQuickImagePreviewProvider);
    }
};

QT_END_NAMESPACE

#include "multimedia_plugin.moc"

