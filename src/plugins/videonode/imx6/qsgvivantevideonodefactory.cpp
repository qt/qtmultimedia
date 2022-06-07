// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgvivantevideonodefactory.h"
#include "qsgvivantevideonode.h"
#include <QtGui/QGuiApplication>

QList<QVideoFrameFormat::PixelFormat> QSGVivanteVideoNodeFactory::supportedPixelFormats(
        QVideoFrame::HandleType handleType) const
{
    const bool isWebGl = QGuiApplication::platformName() == QLatin1String("webgl");
    if (!isWebGl && handleType == QVideoFrame::NoHandle)
        return QSGVivanteVideoNode::getVideoFormat2GLFormatMap().keys();
    else
        return QList<QVideoFrameFormat::PixelFormat>();
}

QSGVideoNode *QSGVivanteVideoNodeFactory::createNode(const QVideoFrameFormat &format)
{
    if (supportedPixelFormats(format.handleType()).contains(format.pixelFormat())) {
        return new QSGVivanteVideoNode(format);
    }
    return 0;
}
