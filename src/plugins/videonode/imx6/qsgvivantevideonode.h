// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
