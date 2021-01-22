/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include "camerabincontainer.h"
#include <QtCore/qregexp.h>
#include <private/qgstutils_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

CameraBinContainer::CameraBinContainer(QObject *parent)
    :QMediaContainerControl(parent)
#if QT_CONFIG(gstreamer_encodingprofiles)
    , m_supportedContainers(QGstCodecsInfo::Muxer)
#endif
{
}

QStringList CameraBinContainer::supportedContainers() const
{
#if QT_CONFIG(gstreamer_encodingprofiles)
    return m_supportedContainers.supportedCodecs();
#else
    return QStringList();
#endif
}

QString CameraBinContainer::containerDescription(const QString &formatMimeType) const
{
#if QT_CONFIG(gstreamer_encodingprofiles)
    return m_supportedContainers.codecDescription(formatMimeType);
#else
    Q_UNUSED(formatMimeType)
    return QString();
#endif
}

QString CameraBinContainer::containerFormat() const
{
    return m_format;
}

void CameraBinContainer::setContainerFormat(const QString &format)
{
#if QT_CONFIG(gstreamer_encodingprofiles)
    if (m_format != format) {
        m_format = format;
        m_actualFormat = format;
        emit settingsChanged();
    }
#endif
}

QString CameraBinContainer::actualContainerFormat() const
{
    return m_actualFormat;
}

void CameraBinContainer::setActualContainerFormat(const QString &containerFormat)
{
#if QT_CONFIG(gstreamer_encodingprofiles)
    m_actualFormat = containerFormat;
#endif
}

void CameraBinContainer::resetActualContainerFormat()
{
    m_actualFormat = m_format;
}

#if QT_CONFIG(gstreamer_encodingprofiles)

GstEncodingContainerProfile *CameraBinContainer::createProfile()
{
    GstCaps *caps = nullptr;

    if (m_actualFormat.isEmpty()) {
        return 0;
    } else {
        QString format = m_actualFormat;
        const QStringList supportedFormats = m_supportedContainers.supportedCodecs();

        //if format is not in the list of supported gstreamer mime types,
        //try to find the mime type with matching extension
        if (!supportedFormats.contains(format)) {
            format.clear();
            QString extension = QGstUtils::fileExtensionForMimeType(m_actualFormat);
            for (const QString &formatCandidate : supportedFormats) {
                if (QGstUtils::fileExtensionForMimeType(formatCandidate) == extension) {
                    format = formatCandidate;
                    break;
                }
            }
        }

        if (format.isEmpty())
            return nullptr;

        caps = gst_caps_from_string(format.toLatin1());
    }

    GstEncodingContainerProfile *profile = (GstEncodingContainerProfile *)gst_encoding_container_profile_new(
                "camerabin2_profile",
                (gchar *)"custom camera profile",
                caps,
                NULL); //preset

    gst_caps_unref(caps);

    return profile;
}

#endif

QT_END_NAMESPACE
