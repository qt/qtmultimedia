// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4l2filedescriptor_p.h"

#include <sys/ioctl.h>
#include <private/qcore_unix_p.h>

#include <linux/videodev2.h>

QT_BEGIN_NAMESPACE

int xioctl(int fd, int request, void *arg)
{
    int res;

    do {
        res = ::ioctl(fd, request, arg);
    } while (res == -1 && EINTR == errno);

    return res;
}

QV4L2FileDescriptor::QV4L2FileDescriptor(int descriptor) : m_descriptor(descriptor)
{
    Q_ASSERT(descriptor >= 0);
}

QV4L2FileDescriptor::~QV4L2FileDescriptor()
{
    qt_safe_close(m_descriptor);
}

bool QV4L2FileDescriptor::call(int request, void *arg) const
{
    return ::xioctl(m_descriptor, request, arg) >= 0;
}

bool QV4L2FileDescriptor::requestBuffers(quint32 memoryType, quint32 &buffersCount) const
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

QT_END_NAMESPACE
