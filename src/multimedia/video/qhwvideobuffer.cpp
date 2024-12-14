// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhwvideobuffer_p.h"

QT_BEGIN_NAMESPACE

QVideoFrameTextures::~QVideoFrameTextures() = default;

QVideoFrameTexturesHandles::~QVideoFrameTexturesHandles() = default;

QHwVideoBuffer::QHwVideoBuffer(QVideoFrame::HandleType type, QRhi *rhi) : m_type(type), m_rhi(rhi)
{
}

// must be out-of-line to ensure correct working of dynamic_cast when QHwVideoBuffer is created in tests
QHwVideoBuffer::~QHwVideoBuffer() = default;

QT_END_NAMESPACE
