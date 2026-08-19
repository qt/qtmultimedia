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
#include <QtMultimedia/private/qdmabuftextureimporter_p.h>
#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/qsize.h>
#include <QtCore/qspan.h>
#include <QtCore/qvarlengtharray.h>
#include <array>
#include <memory>

static_assert(QT_CONFIG(linux_dmabuf));

QT_BEGIN_NAMESPACE

class QRhi;

namespace QtMultimediaPrivate {

class Q_MULTIMEDIA_EXPORT QDmaBufVideoBuffer final : public QHwVideoBuffer
{
public:
    QDmaBufVideoBuffer(QVideoFrameFormat::PixelFormat format, QSize size,
                       QSpan<const DmaBufPlane> planes, std::shared_ptr<void> keepAlive = {});
    ~QDmaBufVideoBuffer() override;

    bool isDmaBuf() const override { return true; }

    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &oldTextures) override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    QVideoFrameFormat::PixelFormat pixelFormat() const { return m_format; }
    QSize size() const { return m_size; }

private:
    const QVideoFrameFormat::PixelFormat m_format;
    const QSize m_size;
    std::array<DmaBufPlane, 4> m_planes;
    const int m_planeCount = 0;

    const std::shared_ptr<void> m_keepAlive;

    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    QVarLengthArray<std::pair<void *, size_t>, 4> m_mappedRegions;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QDMABUFVIDEOBUFFER_P_H
