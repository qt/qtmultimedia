// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqnxmediautil_p.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMutex>
#include <QMutex>
#include <QString>
#include <QXmlStreamReader>

#include <mm/renderer.h>
#include <mm/renderer/types.h>

QT_BEGIN_NAMESPACE

struct MmError {
    int errorCode;
    const char *name;
};

#define MM_ERROR_ENTRY(error) { error, #error }
static const MmError mmErrors[] = {
    MM_ERROR_ENTRY(MMR_ERROR_NONE),
    MM_ERROR_ENTRY(MMR_ERROR_UNKNOWN ),
    MM_ERROR_ENTRY(MMR_ERROR_INVALID_PARAMETER ),
    MM_ERROR_ENTRY(MMR_ERROR_INVALID_STATE),
    MM_ERROR_ENTRY(MMR_ERROR_UNSUPPORTED_VALUE),
    MM_ERROR_ENTRY(MMR_ERROR_UNSUPPORTED_MEDIA_TYPE),
    MM_ERROR_ENTRY(MMR_ERROR_MEDIA_PROTECTED),
    MM_ERROR_ENTRY(MMR_ERROR_UNSUPPORTED_OPERATION),
    MM_ERROR_ENTRY(MMR_ERROR_READ),
    MM_ERROR_ENTRY(MMR_ERROR_WRITE),
    MM_ERROR_ENTRY(MMR_ERROR_MEDIA_UNAVAILABLE),
    MM_ERROR_ENTRY(MMR_ERROR_MEDIA_CORRUPTED),
    MM_ERROR_ENTRY(MMR_ERROR_OUTPUT_UNAVAILABLE),
    MM_ERROR_ENTRY(MMR_ERROR_NO_MEMORY),
    MM_ERROR_ENTRY(MMR_ERROR_RESOURCE_UNAVAILABLE),
    MM_ERROR_ENTRY(MMR_ERROR_MEDIA_DRM_NO_RIGHTS),
    MM_ERROR_ENTRY(MMR_ERROR_DRM_CORRUPTED_DATA_STORE),
    MM_ERROR_ENTRY(MMR_ERROR_DRM_OUTPUT_PROTECTION),
    MM_ERROR_ENTRY(MMR_ERROR_DRM_OPL_HDMI),
    MM_ERROR_ENTRY(MMR_ERROR_DRM_OPL_DISPLAYPORT),
    MM_ERROR_ENTRY(MMR_ERROR_DRM_OPL_DVI),
    MM_ERROR_ENTRY(MMR_ERROR_DRM_OPL_ANALOG_VIDEO),
    MM_ERROR_ENTRY(MMR_ERROR_DRM_OPL_ANALOG_AUDIO),
    MM_ERROR_ENTRY(MMR_ERROR_DRM_OPL_TOSLINK),
    MM_ERROR_ENTRY(MMR_ERROR_DRM_OPL_SPDIF),
    MM_ERROR_ENTRY(MMR_ERROR_DRM_OPL_BLUETOOTH),
    MM_ERROR_ENTRY(MMR_ERROR_DRM_OPL_WIRELESSHD),
};
static const unsigned int numMmErrors = sizeof(mmErrors) / sizeof(MmError);

template <typename T, size_t N>
constexpr size_t countof(T (&)[N])
{
    return N;
}

QString keyValueMapsLocation()
{
    QByteArray qtKeyValueMaps = qgetenv("QT_KEY_VALUE_MAPS");
    if (qtKeyValueMaps.isNull())
        return QString::fromUtf8("/etc/qt/keyvaluemaps");
    else
        return QString::fromUtf8(qtKeyValueMaps);
}

QJsonObject loadMapObject(const QString &keyValueMapPath)
{
    QFile mapFile(keyValueMapsLocation() + keyValueMapPath);
    if (mapFile.open(QIODevice::ReadOnly)) {
        QByteArray mapFileContents = mapFile.readAll();
        QJsonDocument mapDocument = QJsonDocument::fromJson(mapFileContents);
        if (mapDocument.isObject()) {
            QJsonObject mapObject = mapDocument.object();
            return mapObject;
        }
    }
    return QJsonObject();
}

QString mmErrorMessage(const QString &msg, mmr_context_t *context, int *errorCode)
{
    const mmr_error_info_t * const mmError = mmr_error_info(context);

    if (errorCode)
        *errorCode = mmError->error_code;

    if (mmError->error_code < numMmErrors) {
        return QString::fromLatin1("%1: %2 (code %3)").arg(msg).arg(QString::fromUtf8(mmErrors[mmError->error_code].name))
                                          .arg(mmError->error_code);
    } else {
        return QString::fromLatin1("%1: Unknown error code %2").arg(msg).arg(mmError->error_code);
    }
}

bool checkForDrmPermission()
{
    QDir sandboxDir = QDir::home(); // always returns 'data' directory
    sandboxDir.cdUp(); // change to app sandbox directory

    QFile file(sandboxDir.filePath(QString::fromUtf8("app/native/bar-descriptor.xml")));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "checkForDrmPermission: Unable to open bar-descriptor.xml";
        return false;
    }

    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        reader.readNextStartElement();
        if (reader.name() == QLatin1String("action")
            || reader.name() == QLatin1String("permission")) {
            if (reader.readElementText().trimmed() == QLatin1String("access_protected_media"))
                return true;
        }
    }

    return false;
}

QT_END_NAMESPACE
