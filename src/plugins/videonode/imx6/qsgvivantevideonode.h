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
******************************************************************************/

#ifndef QSGVIDEONODE_VIVANTE_H
#define QSGVIDEONODE_VIVANTE_H

#include <private/qsgvideonode_p.h>
#include <QVideoFrameFormat>

class QSGVivanteVideoMaterial;
class QSGVivanteVideoNode : public QSGVideoNode
{
public:
    QSGVivanteVideoNode(const QVideoFrameFormat &format);
    ~QSGVivanteVideoNode();

    QVideoFrameFormat::PixelFormat pixelFormat() const { return mFormat.pixelFormat(); }
    QVideoFrame::HandleType handleType() const { return QVideoFrame::NoHandle; }
    void setCurrentFrame(const QVideoFrame &frame, FrameFlags flags);

    static const QMap<QVideoFrameFormat::PixelFormat, GLenum>& getVideoFormat2GLFormatMap();
    static int getBytesForPixelFormat(QVideoFrameFormat::PixelFormat pixelformat);

private:
    QVideoFrameFormat mFormat;
    QSGVivanteVideoMaterial *mMaterial;

    static QMap<QVideoFrameFormat::PixelFormat, GLenum> static_VideoFormat2GLFormatMap;
};

#endif // QSGVIDEONODE_VIVANTE_H
