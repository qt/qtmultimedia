// Copyright (C) 2016 Research In Motion
// Copyright (C) 2021 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxvideosink_p.h"

QT_BEGIN_NAMESPACE

QQnxVideoSink::QQnxVideoSink(QVideoSink *parent)
    : QPlatformVideoSink(parent)
{
}

void QQnxVideoSink::setRhi(QRhi *rhi)
{
    m_rhi = rhi;
}

QRhi *QQnxVideoSink::rhi() const
{
    return m_rhi;
}

QT_END_NAMESPACE

#include "moc_qqnxvideosink_p.cpp"
