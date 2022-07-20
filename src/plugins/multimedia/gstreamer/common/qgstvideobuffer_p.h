// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTVIDEOBUFFER_P_H
#define QGSTVIDEOBUFFER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtmultimediaglobal_p.h>
#include <private/qabstractvideobuffer_p.h>
#include <QtCore/qvariant.h>

#include <qgst_p.h>
#include <gst/video/video.h>

QT_BEGIN_NAMESPACE
class QVideoFrameFormat;
class QGstreamerVideoSink;
class QOpenGLContext;

class Q_MULTIMEDIA_EXPORT QGstVideoBuffer : public QAbstractVideoBuffer
{
public:

    QGstVideoBuffer(GstBuffer *buffer, const GstVideoInfo &info, QGstreamerVideoSink *sink,
                    const QVideoFrameFormat &frameFormat, QGstCaps::MemoryFormat format);
    QGstVideoBuffer(GstBuffer *buffer, const QVideoFrameFormat &format, const GstVideoInfo &info)
        : QGstVideoBuffer(buffer, info, nullptr, format, QGstCaps::CpuMemory)
    {}
    ~QGstVideoBuffer();

    GstBuffer *buffer() const { return m_buffer; }
    QVideoFrame::MapMode mapMode() const override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    virtual std::unique_ptr<QVideoFrameTextures> mapTextures(QRhi *) override;
private:
    QGstCaps::MemoryFormat memoryFormat = QGstCaps::CpuMemory;
    QVideoFrameFormat m_frameFormat;
    QRhi *m_rhi = nullptr;
    mutable GstVideoInfo m_videoInfo;
    mutable GstVideoFrame m_frame;
    GstBuffer *m_buffer = nullptr;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
    Qt::HANDLE eglDisplay = nullptr;
    QFunctionPointer eglImageTargetTexture2D = nullptr;
};

QT_END_NAMESPACE

#endif
