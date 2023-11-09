// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qwasmvideosink_p.h"

QT_BEGIN_NAMESPACE

QWasmVideoSink::QWasmVideoSink(QVideoSink *parent)
    : QPlatformVideoSink(parent)
{
}

void QWasmVideoSink::setRhi(QRhi *rhi)
{
    if (rhi && rhi->backend() != QRhi::OpenGLES2)
        rhi = nullptr;
    if (m_rhi == rhi)
        return;
    m_rhi = rhi;
}

QT_END_NAMESPACE

#include "moc_qwasmvideosink_p.cpp"
