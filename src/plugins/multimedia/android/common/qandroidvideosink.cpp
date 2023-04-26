// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidvideosink_p.h"
#include <rhi/qrhi.h>

#include <QtCore/qdebug.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

QAndroidVideoSink::QAndroidVideoSink(QVideoSink *parent)
    : QPlatformVideoSink(parent)
{
}

QAndroidVideoSink::~QAndroidVideoSink()
{
}

void QAndroidVideoSink::setRhi(QRhi *rhi)
{
    if (rhi && rhi->backend() != QRhi::OpenGLES2)
        rhi = nullptr;
    if (m_rhi == rhi)
        return;

    m_rhi = rhi;
    emit rhiChanged(rhi);
}

QT_END_NAMESPACE

#include "moc_qandroidvideosink_p.cpp"
