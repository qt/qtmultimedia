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

#include "qandroidsgvideonodeplugin.h"
#include "qandroidsgvideonode.h"

QT_BEGIN_NAMESPACE

QList<QVideoFrame::PixelFormat> QAndroidSGVideoNodeFactoryPlugin::supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType) const
{
    QList<QVideoFrame::PixelFormat> pixelFormats;

    if (handleType == QAbstractVideoBuffer::GLTextureHandle)
        pixelFormats.append(QVideoFrame::Format_ABGR32);

    return pixelFormats;
}

QSGVideoNode *QAndroidSGVideoNodeFactoryPlugin::createNode(const QVideoSurfaceFormat &format)
{
    if (supportedPixelFormats(format.handleType()).contains(format.pixelFormat()))
        return new QAndroidSGVideoNode(format);

    return 0;
}


QT_END_NAMESPACE
