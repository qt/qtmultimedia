// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4l2filedescriptor_p.h"

#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtCore/private/qcore_unix_p.h>

QT_BEGIN_NAMESPACE

namespace QtV4L2 {

QV4L2FileDescriptor::QV4L2FileDescriptor(QUniqueFileDescriptorHandle descriptor)
    : QUniqueFileDescriptorHandle(std::move(descriptor))
{
    Q_ASSERT(isValid());
}

q23::expected<int, std::error_code> QV4L2FileDescriptor::call(int request, void *arg) const
{
    const int result = QtMultimediaPrivate::qt_safe_ioctl(get(), request, arg);
    if (result < 0)
        return q23::unexpected(std::error_code{ errno, std::system_category() });

    return result;
}

q23::expected<int, std::error_code> QV4L2FileDescriptor::requestBuffers(v4l2_memory memoryType,
                                                                        uint32_t bufferCount) const
{
    v4l2_requestbuffers req = {};
    req.count = bufferCount;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = memoryType;

    auto callResult = call(VIDIOC_REQBUFS, &req);
    if (!callResult)
        return q23::unexpected(callResult.error());

    return int(req.count);
}
bool QV4L2FileDescriptor::startStream()
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (!call(VIDIOC_STREAMON, &type))
        return false;

    m_streamStarted = true;
    return true;
}

bool QV4L2FileDescriptor::stopStream()
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    const auto result = call(VIDIOC_STREAMOFF, &type);
    m_streamStarted = false;
    return bool(result);
}

} // namespace QtV4L2

QT_END_NAMESPACE
