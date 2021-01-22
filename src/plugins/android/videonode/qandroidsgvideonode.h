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

#ifndef QANDROIDSGVIDEONODE_H
#define QANDROIDSGVIDEONODE_H

#include <private/qsgvideonode_p.h>
#include <qmutex.h>

QT_BEGIN_NAMESPACE

class QAndroidSGVideoNodeMaterial;

class QAndroidSGVideoNode : public QSGVideoNode
{
public:
    QAndroidSGVideoNode(const QVideoSurfaceFormat &format);
    ~QAndroidSGVideoNode();

    void setCurrentFrame(const QVideoFrame &frame, FrameFlags flags);
    QVideoFrame::PixelFormat pixelFormat() const { return m_format.pixelFormat(); }
    QAbstractVideoBuffer::HandleType handleType() const { return QAbstractVideoBuffer::GLTextureHandle; }

    void preprocess();

private:
    QAndroidSGVideoNodeMaterial *m_material;
    QMutex m_frameMutex;
    QVideoFrame m_frame;
    QVideoSurfaceFormat m_format;
};

QT_END_NAMESPACE

#endif // QANDROIDSGVIDEONODE_H
