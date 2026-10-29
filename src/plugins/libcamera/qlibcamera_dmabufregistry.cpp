// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlibcamera_dmabufregistry_p.h"
#include "qlibcameralogging_p.h"

#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtCore/private/qsystemerror_p.h>

#include <libcamera/base/span.h>
#include <libcamera/camera.h>
#include <libcamera/framebuffer.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>

#include <fcntl.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <utility>

#if QT_CONFIG(linux_dmabuf)

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QLibCamera {

using namespace libcamera;

namespace {

using QtMultimediaPrivate::DmaBufLayout;
using QtMultimediaPrivate::QDmaBuf;

constexpr std::size_t kMinFreeBuffers = 2;

// Duplicates the fd that all planes share, sized to cover every plane.
q23::expected<std::shared_ptr<QDmaBuf>, QString>
dmaBufForPlanes(Span<const FrameBuffer::Plane> planes)
{
    if (planes.empty())
        return q23::unexpected(u"Capture buffer has no planes"_s);

    const SharedFD fd = planes[0].fd;
    qsizetype size = 0;
    for (const FrameBuffer::Plane &plane : planes) {
        if (plane.fd != fd)
            return q23::unexpected(u"Capture buffer planes are not in one DMA-BUF"_s);
        if (plane.offset == FrameBuffer::Plane::kInvalidOffset)
            return q23::unexpected(u"Capture buffer plane has no offset"_s);
        size = std::max(size, qsizetype(plane.offset) + qsizetype(plane.length));
    }

    libcamera::UniqueFD dup = fd.dup();

    if (!dup.isValid()) {
        return q23::unexpected(
                u"Cannot duplicate capture buffer fd: %1"_s.arg(QSystemError::stdString(errno)));
    }

    return std::make_shared<QDmaBuf>(QUniqueFileDescriptorHandle{ dup.release() }, size);
}

// The planes must sit exactly where the layout places them.
bool planesMatchLayout(Span<const FrameBuffer::Plane> planes, const DmaBufLayout &layout)
{
    if (planes.size() != std::size_t(layout.size()))
        return false;

    for (qsizetype i = 0; i < layout.size(); ++i) {
        const FrameBuffer::Plane &plane = planes[std::size_t(i)];
        if (plane.offset != layout[i].offset || qsizetype(plane.length) < layout[i].size)
            return false;
    }
    return true;
}

} // namespace

std::shared_ptr<QLibCameraDmaBufRegistry>
QLibCameraDmaBufRegistry::create(std::shared_ptr<Camera> camera,
                                 std::shared_ptr<QLibCameraPendingControls> pendingControls,
                                 QSpan<const std::unique_ptr<libcamera::Request>> requests,
                                 Stream *stream, const QVideoFrameFormat &format, int bytesPerLine)
{
    using namespace QtMultimediaPrivate;

    if (requests.empty() || requests.size() > kCaptureBufferCount) {
        qCDebug(lcLibCamera) << "Cannot share" << requests.size() << "capture buffers as DMA-BUFs";
        return nullptr;
    }

    std::vector<Buffer> buffers;
    std::optional<DmaBufLayout> layout;
    for (const std::unique_ptr<Request> &request : requests) {
        FrameBuffer *frameBuffer = request->findBuffer(stream);
        if (!frameBuffer) {
            qCDebug(lcLibCamera) << "Capture request has no buffer for the stream";
            return nullptr;
        }

        const Span<const FrameBuffer::Plane> planes = frameBuffer->planes();
        q23::expected<std::shared_ptr<QDmaBuf>, QString> dmaBuf = dmaBufForPlanes(planes);
        if (!dmaBuf) {
            qCDebug(lcLibCamera) << "Capture buffer cannot be shared as a DMA-BUF:"
                                 << dmaBuf.error();
            return nullptr;
        }

        if (buffers.empty()) {
            layout = dmaBufLayoutForFormat(format.pixelFormat(), format.frameSize(), bytesPerLine,
                                           quint32((*dmaBuf)->size()));
        }

        if (layout && !planesMatchLayout(planes, *layout)) {
            qCDebug(lcLibCamera) << "Capture buffer planes do not match the zero-copy layout";
            return nullptr;
        }

        buffers.push_back(Buffer{ request.get(), std::move(*dmaBuf) });
    }

    return std::shared_ptr<QLibCameraDmaBufRegistry>(new QLibCameraDmaBufRegistry(
            std::move(camera), std::move(pendingControls), std::move(buffers), std::move(layout)));
}

QLibCameraDmaBufRegistry::QLibCameraDmaBufRegistry(
        std::shared_ptr<Camera> camera, std::shared_ptr<QLibCameraPendingControls> pendingControls,
        std::vector<Buffer> buffers, std::optional<DmaBufLayout> layout)
    : CaptureSlotRegistry(buffers.size(), kMinFreeBuffers),
      m_camera(std::move(camera)),
      m_pendingControls(std::move(pendingControls)),
      m_buffers(std::move(buffers)),
      m_layout(std::move(layout))
{
}

std::shared_ptr<QDmaBuf> QLibCameraDmaBufRegistry::dmaBufFor(const Request *request) const
{
    namespace ranges = QtMultimediaPrivate::ranges;

    const auto it = ranges::find_if(m_buffers, [&](const Buffer &buffer) {
        return buffer.request == request;
    });
    if (it == m_buffers.end())
        return nullptr;
    return it->dmaBuf;
}

void QLibCameraDmaBufRegistry::recycleSlot(Request *request) const
{
    requeueRequest(*m_camera, request, *m_pendingControls);
}

} // namespace QLibCamera

QT_END_NAMESPACE

#endif // QT_CONFIG(linux_dmabuf)
