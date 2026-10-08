// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTDMABUFVIDEOBUFFER_P_H
#define QGSTDMABUFVIDEOBUFFER_P_H

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

#include <QtMultimedia/private/qdmabufvideobuffer_p.h>
#include <QtCore/qsize.h>
#include <QtCore/qspan.h>

#include <common/qgst_p.h>
#include <common/qgstbufferprovider_p.h>

#include <gst/video/video.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QGstDmaBufVideoBuffer final : public QtMultimediaPrivate::QDmaBufVideoBuffer,
                                    public QGstBufferProvider
{
public:
    static std::unique_ptr<QGstDmaBufVideoBuffer> create(QGstBufferHandle buffer,
                                                         const QGstVideoInfo &videoInfo,
                                                         const QVideoFrameFormat &frameFormat);
    ~QGstDmaBufVideoBuffer() override;

    GstBuffer *gstBuffer() const override { return m_buffer.get(); }

private:
    QGstDmaBufVideoBuffer(QSpan<const QtMultimediaPrivate::DmaBufPlane> planes,
                          QtMultimediaPrivate::DmaBufImportMode importMode, QGstBufferHandle buffer,
                          QVideoFrameFormat::PixelFormat format, QSize size);

    const QGstBufferHandle m_buffer;
};

QT_END_NAMESPACE

#endif
