// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/qqmlextensionplugin.h>

#include <private/qtmultimediaquickglobal_p.h>
#include <private/qquickimagepreviewprovider_p.h>

QT_BEGIN_NAMESPACE

class QMultimediaQuickModule : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)

public:
    explicit QMultimediaQuickModule(QObject *parent = nullptr)
        : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = qml_register_types_QtMultimedia;
        Q_UNUSED(registration);
    }

    void initializeEngine(QQmlEngine *engine, [[maybe_unused]] const char *uri) override
    {
        using namespace Qt::Literals;
        engine->addImageProvider(u"QtMultimediaCameraPreviewImageProvider"_s, new QQuickImagePreviewProvider);
    }
};

QT_END_NAMESPACE

#include "multimedia_plugin.moc"

