// Copyright (C) 2022 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQNXCAMERAFRAMEBUFFER_H
#define QQNXCAMERAFRAMEBUFFER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qabstractvideobuffer_p.h>

#include <QtCore/qsize.h>

#include <camera/camera_api.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QRhi;

class QQnxCameraFrameBuffer : public QAbstractVideoBuffer
{
public:
    explicit QQnxCameraFrameBuffer(const camera_buffer_t *buffer, QRhi *rhi = nullptr);

    QQnxCameraFrameBuffer(const QQnxCameraFrameBuffer&) = delete;
    QQnxCameraFrameBuffer& operator=(const QQnxCameraFrameBuffer&) = delete;

    QVideoFrame::MapMode mapMode() const override;
    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    QVideoFrameFormat::PixelFormat pixelFormat() const;

    QSize size() const;

private:
    QRhi *m_rhi;

    QVideoFrameFormat::PixelFormat m_pixelFormat;

    std::unique_ptr<unsigned char[]> m_data;

    size_t m_dataSize;

    MapData m_mapData;

    QSize m_frameSize;
};

QT_END_NAMESPACE

#endif
