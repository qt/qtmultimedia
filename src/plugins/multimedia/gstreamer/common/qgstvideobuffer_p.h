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

#include <private/qhwvideobuffer_p.h>
#include <QtCore/qvariant.h>

#include <common/qgst_p.h>
#include <gst/video/video.h>

QT_BEGIN_NAMESPACE
class QVideoFrameFormat;
class QGstreamerVideoSink;
class QOpenGLContext;

class QGstVideoBuffer final : public QHwVideoBuffer
{
public:
    QGstVideoBuffer(QGstBufferHandle buffer, const GstVideoInfo &info, QGstreamerVideoSink *sink,
                    const QVideoFrameFormat &frameFormat, QGstCaps::MemoryFormat format);
    ~QGstVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    QVideoFrameTexturesUPtr mapTextures(QRhi &, QVideoFrameTexturesUPtr& /*oldTextures*/) override;

private:
    const QGstCaps::MemoryFormat m_memoryFormat = QGstCaps::CpuMemory;
    const QVideoFrameFormat m_frameFormat;
    QRhi *m_rhi = nullptr;
    mutable GstVideoInfo m_videoInfo;
    mutable GstVideoFrame m_frame{};
    const QGstBufferHandle m_buffer;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
    Qt::HANDLE eglDisplay = nullptr;
    QFunctionPointer eglImageTargetTexture2D = nullptr;
};

QT_END_NAMESPACE

#endif
