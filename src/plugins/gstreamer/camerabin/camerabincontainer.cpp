/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "camerabincontainer.h"
#include <QtCore/qregexp.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

CameraBinContainer::CameraBinContainer(QObject *parent)
    :QMediaContainerControl(parent),
      m_supportedContainers(QGstCodecsInfo::Muxer)
{
    //extension for containers hard to guess from mimetype
    m_fileExtensions["video/x-matroska"] = "mkv";
    m_fileExtensions["video/quicktime"] = "mov";
    m_fileExtensions["video/x-msvideo"] = "avi";
    m_fileExtensions["video/msvideo"] = "avi";
    m_fileExtensions["audio/mpeg"] = "mp3";
    m_fileExtensions["application/x-shockwave-flash"] = "swf";
    m_fileExtensions["application/x-pn-realmedia"] = "rm";
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
        emit settingsChanged();
    }
}

GstEncodingContainerProfile *CameraBinContainer::createProfile()
{
    GstCaps *caps;

    if (m_format.isEmpty()) {
        caps = gst_caps_new_any();
    } else {
        QString format = m_format;
        QStringList supportedFormats = m_supportedContainers.supportedCodecs();

        //if format is not in the list of supported gstreamer mime types,
        //try to find the mime type with matching extension
        if (!supportedFormats.contains(format)) {
            QString extension = suggestedFileExtension(m_format);
            foreach (const QString &formatCandidate, supportedFormats) {
                if (suggestedFileExtension(formatCandidate) == extension) {
                    format = formatCandidate;
                    break;
                }
            }
        }

        caps = gst_caps_from_string(format.toLatin1());
    }

    return (GstEncodingContainerProfile *)gst_encoding_container_profile_new(
                                        "camerabin2_profile",
                                        (gchar *)"custom camera profile",
                                        caps,
                                        NULL); //preset
}

/*!
  Suggest file extension for current container mimetype.
 */
QString CameraBinContainer::suggestedFileExtension(const QString &containerFormat) const
{
    //for container names like avi instead of video/x-msvideo, use it as extension
    if (!containerFormat.contains('/'))
        return containerFormat;

    QString format = containerFormat.left(containerFormat.indexOf(','));
    QString extension = m_fileExtensions.value(format);

    if (!extension.isEmpty() || format.isEmpty())
        return extension;

    QRegExp rx("[-/]([\\w]+)$");

    if (rx.indexIn(format) != -1)
        extension = rx.cap(1);

    return extension;
}

QT_END_NAMESPACE
