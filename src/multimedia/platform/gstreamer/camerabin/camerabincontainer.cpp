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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include "camerabincontainer.h"
#include <private/qgstutils_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

CameraBinContainer::CameraBinContainer(QObject *parent)
    :QMediaContainerControl(parent)
    , m_supportedContainers(QGstCodecsInfo::Muxer)
{
}

QStringList CameraBinContainer::supportedContainers() const
{
    return m_supportedContainers.supportedCodecs();
}

QString CameraBinContainer::containerDescription(const QString &formatMimeType) const
{
    return m_supportedContainers.codecDescription(formatMimeType);
}

QString CameraBinContainer::containerFormat() const
{
    return m_format;
}

void CameraBinContainer::setContainerFormat(const QString &format)
{
    if (m_format != format) {
        m_format = format;
        m_actualFormat = format;
        emit settingsChanged();
    }
}

QString CameraBinContainer::actualContainerFormat() const
{
    return m_actualFormat;
}

void CameraBinContainer::setActualContainerFormat(const QString &containerFormat)
{
    m_actualFormat = containerFormat;
}

void CameraBinContainer::resetActualContainerFormat()
{
    m_actualFormat = m_format;
}

GstEncodingContainerProfile *CameraBinContainer::createProfile()
{
    GstCaps *caps = nullptr;

    if (m_actualFormat.isEmpty())
        return 0;

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

    GstEncodingContainerProfile *profile = (GstEncodingContainerProfile *)gst_encoding_container_profile_new(
                "camerabin2_profile",
                (gchar *)"custom camera profile",
                caps,
                NULL); //preset

    gst_caps_unref(caps);

    return profile;
}

QT_END_NAMESPACE
