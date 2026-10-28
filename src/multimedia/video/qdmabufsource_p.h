// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QDMABUFSOURCE_P_H
#define QDMABUFSOURCE_P_H

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

#include <QtMultimedia/qabstractvideobuffer.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <QtCore/qsize.h>
#include <QtCore/qvarlengtharray.h>

#include <cstdint>
#include <memory>
#include <optional>

#if QT_CONFIG(linux_dmabuf)

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

class QDmaBuf;

// Byte placement of one plane inside a single DMA-BUF.
struct DmaBufPlaneLayout
{
    uint32_t offset = 0;
    uint32_t pitch = 0;
    qsizetype size = 0;
};

using DmaBufLayout = QVarLengthArray<DmaBufPlaneLayout, 4>;

// Places every plane of a frame in one DMA-BUF, in plane order, all sharing bytesPerLine.
// Returns nullopt when the layout cannot be described or does not fit into sizeImage.
//
// LATER: fully-planar formats (YUV420P, YV12, YUV422P) and multi-fd layouts (NV12M via the
// V4L2 multi-planar API) are not supported. They need one fd and one pitch per plane
// (v4l2_plane arrays), and chroma pitch scaling per plane.
Q_MULTIMEDIA_EXPORT std::optional<DmaBufLayout>
dmaBufLayoutForFormat(QVideoFrameFormat::PixelFormat format, QSize frameSize, int bytesPerLine,
                      quint32 sizeImage);

// Maps a DMA-BUF read-only and exposes each plane of its layout. Plane 0 must start at offset 0,
// so the mapping base is the first plane's data pointer.
class Q_MULTIMEDIA_EXPORT DmaBufSource
{
public:
    DmaBufSource();
    DmaBufSource(std::shared_ptr<const QDmaBuf> buffer, DmaBufLayout layout);

    std::optional<QAbstractVideoBuffer::MapData> mapReadOnly() const;
    void unmap(QAbstractVideoBuffer::MapData &mapData) const;

protected:
    std::shared_ptr<const QDmaBuf> m_buffer;
    DmaBufLayout m_layout;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QT_CONFIG(linux_dmabuf)

#endif // QDMABUFSOURCE_P_H
