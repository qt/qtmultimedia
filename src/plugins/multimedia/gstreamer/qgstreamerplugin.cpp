// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qplatformmediaplugin_p.h>

#include <qgstreamerintegration_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "gstreamer.json")

public:
    QGstreamerMediaPlugin() = default;

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == u"gstreamer")
            return new QGstreamerIntegration;
        return nullptr;
    }
};

QT_END_NAMESPACE

#include "qgstreamerplugin.moc"
