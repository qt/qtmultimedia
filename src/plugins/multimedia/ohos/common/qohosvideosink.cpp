// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosvideosink_p.h"

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

QOhosVideoSink::QOhosVideoSink(QVideoSink *parent) : QPlatformVideoSink(parent) { }

QOhosVideoSink::~QOhosVideoSink() = default;

void QOhosVideoSink::setRhi(QRhi *rhi)
{
    if (m_rhi == rhi)
        return;
    m_rhi = rhi;
    emit rhiChanged();
}

QT_END_NAMESPACE

#include "moc_qohosvideosink_p.cpp"
