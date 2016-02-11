/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "avfmediacontainercontrol.h"

#include <AVFoundation/AVMediaFormat.h>

QT_BEGIN_NAMESPACE

struct ContainerInfo
{
    QString description;
    NSString *fileType;

    ContainerInfo() : fileType(nil) { }
    ContainerInfo(const QString &desc, NSString *type)
        : description(desc), fileType(type)
    { }
};

typedef QMap<QString, ContainerInfo> SupportedContainers;
Q_GLOBAL_STATIC(SupportedContainers, containers);

AVFMediaContainerControl::AVFMediaContainerControl(AVFCameraService *)
    : QMediaContainerControl()
    , m_format(QStringLiteral("mov")) // .mov is the default container format on Apple platforms
{
    if (containers->isEmpty()) {
        containers->insert(QStringLiteral("mov"),
                           ContainerInfo(QStringLiteral("QuickTime movie file format"),
                                         AVFileTypeQuickTimeMovie));
        containers->insert(QStringLiteral("mp4"),
                           ContainerInfo(QStringLiteral("MPEG-4 file format"),
                                         AVFileTypeMPEG4));
        containers->insert(QStringLiteral("m4v"),
                           ContainerInfo(QStringLiteral("iTunes video file format"),
                                         AVFileTypeAppleM4V));
#ifdef Q_OS_IOS
        containers->insert(QStringLiteral("3gp"),
                           ContainerInfo(QStringLiteral("3GPP file format"),
                                         AVFileType3GPP));
#endif
    }
}

QStringList AVFMediaContainerControl::supportedContainers() const
{
    return containers->keys();
}

QString AVFMediaContainerControl::containerFormat() const
{
    return m_format;
}

void AVFMediaContainerControl::setContainerFormat(const QString &format)
{
    if (!containers->contains(format)) {
        qWarning("Unsupported container format: '%s'", format.toLocal8Bit().constData());
        return;
    }

    m_format = format;
}

QString AVFMediaContainerControl::containerDescription(const QString &formatMimeType) const
{
    return containers->value(formatMimeType).description;
}

NSString *AVFMediaContainerControl::fileType() const
{
    return containers->value(m_format).fileType;
}

QT_END_NAMESPACE
