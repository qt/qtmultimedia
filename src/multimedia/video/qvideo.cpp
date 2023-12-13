// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideo.h"

QT_BEGIN_NAMESPACE

/*!
    \namespace QVideo
    \since 6.7
    \inmodule QtMultimedia
    \brief Enumerations for camera and video functionality.
*/

/*!
    \enum QVideo::RotationAngle
    \since 6.7

    The angle of the clockwise rotation that should be applied to a video
    frame before displaying.

    \value Rotation0 No rotation required, the frame has correct orientation
    \value Rotation90 The frame should be rotated by 90 degrees
    \value Rotation180 The frame should be rotated by 180 degrees
    \value Rotation270 The frame should be rotated by 270 degrees
*/

QT_END_NAMESPACE

#include "moc_qvideo.cpp"
