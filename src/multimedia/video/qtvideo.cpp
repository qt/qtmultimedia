// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtvideo.h"

QT_BEGIN_NAMESPACE

/*!
    \namespace QtVideo
    \since 6.7
    \inmodule QtMultimedia
    \brief Enumerations for camera and video functionality.
*/

/*!
    \enum QtVideo::Rotation
    \since 6.7

    The angle of the clockwise rotation that should be applied to a video
    frame before displaying.

    \value None No rotation required, the frame has correct orientation
    \value Clockwise90 The frame should be rotated clockwise by 90 degrees
    \value Clockwise180 The frame should be rotated clockwise by 180 degrees
    \value Clockwise270 The frame should be rotated clockwise by 270 degrees
*/

/*!
    \enum QtVideo::MapMode

    Enumerates how a video buffer's data is mapped to system memory.

    \value NotMapped
    The video buffer is not mapped to memory.
    \value ReadOnly
    The mapped memory is populated with data from the video buffer when mapped,
    but the content of the mapped memory may be discarded when unmapped.
    \value WriteOnly
    The mapped memory is uninitialized when mapped, but the possibly modified
    content will be used to populate the video buffer when unmapped.
    \value ReadWrite
    The mapped memory is populated with data from the video
    buffer, and the video buffer is repopulated with the content of the mapped
    memory when it is unmapped.

    \sa QVideoFrame::mapMode(), map()
*/

QT_END_NAMESPACE

#include "moc_qtvideo.cpp"
