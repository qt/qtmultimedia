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
    frame before displaying. The rotation is performed in video coordinates,
    where the Y-axis points downwards on the display. It means that
    the rotation direction complies with \l QTransform::rotate.

    \value None No rotation required, the frame has correct orientation
    \value Clockwise90 The frame should be rotated clockwise by 90 degrees
    \value Clockwise180 The frame should be rotated clockwise by 180 degrees
    \value Clockwise270 The frame should be rotated clockwise by 270 degrees
*/

QT_END_NAMESPACE

#include "moc_qtvideo.cpp"
