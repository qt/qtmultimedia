// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QLIBCAMERA_DMABUFREGISTRY_P_H
#define QLIBCAMERA_DMABUFREGISTRY_P_H

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

#include "qlibcamera_stream_p.h"

#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qcaptureslotregistry_p.h>
#include <QtMultimedia/private/qdmabufallocator_p.h>
#include <QtMultimedia/private/qdmabufsource_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <QtCore/qspan.h>

#include <memory>
#include <optional>
#include <vector>

namespace libcamera {
class Camera;
class Request;
class Stream;
} // namespace libcamera

QT_BEGIN_NAMESPACE

namespace QLibCamera {

#if QT_CONFIG(linux_dmabuf)

// Owns the DMA-BUFs behind the request buffers of a stream. A frame that wraps a buffer holds
// the buffer's request as a capture slot, which keeps the request out of the camera queue until
// the frame releases it.
class QLibCameraDmaBufRegistry final
    : public QtMultimediaPrivate::CaptureSlotRegistry<QLibCameraDmaBufRegistry,
                                                      libcamera::Request *, kCaptureBufferCount>
{
public:
    using DmaBufLayout = QtMultimediaPrivate::DmaBufLayout;
    using QDmaBuf = QtMultimediaPrivate::QDmaBuf;

    // Returns nullptr when the buffers cannot be shared as DMA-BUFs; frames are then copied.
    static std::shared_ptr<QLibCameraDmaBufRegistry>
    create(std::shared_ptr<libcamera::Camera>, std::shared_ptr<QLibCameraPendingControls>,
           QSpan<const std::unique_ptr<libcamera::Request>>, libcamera::Stream *,
           const QVideoFrameFormat &, int bytesPerLine);

    std::shared_ptr<QDmaBuf> dmaBufFor(const libcamera::Request *request) const;

    // Plane placement shared by all buffers; nullopt for formats without a zero-copy layout.
    const std::optional<DmaBufLayout> &layout() const { return m_layout; }

    void recycleSlot(libcamera::Request *request) const;

private:
    struct Buffer
    {
        libcamera::Request *request;
        std::shared_ptr<QDmaBuf> dmaBuf;
    };

    QLibCameraDmaBufRegistry(std::shared_ptr<libcamera::Camera> camera,
                             std::shared_ptr<QLibCameraPendingControls> pendingControls,
                             std::vector<Buffer> buffers, std::optional<DmaBufLayout> layout);

    const std::shared_ptr<libcamera::Camera> m_camera;
    const std::shared_ptr<QLibCameraPendingControls> m_pendingControls;
    const std::vector<Buffer> m_buffers; // CHECK: Should this be a (flat)map for faster lookup?
    const std::optional<DmaBufLayout> m_layout;
};

#endif // QT_CONFIG(linux_dmabuf)

} // namespace QLibCamera

QT_END_NAMESPACE

#endif // QLIBCAMERA_DMABUFREGISTRY_P_H
