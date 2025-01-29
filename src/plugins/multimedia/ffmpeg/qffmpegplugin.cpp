// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qplatformmediaplugin_p.h>

#include <qffmpegmediaintegration_p.h>

QT_BEGIN_NAMESPACE

class QFFmpegMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "ffmpeg.json")

public:
    QPlatformMediaIntegration *create(const QString &name) override
    {
        if (name == u"ffmpeg")
            return new QFFmpegMediaIntegration;
        return nullptr;
    }
};

QT_END_NAMESPACE

#include "qffmpegplugin.moc"
