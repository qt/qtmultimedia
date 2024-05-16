// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformaudiobufferinput_p.h"

QT_BEGIN_NAMESPACE

void QPlatformAudioBufferInput::setEncoderReadinessGetter(
        std::function<bool()> encoderReadinessGetter)
{
    m_encoderReadinessGetter = std::move(encoderReadinessGetter);
    emit encoderUpdated();
}

QT_END_NAMESPACE

#include "moc_qplatformaudiobufferinput_p.cpp"
