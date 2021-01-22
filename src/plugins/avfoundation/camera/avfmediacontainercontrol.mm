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
