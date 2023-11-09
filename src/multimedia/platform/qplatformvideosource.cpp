// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qplatformvideosource_p.h"

QT_BEGIN_NAMESPACE

std::optional<int> QPlatformVideoSource::ffmpegHWPixelFormat() const
{
    return {};
};

QT_END_NAMESPACE

//#include "moc_qplatformvideosource_p.cpp
