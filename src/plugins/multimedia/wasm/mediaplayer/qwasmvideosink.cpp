// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmvideosink_p.h"

#include <QtGui/rhi/qrhi.h>

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
