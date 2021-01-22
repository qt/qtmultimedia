/****************************************************************************
**
** Copyright (C) 2016 Pelagicore AG
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

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "qsgvivantevideonode.h"
#include "qsgvivantevideomaterialshader.h"
#include "qsgvivantevideomaterial.h"

QMap<QVideoFrame::PixelFormat, GLenum> QSGVivanteVideoNode::static_VideoFormat2GLFormatMap = QMap<QVideoFrame::PixelFormat, GLenum>();

QSGVivanteVideoNode::QSGVivanteVideoNode(const QVideoSurfaceFormat &format) :
    mFormat(format)
{
    setFlag(QSGNode::OwnsMaterial, true);
    mMaterial = new QSGVivanteVideoMaterial();
    setMaterial(mMaterial);
}

QSGVivanteVideoNode::~QSGVivanteVideoNode()
{
}

void QSGVivanteVideoNode::setCurrentFrame(const QVideoFrame &frame, FrameFlags flags)
{
    mMaterial->setCurrentFrame(frame, flags);
    markDirty(DirtyMaterial);
}

const QMap<QVideoFrame::PixelFormat, GLenum>& QSGVivanteVideoNode::getVideoFormat2GLFormatMap()
{
    if (static_VideoFormat2GLFormatMap.isEmpty()) {
        static_VideoFormat2GLFormatMap.insert(QVideoFrame::Format_YUV420P,  GL_VIV_I420);
        static_VideoFormat2GLFormatMap.insert(QVideoFrame::Format_YV12,     GL_VIV_YV12);
        static_VideoFormat2GLFormatMap.insert(QVideoFrame::Format_NV12,     GL_VIV_NV12);
        static_VideoFormat2GLFormatMap.insert(QVideoFrame::Format_NV21,     GL_VIV_NV21);
        static_VideoFormat2GLFormatMap.insert(QVideoFrame::Format_UYVY,     GL_VIV_UYVY);
        static_VideoFormat2GLFormatMap.insert(QVideoFrame::Format_YUYV,     GL_VIV_YUY2);
        static_VideoFormat2GLFormatMap.insert(QVideoFrame::Format_RGB32,    GL_BGRA_EXT);
        static_VideoFormat2GLFormatMap.insert(QVideoFrame::Format_ARGB32,   GL_BGRA_EXT);
        static_VideoFormat2GLFormatMap.insert(QVideoFrame::Format_BGR32,    GL_RGBA);
        static_VideoFormat2GLFormatMap.insert(QVideoFrame::Format_BGRA32,   GL_RGBA);
        static_VideoFormat2GLFormatMap.insert(QVideoFrame::Format_RGB565,   GL_RGB565);
    }

    return static_VideoFormat2GLFormatMap;
}


int QSGVivanteVideoNode::getBytesForPixelFormat(QVideoFrame::PixelFormat pixelformat)
{
    switch (pixelformat) {
    case QVideoFrame::Format_YUV420P: return 1;
    case QVideoFrame::Format_YV12: return 1;
    case QVideoFrame::Format_NV12: return 1;
    case QVideoFrame::Format_NV21: return 1;
    case QVideoFrame::Format_UYVY: return 2;
    case QVideoFrame::Format_YUYV: return 2;
    case QVideoFrame::Format_RGB32: return 4;
    case QVideoFrame::Format_ARGB32: return 4;
    case QVideoFrame::Format_BGR32: return 4;
    case QVideoFrame::Format_BGRA32: return 4;
    case QVideoFrame::Format_RGB565: return 2;
    default: return 1;
    }
}



