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

bool QV4L2FileDescriptor::call(int request, void *arg) const
{
    return QtMultimediaPrivate::qt_safe_ioctl(get(), request, arg) >= 0;
}

bool QV4L2FileDescriptor::requestBuffers(v4l2_memory memoryType, quint32 &buffersCount) const
{
    v4l2_requestbuffers req = {};
    req.count = buffersCount;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = memoryType;

    if (!call(VIDIOC_REQBUFS, &req))
        return false;

    buffersCount = req.count;
    return true;
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
    auto result = call(VIDIOC_STREAMOFF, &type);
    m_streamStarted = false;
    return result;
}

} // namespace QtV4L2

QT_END_NAMESPACE
