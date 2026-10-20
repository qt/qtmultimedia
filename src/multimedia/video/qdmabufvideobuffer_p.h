// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QDMABUFVIDEOBUFFER_P_H
#define QDMABUFVIDEOBUFFER_P_H

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


#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/qsize.h>
#include <QtCore/qspan.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/private/quniquehandle_types_p.h>

#include <memory>

static_assert(QT_CONFIG(linux_dmabuf));

QT_BEGIN_NAMESPACE

class QRhi;

namespace QtMultimediaPrivate {

class Q_MULTIMEDIA_EXPORT QDmaBufVideoBuffer : public QHwVideoBuffer
{
public:
    QDmaBufVideoBuffer(QVideoFrameFormat::PixelFormat format, QSize size,
                       QSpan<const DmaBufPlane> planes, std::shared_ptr<void> keepAlive = {},
                       DmaBufImportMode importMode = DmaBufImportMode::PerPlane);
    ~QDmaBufVideoBuffer() override;

    bool isDmaBuf() const override { return true; }

    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi) override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    QVideoFrameFormat::PixelFormat pixelFormat() const { return m_format; }
    QSize size() const { return m_size; }
    DmaBufImportMode importMode() const { return m_importMode; }

private:
    const QVideoFrameFormat::PixelFormat m_format;
    const QSize m_size;
    QVarLengthArray<DmaBufPlane, 4> m_planes; // later: std::inplace_vector

    const std::shared_ptr<void> m_keepAlive;
    const DmaBufImportMode m_importMode;

    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    struct MappedRegion
    {
        QUniqueFileDescriptorHandle fd;
        QSpan<std::byte> data;
    };
    QVarLengthArray<MappedRegion, 4> m_mappedRegions;

    static void unmapRegions(QSpan<const MappedRegion> regions);
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QDMABUFVIDEOBUFFER_P_H
